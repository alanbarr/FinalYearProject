#include "ch.h"
#include "hal.h"
#include "board.h"
#include "chstreams.h"
#include "chprintf.h"

#if 0
#include "mpl3115a2.h"
#include "tsl2561.h"
#endif

#include <string.h>
#include "fyp.h"
#include "cc3000_server.h"
#include "cc3000_chibios_api.h"

I2CConfig i2cConfig;
i2cflags_t errorFlags;
msg_t i2cReturn;

Mutex printMtx;
static Mutex cc3000ApiMutex;

float pressure;
float temperature;
uint16_t lux;
uint16_t channel0;
uint16_t channel1;

/* Access point config - arguments to wlan_connect */
#define SSID                "FYP"
#define SSID_LEN            strlen(SSID)
#define SEC_TYPE            WLAN_SEC_UNSEC
#define KEY                 NULL
#define KEY_LEN             0
#define BSSID               NULL

#define CC3000_SPI_DRIVER              SPID2
#define CC3000_EXT_DRIVER              EXTD1

#define SUCCESS             0
#define ERROR               -1

static SPIConfig cc3000SpiConfig;
static EXTConfig cc3000ExtConfig;

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

void initaliseCC3000(void)
{
#ifdef STM32L1XX_MD

    /* SPI Config */
    cc3000SpiConfig.end_cb = NULL;
    cc3000SpiConfig.ssport = CHIBIOS_CC3000_PORT;
    cc3000SpiConfig.sspad = CHIBIOS_CC3000_NSS_PAD;
    cc3000SpiConfig.cr1 = SPI_CR1_CPHA |    /* 2nd clock transition first data capture edge */
                      (SPI_CR1_BR_1 | SPI_CR1_BR_0 );   /* BR: 011 - 2 MHz  */
 
    /* Setup SPI pins */
    palSetPad(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_NSS_PAD);
    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_NSS_PAD,
                  PAL_MODE_OUTPUT_PUSHPULL |
                  PAL_STM32_OSPEED_LOWEST);     /* 400 kHz */

    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_SCK_PAD,
                  PAL_MODE_ALTERNATE(5) |       /* SPI */
                  PAL_STM32_OTYPE_PUSHPULL |
                  PAL_STM32_OSPEED_MID2);       /* 10 MHz */

    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_MISO_PAD,
                  PAL_MODE_ALTERNATE(5));       /* SPI */

    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_MOSI_PAD,
                  PAL_MODE_ALTERNATE(5) |       /* SPI */
                  PAL_STM32_OTYPE_PUSHPULL |
                  PAL_STM32_OSPEED_MID2);       /* 10 MHz */

    /* Setup IRQ pin */
    palSetPadMode(CHIBIOS_CC3000_IRQ_PORT, CHIBIOS_CC3000_IRQ_PAD,
                  PAL_MODE_INPUT);

    /* Setup WLAN EN pin.
       With the pin low, we sleep here to make sure CC3000 is off.  */
    palClearPad(CHIBIOS_CC3000_WLAN_EN_PORT, CHIBIOS_CC3000_WLAN_EN_PAD);
    palSetPadMode(CHIBIOS_CC3000_WLAN_EN_PORT, CHIBIOS_CC3000_WLAN_EN_PAD,
                  PAL_MODE_OUTPUT_PUSHPULL |
                  PAL_STM32_OSPEED_LOWEST);     /* 400 kHz */

#endif /* STM32L1XX_MD */

    chMtxInit(&cc3000ApiMutex);

    extObjectInit(&CC3000_EXT_DRIVER);
    spiObjectInit(&CC3000_SPI_DRIVER);
    
    chThdSleep(MS2ST(500));
    cc3000ChibiosWlanInit(&CC3000_SPI_DRIVER, &cc3000SpiConfig,
                          &CC3000_EXT_DRIVER, &cc3000ExtConfig,
                          0,0,0);

}

int main(void)
{

    halInit();
    chSysInit();

    chMtxInit(&printMtx);

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

#if 0
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
#endif
    initaliseCC3000();
    wlan_start(0);
 
    PRINT("Attempting to connect to network...", NULL);
    if (wlan_connect(SEC_TYPE, SSID, SSID_LEN, BSSID, KEY, KEY_LEN) != SUCCESS)
    {
        PRINT("Unable to connect to access point.", NULL);
    }
    PRINT("Waiting for DHCP...", NULL);
    while (cc3000AsyncData.dhcp.present != 1)
    {
        chThdSleep(MS2ST(5));
    }
    PRINT("Received!", NULL);

    cc3000HttpServerStart(&cc3000ApiMutex);

    while(1)
    {
        chThdSleep(1000);
        palTogglePad(LED_PORT, LED_STATUS);
    }
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




