#include <kernel/drivers/driver_manager.h>

#include <kernel/drivers/pci.h>

influx::drivers::driver_manager::driver_manager() : _log("Driver Manager")
{
	_drivers.push_back(new pci());
}

void influx::drivers::driver_manager::load_drivers()
{
	for (driver *drv : _drivers)
	{
		// Load the driver
		_log("Loading %S driver..\n", drv->driver_name());
		drv->load();
	}
}