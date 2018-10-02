struct tm
{
    int tm_sec;      /* seconds after the minute   - [0,59]  */
    int tm_min;      /* minutes after the hour     - [0,59]  */
    int tm_hour;     /* hours after the midnight   - [0,23]  */
    int tm_mday;     /* day of the month           - [1,31]  */
    int tm_mon;      /* months since January       - [0,11]  */
    int tm_year;     /* years since 1900                     */
    int tm_wday;     /* days since Sunday          - [0,6]   */
    int tm_yday;     /* days since Jan 1st         - [0,365] */
    int tm_isdst;    /* Daylight Savings Time flag           */

};

inline unsigned long mktime(struct tm * time_ptr)
{
    const int mon_days[] =
          {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (time_ptr->tm_sec > 59)
    {
        time_ptr->tm_min += 1;
        time_ptr->tm_sec = 0;
    }
    if (time_ptr->tm_min > 59)
    {
        time_ptr->tm_hour += 1;
        time_ptr->tm_min = 0;
    }
    if (time_ptr->tm_hour > 23)
    {
        time_ptr->tm_yday += 1;
        time_ptr->tm_mday += 1;
        time_ptr->tm_hour = 0;
        if (time_ptr->tm_mday > mon_days[time_ptr->tm_mon])
        {
            time_ptr->tm_mon += 1;
            time_ptr->tm_mday = 1;
        }
    }
    if (time_ptr->tm_mon > 11 || time_ptr->tm_yday > 365)
    {
        time_ptr->tm_mon = 0;
        time_ptr->tm_yday = 0;
        time_ptr->tm_mday = 1;
    }
    return time_ptr->tm_yday * 86400L + time_ptr->tm_hour * 3600L
            + time_ptr->tm_min * 60L + time_ptr->tm_sec;
};

inline int convert_temp(char* dest, float temp, int units)
{
    int temp_i = (int)temp;
    dest[0] = '0' + temp_i / 100;
    dest[1] = '0' + temp_i / 10;
    dest[2] = '0' + temp_i % 10;
    dest[3] = '.';
    temp_i = (int)(temp * 10);
    dest[4] = '0' + temp_i % 10;
    dest[5] = ' ';
    if (units == 1)
        dest[6] = 'F';
    else
        dest[6] = 'C';
    dest[7] = '\0';
    return 0;
};

inline int convert_time(char* dest, struct tm * time_ptr)
{
    dest[0] = '0' + time_ptr->tm_hour / 10;
    dest[1] = '0' + time_ptr->tm_hour % 10;
    dest[2] = ':';
    dest[3] = '0' + time_ptr->tm_min / 10;
    dest[4] = '0' + time_ptr->tm_min % 10;
    dest[5] = ':';
    dest[6] = '0' + time_ptr->tm_sec / 10;
    dest[7] = '0' + time_ptr->tm_sec % 10;
    dest[8] = '\0';
    return 0;
};

/**
 * @brief Creates the formatted date.
 *
 * @param dest The destination date. Must be already allocated.
 * @param time_ptr The time count.
 *
 * @return 0 if success.
 */
inline int convert_date(char* dest, struct tm * time_ptr)
{
    switch (time_ptr->tm_mon)
    {
    case 0:
        dest[0] = 'J';
        dest[1] = 'a';
        dest[2] = 'n';
        break;
    case 1:
        dest[0] = 'F';
        dest[1] = 'e';
        dest[2] = 'b';
        break;
    case 2:
        dest[0] = 'M';
        dest[1] = 'a';
        dest[2] = 'r';
        break;
    case 3:
        dest[0] = 'A';
        dest[1] = 'p';
        dest[2] = 'r';
        break;
    case 4:
        dest[0] = 'M';
        dest[1] = 'a';
        dest[2] = 'y';
        break;
    case 5:
        dest[0] = 'J';
        dest[1] = 'u';
        dest[2] = 'n';
        break;
    case 6:
        dest[0] = 'J';
        dest[1] = 'u';
        dest[2] = 'l';
        break;
    case 7:
        dest[0] = 'A';
        dest[1] = 'u';
        dest[2] = 'g';
        break;
    case 8:
        dest[0] = 'S';
        dest[1] = 'e';
        dest[2] = 'p';
        break;
    case 9:
        dest[0] = 'O';
        dest[1] = 'c';
        dest[2] = 't';
        break;
    case 10:
        dest[0] = 'N';
        dest[1] = 'o';
        dest[2] = 'v';
        break;
    case 11:
        dest[0] = 'D';
        dest[1] = 'e';
        dest[2] = 'c';
        break;
    }
    dest[3] = ' ';
    dest[4] = '0' + time_ptr->tm_mday / 10;
    dest[5] = '0' + time_ptr->tm_mday % 10;
    dest[6] = '\0';
    return 0;
};

inline void increment_tm(struct tm * time_ptr, int seconds)
{
    time_ptr->tm_sec += seconds;
    mktime(time_ptr);
};

inline void initialize_tm(struct tm * time_ptr)
{
    const int mon_days[] =
          {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    unsigned long rawtime = 3298882;
    time_ptr->tm_yday = rawtime / 86400;
    rawtime -= time_ptr->tm_yday * 86400;
    time_ptr->tm_hour = rawtime / 3600;
    rawtime -= time_ptr->tm_hour * 3600;
    time_ptr->tm_min = rawtime / 60;
    rawtime -= time_ptr->tm_min * 60;
    time_ptr->tm_sec = rawtime;
    time_ptr->tm_year = 0;
    int yday = time_ptr->tm_yday;
    if (yday <= mon_days[0])
        time_ptr->tm_mon = 0;
    else
    {
        int i;
        for (i = 1; i <= 11; i++)
        {
            yday -= mon_days[i-1];
            if (yday <= mon_days[i])
            {
                time_ptr->tm_mon = i;
                break;
            }
        }
    }
    time_ptr->tm_mday = yday;
    mktime(time_ptr);
};

inline long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    (x > in_max) ? x = in_max : (x < in_min) ? x = in_min : 1;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
};
