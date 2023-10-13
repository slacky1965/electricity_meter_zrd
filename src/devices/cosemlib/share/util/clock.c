#include "clock.h"
#include "csm_array.h"

#define CLOCK_DEFAULT_YEAR  2000U
#define CLOCK_DEFAULT_MONTH 1U
#define CLOCK_DEFAULT_DAY   1U
#define CLOCK_DEFAULT_HOUR  0U
#define CLOCK_DEFAULT_MINUTE 0U
#define CLOCK_DEFAULT_SECOND 0U


static const uint32_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


/*
static uint16_t clock_deviation = 0U;
static uint8_t clock_dst = 0U;
static uint8_t clock_status;
*/

/* scalar date routines    --    public domain by Ray Gardner
** These will work over the range 1-01-01 thru 14699-12-31
** The functions written by Ray are isleap, months_to_days,
** years_to_days, ymd_to_scalar, scalar_to_ymd.
** modified slightly by Paul Edwards
*/

static int isleap (uint32_t yr) {
  return yr % 400 == 0 || (yr % 4 == 0 && yr % 100 != 0);
}

static uint32_t months_to_days (uint32_t month) {
  return (month * 3057 - 3007) / 100;
}

static uint32_t years_to_days (uint32_t yr) {
  return yr * 365L + yr / 4 - yr / 100 + yr / 400;
}

static uint32_t ymd_to_scalar (uint32_t yr, uint32_t mo, uint32_t day) {
  uint32_t scalar;

  scalar = day + months_to_days(mo);
  if (mo > 2) /* adjust if past February */
    scalar -= isleap(yr) ? 1 : 2;
  yr--;
  scalar += years_to_days(yr);
  return scalar;
}

static void scalar_to_ymd (uint32_t scalar,
               uint32_t *pyr,
               uint32_t *pmo,
               uint32_t *pday) {
  uint32_t n; // compute inverse of years_to_days()

  n = (uint32_t)((scalar * 400L) / 146097L);
  while ((uint32_t)years_to_days(n) < scalar) {
    n++;
  }
  for ( n = (uint32_t)((scalar * 400L) / 146097L); (uint32_t)years_to_days(n) < scalar; )
    n++; // 146097 == years_to_days(400)
  *pyr = n;
  n = (uint32_t)(scalar - years_to_days(n-1));
  if ( n > 59 ) { // adjust if past February
    n += 2;
    if ( isleap(*pyr) )
      n -= n > 61 ? 1 : 2;
  }
  *pmo = (n * 100 + 3007) / 3057; // inverse of months_to_days()
  *pday = n - months_to_days(*pmo);
  return;
}

uint32_t clk_to_epoch(struct tm *timeptr)
{
  uint32_t tt;

  if ((timeptr->tm_year < 70) || (timeptr->tm_year > 120)) {
    tt = (time_t)-1;
  } else {
    tt = ymd_to_scalar(timeptr->tm_year + 1900,
               timeptr->tm_mon + 1,
               timeptr->tm_mday)
      - ymd_to_scalar(1970, 1, 1);
    tt = tt * 24 + timeptr->tm_hour;
    tt = tt * 60 + timeptr->tm_min;
    tt = tt * 60 + timeptr->tm_sec;
  }
  return tt;
}

/*
**  Return the day of the week
*/

uint32_t clk_dow(uint32_t yr, uint32_t mo, uint32_t day)
{
      return (uint32_t)((ymd_to_scalar(yr, mo, day) - 1L) % 7L);
}

uint32_t clk_last_dow(uint32_t yr, uint32_t mo, uint32_t dow)
{
    uint32_t last_day = 0U;

    if (clk_is_valid_month(mo))
    {
        // First get the dow of the last day of the month
        last_day = days[mo -1];
        uint32_t last_dow = clk_dow(yr, mo, last_day);

        if (dow <= last_dow)
        {
            last_day -= last_dow - dow;
        }
        else
        {
            last_day -= 7 - (dow - last_dow);
        }
    }
    return last_day;
}


/*
  Note: Pebble doesn't think about timezones. So we pass a time_t to
  gmtime that's already been offset for the desired TZ, and get a struct tm
  object back in that TZ. I don't know how seedy actual developers will
  think that is.
*/

// timer is the number of seconds from epoch (unix time)
void clk_to_datetime(const uint32_t timer, struct tm *tms)
{
    uint32_t yr, mo, da;
    uint32_t secs, days;

    days = timer / (60L*60*24);
    secs = timer % (60L*60*24);
    scalar_to_ymd(days + ymd_to_scalar(1970, 1, 1), &yr, &mo, &da);

    tms->tm_year = yr - 1900;
    tms->tm_mon = mo - 1;
    tms->tm_mday = da;
    tms->tm_yday = (int)(ymd_to_scalar(tms->tm_year + 1900, mo, da)
              - ymd_to_scalar(tms->tm_year + 1900, 1, 1));
    tms->tm_wday = clk_dow(tms->tm_year + 1900, mo, da);
    tms->tm_isdst = -1;
    tms->tm_sec = (int)(secs % 60);
    secs /= 60;
    tms->tm_min = (int)(secs % 60);
    secs /= 60;
    tms->tm_hour = (int)secs;
}


uint32_t clk_is_valid_month(uint32_t mo)
{
    uint32_t ret = 0;

    if ((mo >=1) && (mo <= 12))
    {
        ret = 1;
    }

    return ret;
}

/*
**  Determine if a given date is valid
*/

uint32_t clk_is_valid_date(uint32_t yr, uint32_t mo, uint32_t day)
{
    uint32_t ret = 0;

    if (clk_is_valid_month(mo))
    {
        if ((day >= 1) && (day <= (days[mo - 1] + (2 == mo && isleap(yr)))))
        {
            ret = 1;
        }
    }
    return ret;
}

uint8_t clk_is_valid_time(uint8_t h, uint8_t m, uint8_t s)
{
    uint8_t ret = 1U;

    if (h > 23U)
    {
        ret = 0U;
    }
    if (m > 59U)
    {
        ret = 0U;
    }
    if (s > 59U)
    {
        ret = 0U;
    }
    return ret;
}

uint8_t clk_is_valid_deviation(int16_t d)
{
    uint8_t ret = 1U;

    if (d < -720)
    {
        ret = 0U;
    }
    if (d > 720)
    {
        ret = 0U;
    }
    return ret;
}


/*
**  Return the day of the year (1 - 365/6)
*/

uint32_t clk_daynum(uint32_t year, uint32_t month, uint32_t day)
{
    uint32_t jan1date = ymd_to_scalar(year, 1, 1);
    return ymd_to_scalar(year, month, day) - jan1date + 1L;
}

/*
**  Return the week of the year (1 - 52, 0 - 52 if ISO)
*/

uint32_t clk_weeknum(uint32_t year, uint32_t month, uint32_t day)
{
    uint32_t wn, j1n, dn = clk_daynum(year, month, day);
    uint32_t jan1date = ymd_to_scalar(year, 1, 1);

    dn += (j1n = (uint32_t)((jan1date - (uint32_t)1) % 7L)) - 1;
    wn = dn / 7;
    wn += (j1n < 4);
    return wn;
}

/*
**  Return the phase of the moon (0 -> 7, 0 = new, 4 = full)
*/

char *MoonPhaseText[8] = {"new", "waxing crescent", "first quarter",
      "waxing gibbous", "full", "waning gibbous", "third quarter",
      "waning crescent"
      };

uint32_t moonphase(uint32_t yr, uint32_t mo, uint32_t dy)
{
      uint32_t date = (uint32_t)ymd_to_scalar(yr, mo, dy);

      date *= 9693L;
      date /= 35780L;
      date -= 4L;
      date %= 8L;
      return (uint32_t)date;
}




/*
**  ADDTIME.C - Add a time period to a base time, normalizing the result
**
**  arguments: 1  - Base hours (0 -> 24)
**             2  - Base minutes (0 -> 59)
**             3  - Base seconds (0 -> 59)
**             4  - Span hours
**             5  - Span minutes
**             6  - Span seconds
**             7  - Address of returned hours (0 -> 23)
**             9  - Address of returned minutes (0 -> 59)
**             10 - Address of returned seconds (0 -> 59)
**             11 - Address of number of days to add to result
**
**  returns: 0 if no error, non-zero if base time range error
**
**  Notes: 1) Span values may be negative.
**         2) Overflowing midnight will cause a positive number of days to be
**            returned.
**         3) Underflowing midnight will cause a negative number of days to be
**            returned.
**
**  Original Copyright 1994 by Bob Stout as part of
**  the MicroFirm Function Library (MFL)
**
**  The user is granted a free limited license to use this source file
**  to create royalty-free programs, subject to the terms of the
**  license restrictions specified in the LICENSE.MFL file.
*/

//#include <stdlib.h>

int add_time(uint32_t basehrs, uint32_t basemins, uint32_t basesecs,
             int spanhrs, int spanmins, int spansecs,
             uint32_t *hrs, uint32_t *mins, uint32_t *secs, uint32_t *days)
{
    int h, m, s;
      div_t r;

      if (basehrs > 24 || basemins > 59 || basesecs > 59)
            return -1;

      if (24 == basehrs)
            basehrs = 0;
      h = (int)basehrs  + spanhrs;
      m = (int)basemins + spanmins;
      s = (int)basesecs + spansecs;

      r = div(s, 60);
      if (s < 0)
      {
            r.rem += 60;
            --r.quot;
      }
      *secs = r.rem;
      m += r.quot;

      r = div(m, 60);
      if (m < 0)
      {
            r.rem += 60;
            --r.quot;
      }
      *mins = r.rem;
      h += r.quot;

      r = div(h, 24);
      if (h < 0)
      {
            r.rem += 24;
            --r.quot;
      }
      *hrs = r.rem;
      *days = r.quot;

      return 0;
}

/*
**  Encode DST rules here - defaults to standard U.S. rules.
*/

uint32_t    DST_start_mo = 4;             /* Month when DST starts      */
uint32_t    DST_start_dt = 1;             /* Date when it can start     */
enum DOW_T  DST_start_dy = SUNDAY;        /* Day of week, or DOW_IGNORE */

uint32_t    DST_stop_mo = 10;             /* Month when DST stops       */
uint32_t    DST_stop_dt = 31;             /* Date when it can stop      */
enum DOW_T  DST_stop_dy = SUNDAY;         /* Day of week, or DOW_IGNORE */

/*
**  clk_is_dst()
**
**  Parameters: 1 - Year of interest.
**              2 - Month to check.
**              3 - Day to check.
**              4 - Pointer to storage for scalar date representation of
**                  the year's DST start date.
**              5 - Pointer to storage for scalar date representation of
**                  the year's DST stop date.
**
**  Returns: 1 if date is in DST range
**           0 if date is not in DST  range
**           -1 if date is invalid,
*/

int clk_is_dst(uint32_t  yr,
            uint32_t  mo,
            uint32_t  dy)
{
      uint32_t date;
      uint32_t Start;
      uint32_t Stop;

      if (!clk_is_valid_date(yr, mo, dy))
            return -1;
      else  date = ymd_to_scalar(yr, mo, dy);

      Start = ymd_to_scalar(yr, DST_start_mo, DST_start_dt);
      Stop  = ymd_to_scalar(yr, DST_stop_mo, DST_stop_dt);

      if (DST_start_dy != DOW_IGNORE)
      {
            while (DST_start_dy != (enum DOW_T)(Start % 7U))
            {
                  ++Start;
            }
      }

      if (DST_stop_dy != DOW_IGNORE)
      {
            while (DST_stop_dy != (enum DOW_T)(Stop % 7U))
            {
                  --Stop;
            }
      }

      return ((date >= Start) && (date < Stop));
}

void clk_cosem_update_status(clk_datetime_t *clk)
{
    clk->status = 0U;
    uint8_t invalid_bit = 0U;
    uint8_t dst_bit = 0U;

    int isDst = clk_is_dst(clk->date.year, clk->date.month, clk->date.day);

    if (!clk_is_valid_date(clk->date.year, clk->date.month, clk->date.day) ||
        !clk_is_valid_time(clk->time.hour, clk->time.minute, clk->time.second) ||
        !clk_is_valid_deviation(clk->deviation) ||
        (isDst < 0))
    {
        invalid_bit = 1;
    }
    else
    {
        if (isDst)
        {
            dst_bit = 1U;
        }
    }

    clk->status = (dst_bit << 7U) | invalid_bit;
}

void clk_cosem_init(clk_datetime_t *clk)
{
    // Default date-time parameters
    clk->date.year = CLOCK_DEFAULT_YEAR;
    clk->date.month = CLOCK_DEFAULT_MONTH;
    clk->date.day = CLOCK_DEFAULT_DAY;
    clk->date.dow = clk_dow(CLOCK_DEFAULT_YEAR, CLOCK_DEFAULT_MONTH, CLOCK_DEFAULT_DAY);
    clk->time.hour = CLOCK_DEFAULT_HOUR;
    clk->time.minute = CLOCK_DEFAULT_MINUTE;
    clk->time.second = CLOCK_DEFAULT_SECOND;
    clk->time.hundredths = 0U;
    clk->deviation = -60;

    clk_cosem_update_status(clk);
}

// FFFFFFFFFFFFFFFFFF8000FF
void clk_set_undefined(clk_datetime_t *clk)
{
	clk->date.year = 0xFFFFU;
	clk->date.month = 0xFFU;
	clk->date.day = 0xFFU;
	clk->date.dow = 0xFFU;
	clk->time.hour = 0xFFU;
	clk->time.minute = 0xFFU;
	clk->time.second = 0xFFU;
	clk->time.hundredths = 0U;
	clk->deviation = -32768; // 0x8000
	clk->status = 0xFFU;
}

int clk_datetime_to_cosem(const clk_datetime_t *clk, csm_array *array)
{
    int valid = clk_date_to_cosem(&clk->date, array);
    valid = valid && clk_time_to_cosem(&clk->time, array);

    valid = valid && csm_array_write_u8(array, (uint8_t)(clk->deviation >> 8U) & 0xFFU);
    valid = valid && csm_array_write_u8(array, (uint8_t)(clk->deviation & 0xFFU));
    valid = valid && csm_array_write_u8(array, clk->status);

    return valid;
}

int clk_date_to_cosem(const clk_date_t *date, csm_array *array)
{
    int valid = csm_array_write_u16(array, date->year);
    valid = valid && csm_array_write_u8(array, date->month);
    valid = valid && csm_array_write_u8(array, date->day);
    valid = valid && csm_array_write_u8(array, date->dow);

    return valid;
}

int clk_time_to_cosem(const clk_time_t *time, csm_array *array)
{
    int valid = csm_array_write_u8(array, time->hour);
    valid = valid && csm_array_write_u8(array, time->minute);
    valid = valid && csm_array_write_u8(array, time->second);
    valid = valid && csm_array_write_u8(array, time->hundredths);

    return valid;
}

int clk_datetime_from_cosem(clk_datetime_t *clk, csm_array *array)
{
    int valid = clk_date_from_cosem(&clk->date, array);
    valid = valid && clk_time_from_cosem(&clk->time, array);

    uint8_t deviationhi;
    uint8_t deviationlo;
    valid = valid && csm_array_read_u8(array, &deviationhi);
    valid = valid && csm_array_read_u8(array, &deviationlo);

    clk->deviation = (((uint16_t)deviationhi) << 8) + deviationlo;

    valid = valid && csm_array_read_u8(array, &clk->status);

    return valid;
}

int clk_date_from_cosem(clk_date_t *date, csm_array *array)
{
    int valid = csm_array_read_u16(array, &date->year);
    valid = valid && csm_array_read_u8(array, &date->month);
    valid = valid && csm_array_read_u8(array, &date->day);
    valid = valid && csm_array_read_u8(array, &date->dow);

    return valid;
}

int clk_time_from_cosem(clk_time_t *time, csm_array *array)
{
    int valid = csm_array_read_u8(array, &time->hour);
    valid = valid && csm_array_read_u8(array, &time->minute);
    valid = valid && csm_array_read_u8(array, &time->second);
    valid = valid && csm_array_read_u8(array, &time->hundredths);

    return valid;
}

void clk_print_datetime(const clk_datetime_t *clk)
{
    (void) clk;
}

void clk_print_date(const clk_date_t *date)
{
    (void) date;
}

void clk_print_time(const clk_time_t *time)
{
    (void) time;
}



