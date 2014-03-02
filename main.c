#include "ch.h"
#include "hal.h"
#include "board.h"
#include "chstreams.h"
#include "chprintf.h"

#include "mpl3115a2.h"
#include "tsl2561.h"

#include <string.h>
#include "fyp.h"
#include "cc3000_server.h"

I2CConfig i2cConfig;
i2cflags_t errorFlags;
msg_t i2cReturn;

Mutex printMtx;

float pressure;
float temperature;
uint16_t lux;
uint16_t channel0;
uint16_t channel1;

uint8_t zero = 0x00;

static void i2cErrorHandler(void)
{
    palSetPad(LED_PORT, LED_ERROR);
    PRINT("i2cReturn: 0x%x.", i2cReturn);
    
    if (i2cReturn == RDY_RESET)
    {
        errorFlags = i2cGetErrors(&I2C_DRIVER);
        PRINT("Errors: 0x%x.", errorFlags);
    }
    while(1);
}


int main(void)
{
    controlInformation control;
    httpInformation http;

    halInit();
    chSysInit();

    /* Led for status */
    palClearPad(LED_PORT, LED_STATUS);
    palSetPadMode(LED_PORT, LED_STATUS, PAL_MODE_OUTPUT_PUSHPULL);
    
    /* Led for error */
    palClearPad(LED_PORT, LED_ERROR);
    palSetPadMode(LED_PORT, LED_ERROR, PAL_MODE_OUTPUT_PUSHPULL);
 
#if 1
    /* Serial for debugging */
    sdStart(&SERIAL_DRIVER, NULL);
    palSetPadMode(SERIAL_PORT, SERIAL_TX, PAL_MODE_ALTERNATE(7));
    palSetPadMode(SERIAL_PORT, SERIAL_RX, PAL_MODE_ALTERNATE(7));
#endif

    /* Pins for I2C */
    palSetPadMode(I2C_PORT, I2C_SDA, PAL_MODE_ALTERNATE(4) | 
                                     PAL_STM32_OTYPE_OPENDRAIN |
                                     PAL_STM32_OSPEED_LOWEST);
    palSetPadMode(I2C_PORT, I2C_SCL, PAL_MODE_ALTERNATE(4) | 
                                     PAL_STM32_OTYPE_OPENDRAIN |
                                     PAL_STM32_OSPEED_LOWEST);

    PRINT("About to configure I2C.", NULL);

    i2cObjectInit(&I2C_DRIVER);

    i2cConfig.op_mode = OPMODE_I2C;
    i2cConfig.duty_cycle = STD_DUTY_CYCLE;
    i2cConfig.clock_speed = 100000;
    
    PRINT("About to start I2C.", NULL);

    i2cStart(&I2C_DRIVER, &i2cConfig);
    
    cc3000HttpServerStart(&control, &http, (Mutex *) NULL);

#if 0
    rtcTest();
#endif

#if 0
    while (1)
    {
        if (RDY_OK != (i2cReturn = mplOneShotReadBarometer(&I2C_DRIVER,
                                                           MPL3115A2_DEFAULT_ADDR,
                                                           &pressure, 
                                                           &temperature)))
        {
            i2cErrorHandler();
        }

        PRINT("Pressure is: %x", (uint32_t) pressure);
        PRINT("Temperature is: %x", (uint32_t) temperature);
        
        PRINT("Pressure is: %f", pressure);
        PRINT("Temperature is: %f", temperature);

        palSetPad(LED_PORT, LED_STATUS);
        chThdSleep(MS2ST(500));
        palClearPad(LED_PORT, LED_STATUS);
    }

    while (1)
    {
        if (RDY_OK != (i2cReturn = tslReadLuxConvertSleep(&I2C_DRIVER,
                                                          TSL2561_ADDR_FLOAT,
                                                          &lux)))
        {
            i2cErrorHandler();
        }


        PRINT("Lux is: %u\n", lux);

        palSetPad(LED_PORT, LED_STATUS);
        chThdSleep(1000);
        palClearPad(LED_PORT, LED_STATUS);
    }

#endif


    palSetPad(LED_PORT, LED_STATUS);


    while(1);

    return 0;
}




