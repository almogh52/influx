#include <kernel/drivers/pci.h>

#include <kernel/ports.h>

influx::drivers::pci::pci() : _log("PCI Driver") {}

void influx::drivers::pci::detect_devices() {
    // For each device in bus try to detect a device
    for (uint16_t bus = 0; bus < AMOUNT_OF_BUSES; ++bus) {
        for (uint8_t device = 0; device < AMOUNT_OF_DEVICES_PER_BUS; ++device) {
            detect_device(bus, device);
        }
    }
}

uint8_t influx::drivers::pci::read_config_byte(uint16_t bus, uint8_t device, uint8_t function,
                                               uint8_t offset) {
    uint32_t value = read_config_dword(bus, device, function, offset);

    return (value >> ((offset & 3) * 8)) & 0xff;
}

uint16_t influx::drivers::pci::read_config_word(uint16_t bus, uint8_t device, uint8_t function,
                                                uint8_t offset) {
    uint32_t value = read_config_dword(bus, device, function, offset);

    return (value >> ((offset & 3) * 8)) & 0xffff;
}

uint32_t influx::drivers::pci::read_config_dword(uint16_t bus, uint8_t device, uint8_t function,
                                                 uint8_t offset) {
    uint32_t address = calc_address(bus, device, function, offset);

    // Send the address location to PCI
    ports::out(address, PCI_CONFIG_ADDRESS);

    // Read the returned data from the PCI
    return ports::in<uint32_t>(PCI_CONFIG_DATA);
}

void influx::drivers::pci::write_config_byte(uint16_t bus, uint8_t device, uint8_t function,
                                             uint8_t offset, uint8_t data) {
    uint32_t dword = read_config_dword(bus, device, function, offset);

    // Insert the data in wanted offset
    dword &= ~(0xff << ((offset & 3) * 8));
    dword |= (data << ((offset & 3) * 8));

    // Write the updated dword
    write_config_dword(bus, device, function, offset, dword);
}

void influx::drivers::pci::write_config_word(uint16_t bus, uint8_t device, uint8_t function,
                                             uint8_t offset, uint16_t data) {
    uint32_t dword = read_config_dword(bus, device, function, offset);

    // Insert the data in wanted offset
    dword &= ~(0xffff << ((offset & 3) * 8));
    dword |= (data << ((offset & 3) * 8));

    // Write the updated dword
    write_config_dword(bus, device, function, offset, dword);
}

void influx::drivers::pci::write_config_dword(uint16_t bus, uint8_t device, uint8_t function,
                                              uint8_t offset, uint32_t data) {
    uint32_t address = calc_address(bus, device, function, offset);

    // Send the address location to PCI
    ports::out(address, PCI_CONFIG_ADDRESS);

    // Send the data to PCI
    ports::out(data, PCI_CONFIG_DATA);
}

const influx::structures::vector<pci_descriptor_t> &influx::drivers::pci::descriptors() {
    return _descriptors;
}

uint32_t influx::drivers::pci::calc_address(uint16_t bus, uint8_t device, uint8_t function,
                                            uint8_t offset) {
    return (1 << 31)                      // Enable bit
           | ((uint32_t)bus << 16)        // Bus number
           | (((uint32_t)device << 11))   // Device number
           | (((uint32_t)function << 8))  // Function number
           | ((uint32_t)offset & 0xfc);   // Register offset
}

uint16_t influx::drivers::pci::get_vendor_id(uint16_t bus, uint8_t device, uint8_t function) {
    uint32_t dword = read_config_dword(bus, device, function, VENDOR_ID_OFFSET);

    return (uint16_t)dword;
}

uint16_t influx::drivers::pci::get_device_id(uint16_t bus, uint8_t device, uint8_t function) {
    uint32_t dword = read_config_dword(bus, device, function, DEVICE_ID_OFFSET);

    return (uint16_t)(dword >> 16);
}

uint8_t influx::drivers::pci::get_class_code(uint16_t bus, uint8_t device, uint8_t function) {
    uint32_t dword = read_config_dword(bus, device, function, CLASS_CODE_OFFSET);

    return (uint8_t)(dword >> 24);
}

uint8_t influx::drivers::pci::get_subclass(uint16_t bus, uint8_t device, uint8_t function) {
    uint32_t dword = read_config_dword(bus, device, function, SUBCLASS_OFFSET);

    return (uint8_t)(dword >> 16);
}

uint8_t influx::drivers::pci::get_prog_if(uint16_t bus, uint8_t device, uint8_t function) {
    uint32_t dword = read_config_dword(bus, device, function, PROG_IF_OFFSET);

    return (uint8_t)(dword >> 8);
}

uint8_t influx::drivers::pci::get_header_type(uint16_t bus, uint8_t device, uint8_t function) {
    uint32_t dword = read_config_dword(bus, device, function, HEADER_TYPE_OFFSET);

    return (uint8_t)(dword >> 16);
}

void influx::drivers::pci::detect_device(uint16_t bus, uint8_t device) {
    uint16_t vendor_id = get_vendor_id(bus, device, 0);
    uint8_t header_type = get_header_type(bus, device, 0);

    // If the vendor id is invalid, ignore device
    if (vendor_id == 0xffff) {
        return;
    }

    // Detect the first function
    detect_function(bus, device, 0);

    // Check if the device has multiple functions
    if ((header_type & 0x80) != 0) {
        for (uint8_t function = 1; function < 8; function++) {
            detect_function(bus, device, function);
        }
    }
}

void influx::drivers::pci::detect_function(uint16_t bus, uint8_t device, uint8_t function) {
    pci_descriptor_t descriptor{.vendor_id = get_vendor_id(bus, device, function),
                                .device_id = get_device_id(bus, device, function),
                                .class_code = get_class_code(bus, device, function),
                                .subclass = get_subclass(bus, device, function),
                                .prog_if = get_prog_if(bus, device, function)};

    // Ignore invalid descriptors
    if (descriptor.vendor_id == 0xffff) {
        return;
    }

    // Print the device found
    _log("Found PCI device %d:%d:%d - Vendor: %d, Class: %d, Subclass: %d\n", bus, device,
         descriptor.device_id, descriptor.vendor_id, descriptor.class_code, descriptor.subclass);

    // Add the descriptor to the list of descriptors
    _descriptors += descriptor;
}