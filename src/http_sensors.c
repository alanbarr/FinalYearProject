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
#include "fyp.h"
#include "string.h"
#include "mpl3115a2.h"
#include "tsl2561.h"

#define LUX_STRING_SIZE         7 /* "xxx lx" + NULL*/
#define TEMPERATURE_STRING_SIZE 8  /* "xx.xx C" + NULL */
#define HTTP_RESPONSE_SIZE      100
char httpResponse[HTTP_RESPONSE_SIZE]; /* XXX not thread safe */

I2CConfig i2cConfig;
i2cflags_t errorFlags;

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
    

}

void deinitialiseSensorHw(void)
{
    /* I2C for sensors */
    palSetPadMode(I2C_PORT, I2C_SDA, PAL_MODE_UNCONNECTED);
    palSetPadMode(I2C_PORT, I2C_SCL, PAL_MODE_UNCONNECTED);

}

uint32_t httpGetPressure(const clarityHttpRequestInformation * info, 
                                clarityConnectionInformation * conn)
{
#define PRESSURE_STRING_SIZE        12 /* "101.33 kPa "*/
    char pressureStr[PRESSURE_STRING_SIZE]; 
    float temperature;
    float pressure;

    int32_t httpResponseSize;
    int32_t temp = 0;
    (void)info;

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
    temp = snprintf(pressureStr, PRESSURE_STRING_SIZE-1, "%.2f kPa", pressure); 
    pressureStr[temp+1] = 0;
#if 0
    PRINT("Pressure Str: %s", pressureStr); /* TODO UNITS */ 
    PRINT("HTTP Str: %s", httpResponse);
#endif

    httpResponseSize = clarityHttpBuildResponseTextPlain(httpResponse,
                                                 sizeof(httpResponse),
                                                 200, "OK",
                                                 pressureStr);

    if ((temp = claritySendInCb(conn, httpResponse, httpResponseSize)) 
              != httpResponseSize)
    {
        PRINT("Send failed.", NULL);
        while(1);
        return 1;
    }

    return 0;
#undef PRESSURE_STRING_SIZE
}

uint32_t httpGetTemperature(const clarityHttpRequestInformation * info, 
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

uint32_t httpGetLux(const clarityHttpRequestInformation * info, 
                           clarityConnectionInformation * conn)
{
    (void)info;
    (void)conn;

#if 1
    uint16_t lux;
    char luxString[LUX_STRING_SIZE];
    uint16_t httpResponseSize;
    int16_t temp;
    msg_t i2cReturn;

    //clarityCC3000ApiLck();
    spiAcquireBus(&CC3000_SPI_DRIVER);
    i2cAcquireBus(&I2C_DRIVER);

    i2cStart(&I2C_DRIVER, &i2cConfig);
    if (RDY_OK != (i2cReturn = tslReadLuxConvertSleep(&I2C_DRIVER,
                                                      TSL2561_ADDR_FLOAT,
                                                      &lux)))
    {
        PRINT("Error", NULL);
    }

    i2cStop(&I2C_DRIVER);
    i2cReleaseBus(&I2C_DRIVER);
    spiReleaseBus(&CC3000_SPI_DRIVER);
    //clarityCC3000ApiUnlck();
  
    temp = snprintf(luxString, LUX_STRING_SIZE-1, "%d lx", lux); 
    luxString[temp+1] = 0;
    
    httpResponseSize = clarityHttpBuildResponseTextPlain(httpResponse,
                                                         sizeof(httpResponse),
                                                         200, "OK",
                                                         luxString);

    if (claritySendInCb(conn, httpResponse, httpResponseSize) != httpResponseSize)
    {
        PRINT("Send failed.", NULL);
        while(1);
        return 1;
    }


    PRINT("Lux str is: %s\n", luxString);
#endif
    return 0;
}


clarityError httpPostTemperature(clarityHttpPersistant * persistant)
{
    clarityError rtn;
    char buf[100];
    clarityTransportInformation tcp;
    clarityHttpResponseInformation response;
    float temperature = 0;
    float pressure = 0;
    int16_t postLen = 0;
    int16_t temp = 0;
    char tString[TEMPERATURE_STRING_SIZE];
    msg_t i2cReturn;

    memset(buf, 0, sizeof(buf));
    memset(&tcp, 0, sizeof(tcp));
    memset(&response, 0, sizeof(response));

    tcp.type = CLARITY_TRANSPORT_TCP;
    tcp.addr.type = CLARITY_ADDRESS_IP;
    tcp.addr.addr.ip = 0x0A000001;
    tcp.port = 9000;
    
    spiAcquireBus(&CC3000_SPI_DRIVER);
    i2cAcquireBus(&I2C_DRIVER);
    i2cStart(&I2C_DRIVER, &i2cConfig);

    if (RDY_OK != (i2cReturn = mplOneShotReadBarometer(&I2C_DRIVER,
                                                       MPL3115A2_DEFAULT_ADDR,
                                                       &pressure, 
                                                       &temperature)))
    {
        return 1;
    }


    i2cStop(&I2C_DRIVER);
    i2cReleaseBus(&I2C_DRIVER);
    spiReleaseBus(&CC3000_SPI_DRIVER);
 
    temp = snprintf(tString, TEMPERATURE_STRING_SIZE-1, "%.2f C", temperature); 
    tString[temp+1] = 0;

    postLen = clarityHttpBuildPost(buf, sizeof(buf), "/cc3000", "/temperature", 
                                   tString, persistant);

    rtn = clarityHttpSendRequest(&tcp, persistant, buf, sizeof(buf), postLen, &response);

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

clarityError httpPostLux(clarityHttpPersistant * persistant)
{
    clarityError rtn;
    char buf[100];
    clarityTransportInformation tcp;
    clarityHttpResponseInformation response;
    uint16_t lux = 0;
    int16_t postLen = 0;
    int16_t temp = 0;
    char luxString[LUX_STRING_SIZE];

    memset(buf, 0, sizeof(buf));
    memset(&tcp, 0, sizeof(tcp));
    memset(&response, 0, sizeof(response));

    tcp.type = CLARITY_TRANSPORT_TCP;
    tcp.addr.type = CLARITY_ADDRESS_IP;
    tcp.addr.addr.ip = 0x0A000001;
    tcp.port = 9000;
    
    spiAcquireBus(&CC3000_SPI_DRIVER);
    i2cAcquireBus(&I2C_DRIVER);
    i2cStart(&I2C_DRIVER, &i2cConfig);

    if (RDY_OK != (tslReadLuxConvertSleep(&I2C_DRIVER,
                                          TSL2561_ADDR_FLOAT,
                                          &lux)))
    {
        PRINT("Error", NULL);
    }

    i2cStop(&I2C_DRIVER);
    i2cReleaseBus(&I2C_DRIVER);
    spiReleaseBus(&CC3000_SPI_DRIVER);
 
    temp = snprintf(luxString, LUX_STRING_SIZE-1, "%d lx", lux); 
    luxString[temp+1] = 0;

    postLen = clarityHttpBuildPost(buf, sizeof(buf), "/cc3000", "/lux",
                                   luxString, persistant);

    rtn = clarityHttpSendRequest(&tcp, persistant, buf, sizeof(buf),
                                 postLen, &response);

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

