#include <kernel/drivers/driver_manager.h>

#include <kernel/algorithm.h>
#include <kernel/drivers/ata/ata.h>
#include <kernel/drivers/graphics/bga/bga.h>
#include <kernel/drivers/pci.h>
#include <kernel/drivers/ps2_keyboard.h>
#include <kernel/drivers/time/cmos.h>
#include <kernel/drivers/time/pit.h>

influx::drivers::driver_manager::driver_manager() : _log("Driver Manager", console_color::green) {
    _drivers.push_back(new pit());
    _drivers.push_back(new pci());
    _drivers.push_back(new ata::ata());
    _drivers.push_back(new graphics::bga());
    _drivers.push_back(new cmos());
    _drivers.push_back(new ps2_keyboard());
}

void influx::drivers::driver_manager::load_drivers() {
    structures::vector<driver *> failed_drivers;

    _log("Loading drivers..\n");

    // Try to load all drivers
    for (driver *drv : _drivers) {
        // Load the driver
        _log("Loading %S driver..\n", drv->driver_name());
        if (!drv->load()) {
            _log("Failed to load %S driver.. Unloading..\n", drv->driver_name());
            failed_drivers.push_back(drv);
        }
    }

    // Unload all failed drivers
    for (driver *drv : failed_drivers) {
        _drivers.erase(algorithm::find(_drivers.begin(), _drivers.end(), drv));
        delete drv;
    }

    _log("Finished loading drivers..\n");
}

influx::drivers::driver *influx::drivers::driver_manager::get_driver(
    const influx::structures::string &driver_name) const {
    // For each driver check if it's name matches the name of the wanted driver
    for (driver *drv : _drivers) {
        if (drv->driver_name() == driver_name) {
            return drv;
        }
    }

    return nullptr;
}