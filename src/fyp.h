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

#ifndef __FYP_H__
#define __FYP_H__

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "clarity_api.h"

/* Serial  */
#define SERIAL_PORT             GPIOA
#define SERIAL_TX               9
#define SERIAL_RX               10
#define SERIAL_DRIVER           SD1

/* LEDs */
#define LED_PORT                GPIOB
#define LED_STATUS              7
#define LED_ERROR               6

/* Push Button */
#define BUTTON_PORT             GPIOA
#define BUTTON_PAD              0

/* I2C */
#define I2C_PORT                GPIOB
#define I2C_SCL                 10          
#define I2C_SDA                 11
#define I2C_DRIVER              I2CD2

/* RTC */
#define RTC_DRIVER              RTCD1

/* CC3000 */
#define CC3000_SPI_DRIVER       SPID2
#define CC3000_EXT_DRIVER       EXTD1


extern Mutex printMtx;

/* Enable / Disable chprintf's */
#define PRINT(fmt, ...)                                                     \
        chMtxLock(&printMtx);                                               \
        chprintf((BaseSequentialStream*)&SERIAL_DRIVER,                     \
                 "(%s:%d) " fmt "\n\r", __FILE__, __LINE__, __VA_ARGS__);   \
        chMtxUnlock();


int32_t updateRtcWithSntp(void);
void rtcRetrieve(RTCDriver * driver, clarityTimeDate * info);
void rtcStore(RTCDriver * driver, const clarityTimeDate * info);
int32_t configureRtcAlarmAndStandby(RTCDriver * rtcDriver, uint32_t seconds);

void initialiseSensorHw(void);
void deinitialiseSensorHw(void);

uint32_t httpGetPressure(const clarityHttpRequestInformation * info, 
                         clarityConnectionInformation * conn);
uint32_t httpGetTemperature(const clarityHttpRequestInformation * info, 
                            clarityConnectionInformation * conn);
uint32_t httpGetLux(const clarityHttpRequestInformation * info, 
                    clarityConnectionInformation * conn);
clarityError httpPostTemperature(clarityHttpPersistant * persistant);
clarityError httpPostLux(clarityHttpPersistant * persistant);


typedef enum {
    EEPROM_ERROR_OK     = 0,
    EEPROM_ERROR_TRUE   = 0,
    EEPROM_ERROR_FALSE  = 1,
    EEPROM_ERROR_CORRUPT= 2,
    EEPROM_ERROR_LOCK   = 3,
    EEPROM_ERROR_MISC   = 4
} eepromError;

eepromError eepromWasLastShutdownOk(void);
eepromError eepromAcknowledgeLastShutdownError(void);
eepromError eepromRecordUnresponsiveShutdown(void);
eepromError eepromWipeStore(void);


#endif /*__FYP_H__*/
