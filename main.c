/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"
#include "board.h"
#include "chstreams.h"
#include "chprintf.h"
#include "string.h"

#include "cc3000_chibios.h"
#include "socket.h"
#include "hci.h"
#include "nvmem.h"
#include "netapp.h"

/* Serial driver to be used */
#define SERIAL_DRIVER          SD1
#if 1
#define PRINT(fmt, ...)                                                     \
        chprintf((BaseSequentialStream*)&SERIAL_DRIVER,                     \
                 "(%s:%d) " fmt "\n\r", __FILE__, __LINE__, __VA_ARGS__)
#else
#define PRINT(...)  
#endif

/* LED for notification setup */
#define LED_PORT            GPIOB
#define LED_PIN             GPIOB_LED4

/* LED for error setup */
#define LED_ERROR_PORT      GPIOB
#define LED_ERROR_PIN       GPIOB_LED3

/* Remote information */
#define REMOTE_IP           0xA000001 /* 10.0.0.1 */
#define REMOTE_PORT         44444

/* Messages */
#define TX_MSG              "Hello World from CC3000"
#define TX_MSG_SIZE         strlen(TX_MSG)
#define RX_MSG_EXP          "Hello CC3000"
#define RX_MSG_EXP_SIZE     strlen(RX_MSG_EXP)

/* Access point config - arguments to wlan_connect */
#define SSID                "FYP"
#define SSID_LEN            strlen(SSID)
#define SEC_TYPE            WLAN_SEC_UNSEC
#define KEY                 NULL
#define KEY_LEN             0
#define BSSID               NULL

#define SUCCESS             0
#define ERROR               -1

typedef struct {
#define NETAPP_IPCONFIG_MAC_OFFSET              (20)
    bool okToShutdown;
    bool stopSmartConfig;
    bool smartConfigFinished;
    bool dhcp;
    bool dhcpConfigured;
    bool connected;
} cc3000AsyncData;

cc3000AsyncData asyncData;

/* Async Event handler */
void cbWlanAsyncEvent(long lEventType, char * data, unsigned char length)
{
    (void)length;

    PRINT("In cbWlanAsyncEvent with lEventType: %x.", lEventType);

    if (lEventType == HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE)
    {
        asyncData.smartConfigFinished = TRUE;
        asyncData.stopSmartConfig     = TRUE;  
    }
    
    if (lEventType == HCI_EVNT_WLAN_UNSOL_CONNECT)
    {
        asyncData.connected = TRUE;
    }
    
    if (lEventType == HCI_EVNT_WLAN_UNSOL_DISCONNECT)
    {       
        asyncData.connected = FALSE;
        asyncData.dhcp      = FALSE;
        asyncData.dhcpConfigured = FALSE;
    }
    
    if (lEventType == HCI_EVNT_WLAN_UNSOL_DHCP)
    {
        // Notes: 
        // 1) IP config parameters are received swapped
        // 2) IP config parameters are valid only if status is OK, i.e. ulCC3000DHCP becomes 1
        
        // only if status is OK, the flag is set to 1 and the addresses are valid
        if ( *(data + NETAPP_IPCONFIG_MAC_OFFSET) == 0)
        {
            asyncData.dhcp = TRUE;
        }
        else
        {
            asyncData.dhcp = FALSE;
        }
    }
    
    if (lEventType == HCI_EVENT_CC3000_CAN_SHUT_DOWN)
    {
        asyncData.okToShutdown = TRUE;
    }
}


static void cc3000UdpTest(void)
{
    uint8_t patchVer[2];
    int sock;
    sockaddr_in destAddr;
    sockaddr_in localAddr;
    sockaddr fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    char rxBuffer[32];
    int recvRtn = 0;
    tNetappIpconfigRetArgs ipConfig;

    PRINT("Before cc3000ChibiosWlanInit", NULL);
    cc3000ChibiosWlanInit(cbWlanAsyncEvent,0,0,0);
    PRINT("After cc3000ChibiosWlanInit", NULL);

    PRINT("Before wlan_start", NULL);
    wlan_start(0);
    PRINT("After wlan_start", NULL);

    /* Read version info */
    nvmem_read_sp_version(patchVer);
    PRINT("--Start of nvmem_read_sp_version--", NULL);
    PRINT("Package ID: %d", patchVer[0]);
    PRINT("Build Version: %d", patchVer[1]);
    PRINT("--End of nvmem_read_sp_version--", NULL);

    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(REMOTE_PORT);
    destAddr.sin_addr.s_addr = htonl(REMOTE_IP); 

    localAddr.sin_family = AF_INET;
    localAddr.sin_port = 0;
    localAddr.sin_addr.s_addr = 0;
    
    PRINT("Attempting to connect to network...", NULL);
    if (wlan_connect(SEC_TYPE, SSID, SSID_LEN, BSSID, KEY, KEY_LEN) != SUCCESS)
    {
        PRINT("Unable to connect to access point.", NULL);
        return;
    }

    while (asyncData.connected != 1)
    {
        chThdSleep(MS2ST(5));
    }

    PRINT("Connected!", NULL);
    

    PRINT("Waiting for DHCP...", NULL);
    while (asyncData.dhcp != 1)
    {
        chThdSleep(MS2ST(5));
    }
    PRINT("Received!", NULL);

    PRINT("Finding IP information...", NULL);
    netapp_ipconfig(&ipConfig);
    PRINT("Found!", NULL);

    PRINT("Creating socket...", NULL);
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == ERROR)
    {
        PRINT("socket() returned error.", NULL);
        return;
    }
    PRINT("Created!", NULL);

 
#if 0
    PRINT("Binding socket...", NULL);
    if ((bind(sock, (sockaddr *)&localAddr,
                   sizeof(localAddr)) == ERROR))
    {
        PRINT("bind() returned error.", NULL);
        return;
    }
    PRINT("Bound!", NULL);
#endif

    while (1)
    {
        PRINT("Sending...", NULL);
        if (sendto(sock, TX_MSG, TX_MSG_SIZE, 0,
                        (sockaddr*)&destAddr,
                        sizeof(destAddr)) == ERROR)
        {
            PRINT("sendto() returned error.", NULL);
            return;
        }
        PRINT("Sent!", NULL);

        PRINT("Receiving...", NULL);
        if ((recvRtn = recvfrom(sock, rxBuffer, sizeof(rxBuffer),
                                     0, &fromAddr, &fromLen) == ERROR))
        {
            PRINT("recvfrom() returned error.", NULL);
            return;
        }
        PRINT("Received!", NULL);

        if (recvRtn == RX_MSG_EXP_SIZE)
        {
            if (strcmp(rxBuffer, RX_MSG_EXP) == 0)
            {
                palTogglePad(LED_PORT, LED_PIN);
                PRINT("Received the expected message: %s", rxBuffer);
            }
        }
        chThdSleep(S2ST(3));
        palTogglePad(LED_PORT, LED_PIN);
    }

    /* ALAN TODO when can the device be shutdown */

    wlan_stop(); 
    while(1);
}


int main(void) 
{
    halInit();
    chSysInit();

    /* Led for status */
    palClearPad(LED_PORT, LED_PIN);
    palSetPadMode(LED_PORT, LED_PIN, PAL_MODE_OUTPUT_PUSHPULL);

    /* Led for error */
    palClearPad(LED_ERROR_PORT, LED_ERROR_PIN);
    palSetPadMode(LED_ERROR_PORT, LED_ERROR_PIN, PAL_MODE_OUTPUT_PUSHPULL);

    /* Serial for debugging */
    sdStart(&SERIAL_DRIVER, NULL);
    palSetPadMode(GPIOA, 9, PAL_MODE_ALTERNATE(7));
    palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(7));

    cc3000UdpTest();

    /* Only return on error */
    palSetPad(LED_ERROR_PORT, LED_ERROR_PIN);
    while (1);

  return 0;
}


