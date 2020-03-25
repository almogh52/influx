#pragma once
#include <kernel/drivers/driver.h>

#define CMOS_REGISTER_PORT 0x70
#define CMOS_DATA_PORT 0x71

#define CMOS_RTC_SECOND_REGISTER 0x0
#define CMOS_RTC_MINUTE_REGISTER 0x2
#define CMOS_RTC_HOUR_REGISTER 0x4
#define CMOS_RTC_DAY_REGISTER 0x7
#define CMOS_RTC_MONTH_REGISTER 0x8
#define CMOS_RTC_YEAR_REGISTER 0x9
#define CMOS_RTC_CENTURY_REGISTER 0x32
#define CMOS_RTC_STATUS_REGISTER_A 0xA
#define CMOS_RTC_STATUS_REGISTER_B 0xB

#define CMOS_RTC_UPDATE_IN_PROGRESS 0x80

#define CMOS_RTC_24_HOUR 0x2
#define CMOS_RTC_BINARY_MODE 0x4

#define MINUTE_SECONDS 60
#define HOUR_SECONDS (MINUTE_SECONDS * 60)
#define DAY_SECONDS (HOUR_SECONDS * 24)

#define AMOUNT_OF_MONTHS 12

#define UNIX_TIMESTAMP_YEAR 1970

#define FIRST_LEAP_YEAR_SINCE_1970 1972
#define AMOUNT_OF_LEAP_SECONDS_SINCE_1970 27

namespace influx {
namespace drivers {
class cmos : public driver {
   public:
    cmos();

    virtual bool load();

    uint64_t get_unix_timestamp() const;

   private:
    uint8_t read_rtc_register(uint8_t reg) const;
    bool update_in_progress() const;

    uint64_t amount_of_leap_years(uint64_t current_year) const;
};
};  // namespace drivers
};  // namespace influx