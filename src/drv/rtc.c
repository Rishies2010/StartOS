#include "../drv/rtc.h"
#include "../libk/ports.h"
#include "../libk/debug/log.h"
#include <stdint.h>
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71
static volatile uint16_t millisecond_counter = 0;
static rtc_time_t boot_time;

/// @brief Read the CMOS Register to get the time values.
/// @param reg The register's address.
/// @return The time data returned via the CMOS_DATA register address.
static uint8_t read_cmos_register(uint8_t reg)
{
    outportb(CMOS_ADDRESS, reg);
    return inportb(CMOS_DATA);
}

/// @brief Converts the BCD data to binary format.
/// @param bcd The BCD data.
/// @return The BCD Data converted to its binary equivalent.
static uint8_t bcd_to_binary(uint8_t bcd)
{
    return ((bcd / 16) * 10) + (bcd & 0x0F);
}

/// @brief Enable the RTC Periodic Interrupts feature.
/// @param void
static void rtc_enable_periodic_updates(void)
{
    uint8_t prev = read_cmos_register(0x0B);
    outportb(CMOS_ADDRESS, 0x0B);
    outportb(CMOS_DATA, prev | 0x40);
    outportb(CMOS_ADDRESS, 0x0A);
    uint8_t rate = (read_cmos_register(0x0A) & 0xF0) | 0x06;
    outportb(CMOS_DATA, rate);
}

/// @brief Initializes the RTC Timesystem.
/// @param void
void rtc_initialize(void)
{
    boot_time = rtc_get_time();
    rtc_enable_periodic_updates();
    log("RTC Timesystem initialized.", 1, 0);
}

/// @brief Handles RTC Periodic Updates
/// @param void
void rtc_interrupt_handler(void)
{
    millisecond_counter++;
    if (millisecond_counter >= 1000)
    {
        millisecond_counter = 0;
    }
    read_cmos_register(0x0C);
}

/// @brief Used to get the RTC Time at the time of call.
/// @param void
/// @return the time object, based on the rtc_time_t structure, after getting the values
rtc_time_t rtc_get_time(void)
{
    rtc_time_t time;
    time.seconds = bcd_to_binary(read_cmos_register(0x00));
    time.minutes = bcd_to_binary(read_cmos_register(0x02));
    time.hours = bcd_to_binary(read_cmos_register(0x04));
    time.day = bcd_to_binary(read_cmos_register(0x07));
    time.month = bcd_to_binary(read_cmos_register(0x08));
    time.year = bcd_to_binary(read_cmos_register(0x09));
    time.milliseconds = millisecond_counter;
    return time;
}

/// @brief Used to calculate the total uptime (Time spent since OS Boot) of the OS.
/// @param start A pointer to a constant rtc_time_t structure representing the start time.
/// @param end A pointer to a constant rtc_time_t structure representing the end time.
/// @return The calculated uptime.
uint32_t rtc_calculate_uptime(const rtc_time_t *start, const rtc_time_t *end)
{
    uint32_t start_ms = start->hours * 3600000 + start->minutes * 60000 + start->seconds * 1000 + start->milliseconds;
    uint32_t end_ms = end->hours * 3600000 + end->minutes * 60000 + end->seconds * 1000 + end->milliseconds;
    if (end_ms < start_ms)
    {
        end_ms += 24 * 3600000;
    }
    return end_ms - start_ms;
}

/// @brief Stores the time of OS boot in a static variable.
/// @param void
/// @return The boot_time.
rtc_time_t rtc_boottime(void)
{
    return boot_time;
}

/// @brief Pauses the ongoing function for a given while.
/// @param time Time in milliseconds.
void sleep(uint32_t time)
{
    uint32_t seconds = time / 1000;
    uint16_t milliseconds = time % 1000;

    rtc_time_t start_time = rtc_get_time();

    uint32_t target_seconds = start_time.seconds + seconds;
    uint16_t target_milliseconds = start_time.milliseconds + milliseconds;

    if (target_milliseconds >= 1000)
    {
        target_milliseconds -= 1000;
        target_seconds++;
    }

    if (target_seconds >= 60)
    {
        target_seconds -= 60;
    }

    while (1)
    {
        rtc_time_t current_time = rtc_get_time();
        if (current_time.seconds > target_seconds ||
            (current_time.seconds == target_seconds && current_time.milliseconds >= target_milliseconds))
        {
            break;
        }
    }
}