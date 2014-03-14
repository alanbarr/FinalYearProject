/*******************************************************************************
* Copyright (c) 2014, Alan Barr
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "ch.h"
#include "hal.h"
#include "string.h"
#include "fyp.h"
#include "chprintf.h"

typedef enum {
    MON = 1,
    TUES,
    WED,
    THURS,
    FRI,
    SAT,
    SUNDAY
} rtcWeekday;
    
typedef struct {
    bool pm;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} rtcTimeInformation;

typedef struct {
    uint8_t year;
    uint8_t month;
    uint8_t date;
    rtcWeekday day;
} rtcDateInformation;

typedef struct {
    rtcTimeInformation time;
    rtcDateInformation date;
} rtcInformation;

#define DR_YT_SHIFT     20
#define DR_YU_SHIFT     16
#define DR_WDU_SHIFT    13
#define DR_MT_SHIFT     12
#define DR_MU_SHIFT     8
#define DR_DT_SHIFT     4
#define DR_DU_SHIFT     0

#define DR_YT_MASK      (0xF<<DR_YT_SHIFT)
#define DR_YU_MASK      (0XF<<DR_YU_SHIFT)
#define DR_MT_MASK      (0x1<<DR_MT_SHIFT)
#define DR_MU_MASK      (0xF<<DR_MU_SHIFT)
#define DR_DT_MASK      (0x3<<DR_DT_SHIFT)
#define DR_DU_MASK      (0xF<<DR_DU_SHIFT)
#define DR_WDU_MASK     (0x7<<DR_WDU_SHIFT)

#define TR_PM_SHIFT     22
#define TR_HT_SHIFT     20
#define TR_HU_SHIFT     16
#define TR_MNT_SHIFT    12
#define TR_MNU_SHIFT    8
#define TR_ST_SHIFT     4
#define TR_SU_SHIFT     0

#define TR_PM_MASK      (0x1<<TR_PM_SHIFT)
#define TR_HT_MASK      (0x3<<TR_HT_SHIFT)
#define TR_HU_MASK      (0xF<<TR_HU_SHIFT)
#define TR_MNT_MASK     (0x3<<TR_MNT_SHIFT)
#define TR_MNU_MASK     (0xF<<TR_MNU_SHIFT)
#define TR_ST_MASK      (0x3<<TR_ST_SHIFT)
#define TR_SU_MASK      (0xF<<TR_SU_SHIFT)

void rtcStore(RTCDriver * driver, const rtcInformation * info)
{
    RTCTime chRtcTime;
    uint32_t dateRegister = 0;
    uint32_t timeRegister = 0;
    uint32_t temp = 0;

    /* Date */
    temp = info->date.year / 10;                    /* Year Tens */
    dateRegister |= temp << DR_YT_SHIFT;
    temp = info->date.year - (temp * 10);           /* Year Units */
    dateRegister |= temp << DR_YU_SHIFT;
    dateRegister |= info->date.day << DR_WDU_SHIFT; /* Weekday */
    temp = info->date.month / 10;                   /* Month Tens */
    dateRegister |= temp << DR_MT_SHIFT;
    temp = info->date.month - (temp * 10);          /* Month Units */
    dateRegister |= temp << DR_MU_SHIFT;
    temp = info->date.date / 10;                    /* Date Tens */
    dateRegister |= temp << DR_DT_SHIFT;
    temp = info->date.date - (temp * 10);           /* Date Units */
    dateRegister |= temp << DR_DU_SHIFT;

    /* Time */
    temp = info->time.hour / 10;                    /* Hours Tens */
    timeRegister |= temp << TR_HT_SHIFT;
    temp = info->time.hour - (temp * 10);           /* Hours Units */
    timeRegister |= temp << TR_HU_SHIFT;
    temp = info->time.minute / 10;                  /* Minute Tens */
    timeRegister |= temp << TR_MNT_SHIFT;
    temp = info->time.minute - (temp * 10);         /* Minute Units */
    timeRegister |= temp << TR_MNU_SHIFT;
    temp = info->time.second / 10;                  /* Second Tens */
    timeRegister |= temp << TR_ST_SHIFT;
    temp = info->time.second - (temp * 10);         /* Second Units */
    timeRegister |= temp << TR_SU_SHIFT;

    if (info->time.pm == true)
    {
        timeRegister |= (1 << TR_PM_SHIFT);
    }
  
    memset(&chRtcTime, 0, sizeof(chRtcTime));
    chRtcTime.tv_date = dateRegister;
    chRtcTime.tv_time = timeRegister;
    chRtcTime.h12 = false;

    PRINT("Set: date reg: %X, time reg: %X", chRtcTime.tv_date, chRtcTime.tv_time);
    rtcSetTime(driver, &chRtcTime);
}


void rtcRetrieve(RTCDriver * driver, rtcInformation * info)
{
    RTCTime chRtcTime;
    
    uint8_t temp;

    rtcGetTime(driver, &chRtcTime);


    PRINT("Get: date reg: %X, time reg: %X", chRtcTime.tv_date, chRtcTime.tv_time);

    /* Date */
    temp = ((chRtcTime.tv_date & DR_YT_MASK) >> DR_YT_SHIFT) * 10;  /* Year */
    temp += (chRtcTime.tv_date & DR_YU_MASK) >> DR_YU_SHIFT;
    info->date.year = temp;              
    temp = ((chRtcTime.tv_date & DR_MT_MASK) >> DR_MT_SHIFT) * 10;  /* Month */
    temp += (chRtcTime.tv_date & DR_MU_MASK) >> DR_MU_SHIFT;
    info->date.month = temp;              
    temp = ((chRtcTime.tv_date & DR_DT_MASK) >> DR_DT_SHIFT) * 10;  /* Date */
    temp += (chRtcTime.tv_date & DR_DU_MASK) >> DR_DU_SHIFT;
    info->date.date = temp;             
    temp = (chRtcTime.tv_date & DR_WDU_MASK) >> DR_WDU_SHIFT;       /* Weekday */
    info->date.day = temp;           


    /* Time */
    temp = ((chRtcTime.tv_time & TR_HT_MASK) >> TR_HT_SHIFT) * 10;  /* Hour */
    temp += (chRtcTime.tv_time & TR_HU_MASK) >> TR_HU_SHIFT;
    info->time.hour = temp;              
    temp = ((chRtcTime.tv_time & TR_MNT_MASK) >> TR_MNT_SHIFT) * 10; /* Minute */
    temp += (chRtcTime.tv_time & TR_MNU_MASK) >> TR_MNU_SHIFT;
    info->time.minute = temp;              
    temp = ((chRtcTime.tv_time & TR_ST_MASK) >> TR_ST_SHIFT) * 10;  /* Second */
    temp += (chRtcTime.tv_time & TR_SU_MASK) >> TR_SU_SHIFT;
    info->time.second = temp;             
}



void rtcTest(void)
{
    rtcInformation setInfo;
    rtcInformation getInfo;

    memset(&setInfo, 0, sizeof(setInfo));
    memset(&getInfo, 0, sizeof(getInfo));

    setInfo.time.pm = false;
    setInfo.time.hour = 20;
    setInfo.time.minute = 23;
    setInfo.time.second = 00;

    setInfo.date.year = 14;
    setInfo.date.month = 02;
    setInfo.date.date = 17;
    setInfo.date.day = MON;

    rtcStore(&RTC_DRIVER, &setInfo);

    chThdSleep(5000);

    rtcRetrieve(&RTC_DRIVER, &getInfo);
 
    if (
        getInfo.time.second > setInfo.time.second 
        &&
        getInfo.time.pm == setInfo.time.pm 
        &&
        getInfo.time.hour == setInfo.time.hour 
        &&
        getInfo.time.minute == setInfo.time.minute 
        &&
        getInfo.date.year == setInfo.date.year 
        &&
        getInfo.date.month == setInfo.date.month 
        &&
        getInfo.date.date == setInfo.date.date
        &&
        getInfo.date.day == setInfo.date.day 
    ) 
    {
        PRINT("As expected", NULL);
    }
    else
    {
 
        PRINT("NOT as expected", NULL);
    }
}
