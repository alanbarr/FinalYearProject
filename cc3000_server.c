#include "socket.h"
#include "clarity_api.h"
#include "cc3000_chibios_api.h"
#include "string.h"
#include "fyp.h"


static WORKING_AREA(httpWorkingArea, 512);

static int serverSocket = -1;
static Thread * httpServerThd = NULL;
static bool killHttpServer = false;
static char rxBuf[1000];
static Mutex * cc3000Mtx = NULL;
static controlInformation * controlInfo = NULL;
static httpInformation * httpInfo;

static msg_t cc3000HttpServer(void * arg)
{
    int32_t acceptedSocket = -1;
    sockaddr acceptedAddr = {0};
    socklen_t acceptedAddrLen = sizeof(acceptedAddr);
    const char * httpRtn = NULL;
    int16_t rxBytes = 0;
    
    (void)arg;

    while (1)
    {

        if (killHttpServer == true)
        {
            break;
        }
 
        memset(&acceptedAddr, 0, sizeof(acceptedAddr));

        if (cc3000Mtx != NULL)
        {
            chMtxLock(cc3000Mtx);
        }

        if ((acceptedSocket = accept(serverSocket, &acceptedAddr,
                                     &acceptedAddrLen)) == -1)
        {
            PRINT("accept() returned error.", NULL);
        }
        
        else if ((rxBytes = recv(acceptedSocket, rxBuf, sizeof(rxBuf), 0)) == -1)
        {
            PRINT("recv() returned error.", NULL);
        }
        
        if (cc3000Mtx != NULL)
        {
            chMtxUnlock();
        }
        
        if (rxBytes > 0)
        {
            if ((httpRtn = clarityProcess(controlInfo, httpInfo, 
                                          (connectionInformation *)NULL,
                                          rxBuf, rxBytes)) == NULL)
            {
                PRINT("clarityProcess() returned NULL.", NULL);
            }
        }
 
        if (closesocket(acceptedSocket) == -1)
        {
            PRINT("closesocket() failed for acceptedSocket.", NULL);
        }

        chThdSleep(MS2ST(500));
    }

    if (closesocket(serverSocket) == -1)
    {
        PRINT("closesocket() failed for serverSocket.", NULL);
    }
    
    return 0;
}

uint32_t cc3000HttpServerKill(void)
{
    killHttpServer = true;
    /* TODO ensure server ends */
    return 0;
}

uint32_t cc3000HttpServerStart(controlInformation * control,
                               httpInformation * http,
                               Mutex * cc3000ApiMtx)
{
    sockaddr_in serverAddr = {0};

    controlInfo = control;
    httpInfo = http;
    cc3000Mtx = cc3000ApiMtx;

    /* We need dhcp info here */
    if (cc3000AsyncData.dhcp.present != 1)
    {
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = 80;
    serverAddr.sin_addr.s_addr = *(int *)cc3000AsyncData.dhcp.info.aucIP; 
    PRINT("ALAN DEBUG DHCP ADDR IS %x", serverAddr.sin_addr.s_addr);

    if (cc3000Mtx != NULL)
    {
        chMtxLock(cc3000Mtx);
    }

    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        PRINT("socket() returned error.", NULL);
    }

    else if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        serverSocket = -1;
        PRINT("bind() returned error.", NULL);
    }

    else if (listen(serverSocket, 2) == -1)
    {
        serverSocket = -1;
        PRINT("bind() returned error.", NULL);
    }
     
    if (cc3000Mtx != NULL)
    {
        chMtxUnlock();
    }

    if (serverSocket != -1)
    {
        httpServerThd = chThdCreateStatic(httpWorkingArea,
                                          sizeof(httpWorkingArea),
                                          NORMALPRIO,
                                          cc3000HttpServer, NULL);
    }

    return 0;
}


