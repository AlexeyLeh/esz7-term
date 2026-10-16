/*!
\file   timecode.c
\author Aleksey Lekhtin
\date   05.11.2024
\brief  Time and date string
*/

/******************************************************************************
 * Copyright (C) 2023 - 2024 E-Service.  All rights reserved.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#include "cfg.h"

#ifdef LINUX_VARIANT

#include "timecode.h"
#include <stdio.h>
#include <string.h>

void get_time(struct timespec *tm)
{
    //memset(tm, 0, sizeof(struct timespec));
    int ret = clock_gettime(CLOCK_REALTIME, tm);
    if (ret == -1) {
        memset(tm, 0, sizeof(struct timespec));
        return;
    }
    tzset();
    tm->tv_sec += 3600 * 5; // GMT+5
}

static const char *get_datetime(const struct timespec *tm, TIME_TZ tz)
{
    static char buf[64] = {0};
    struct tm *ptr, gmtval;
    if (tz == TZ_LOCAL) {
        ptr = localtime_r(&tm->tv_sec, &gmtval);
    } else {
        ptr = gmtime_r(&tm->tv_sec, &gmtval);
    }
    if (!ptr) {
        buf[0] = '\0';
        return buf;
    }

    //strftime(buf, sizeof(buf) - 1, "%Y-%m-%d %H:%M:%S", ptr);
    strftime(buf, sizeof(buf) - 1, "%d-%m-%Y %H:%M:%S", ptr);
    return buf;
}

const char *get_local_datetime(const struct timespec *tm)
{
    return get_datetime(tm, TZ_LOCAL);
}

//const char *get_utc_datetime(const struct timespec *tm)
//{
//    return get_datetime(tm, TZ_UTC);
//}

//static const char *timespec_to_ns_str(const struct timespec *ts)
//{
//    static char buf[32] = {0};
//    // strftime(buf, sizeof(buf), "%T", localtime(&ts->tv_sec));
//    struct tm gmtval;
//    if (!gmtime_r(&ts->tv_sec, &gmtval)) {
//        buf[0] = '\0';
//        return buf;
//    }
//    size_t ln = strftime(buf, 20, "%T", &gmtval);
//    sprintf(buf + ln, ".%09ld", ts->tv_nsec);
//    return buf;
//}

const char *timespec_to_sec_str(const struct timespec *ts)
{
    static char buf[64] = {0};
    const int SECS_MINUTE = 60;
    const int SECS_HOUR = SECS_MINUTE * 60;
    const int SECS_DAY = SECS_HOUR * 24;

    int32_t t = ts->tv_sec;

    int day = t / SECS_DAY;
    t -= day * SECS_DAY;
    if (t < 0) {
        --day;
        t += SECS_DAY;
    }

    int hour = t / SECS_HOUR;
    t -= hour * SECS_HOUR;

    int min = t / SECS_MINUTE;
    t -= min * SECS_MINUTE;

    int sec = t;
    snprintf(buf, sizeof(buf) - 1, "%d:%d:%d:%d", day, hour, min, sec);
    return buf;
}

void diff__time(struct timespec *tm_begin, struct timespec *tm_end, struct timespec *diff)
{
    diff->tv_sec = tm_end->tv_sec - tm_begin->tv_sec;
    diff->tv_nsec = tm_end->tv_nsec - tm_begin->tv_nsec;
    if (diff->tv_nsec < 0) {
        diff->tv_nsec += 1000000000; // nsec/sec
        diff->tv_sec--;
    }
}

const char *diff_time_tostr(const struct timespec *tm_begin, const struct timespec *tm_end)
{
    static struct timespec tm;
    tm.tv_sec = tm_end->tv_sec - tm_begin->tv_sec;
    tm.tv_nsec = tm_end->tv_nsec - tm_begin->tv_nsec;
    if (tm.tv_nsec < 0) {
        tm.tv_nsec += 1000000000; // nsec/sec
        tm.tv_sec--;
    }
    return timespec_to_sec_str(&tm);
}

#endif // LINUX_VARIANT
