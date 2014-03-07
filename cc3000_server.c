#include "socket.h"
#include "clarity_api.h"
#include "cc3000_chibios_api.h"
#include "string.h"
#include "fyp.h"
#include <stdio.h>

#include "mpl3115a2.h"
#include "tsl2561.h"

static WORKING_AREA(httpWorkingArea, 1024);

static Thread * httpServerThd = NULL;
static bool killHttpServer = false;
static char rxBuf[1000];

static controlInformation controlInfo;
static httpInformation httpInfo;
static int serverSocket;

static Mutex * cc3000Mtx;

#define CC3000_API_MTX_LCK()    \
    if (cc3000Mtx != NULL)      \
    {                           \
        chMtxLock(cc3000Mtx);   \
    }

#define CC3000_API_MTX_UNLCK()  \
    if (cc3000Mtx != NULL)      \
    {                           \
        chMtxUnlock();          \
    }

static uint32_t httpGetRoot(const httpInformation * info, 
                            connectionInformation * conn)
{
    static const char * rootStr = "Hello, World";
    uint32_t txBytes;

    PRINT("in %s!!!!!!!!!!!!", __FUNCTION__);

    (void)info;
    (void)conn;

    CC3000_API_MTX_LCK();

    if ((txBytes = send(conn->socket, rootStr, strlen(rootStr), 0)) != 
        strlen(rootStr))
    {
        PRINT("Send failed.", NULL);
        while(1);
        return 1;
    }
 
    CC3000_API_MTX_UNLCK();

    return 0;
}

#define TEMP_STRING_SIZE        12 /* "101.33 kPa "*/
#define HTTP_RESPONSE_SIZE      100
static uint32_t httpGetPressure(const httpInformation * info, 
                                connectionInformation * conn)
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
    PRINT("Pressure Str: %s", pressureStr); /* TODO UNITS */ 

    PRINT("HTTP Str: %s", httpResponse);

    httpResponseSize = clarityHttpResponseTextPlain(httpResponse,
                                                 sizeof(httpResponse),
                                                 200, "OK",
                                                 pressureStr);

    CC3000_API_MTX_LCK();
    if ((temp = send(conn->socket, httpResponse, httpResponseSize, 0)) != httpResponseSize)
    {
        PRINT("Send failed.", NULL);
        while(1);
        return 1;
    }

    CC3000_API_MTX_UNLCK();
    return 0;
}

static uint32_t httpGetTemperature(const httpInformation * info, 
                                   connectionInformation * conn)
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


static uint32_t httpGetLux(const httpInformation * info, 
                                   connectionInformation * conn)
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

static void initaliseControl(void)
{
    memset(&controlInfo, 0, sizeof (controlInfo));

    controlInfo.deviceName = "STM32_CC3000";

    controlInfo.resources[0].name = "/";
    controlInfo.resources[0].methods[0].type = GET;
    controlInfo.resources[0].methods[0].callback = httpGetRoot;
 
    controlInfo.resources[1].name = "/temperature";
    controlInfo.resources[1].methods[0].type = GET;
    controlInfo.resources[1].methods[0].callback = httpGetTemperature;

    controlInfo.resources[2].name = "/pressure";
    controlInfo.resources[2].methods[0].type = GET;
    controlInfo.resources[2].methods[0].callback = httpGetPressure;
 
    controlInfo.resources[3].name = "/lux";
    controlInfo.resources[3].methods[0].type = GET;
    controlInfo.resources[3].methods[0].callback = httpGetLux;

}


static msg_t cc3000HttpServer(void * arg)
{
    sockaddr acceptedAddr;
    socklen_t acceptedAddrLen = sizeof(acceptedAddr);
    const char * httpRtn = NULL;
    int16_t rxBytes = 0;
    connectionInformation accepted;
    
    (void)arg;

    memset(&acceptedAddr, 0, sizeof(acceptedAddr));

    while (1)
    {
        PRINT("Server: top of while 1", NULL);

        if (killHttpServer == true)
        {
            break;
        }
 
        memset(&acceptedAddr, 0, sizeof(acceptedAddr));
        memset(rxBuf, 0, sizeof(rxBuf));

        CC3000_API_MTX_LCK();

        while (true)
        {
            if ((accepted.socket = accept(serverSocket, &acceptedAddr,
                                         &acceptedAddrLen)) < 0)
            {
                if (accepted.socket == -1)
                {
                    PRINT("accept() returned error: %d", accepted.socket);
                }
                else
                {
                    chThdSleep(MS2ST(100));
                }
            }

            else
            {
                break;
            }
        }
        
        if ((rxBytes = recv(accepted.socket, rxBuf, sizeof(rxBuf), 0)) == -1)
        {
            PRINT("recv() returned error.", NULL);
        }
        
        CC3000_API_MTX_UNLCK();

        if (rxBytes > 0)
        {
            if ((httpRtn = clarityProcess(&controlInfo,
                                          &httpInfo, 
                                          &accepted,
                                          rxBuf, rxBytes)) == NULL)
            {
                PRINT("clarityProcess() returned NULL.", NULL);
            }
        }
 
        if (closesocket(accepted.socket) == -1)
        {
            PRINT("closesocket() failed for acceptedSocket.", NULL);
        }

        chThdSleep(MS2ST(500));
    }

    CC3000_API_MTX_LCK();
    if (closesocket(serverSocket) == -1)
    {
        PRINT("closesocket() failed for serverSocket.", NULL);
    }
    CC3000_API_MTX_UNLCK();

    return 0;
}

uint32_t cc3000HttpServerKill(void)
{
    killHttpServer = true;
    /* TODO ensure server ends */
    return 0;
}

uint32_t cc3000HttpServerStart(Mutex * cc3000ApiMtx)
{
    sockaddr_in serverAddr = {0};
    cc3000Mtx = cc3000ApiMtx;
    memset(rxBuf, 0, sizeof(rxBuf));
    uint16_t blockOpt = SOCK_ON;

    initaliseControl();

    /* We need dhcp info here */
    if (cc3000AsyncData.dhcp.present != 1)
    {
        PRINT("No DHCP info present", NULL);
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);
#if 0
    serverAddr.sin_addr.s_addr = *(int *)cc3000AsyncData.dhcp.info.aucIP; 
#endif
    PRINT("ALAN DEBUG DHCP ADDR IS %x", serverAddr.sin_addr.s_addr);


    CC3000_API_MTX_LCK();

    if ((serverSocket = socket(AF_INET, SOCK_STREAM,
                               IPPROTO_TCP)) == -1)
    {
        PRINT("socket() returned error.", NULL);
        while(1);
    }
    else if (setsockopt(serverSocket, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK,
                        &blockOpt, sizeof(blockOpt)) == -1)
    {
        PRINT("setsockopt() returned error.", NULL);
        while(1);
    }

    else if (bind(serverSocket, (sockaddr *)&serverAddr,
                  sizeof(serverAddr)) == -1)
    {
        serverSocket = -1;
        PRINT("bind() returned error.", NULL);
        while(1);
    }

    else if (listen(serverSocket, 1) != 0)
    {
        serverSocket = -1;
        PRINT("bind() returned error.", NULL);
        while(1);
    }
     
    CC3000_API_MTX_UNLCK();

    if (serverSocket != -1)
    {
        httpServerThd = chThdCreateStatic(httpWorkingArea,
                                          sizeof(httpWorkingArea),
                                          NORMALPRIO,
                                          cc3000HttpServer, NULL);
    }

    return 0;
}


