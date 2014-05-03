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

#include "fyp.h"
#include "cc3000_chibios_api.h"
#include "clarity_api.h"

/* Key configuration information */
#define SSID           "FYP"
#define SSID_LEN        strlen(SSID)
#define SEC_TYPE        WLAN_SEC_UNSEC

#define SERVER_URL      "cactuar.eipi.co.uk"
#define SERVER_PORT     9000
#define STANDBY_TIME_S  60

#define DEBUG_TIME_MEASURING  FALSE

Mutex printMtx;
static Mutex cc3000ApiMutex;
static clarityHttpServerInformation controlInfo;


static clarityAccessPointInformation ap =   {SSID,
                                             WLAN_SEC_UNSEC,
                                             "",
                                             {
                                                false,
                                                0x0A00000A,
                                                0xFFFFFF00,
                                                0x0A000001,
                                                0x08080808
                                             },
                                            };

static SPIConfig cc3000SpiConfig;
static EXTConfig cc3000ExtConfig;

uint8_t zero = 0x00;

void debugPrint(const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    chMtxLock(&printMtx);
    chvprintf((BaseSequentialStream*)&SERIAL_DRIVER, fmt, ap);
    chMtxUnlock();
    va_end(ap);
}

static uint32_t httpGetRoot(const clarityHttpRequestInformation * info, 
                            clarityConnectionInformation * conn)
{
    static const char * rootStr = "You are seeing this as a result of a GET "
                                  "request on the root resource of this server.";
    uint32_t txBytes;

    (void)info;

    if ((txBytes = clarityHttpServerSendInCb(conn, rootStr, strlen(rootStr))) != 
         strlen(rootStr))
    {
        PRINT("Send failed.", NULL);
        while(1);
        return 1;
    }

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
    cc3000SpiConfig.ssport = CHIBIOS_CC3000_NSS_PORT;
    cc3000SpiConfig.sspad = CHIBIOS_CC3000_NSS_PAD;
#if 1
    cc3000SpiConfig.cr1 = SPI_CR1_CPHA |    /* 2nd clock transition first data capture edge */
                      (SPI_CR1_BR_1 | SPI_CR1_BR_0 );   /* BR: 011 - 2 MHz  */
    /* Setup SPI pins */
    palSetPad(CHIBIOS_CC3000_NSS_PORT, CHIBIOS_CC3000_NSS_PAD);
    palSetPadMode(CHIBIOS_CC3000_NSS_PORT, CHIBIOS_CC3000_NSS_PAD,
                  PAL_MODE_OUTPUT_PUSHPULL |
                  PAL_STM32_OSPEED_LOWEST);     /* 400 kHz */

    palSetPadMode(CHIBIOS_CC3000_SPI_PORT, CHIBIOS_CC3000_SCK_PAD,
                  PAL_MODE_ALTERNATE(5) |       /* SPI */
                  PAL_STM32_OTYPE_PUSHPULL |
                  PAL_STM32_OSPEED_MID2);       /* 10 MHz */

    palSetPadMode(CHIBIOS_CC3000_SPI_PORT, CHIBIOS_CC3000_MISO_PAD,
                  PAL_MODE_ALTERNATE(5));       /* SPI */

    palSetPadMode(CHIBIOS_CC3000_SPI_PORT, CHIBIOS_CC3000_MOSI_PAD,
                  PAL_MODE_ALTERNATE(5) |       /* SPI */
                  PAL_STM32_OTYPE_PUSHPULL |
                  PAL_STM32_OSPEED_MID2);       /* 10 MHz */
#else

#if 0
    cc3000SpiConfig.cr1 = SPI_CR1_CPHA |    /* 2nd clock transition first data capture edge */
                          (SPI_CR1_BR_0);   /* BR: 001 - 8 MHz  */
#endif

    cc3000SpiConfig.cr1 = SPI_CR1_CPHA |    /* 2nd clock transition first data capture edge */
                      (SPI_CR1_BR_1 | SPI_CR1_BR_0 );   /* BR: 011 - 2 MHz  */
    /* Setup SPI pins */
    palSetPad(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_NSS_PAD);
    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_NSS_PAD,
                  PAL_MODE_OUTPUT_PUSHPULL |
                  PAL_STM32_OSPEED_HIGHEST);

    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_SCK_PAD,
                  PAL_MODE_ALTERNATE(5) |       /* SPI */
                  PAL_STM32_OTYPE_PUSHPULL |
                  PAL_STM32_OSPEED_HIGHEST); 

    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_MISO_PAD,
                  PAL_MODE_ALTERNATE(5));     

    palSetPadMode(CHIBIOS_CC3000_PORT, CHIBIOS_CC3000_MOSI_PAD,
                  PAL_MODE_ALTERNATE(5) |       /* SPI */
                  PAL_STM32_OTYPE_PUSHPULL |
                  PAL_STM32_OSPEED_MID2);  
#endif

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
                          0,0,0, debugPrint);

}

static void deinitialiseCC3000(void)
{
    cc3000ChibiosShutdown();
    
    palSetPadMode(CHIBIOS_CC3000_NSS_PORT, CHIBIOS_CC3000_NSS_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_SPI_PORT, CHIBIOS_CC3000_SCK_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_SPI_PORT, CHIBIOS_CC3000_MISO_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_SPI_PORT, CHIBIOS_CC3000_MOSI_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_IRQ_PORT, CHIBIOS_CC3000_IRQ_PAD,
                  PAL_MODE_UNCONNECTED);

    palSetPadMode(CHIBIOS_CC3000_WLAN_EN_PORT, CHIBIOS_CC3000_WLAN_EN_PAD,
                  PAL_MODE_UNCONNECTED);
}

static void cc3000Unresponsive(void)
{
    PRINT("Clarity thinks CC3000 was unresponsive...", NULL);

    /* TODO debugging XXX */
#if 1
    while(1)
    {
        palTogglePad(LED_PORT, LED_ERROR);
        chThdSleep(MS2ST(1000));
    }
#endif

    eepromRecordUnresponsiveShutdown();
    configureRtcAlarmAndStandby(&RTC_DRIVER, STANDBY_TIME_S);

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


clarityError httpPostShutdownError(clarityTransportInformation * tcp,
                                   clarityHttpPersistant * persistant)
{
    clarityError rtn;
    char buf[110];
    clarityHttpResponseInformation response;
    int16_t postLen = 0;

    memset(buf, 0, sizeof(buf));
    memset(&response, 0, sizeof(response));

    postLen = clarityHttpBuildPost(buf, sizeof(buf), "/cc3000", "/shutdown_errors",
                                   "1 unitless", persistant);

    if ((rtn = clarityHttpSendRequest(tcp, persistant, buf, sizeof(buf),
                                     postLen, &response)) != CLARITY_SUCCESS)
    {
        PRINT_ERROR();
    }

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

#if DEBUG_TIME_MEASURING == TRUE
    rtcRetrieve(&RTC_DRIVER, &timingData);
    PRINT("Finished. Minute: %d Seconds: %d", timingData.time.minute,  
#endif

    initialiseDebugHw();

    initialiseCC3000();

    initialiseControl(&controlInfo);

    initialiseSensorHw();

    clarityTransportInformation tcp;

    memset(&tcp, 0, sizeof(tcp));
    tcp.type = CLARITY_TRANSPORT_TCP;
    tcp.addr.type = CLARITY_ADDRESS_URL;
    strncpy(tcp.addr.addr.url, SERVER_URL, CLARITY_MAX_URL_LENGTH);
    tcp.port = SERVER_PORT;

    PRINT("Starting...", NULL);

    if (clarityInit(&cc3000ApiMutex, cc3000Unresponsive, &ap, debugPrint) 
            != CLARITY_SUCCESS)
    {
        PRINT_ERROR();
    }

    clarityHttpPersistant persistant;
    memset(&persistant,0,sizeof(persistant));
    persistant.closeOnComplete = false;

    clarityTimeDate time;
    rtcRetrieve(&RTC_DRIVER, &time);
    
    clarityRegisterProcessStarted();

    if (time.date.year < 14)
    {
        PRINT("Time needs updated.", NULL);
        if (updateRtcWithSntp() != 0)
        {
            PRINT_ERROR();
        }
    }
    else
    {
        PRINT("Time doesn't need updated.", NULL);
    }

    if (eepromWasLastShutdownOk() != EEPROM_ERROR_OK)
    {
        PRINT("Last shutdown was not OK.", NULL);

        if (httpPostShutdownError(&tcp, &persistant) == 0)
        {
            if (eepromAcknowledgeLastShutdownError() != EEPROM_ERROR_OK)
            {
                PRINT_ERROR();
            }
        }
        else
        {
            PRINT_ERROR();
        }
    }
    else
    {
        PRINT("Last shutdown was OK.", NULL);
    }

    PRINT("Posting Lux.", NULL);
    if (httpPostLux(&tcp, &persistant) != CLARITY_SUCCESS)
    {
        PRINT_ERROR();
    }
 
    PRINT("Posting Temperature.", NULL);
    if (httpPostTemperature(&tcp, &persistant) != CLARITY_SUCCESS)
    {
        PRINT_ERROR();
    }

    persistant.closeOnComplete = true;

    PRINT("Posting Pressure.", NULL);
    if (httpPostPressure(&tcp, &persistant) != CLARITY_SUCCESS)
    {
        PRINT_ERROR();
    }


#if 0
    if (clarityHttpServerStart(&controlInfo) != CLARITY_SUCCESS)
    {
        PRINT_ERROR();
    }
    PRINT("main sleeping", NULL);

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
#endif

    clarityRegisterProcessFinished();
    PRINT("Done.", NULL);

    clarityShutdown();
    deinitialiseCC3000();

    configureRtcAlarmAndStandby(&RTC_DRIVER, STANDBY_TIME_S);

    return 0;
}


