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
