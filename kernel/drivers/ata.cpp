#include <kernel/drivers/ata.h>

#include <kernel/drivers/ata/command.h>
#include <kernel/drivers/ata/constants.h>
#include <kernel/memory/utils.h>
#include <kernel/ports.h>

influx::drivers::ata::ata() : driver("ATA") {}

void influx::drivers::ata::load() {
    _log("Detecting drives..\n");
    detect_drives();

    // If no drives were found
    if (_drives.empty()) {
        _log("No drives found!\n");
    } else {
        _log("%d drives were found.\n", _drives.size());
    }
}

void influx::drivers::ata::detect_drives() {
    ata_drive drive;

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

void influx::drivers::ata::delay(const influx::drivers::ata_bus &controller) const {
    // Read the status register port 4 times to create a 400ns delay
    ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
}

influx::structures::string influx::drivers::ata::ata_get_string(char *base, uint64_t size) {
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

influx::drivers::ata_drive influx::drivers::ata::identify_drive(
    const influx::drivers::ata_bus &controller, bool slave) {
    ata_drive drive;
    ata_status_register status;

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
    ports::out<uint8_t>((uint8_t)ata_command::identify,
                        (uint16_t)(controller.io_base + ATA_IO_COMMAND_REGISTER));
    delay(controller);

    // Read the status register until BSY and DRQ are clear
    status = read_status_register_with_mask(controller, ATA_STATUS_BSY | ATA_STATUS_DRQ,
                                            ATA_STATUS_DRQ, 10000);
    if (status.raw == 0) {  // Drive not found
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

influx::drivers::ata_status_register influx::drivers::ata::read_status_register(
    const influx::drivers::ata_bus &controller) const {
    ata_status_register status_reg;
    status_reg.raw = ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    return status_reg;
}

influx::drivers::ata_status_register influx::drivers::ata::read_status_register_with_mask(
    const influx::drivers::ata_bus &controller, uint8_t mask, uint8_t value,
    uint64_t amount_of_tries) const {
    ata_status_register status_reg = {0};

    do {
        // Wait 400ns
        delay(controller);

        // Read the status register
        status_reg.raw =
            ports::in<uint8_t>((uint16_t)(controller.io_base + ATA_IO_STATUS_REGISTER));
    } while ((status_reg.raw & mask) != value && --amount_of_tries);

    // If amount of tries reached 0, reset the status register
    if (amount_of_tries == 0) {
        status_reg = {0};
    }

    return status_reg;
}