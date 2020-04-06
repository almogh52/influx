#include <kernel/drivers/ata/slice.h>

#include <kernel/algorithm.h>
#include <kernel/assert.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/utils.h>

influx::drivers::ata::drive_slice::drive_slice() : _driver(nullptr), _start_lba(UINT32_MAX) {}

influx::drivers::ata::drive_slice::drive_slice(influx::drivers::ata::ata *driver,
                                               const influx::drivers::ata::drive &drive,
                                               uint32_t start_lba)
    : _driver(driver), _drive(drive), _start_lba(start_lba) {
    kassert(_driver != nullptr);
}

bool influx::drivers::ata::drive_slice::read(uint64_t address, uint64_t amount, void *buffer) {
    uint32_t start_lba = _start_lba + (uint32_t)(address / ATA_SECTOR_SIZE);
    uint16_t sector_count = (uint16_t)(
        (amount % ATA_SECTOR_SIZE ? (amount / ATA_SECTOR_SIZE) + 1 : amount / ATA_SECTOR_SIZE) +
        (address % ATA_SECTOR_SIZE &&
                 (address + amount) / ATA_SECTOR_SIZE != address / ATA_SECTOR_SIZE
             ? 1
             : 0));

    void *aligned_buffer = kmalloc(sector_count * ATA_SECTOR_SIZE);
    if (aligned_buffer == nullptr) {
        return false;
    }

    // Read the sectors from the drive
    for (uint16_t i = 0; i < (sector_count / MAX_ATA_SECTOR_ACCESS) +
                                 (sector_count % MAX_ATA_SECTOR_ACCESS ? 1 : 0);
         i++) {
        // Read the sectors from the drive
        if (!_driver->access_drive_sectors(
                _drive, access_type::read, start_lba + i * MAX_ATA_SECTOR_ACCESS,
                algorithm::min<uint16_t>(MAX_ATA_SECTOR_ACCESS,
                                         (uint16_t)(sector_count - (i * MAX_ATA_SECTOR_ACCESS))),
                (uint16_t *)((uint8_t *)aligned_buffer +
                             i * MAX_ATA_SECTOR_ACCESS * ATA_SECTOR_SIZE))) {
            kfree(aligned_buffer);
            return false;
        }
    }

    // Copy only the wanted data from the aligned data
    memory::utils::memcpy(buffer, (uint8_t *)aligned_buffer + (address % ATA_SECTOR_SIZE), amount);

    // Free the temp buffer
    kfree(aligned_buffer);

    return true;
}

bool influx::drivers::ata::drive_slice::write(uint64_t address, uint64_t amount, void *data) {
    uint32_t start_lba = _start_lba + (uint32_t)(address / ATA_SECTOR_SIZE);
    uint16_t sector_count = (uint16_t)(
        (amount % ATA_SECTOR_SIZE ? (amount / ATA_SECTOR_SIZE) + 1 : amount / ATA_SECTOR_SIZE) +
        (address % ATA_SECTOR_SIZE &&
                 (address + amount) / ATA_SECTOR_SIZE != address / ATA_SECTOR_SIZE
             ? 1
             : 0));

    void *aligned_buffer = kmalloc(sector_count * ATA_SECTOR_SIZE);
    if (aligned_buffer == nullptr) {
        return false;
    }

    // If the start address is in the middle of a sector, read the sector
    if (address % ATA_SECTOR_SIZE || sector_count == 1) {
        if (!_driver->access_drive_sectors(_drive, access_type::read, start_lba, 1,
                                           (uint16_t *)aligned_buffer)) {
            kfree(aligned_buffer);
            return false;
        }
    }

    // If the end is in the middle of a sector, read the last sector
    if ((address + amount) % ATA_SECTOR_SIZE && sector_count > 1) {
        if (!_driver->access_drive_sectors(
                _drive, access_type::read,
                (uint32_t)(start_lba + (address + amount) / ATA_SECTOR_SIZE), 1,
                (uint16_t *)((uint8_t *)aligned_buffer + (sector_count - 1) * ATA_SECTOR_SIZE))) {
            kfree(aligned_buffer);
            return false;
        }
    }

    // Copy the data to the buffer
    memory::utils::memcpy((uint8_t *)aligned_buffer + address % ATA_SECTOR_SIZE, data, amount);

    // Write the sectors to the drive
    for (uint16_t i = 0; i < (sector_count / MAX_ATA_SECTOR_ACCESS) +
                                 (sector_count % MAX_ATA_SECTOR_ACCESS ? 1 : 0);
         i++) {
        // Write the sectors to the drive
        if (!_driver->access_drive_sectors(
                _drive, access_type::write, start_lba + i * MAX_ATA_SECTOR_ACCESS,
                algorithm::min<uint16_t>(MAX_ATA_SECTOR_ACCESS,
                                         (uint16_t)(sector_count - (i * MAX_ATA_SECTOR_ACCESS))),
                (uint16_t *)((uint8_t *)aligned_buffer +
                             i * MAX_ATA_SECTOR_ACCESS * ATA_SECTOR_SIZE))) {
            kfree(aligned_buffer);
            return false;
        }
    }

    // Free the temp buffer
    kfree(aligned_buffer);

    return true;
}