#pragma once
#include <kernel/drivers/driver.h>

#include <kernel/logger.h>
#include <kernel/structures/vector.h>
#include <pci_descriptor.h>
#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA 0xcfc

#define VENDOR_ID_OFFSET 0x0
#define DEVICE_ID_OFFSET 0x0
#define CLASS_CODE_OFFSET 0x8
#define SUBCLASS_OFFSET 0x8
#define PROG_IF_OFFSET 0x8
#define HEADER_TYPE_OFFSET 0xC
#define BAR0_OFFSET 0x10
#define BAR1_OFFSET 0x14
#define BAR2_OFFSET 0x18
#define BAR3_OFFSET 0x1C
#define BAR4_OFFSET 0x20
#define BAR5_OFFSET 0x24

#define AMOUNT_OF_BUSES 256
#define AMOUNT_OF_DEVICES_PER_BUS 32
#define AMOUNT_OF_FUNCTION_PER_DEVICE 8

namespace influx {
namespace drivers {
class pci : public driver {
   public:
    pci();

    virtual bool load();

    uint8_t read_config_byte(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint16_t read_config_word(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t read_config_dword(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset);

    void write_config_byte(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset,
                           uint8_t data);
    void write_config_word(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset,
                           uint16_t data);
    void write_config_dword(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset,
                            uint32_t data);

    const structures::vector<pci_descriptor_t>& descriptors();

   private:
    structures::vector<pci_descriptor_t> _descriptors;

    void detect_devices();

    uint32_t calc_address(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset);

    uint16_t get_vendor_id(uint16_t bus, uint8_t device, uint8_t function);
    uint16_t get_device_id(uint16_t bus, uint8_t device, uint8_t function);
    uint8_t get_class_code(uint16_t bus, uint8_t device, uint8_t function);
    uint8_t get_subclass(uint16_t bus, uint8_t device, uint8_t function);
    uint8_t get_prog_if(uint16_t bus, uint8_t device, uint8_t function);
    uint8_t get_header_type(uint16_t bus, uint8_t device, uint8_t function);
    uint32_t get_bar(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset);

    void detect_device(uint16_t bus, uint8_t device);
    void detect_function(uint16_t bus, uint8_t device, uint8_t function);
};
};  // namespace drivers
};  // namespace influx