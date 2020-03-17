#include <kernel/drivers/ata/ata.h>

#include <kernel/drivers/ata/command.h>
#include <kernel/drivers/ata/constants.h>
#include <kernel/kernel.h>
#include <kernel/memory/utils.h>
#include <kernel/ports.h>

void influx::drivers::ata::primary_irq(influx::interrupts::regs *context,
                                       influx::drivers::ata::ata *ata) {
    // If the scheduler is loaded
    if (kernel::scheduler() != nullptr) {
        ata->_primary_irq_notifier.notify();
    } else {
        // Notify the ATA driver for primary irq
        while (!__sync_bool_compare_and_swap(&ata->_primary_irq_called, 0, 1)) {
            __sync_synchronize();
        }
    }
}

void influx::drivers::ata::secondary_irq(influx::interrupts::regs *context,
                                         influx::drivers::ata::ata *ata) {
    // If the scheduler is loaded
    if (kernel::scheduler() != nullptr) {
        ata->_secondary_irq_notifier.notify();
    } else {
        // Notify the ATA driver for secondary irq
        while (!__sync_bool_compare_and_swap(&ata->_secondary_irq_called, 0, 1)) {
            __sync_synchronize();
        }
    }
}

influx::drivers::ata::ata::ata()
    : driver("ATA"), _primary_irq_called(0), _secondary_irq_called(0) {}

void influx::drivers::ata::ata::load() {
    // Register IRQ handlers
    _log("Registering IRQs handlers..\n");
    kernel::interrupt_manager()->set_irq_handler(ATA_PRIMARY_IRQ,
                                                 (uint64_t)influx::drivers::ata::primary_irq, this);
    kernel::interrupt_manager()->set_irq_handler(
        ATA_SECONDARY_IRQ, (uint64_t)influx::drivers::ata::secondary_irq, this);

    // Disable IRQs for both ATA controllers
    ports::out<uint8_t>(ATA_DISABLE_INTERRUTPS,
                        ata_primary_bus.control_base + ATA_CONTROL_DEVICE_REGISTER);
    ports::out<uint8_t>(ATA_DISABLE_INTERRUTPS,
                        ata_secondary_bus.control_base + ATA_CONTROL_DEVICE_REGISTER);

    // Detect the available drives
    _log("Detecting drives..\n");
    detect_drives();

    // If no drives were found
    if (_drives.empty()) {
        _log("No drives found!\n");
    } else {
        _log("%d drives were found.\n", _drives.size());
    }

    // Re-enable IRQs for both ATA controllers
    ports::out<uint8_t>(ATA_ENABLE_INTERRUTPS,
                        ata_primary_bus.control_base + ATA_CONTROL_DEVICE_REGISTER);
    ports::out<uint8_t>(ATA_ENABLE_INTERRUTPS,
                        ata_secondary_bus.control_base + ATA_CONTROL_DEVICE_REGISTER);
}

void influx::drivers::ata::ata::wait_for_primary_irq() {
    // If the scheduler is loaded
    if (kernel::scheduler() != nullptr) {
        _primary_irq_notifier.wait();
    } else {
        // Wait for the IRQ notifiction and reset the variable
        while (!__sync_bool_compare_and_swap(&_primary_irq_called, 1, 0)) {
            __sync_synchronize();
        }
    }
}

void influx::drivers::ata::ata::wait_for_secondary_irq() {
    // If the scheduler is loaded
    if (kernel::scheduler() != nullptr) {
        _secondary_irq_notifier.wait();
    } else {
        // Wait for the IRQ notifiction and reset the variable
        while (!__sync_bool_compare_and_swap(&_secondary_irq_called, 1, 0)) {
            __sync_synchronize();
        }
    }
}

void influx::drivers::ata::ata::detect_drives() {
    drive drive;

    // Try to identify the master drive for the primary ATA bus
    drive = identify_drive(ata_primary_bus, false);
    if (drive.present) {
        _drives.push_back(drive);
    }

    // Try to identify the slave drive for the primary ATA bus
    drive = identify_drive(ata_primary_bus, true);
    if (drive.present) {
        _drives.push_back(drive);
    }

    // Try to identify the master drive for the secondary ATA bus
    drive = identify_drive(ata_secondary_bus, false);
    if (drive.present) {
        _drives.push_back(drive);
    }

    // Try to identify the slave drive for the secondary ATA bus
    drive = identify_drive(ata_secondary_bus, true);
    if (drive.present) {
        _drives.push_back(drive);
    }
}

bool influx::drivers::ata::ata::access_drive_sectors(const influx::drivers::ata::drive &drive,
                                                     influx::drivers::ata::access_type access_type,
                                                     uint32_t lba, uint16_t amount_of_sectors,
                                                     uint16_t *data) {
    status_register status_reg;

    // Select the drive
    if (!select_drive(drive)) {
        _log("Selecting drive falied!\n");
        return false;
    }

    // Send sector count
    ports::out<uint8_t>(amount_of_sectors >= 256 ? 0 : (uint8_t)amount_of_sectors,
                        (uint16_t)(drive.controller.io_base + ATA_IO_SECTOR_COUNT_REGISTER));

    // Send the LBA
    ports::out<uint8_t>((uint8_t)(0xE0 | ((uint8_t)drive.slave << 4) | ((lba >> 24) & 0x0F)),
                        (uint16_t)(drive.controller.io_base + ATA_IO_DRIVE_REGISTER));
    ports::out<uint8_t>((uint8_t)lba,
                        (uint16_t)(drive.controller.io_base + ATA_IO_SECTOR_NUMBER_REGISTER));
    ports::out<uint8_t>((uint8_t)(lba >> 8),
                        (uint16_t)(drive.controller.io_base + ATA_IO_LCYL_REGISTER));
    ports::out<uint8_t>((uint8_t)(lba >> 16),
                        (uint16_t)(drive.controller.io_base + ATA_IO_HCYL_REGISTER));

    // Send the access command
    ports::out<uint8_t>(
        (uint8_t)(access_type == access_type::read ? command::read : command::write),
        (uint16_t)(drive.controller.io_base + ATA_IO_COMMAND_REGISTER));

    // For each sector
    for (uint16_t sector = 0; sector < amount_of_sectors; sector++) {
        // Wait for the controller to finish loading the drive and check if there are errors
        if ((status_reg =
                 read_status_register_with_mask(drive.controller, ATA_STATUS_BSY, 0, 30000))
                .fetch_failed ||
            status_reg.err) {
            return false;
        }

        // If this is a write operation
        if (access_type == access_type::write) {
            for (uint64_t i = (sector * ATA_SECTOR_SIZE) / sizeof(uint16_t);
                 i < ((sector + 1) * ATA_SECTOR_SIZE) / sizeof(uint16_t); i++) {
                ports::out<uint16_t>(data[i],
                                     (uint16_t)(drive.controller.io_base + ATA_IO_DATA_REGISTER));
            }
        }

        // Wait for IRQ
        if (drive.controller == ata_primary_bus) {
            wait_for_primary_irq();
        } else {
            wait_for_secondary_irq();
        }

        // If an error occurred
        if ((status_reg = read_status_register(drive.controller)).err) {
            return false;
        }

        // If this is a read operation, read data from the data register
        if (access_type == access_type::read) {
            for (uint64_t i = (sector * ATA_SECTOR_SIZE) / sizeof(uint16_t);
                 i < ((sector + 1) * ATA_SECTOR_SIZE) / sizeof(uint16_t); i++) {
                data[i] = ports::in<uint16_t>(
                    (uint16_t)(drive.controller.io_base + ATA_IO_DATA_REGISTER));
            }
        }
    }

    // Flush cache after write
    if (access_type == access_type::write) {
        ports::out<uint8_t>((uint8_t)command::cache_flush,
                            (uint16_t)(drive.controller.io_base + ATA_IO_COMMAND_REGISTER));

        // Wait for the controller to flush the cache
        read_status_register_with_mask(drive.controller, ATA_STATUS_BSY, 0, 10000);
    }

    return true;
}

bool influx::drivers::ata::ata::select_drive(const influx::drivers::ata::drive &drive) {
    // Wait for controller to be ready to select drive
    if (read_status_register_with_mask(drive.controller, ATA_STATUS_BSY | ATA_STATUS_DRQ, 0, 10000)
            .fetch_failed) {
        return false;
    }

    // If the selected drive is the wanted drive
    if (_selected_drive == drive) {
        return true;
    }

    // Select the drive
    ports::out<uint8_t>((uint8_t)(0xA0 | ((uint8_t)drive.slave << 4)),
                        (uint16_t)(drive.controller.io_base + ATA_IO_DRIVE_REGISTER));
    delay(drive.controller);

    // Wait for controller to select the drive
    if (read_status_register_with_mask(drive.controller, ATA_STATUS_BSY | ATA_STATUS_DRQ, 0, 10000)
            .fetch_failed) {
        return false;
    }

    // Set it as the current selected drive
    _selected_drive = drive;

    return true;
}

void influx::drivers::ata::ata::delay(const influx::drivers::ata::bus &controller) const {
    // Read the status register port 4 times to create a 400ns delay
    ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
}

influx::structures::string influx::drivers::ata::ata::ata_get_string(char *base, uint64_t size) {
    uint64_t end = 0;
    char temp = 0;
    char str_buffer[MAX_ATA_STRING_LENGTH] = {0};

    // Copy the characters
    memory::utils::memcpy(str_buffer, base, size);

    // Swap the characters in each byte
    for (uint64_t i = 0; i < size; i += 2) {
        temp = str_buffer[i];
        str_buffer[i] = str_buffer[i + 1];
        str_buffer[i + 1] = temp;
    }

    // Cleanup the output from blank characters
    for (end = size - 1; end > 0; end--) {
        // If found a valid ASCII character, break
        if (str_buffer[end] > 32 && str_buffer[end] < 127) {
            break;
        }
    }

    // Set the end of the string
    str_buffer[end + 1] = '\0';

    return str_buffer;
}

influx::drivers::ata::drive influx::drivers::ata::ata::identify_drive(
    const influx::drivers::ata::bus &controller, bool slave) {
    drive drive;
    status_register status;

    uint32_t identify_data[ATA_IDENTIFY_DATA_SIZE / sizeof(uint32_t)];

    // Set the drive as not present
    drive.present = false;

    // Verify the controller I/O ports
    ports::out<uint8_t>(0xAB, (uint16_t)(controller.io_base + ATA_IO_SECTOR_COUNT_REGISTER));
    ports::out<uint8_t>(0xCD, (uint16_t)(controller.io_base + ATA_IO_SECTOR_NUMBER_REGISTER));
    if (ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_SECTOR_COUNT_REGISTER)) != 0xAB ||
        ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_SECTOR_NUMBER_REGISTER)) !=
            0xCD) {
        return drive;
    }

    // Select the device
    ports::out<uint8_t>((uint8_t)(0xA0 | ((uint8_t)slave << 4)),
                        (uint16_t)(controller.io_base + ATA_IO_DRIVE_REGISTER));
    delay(controller);

    // Reset Sectorcount, LBAlo, LBAmid, and LBAhi IO ports
    ports::out<uint8_t>(0, (uint16_t)(controller.io_base + ATA_IO_SECTOR_COUNT_REGISTER));
    ports::out<uint8_t>(0, (uint16_t)(controller.io_base + ATA_IO_SECTOR_NUMBER_REGISTER));
    ports::out<uint8_t>(0, (uint16_t)(controller.io_base + ATA_IO_LCYL_REGISTER));
    ports::out<uint8_t>(0, (uint16_t)(controller.io_base + ATA_IO_HCYL_REGISTER));

    // Send the IDENTIFY command
    ports::out<uint8_t>((uint8_t)command::identify,
                        (uint16_t)(controller.io_base + ATA_IO_COMMAND_REGISTER));
    delay(controller);

    // Read the status register until BSY and DRQ are clear
    status = read_status_register_with_mask(controller, ATA_STATUS_BSY | ATA_STATUS_DRQ,
                                            ATA_STATUS_DRQ, 10000);
    if (status.raw == 0 || status.fetch_failed) {  // Drive not found
        return drive;
    }

    // Read the identify data from the data port
    for (uint8_t i = 0; i < ATA_IDENTIFY_DATA_SIZE / sizeof(uint32_t); i++) {
        identify_data[i] =
            ports::in<uint32_t>((uint16_t)(controller.io_base + ATA_IO_DATA_REGISTER));
    }

    // Set drive's properties
    drive.present = true;
    drive.controller = controller;
    drive.slave = slave;
    drive.size = *(uint32_t *)((char *)identify_data + ATA_IDENTIFY_DATA_SECTOR_NUMBER_OFFSET) *
                 ATA_SECTOR_SIZE;
    drive.model_number =
        ata_get_string((char *)identify_data + ATA_IDENTIFY_DATA_MODEL_STRING_OFFSET,
                       ATA_IDENTIFY_DATA_MODEL_STRING_SIZE);
    drive.serial_number =
        ata_get_string((char *)identify_data + ATA_IDENTIFY_DATA_SERIAL_STRING_OFFSET,
                       ATA_IDENTIFY_DATA_SERIAL_STRING_SIZE);
    drive.firmware_revision =
        ata_get_string((char *)identify_data + ATA_IDENTIFY_DATA_FIRMWARE_STRING_OFFSET,
                       ATA_IDENTIFY_DATA_FIRMWARE_STRING_SIZE);

    _log("Identified a drive (%S - %S) with size %x.\n", &drive.model_number, &drive.serial_number,
         drive.size);

    return drive;
}

influx::drivers::ata::status_register influx::drivers::ata::ata::read_status_register(
    const influx::drivers::ata::bus &controller) const {
    status_register status_reg{0, false};
    status_reg.raw = ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    return status_reg;
}

influx::drivers::ata::status_register influx::drivers::ata::ata::read_status_register_with_mask(
    const influx::drivers::ata::bus &controller, uint8_t mask, uint8_t value,
    uint64_t amount_of_tries) const {
    status_register status_reg = {0, false};

    do {
        // Wait 400ns
        delay(controller);

        // Read the status register
        status_reg.raw =
            ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    } while ((status_reg.raw & mask) != value && --amount_of_tries);

    // If amount of tries reached 0, reset the status register
    if (amount_of_tries == 0) {
        status_reg.fetch_failed = true;
    }

    return status_reg;
}