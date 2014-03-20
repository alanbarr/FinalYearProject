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
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "board.h"
#include "chstreams.h"
#include "chprintf.h"

#include "mpl3115a2.h"
#include "tsl2561.h"

#include "fyp.h"
#include "cc3000_chibios_api.h"
#include "clarity_api.h"

I2CConfig i2cConfig;
i2cflags_t errorFlags;
msg_t i2cReturn;

Mutex printMtx;
static Mutex cc3000ApiMutex;
static clarityHttpServerInformation controlInfo;

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

static clarityAccessPointInformation ap = { "FYP", WLAN_SEC_UNSEC, ""};

#define CC3000_SPI_DRIVER              SPID2
#define CC3000_EXT_DRIVER              EXTD1

static SPIConfig cc3000SpiConfig;
static EXTConfig cc3000ExtConfig;

uint8_t zero = 0x00;

#if 0
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
#endif


static uint32_t httpGetRoot(const clarityHttpRequestInformation * info, 
                            clarityConnectionInformation * conn)
{
    static const char * rootStr = "Hello, World";
    uint32_t txBytes;

    PRINT("in %s!!!!!!!!!!!!", __FUNCTION__);

    (void)info;

    if ((txBytes = claritySendInCb(conn, rootStr, strlen(rootStr))) != 
         strlen(rootStr))
    {
        PRINT("Send failed.", NULL);
        while(1);
        return 1;
    }

    return 0;
}

#define TEMP_STRING_SIZE        12 /* "101.33 kPa "*/
#define HTTP_RESPONSE_SIZE      100
static uint32_t httpGetPressure(const clarityHttpRequestInformation * info, 
                                clarityConnectionInformation * conn)
{
    char pressureStr[TEMP_STRING_SIZE]; 
    float temperature;
    float pressure = 123456.78;

    char httpResponse[HTTP_RESPONSE_SIZE];
    int32_t httpResponseSize;
    int32_t temp = 0;
    (void)info;

    (void)temperature;
#if 0
    if (RDY_OK != (i2cReturn = mplOneShotReadBarometer(&I2C_DRIVER,
                                                       MPL3115A2_DEFAULT_ADDR,
                                                       &pressure, 
                                                       &temperature)))
    {
        return 1;
    }
#endif

    /* Prepare current pressure temp string */
    pressure = pressure / 1000; /* Pa -> kPa */
    temp = snprintf(pressureStr, TEMP_STRING_SIZE-1, "%.2f kPa", pressure); 
    pressureStr[temp+1] = 0;
#if 0
    PRINT("Pressure Str: %s", pressureStr); /* TODO UNITS */ 
    PRINT("HTTP Str: %s", httpResponse);
#endif

    httpResponseSize = clarityHttpBuildResponseTextPlain(httpResponse,
                                                 sizeof(httpResponse),
                                                 200, "OK",
                                                 pressureStr);

    if ((temp = claritySendInCb(conn, httpResponse, httpResponseSize)) != httpResponseSize)
    {
        PRINT("Send failed.", NULL);
        while(1);
        return 1;
    }

    return 0;
}

static uint32_t httpGetTemperature(const clarityHttpRequestInformation * info, 
                                   clarityConnectionInformation * conn)
{
    (void)info;
    (void)conn;

#if 0
    float pressure;
    float temperature;
        if (RDY_OK != (i2cReturn = mplOneShotReadBarometer(&I2C_DRIVER,
                                                           MPL3115A2_DEFAULT_ADDR,
                                                           &pressure, 
                                                           &temperature)))
        {
            return 1;
        }

        PRINT("Pressure is: %x", (uint32_t) pressure);
        PRINT("Temperature is: %x", (uint32_t) temperature);
        
        PRINT("Pressure is: %f", pressure);
        PRINT("Temperature is: %f", temperature);

#endif

    return 0;
}


static uint32_t httpGetLux(const clarityHttpRequestInformation * info, 
                           clarityConnectionInformation * conn)
{
    (void)info;
    (void)conn;

#if 0
    uint16_t lux;
    if (RDY_OK != (i2cReturn = tslReadLuxConvertSleep(&I2C_DRIVER,
                                                      TSL2561_ADDR_FLOAT,
                                                      &lux)))
    {
        i2cErrorHandler();
    }


    PRINT("Lux is: %u\n", lux);
#endif
    return 0;
}

static void initialiseControl(clarityHttpServerInformation * controlInfo)
{
    memset(controlInfo, 0, sizeof(*controlInfo));

    controlInfo->resources[0].name = "/";
    controlInfo->resources[0].methods[0].type = GET;
    controlInfo->resources[0].methods[0].callback = httpGetRoot;
 
    controlInfo->resources[1].name = "/temperature";
    controlInfo->resources[1].methods[0].type = GET;
    controlInfo->resources[1].methods[0].callback = httpGetTemperature;

    controlInfo->resources[2].name = "/pressure";
    controlInfo->resources[2].methods[0].type = GET;
    controlInfo->resources[2].methods[0].callback = httpGetPressure;
 
    controlInfo->resources[3].name = "/lux";
    controlInfo->resources[3].methods[0].type = GET;
    controlInfo->resources[3].methods[0].callback = httpGetLux;

}

static void initialiseCC3000(void)
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

static void deinitialiseCC3000(void)
{
    cc3000ChibiosShutdown();
    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_NSS_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_SCK_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_MISO_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_MOSI_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_IRQ_PORT, CHIBIOS_CC3000_IRQ_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_WLAN_EN_PORT, CHIBIOS_CC3000_WLAN_EN_PAD,
                  PAL_MODE_UNCONNECTED);
}

static void cc3000Unresponsive(void)
{
    PRINT("Clarity thinks CC3000 was unresponsive...", NULL);
}

static void initialiseDebugHw(void)
{
    /* Led for status */
    palClearPad(LED_PORT, LED_STATUS);
    palSetPadMode(LED_PORT, LED_STATUS, PAL_MODE_OUTPUT_PUSHPULL);
    
    /* Led for error */
    palClearPad(LED_PORT, LED_ERROR);
    palSetPadMode(LED_PORT, LED_ERROR, PAL_MODE_OUTPUT_PUSHPULL);
 
    /* Serial for debugging */
    sdStart(&SERIAL_DRIVER, NULL);
    palSetPadMode(SERIAL_PORT, SERIAL_TX, PAL_MODE_ALTERNATE(7));
    palSetPadMode(SERIAL_PORT, SERIAL_RX, PAL_MODE_ALTERNATE(7));

}

void deinitialiseHw(void)
{
    /* Led for status */
    palSetPadMode(LED_PORT, LED_STATUS, PAL_MODE_UNCONNECTED);
    
    /* Led for error */
    palSetPadMode(LED_PORT, LED_ERROR, PAL_MODE_UNCONNECTED);
 
    /* Serial for debugging */
    sdStop(&SERIAL_DRIVER);
    palSetPadMode(SERIAL_PORT, SERIAL_TX, PAL_MODE_UNCONNECTED);
    palSetPadMode(SERIAL_PORT, SERIAL_RX, PAL_MODE_UNCONNECTED);

}

void initialiseSensorHw(void)
{
    /* I2C for sensors */
    palSetPadMode(I2C_PORT, I2C_SDA, PAL_MODE_ALTERNATE(4) | 
                                     PAL_STM32_OTYPE_OPENDRAIN |
                                     PAL_STM32_OSPEED_LOWEST);
    palSetPadMode(I2C_PORT, I2C_SCL, PAL_MODE_ALTERNATE(4) | 
                                     PAL_STM32_OTYPE_OPENDRAIN |
                                     PAL_STM32_OSPEED_LOWEST);

    i2cObjectInit(&I2C_DRIVER);

    i2cConfig.op_mode = OPMODE_I2C;
    i2cConfig.duty_cycle = STD_DUTY_CYCLE;
    i2cConfig.clock_speed = 100000;
    
    i2cStart(&I2C_DRIVER, &i2cConfig);

}

void deinitialiseSensorHw(void)
{
    /* I2C for sensors */
    i2cStop(&I2C_DRIVER);
    palSetPadMode(I2C_PORT, I2C_SDA, PAL_MODE_UNCONNECTED);
    palSetPadMode(I2C_PORT, I2C_SCL, PAL_MODE_UNCONNECTED);

}


clarityError test_client(void)
{

#if 0
    clarityAddressInformation addr;
    addr.type = CLARITY_ADDRESS_IP;
    addr.addr.ip = 0x10000001;
    
    return clarityBuildSendHttpRequest(clarityAddressInformation * addr,
                                         const clarityHttpRequestInformation * request,
                                         char * buf,
                                         uint16_t bufSize,
                                         clarityHttpResponseInformation * response)
#endif
        static const char * request = "POST /test_device/test HTTP/1.0\r\n"
                                      "Content-Length: 8\r\n"
                                      "\r\n"
                                      "ON UNITS";
        clarityError rtn;
        char buf[100];
        clarityTransportInformation tcp;
        clarityHttpResponseInformation response;

        memset(buf, 0, sizeof(buf));
        memset(&tcp, 0, sizeof(tcp));

        tcp.type = CLARITY_TRANSPORT_TCP;
        tcp.addr.type = CLARITY_ADDRESS_IP;
        tcp.addr.addr.ip = 0x0A000001;
        tcp.port = 9000;
        
        strncpy(buf, request, strlen(request));

        rtn = claritySendHttpRequest(&tcp, buf, sizeof(buf),
                                   strlen(request), &response);

        if (response.code == 200)
        {
            PRINT("Response was OK: %d", response.code);
        }
        else
        {
            PRINT("Response was NOT OK: %d.", response.code);
        }
        return rtn;

}

int main(void)
{
    halInit();
    
    chSysInit();

    chMtxInit(&printMtx);

    initialiseDebugHw();

    initialiseCC3000();

    initialiseControl(&controlInfo);

    if (clarityInit(&cc3000ApiMutex, cc3000Unresponsive, &ap) != CLARITY_SUCCESS)
    {
        PRINT("Bugger...", NULL);
    }

    else if (clarityHttpServerStart(&controlInfo) != CLARITY_SUCCESS)
    {
        PRINT("Bugger...", NULL);
    }

    else if (test_client() != CLARITY_SUCCESS)
    {
        PRINT("Bugger...", NULL);
    }
    
#if 1
    else if (updateRtcWithSntp() != 0)
    {
        PRINT("Bugger...", NULL);
    }

#endif

#if 0
    initialiseSensorHw();
#endif
    chThdSleep(S2ST(10));

    PRINT("Shutting down...", NULL)
    if (clarityHttpServerStop() != CLARITY_SUCCESS)
    {
        PRINT("clarityHttpServerStop() failed", NULL);
    }
    
    else if (clarityShutdown() != CLARITY_SUCCESS)
    {
        PRINT("clarityShutdown() failed", NULL);
    }

    PRINT("Shut down.", NULL)

    while(1);

    return 0;
}




