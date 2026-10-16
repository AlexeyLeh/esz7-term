/*!
\file   timecode.h
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

#ifndef TIMECODE__H_
#define TIMECODE__H_
#ifdef LINUX_VARIANT

#include <time.h>

typedef enum TIME_TZ
{
    TZ_UTC,
    TZ_LOCAL
} TIME_TZ;

void get_time(struct timespec *tm);

const char *get_local_datetime(const struct timespec *tm);
// const char *get_utc_datetime(struct timespec *tm);
void diff__time(struct timespec *tm_begin, struct timespec *tm_end, struct timespec *diff);
const char *diff_time_tostr(const struct timespec *tm_begin, const struct timespec *tm_end);
const char *timespec_to_sec_str(const struct timespec *ts);

#endif // LINUX_VARIANT
#endif // TIMECODE__H_
