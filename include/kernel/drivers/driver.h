#pragma once
#include <kernel/logger.h>
#include <kernel/structures/string.h>

namespace influx {
namespace drivers {
class driver {
   public:
    driver(const structures::string& driver_name)
        : _log(driver_name + " Driver"), _driver_name(driver_name){};
    virtual ~driver(){};
    virtual void load() = 0;

    const structures::string& driver_name() const { return _driver_name; };

   protected:
    logger _log;

   private:
    structures::string _driver_name;
};
};  // namespace drivers
};  // namespace influx