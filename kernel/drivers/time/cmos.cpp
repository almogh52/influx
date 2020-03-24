#include <kernel/drivers/time/cmos.h>

#include <kernel/ports.h>

const unsigned short int mon_yday[2][13] = {
    /* Normal years.  */
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
    /* Leap years.  */
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}};

influx::drivers::cmos::cmos() : driver("CMOS") {}

void influx::drivers::cmos::load() {
    // TODO: Check if CMOS has century register using ACPICA
}

uint64_t influx::drivers::cmos::get_unix_timestamp() const {
    uint64_t second = 0, minute = 0, hour = 0, day = 0, month = 0, year = 0, century = 0;
    uint64_t last_second = 0, last_minute = 0, last_hour = 0, last_day = 0, last_month = 0,
             last_year = 0, last_century = 0;

    uint8_t status_register_b = read_rtc_register(CMOS_RTC_STATUS_REGISTER_B);

    uint64_t leap_years = 0, unix_timestamp = 0;

    // Wait for update
    while (update_in_progress())
        ;

    // First read of date
    second = read_rtc_register(CMOS_RTC_SECOND_REGISTER);
    minute = read_rtc_register(CMOS_RTC_MINUTE_REGISTER);
    hour = read_rtc_register(CMOS_RTC_HOUR_REGISTER);
    day = read_rtc_register(CMOS_RTC_DAY_REGISTER);
    month = read_rtc_register(CMOS_RTC_MONTH_REGISTER);
    year = read_rtc_register(CMOS_RTC_YEAR_REGISTER);
    century = read_rtc_register(CMOS_RTC_CENTURY_REGISTER);

    // Reading RTC registers until we get the same values twice in a row
    // This is used to avoid getting dodgy/inconsistent values due to RTC updates
    do {
        // Save the previous date
        last_second = second;
        last_minute = minute;
        last_hour = hour;
        last_day = day;
        last_month = month;
        last_year = year;
        last_century = century;

        // Make sure an update isn't in progress
        while (update_in_progress())
            ;

        // Read date
        second = read_rtc_register(CMOS_RTC_SECOND_REGISTER);
        minute = read_rtc_register(CMOS_RTC_MINUTE_REGISTER);
        hour = read_rtc_register(CMOS_RTC_HOUR_REGISTER);
        day = read_rtc_register(CMOS_RTC_DAY_REGISTER);
        month = read_rtc_register(CMOS_RTC_MONTH_REGISTER);
        year = read_rtc_register(CMOS_RTC_YEAR_REGISTER);
        century = read_rtc_register(CMOS_RTC_CENTURY_REGISTER);
    } while ((last_second != second) || (last_minute != minute) || (last_hour != hour) ||
             (last_day != day) || (last_month != month) || (last_year != year) ||
             (last_century != century));

    // Check if RTC is in BCD mode
    if (!(status_register_b & CMOS_RTC_BINARY_MODE)) {
        second = (second & 0x0F) + ((second / 16) * 10);
        minute = (minute & 0x0F) + ((minute / 16) * 10);
        hour = ((hour & 0x0F) + (((hour & 0x70) / 16) * 10)) | (hour & 0x80);
        day = (day & 0x0F) + ((day / 16) * 10);
        month = (month & 0x0F) + ((month / 16) * 10);
        year = (year & 0x0F) + ((year / 16) * 10);
        century = (century & 0x0F) + ((century / 16) * 10);
    }

    // Convert 12 hour clock to 24 hour clock if necessary
    if (!(status_register_b & CMOS_RTC_24_HOUR) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    // Calculate the full (4-digit) year
    year += century * 100;

    // Calculate amount of leap years
    leap_years = amount_of_leap_years(year);

    // Calculate unix timestamp
    unix_timestamp += ((year - UNIX_TIMESTAMP_YEAR) - leap_years) * mon_yday[0][AMOUNT_OF_MONTHS] *
                      DAY_SECONDS;                                               // Regular years
    unix_timestamp += leap_years * mon_yday[1][AMOUNT_OF_MONTHS] * DAY_SECONDS;  // Leap years
    unix_timestamp += (((year % 4 == 0) ? mon_yday[1][month - 1] : mon_yday[0][month - 1]) + day - 1) *
                      DAY_SECONDS;              // Days since year start
    unix_timestamp += hour * HOUR_SECONDS;      // Hours
    unix_timestamp += minute * MINUTE_SECONDS;  // Minute
    unix_timestamp += second;                   // Second

    return unix_timestamp;
}

uint8_t influx::drivers::cmos::read_rtc_register(uint8_t reg) const {
    ports::out<uint8_t>(reg, CMOS_REGISTER_PORT);

    return ports::in<uint8_t>(CMOS_DATA_PORT);
}

bool influx::drivers::cmos::update_in_progress() const {
    return read_rtc_register(CMOS_RTC_STATUS_REGISTER_A) & CMOS_RTC_UPDATE_IN_PROGRESS;
}

uint64_t influx::drivers::cmos::amount_of_leap_years(uint64_t current_year) const {
    return ((current_year - FIRST_LEAP_YEAR_SINCE_1970) / 4) +
           ((current_year - FIRST_LEAP_YEAR_SINCE_1970) % 4 ? 1 : 0);
}