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
#include "chprintf.h"

/* Serial  */
#define SERIAL_PORT             GPIOA
#define SERIAL_TX               9
#define SERIAL_RX               10
#define SERIAL_DRIVER           SD1

/* LEDs for notification setup */
#define LED_PORT                GPIOB
#define LED_STATUS              7
#define LED_ERROR               6

/* I2C */
#define I2C_PORT                GPIOB
#define I2C_SCL                 10          
#define I2C_SDA                 11
#define I2C_DRIVER              I2CD2
#define RTC_DRIVER              RTCD1


extern Mutex printMtx;

/* Enable / Disable chprintf's */
#define PRINT(fmt, ...)                                                     \
        chMtxLock(&printMtx);                                               \
        chprintf((BaseSequentialStream*)&SERIAL_DRIVER,                     \
                 "(%s:%d) " fmt "\n\r", __FILE__, __LINE__, __VA_ARGS__);   \
        chMtxUnlock();

#endif /*__FYP_H__*/
