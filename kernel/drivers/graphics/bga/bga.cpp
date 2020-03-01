#include <kernel/drivers/graphics/bga/bga.h>

#include <kernel/drivers/graphics/bga/constants.h>
#include <kernel/drivers/pci.h>
#include <kernel/kernel.h>
#include <kernel/memory/virtual_allocator.h>
#include <kernel/ports.h>
#include <memory/paging.h>

influx::drivers::graphics::bga::bga() : driver("BGA"), _video_memory(nullptr) {}

void influx::drivers::graphics::bga::load() {
    drivers::pci *pci_drv = (drivers::pci *)kernel::driver_manager()->get_driver("PCI");

    pci_descriptor_t bga_pci_descriptor = {};

    // Verify BGA available
    if(read_register(VBE_DISPI_INDEX_ID) != VBE_DISPI_ID0) {
        _log("BGA isn't available!\n");
        return;
    }

    // Search for the BGA PCI descriptor
    for (const pci_descriptor_t &pci_device : pci_drv->descriptors()) {
        if (pci_device.device_id == BGA_PCI_DEVICE_ID &&
            pci_device.vendor_id == BGA_PCI_VENDOR_ID) {
            bga_pci_descriptor = pci_device;
        }
    }

    // If the BGA PCI descriptor wasn't found
    if (bga_pci_descriptor.device_id != BGA_PCI_DEVICE_ID) {
        _log("BGA PCI device wasn't found!\n");
        return;
    }
    _log("BGA PCI descriptor found - %d:%d:%d.\n", bga_pci_descriptor.bus, bga_pci_descriptor.device,
         bga_pci_descriptor.function);

    // Map the video memory to virtual memory
    _video_memory = memory::virtual_allocator::allocate(
        SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t), PROT_WRITE | PROT_READ,
        bga_pci_descriptor.bar0 / PAGE_SIZE);
    _log("BGA video memory (%lx) mapped to %lx.\n", bga_pci_descriptor.bar0,
         _video_memory);
}

void influx::drivers::graphics::bga::enable(bool clear_screen) const {
    // Disable BGA
    write_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);

    // Set screen resolution
    write_register(VBE_DISPI_INDEX_XRES, SCREEN_WIDTH);
    write_register(VBE_DISPI_INDEX_YRES, SCREEN_HEIGHT);

    // Set bit depth
    write_register(VBE_DISPI_INDEX_BPP, VBE_DISPI_BPP_32);

    // Re-enable BGA
    write_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED |
                                               (clear_screen ? 0 : VBE_DISPI_NOCLEARMEM));
}

void influx::drivers::graphics::bga::disable() const {
    // Disable BGA
    write_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
}

uint16_t influx::drivers::graphics::bga::read_register(uint16_t register_index) const {
    ports::out<uint16_t>(register_index, VBE_DISPI_IOPORT_INDEX);
    return ports::in<uint16_t>(VBE_DISPI_IOPORT_DATA);
}

void influx::drivers::graphics::bga::write_register(uint16_t register_index, uint16_t value) const {
    ports::out<uint16_t>(register_index, VBE_DISPI_IOPORT_INDEX);
    ports::out<uint16_t>(value, VBE_DISPI_IOPORT_DATA);
}