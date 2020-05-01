#include <kernel/drivers/ata/slice.h>

#include <kernel/algorithm.h>
#include <kernel/assert.h>
#include <kernel/kernel.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/utils.h>

influx::drivers::ata::drive_slice::drive_slice() : _driver(nullptr), _start_lba(UINT32_MAX) {}

influx::drivers::ata::drive_slice::drive_slice(influx::drivers::ata::ata *driver,
                                               const influx::drivers::ata::drive &drive,
                                               uint32_t start_lba)
    : _driver(driver), _drive(drive), _start_lba(start_lba) {
    kassert(_driver != nullptr);
}

uint64_t influx::drivers::ata::drive_slice::read(uint64_t address, uint64_t amount, void *buffer,
                                                 bool interruptible) {
    uint32_t start_lba = _start_lba + (uint32_t)(address / ATA_SECTOR_SIZE);
    uint16_t sector_count = (uint16_t)(
        (amount % ATA_SECTOR_SIZE ? (amount / ATA_SECTOR_SIZE) + 1 : amount / ATA_SECTOR_SIZE) +
        (address % ATA_SECTOR_SIZE &&
                 (address + amount) / ATA_SECTOR_SIZE != address / ATA_SECTOR_SIZE
             ? 1
             : 0));
    uint16_t current_sector_count = 0;

    uint64_t read = 0;

    void *aligned_buffer = kmalloc(sector_count * ATA_SECTOR_SIZE);
    if (aligned_buffer == nullptr) {
        return 0;
    }

    // Read the sectors from the drive
    for (uint16_t i = 0; i < (sector_count / MAX_ATA_SECTOR_ACCESS) +
                                 (sector_count % MAX_ATA_SECTOR_ACCESS ? 1 : 0);
         i++) {
        // If the task was interrupted, break
        if (kernel::scheduler()->interrupted() && interruptible) {
            break;
        }

        // Read the sectors from the drive
        current_sector_count = algorithm::min<uint16_t>(
            MAX_ATA_SECTOR_ACCESS, (uint16_t)(sector_count - (i * MAX_ATA_SECTOR_ACCESS)));
        if (_driver->access_drive_sectors(_drive, access_type::read,
                                          start_lba + i * MAX_ATA_SECTOR_ACCESS,
                                          current_sector_count,
                                          (uint16_t *)((uint8_t *)aligned_buffer +
                                                       i * MAX_ATA_SECTOR_ACCESS * ATA_SECTOR_SIZE),
                                          interruptible) != current_sector_count) {
            break;
        }

        // Increase the amount of read data
        read += current_sector_count * ATA_SECTOR_SIZE;
    }

    // Reduce the amount of read data from the aligned buffer
    read = algorithm::min<uint64_t>(amount, read - (address % ATA_SECTOR_SIZE));

    // Copy only the wanted data from the aligned data
    memory::utils::memcpy(buffer, (uint8_t *)aligned_buffer + (address % ATA_SECTOR_SIZE), read);

    // Free the temp buffer
    kfree(aligned_buffer);

    return read;
}

uint64_t influx::drivers::ata::drive_slice::write(uint64_t address, uint64_t amount, void *data,
                                                  bool interruptible) {
    uint32_t start_lba = _start_lba + (uint32_t)(address / ATA_SECTOR_SIZE);
    uint16_t sector_count = (uint16_t)(
        (amount % ATA_SECTOR_SIZE ? (amount / ATA_SECTOR_SIZE) + 1 : amount / ATA_SECTOR_SIZE) +
        (address % ATA_SECTOR_SIZE &&
                 (address + amount) / ATA_SECTOR_SIZE != address / ATA_SECTOR_SIZE
             ? 1
             : 0));
    uint16_t current_sector_count = 0;

    uint64_t write = 0;

    void *aligned_buffer = kmalloc(sector_count * ATA_SECTOR_SIZE);
    if (aligned_buffer == nullptr) {
        return false;
    }

    // If the start address is in the middle of a sector, read the sector
    if (address % ATA_SECTOR_SIZE || sector_count == 1) {
        if (_driver->access_drive_sectors(_drive, access_type::read, start_lba, 1,
                                          (uint16_t *)aligned_buffer, interruptible) != 1) {
            kfree(aligned_buffer);
            return 0;
        }
    }

    // If the end is in the middle of a sector, read the last sector
    if ((address + amount) % ATA_SECTOR_SIZE && sector_count > 1) {
        if (_driver->access_drive_sectors(
                _drive, access_type::read,
                (uint32_t)(start_lba + (address + amount) / ATA_SECTOR_SIZE), 1,
                (uint16_t *)((uint8_t *)aligned_buffer + (sector_count - 1) * ATA_SECTOR_SIZE),
                interruptible) != 1) {
            kfree(aligned_buffer);
            return 0;
        }
    }

    // Copy the data to the buffer
    memory::utils::memcpy((uint8_t *)aligned_buffer + (address % ATA_SECTOR_SIZE), data, amount);

    // Write the sectors to the drive
    for (uint16_t i = 0; i < (sector_count / MAX_ATA_SECTOR_ACCESS) +
                                 (sector_count % MAX_ATA_SECTOR_ACCESS ? 1 : 0);
         i++) {
        // If the task was interrupted, break
        if (kernel::scheduler()->interrupted() && interruptible) {
            break;
        }

        // Write the sectors to the drive
        current_sector_count = algorithm::min<uint16_t>(
            MAX_ATA_SECTOR_ACCESS, (uint16_t)(sector_count - (i * MAX_ATA_SECTOR_ACCESS)));
        if (_driver->access_drive_sectors(_drive, access_type::write,
                                          start_lba + i * MAX_ATA_SECTOR_ACCESS,
                                          current_sector_count,
                                          (uint16_t *)((uint8_t *)aligned_buffer +
                                                       i * MAX_ATA_SECTOR_ACCESS * ATA_SECTOR_SIZE),
                                          interruptible) != current_sector_count) {
            break;
        }

        // Increase the amount of write data
        write += current_sector_count * ATA_SECTOR_SIZE;
    }

    // Reduce the amount of write to the correct amount minus the alignment
    write = algorithm::min<uint64_t>(amount, write - (address % ATA_SECTOR_SIZE));

    // Free the temp buffer
    kfree(aligned_buffer);

    return write;
}