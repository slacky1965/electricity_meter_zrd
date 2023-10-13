#ifndef CLOCK_H
#define CLOCK_H


#ifdef __cplusplus
extern "C" {
#endif

//#include <stdio.h>
//#include <stdint.h>
#include "tl_common.h"
#include <time.h>


// Cosem stuff
#include "csm_axdr_codec.h"
#include "csm_array.h"


enum DOW_T {DOW_IGNORE = -1,
       MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY};


/*
Member	Type	Meaning	Range
tm_sec	int	seconds after the minute	0-60*
tm_min	int	minutes after the hour	0-59
tm_hour	int	hours since midnight	0-23
tm_mday	int	day of the month	1-31
tm_mon	int	months since January	0-11
tm_year	int	years since 1900
tm_wday	int	days since Sunday	0-6 or monday (ISO date)
tm_yday	int	days since January 1	0-365
tm_isdst	int	Daylight Saving Time flag

The Daylight Saving Time flag (tm_isdst) is greater than zero if Daylight Saving Time is in effect,
zero if Daylight Saving Time is not in effect, and less than zero if the information is not available.

tm_sec is generally 0-59. The extra range is to accommodate for leap seconds in certain systems.

*/

/**
 * The System Time is the time without any time zone or DST compensation.
 * BaseTime = SystemTime + TimeZone
 * LocalTime = BaseTime + DSTDeviation
 *
 */

typedef struct
{
    uint8_t hundredths;
    uint8_t second; // 0-59
    uint8_t minute; // 0-59
    uint8_t hour;   // 0-23
} clk_time_t;

typedef struct
{
    uint8_t day;    // 1-31
    uint8_t dow;    // Day of week [0..6]
    uint8_t month;  // 1-12
    uint16_t year;   // 0-99 (representing 2000-2099)
} clk_date_t;

typedef struct
{
    clk_date_t date;
    clk_time_t time;
    int16_t deviation;
    uint8_t status;
} clk_datetime_t;



// -------------------------------------- RAW UTILITY FUNCTIONS --------------------------------------

uint32_t clk_is_valid_month(uint32_t mo);
uint32_t clk_is_valid_date(uint32_t yr, uint32_t mo, uint32_t day);

// Return the day of the last dow of a given month/year
// Eg: last sunday of march 2017 is day 26
uint32_t clk_last_dow(uint32_t yr, uint32_t mo, uint32_t dow);
uint32_t clk_dow(uint32_t yr, uint32_t mo, uint32_t day);
uint32_t clk_daynum(uint32_t year, uint32_t month, uint32_t day);
uint32_t clk_weeknum(uint32_t year, uint32_t month, uint32_t day);

uint32_t clk_to_epoch(struct tm *timeptr);
void clk_to_datetime(const uint32_t timer, struct tm *tms);
int clk_is_dst(uint32_t yr, uint32_t mo, uint32_t dy);


// -------------------------------------- COSEM DATE TIME --------------------------------------

// Initializers
void clk_cosem_init(clk_datetime_t *clk);
void clk_set_undefined(clk_datetime_t *clk);

// Converters, return TRUE or FALSE
int clk_datetime_to_cosem(const clk_datetime_t *clk, csm_array *array);
int clk_date_to_cosem(const clk_date_t *date, csm_array *array);
int clk_time_to_cosem(const clk_time_t *time, csm_array *array);
int clk_datetime_from_cosem(clk_datetime_t *clk, csm_array *array);
int clk_date_from_cosem(clk_date_t *date, csm_array *array);
int clk_time_from_cosem(clk_time_t *time, csm_array *array);

// Printers
void clk_print_datetime(const clk_datetime_t *clk);
void clk_print_date(const clk_date_t *date);
void clk_print_time(const clk_time_t *time);



#if 0
// Calculate day of week in proleptic Gregorian calendar. Sunday == 0.
int wday(int year, int month, int day)
{
    int adjustment, mm, yy;

    adjustment = (14 - month) / 12;
    mm = month + 12 * adjustment - 2;
    yy = year - adjustment;
    return (day + (13 * mm - 1) / 5 +
        yy + yy / 4 - yy / 100 + yy / 400) % 7;
}

dayofweek(y, m, d)	/* 1 <= m <= 12,  y > 1752 (in the U.K.) */
    {
        static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        y -= m < 3;
        return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    }

// Last Friday of each month

int main(int c, char *v[])
{
    int days[] = {31,29,31,30,31,30,31,31,30,31,30,31};
    int m, y, w;

    if (c < 2 || (y = atoi(v[1])) <= 1700) return 1;
    days[1] -= (y % 4) || (!(y % 100) && (y % 400));
    w = y * 365 + (y - 1) / 4 - (y - 1) / 100 + (y - 1) / 400 + 6;

    for(m = 0; m < 12; m++) {
        w = (w + days[m]) % 7;
        printf("%d-%02d-%d\n", y, m + 1,
            days[m] + (w < 5 ? -2 : 5) - w);
    }

    return 0;
}

int main()
{
    int y;

    for (y = 2008; y <= 2121; y++) {
        if (wday(y, 12, 25) == 0) printf("%04d-12-25\n", y);
    }

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif // CLOCK_H

/**
  Zeller formula

There are two popular formulas that you can use to find the day of the week for a given date. You should be careful when you use these formulas, though, because they only work for the Gregorian calendar. (People in English-speaking countries used a different calendar before September 14, 1752.)

Zeller's Rule

The following formula is named Zeller's Rule after a Reverend Zeller. [x] means the greatest integer that is smaller than or equal to x. You can find this number by just dropping everything after the decimal point. For example, [3.79] is 3. Here's the formula:

    f = k + [(13*m-1)/5] + D + [D/4] + [C/4] - 2*C.
    k is the day of the month. Let's use January 29, 2064 as an example. For this date, k = 29.
    m is the month number. Months have to be counted specially for Zeller's Rule: March is 1, April is 2, and so on to February, which is 12. (This makes the formula simpler, because on leap years February 29 is counted as the last day of the year.) Because of this rule, January and February are always counted as the 11th and 12th months of the previous year. In our example, m = 11.
    D is the last two digits of the year. Because in our example we are using January (see previous bullet) D = 63 even though we are using a date from 2064.
    C stands for century: it's the first two digits of the year. In our case, C = 20.

Now let's substitute our example numbers into the formula.

    f = k + [(13*m-1)/5] + D + [D/4] + [C/4] - 2*C
    = 29 + [(13*11-1)/5] + 63 + [63/4] + [20/4] - 2*20
    = 29 + [28.4] + 63 + [15.75] + [5] - 40
    = 29 + 28 + 63 + 15 + 5 - 40
    = 100.

Once we have found f, we divide it by 7 and take the remainder. Note that if the result for f is negative, care must be taken in calculating the proper remainder. Suppose f = -17. When we divide by 7, we have to follow the same rules as for the greatest integer function; namely we find the greatest multiple of 7 less than -17, so the remainder will be positive (or zero). -21 is the greatest multiple of 7 less than -17, so the remainder is 4 since -21 + 4 = -17. Alternatively, we can say that -7 goes into -17 twice, making -14 and leaving a remainder of -3, then add 7 since the remainder is negative, so -3 + 7 is again a remainder of 4.

A remainder of 0 corresponds to Sunday, 1 means Monday, etc. For our example, 100 / 7 = 14, remainder 2, so January 29, 2064 will be a Tuesday.

   */
