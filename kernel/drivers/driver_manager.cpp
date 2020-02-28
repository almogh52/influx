#include <kernel/drivers/driver_manager.h>

#include <kernel/drivers/pci.h>
#include <kernel/drivers/ata.h>

influx::drivers::driver_manager::driver_manager() : _log("Driver Manager") {
    _drivers.push_back(new pci());
    _drivers.push_back(new ata());
}

void influx::drivers::driver_manager::load_drivers() {
    for (driver *drv : _drivers) {
        // Load the driver
        _log("Loading %S driver..\n", drv->driver_name());
        drv->load();
    }
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