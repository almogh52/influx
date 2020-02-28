#pragma once
#include <kernel/logger.h>
#include <kernel/structures/vector.h>
#include <kernel/drivers/driver.h>

namespace influx {
namespace drivers {
class driver_manager {
   public:
    driver_manager();

    void load_drivers();

    driver *get_driver(const structures::string &driver_name) const;

   private:
    logger _log;
    structures::vector<driver *> _drivers;
};
};  // namespace drivers
};  // namespace influx