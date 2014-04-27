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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "fyp.h"

#define EEPROM_BASE         0x08080000
#define EEPROM_LAST         0x08081FFF
#define EEPROM_SIZE         (EEPROM_LAST - EEPROM_BASE + 1)

#define STORE_ADDR_EEPROM   ((uint32_t *)EEPROM_BASE)

#define compile_time_assert(X)                                              \
    extern int (*compile_assert(void))[sizeof(struct {                      \
                unsigned int compile_assert_failed : (X) ? 1 : -1; })]

typedef struct {
    uint32_t lastShutdownError; /* True or False */
    uint32_t unresponsiveShutdowns;
    uint32_t shutdowns;
} eepromData;

typedef struct {
    eepromData data;
    uint32_t checksum;
} eepromStore;

compile_time_assert(sizeof(eepromStore) == 16); /* We don't want any padding */

/* As per: 4.1.1 Unlocking the Data EEPROM block and the FLASH_PECR register in
 * PM0062 Programming Manual */
static eepromError eepromUnlock(void)
{
#define PEKEY1 0x89ABCDEF
#define PEKEY2 0x02030405
    FLASH->PEKEYR = PEKEY1;
    FLASH->PEKEYR = PEKEY2;

    if ((FLASH->PECR & FLASH_PECR_PELOCK) != 0)
    {
        return EEPROM_ERROR_LOCK;
    }
    
    return EEPROM_ERROR_TRUE;
}

/* As per: 4.1.1 Unlocking the Data EEPROM block and the FLASH_PECR register in
 * PM0062 Programming Manual */
static eepromError eepromLock(void)
{
    FLASH->PECR |= FLASH_PECR_PELOCK;
    return EEPROM_ERROR_OK;
}

/* As per: 4.3.6 Data EEPROM Word Write in PM0062 Programming Manual */
static eepromError eepromWriteWords(uint32_t * address, const uint32_t * data,  uint32_t size)
{
    eepromError rtn;

    chSysLock();
    size /= sizeof(uint32_t);

    if ((rtn = eepromUnlock()) != EEPROM_ERROR_OK)
    {
        return rtn;
    }

    FLASH->PECR |= FLASH_PECR_FTDW; 

    while (size > 0)
    {
        *address = *data;
        address++;
        data++;
        size--;
    }

    eepromLock();

    chSysUnlock();

    return EEPROM_ERROR_OK;
}

static eepromError eepromReadWords(const uint32_t * address, uint32_t * data, uint32_t size)
{
    size /= sizeof(uint32_t);
 
    while (size > 0)
    {
        *data = *address;
        address++;
        data++;
        size--;
    }

    return EEPROM_ERROR_OK;
}

static int32_t generateChecksum(const void * data, uint32_t bytes)
{
    uint32_t checksum = 0;
    uint8_t index = 0;
    uint8_t * pChk = (uint8_t *)&checksum;
    const uint8_t * pD = (uint8_t *)data;

    while(bytes > 0)
    {
# if 0
        PRINT("xor: %u and %u\n", *(pChk + index % 4), *(pD + index));
#endif
        *(pChk + index % 4) ^= *(pD + index);

        index++;
        bytes--;
    }

    return checksum;
}

static eepromError checksumUpdate(eepromStore * store)
{
    store->checksum = generateChecksum(&(store->data), sizeof(store->data));
    return EEPROM_ERROR_OK;
}

static eepromError checksumOk(eepromStore * store)
{
    if (generateChecksum(store, sizeof(*store)) == 0)
    {
        return EEPROM_ERROR_TRUE;
    }

    return EEPROM_ERROR_FALSE;
}


#if 0
bool chibios_test_eeprom(void)
{
    eepromStore tempStore;
    eepromStore tempStoreRead;
    uint32_t chksum;
    bool b;

    memset(&tempStore,0,sizeof(tempStore));
    memset(&tempStoreRead,0,sizeof(tempStore));

    tempStore.data.lastShutdownError = 0x55;
    tempStore.data.unresponsiveShutdowns = 0x53;

    chksum = generateChecksum(&tempStore, sizeof(tempStore));

    PRINT("generated checksum: %x\n", chksum);
    checksumUpdate(&tempStore);
    b = checksumOk(&tempStore);
    PRINT("generated ok?: %d\n", b);

    /* Write stucture to eeprom */
    eepromWriteWords(STORE_ADDR_EEPROM, (uint32_t*)&tempStore, sizeof(tempStore));

    /* Read Structure from eeprom */
    eepromReadWords(STORE_ADDR_EEPROM, (uint32_t*)&tempStoreRead, sizeof(tempStoreRead));
 
    b = checksumOk(&tempStoreRead);
    PRINT("Read ok?: %d\n", b);

    if (memcmp(&tempStore, &tempStoreRead, sizeof(tempStore)) == 0)
    {
        PRINT("memcmp ok", NULL);
    }
    else
    {
        PRINT("memcmp NOT ok", NULL);
    }

    b = checksumOk(&tempStoreRead);

    PRINT("generated ok?: %d\n", b);

    return false;
}
#endif

eepromError eepromStoreGet(eepromStore * store)
{
    return eepromReadWords(STORE_ADDR_EEPROM, (uint32_t*)store, sizeof(*store));
}

eepromError eepromStorePut(const eepromStore * store)
{
    return eepromWriteWords(STORE_ADDR_EEPROM, (const uint32_t*)store, sizeof(*store));
}


eepromError eepromWasLastShutdownOk(void)
{
    eepromStore data;
    
    eepromStoreGet(&data);
    
    if (checksumOk(&data) != EEPROM_ERROR_TRUE)
    {
        return EEPROM_ERROR_CORRUPT;
    }

    if (data.data.lastShutdownError == 0)
    {
        return EEPROM_ERROR_TRUE;
    }
    
    return EEPROM_ERROR_FALSE;
}

eepromError eepromWipeStore(void)
{
    eepromStore data;
    eepromError rtn;
    
    memset(&data, 0, sizeof(data));

    if ((rtn = checksumUpdate(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }
    else if ((rtn = eepromStorePut(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }

    return EEPROM_ERROR_OK;
}

eepromError eepromAcknowledgeLastShutdownError(void)
{
    eepromStore data;
    eepromError rtn;

    if ((rtn = eepromStoreGet(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }

    data.data.lastShutdownError = 0;

    if ((rtn = checksumUpdate(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }
    else if ((rtn = eepromStorePut(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }

    return EEPROM_ERROR_OK;
}

eepromError eepromRecordUnresponsiveShutdown(void)
{
    eepromStore data;
    eepromError rtn;
    
    if ((rtn = eepromStoreGet(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }

    if (checksumOk(&data) != EEPROM_ERROR_TRUE)
    {
        memset(&data, 0, sizeof(data));
    }
    
    data.data.lastShutdownError = 1;
    data.data.unresponsiveShutdowns++;
    
    if ((rtn = checksumUpdate(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }
    else if ((rtn = eepromStorePut(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }
    else 
    {
        return EEPROM_ERROR_OK;
    }

    return EEPROM_ERROR_MISC;
}

#if 0
eepromError eepromRecordShutdown(void)
{
    eepromStore data;
    eepromError rtn;
    
    if ((rtn = eepromStoreGet(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }

    if (checksumOk(&data) != EEPROM_ERROR_TRUE)
    {
        memset(&data, 0, sizeof(data));
        PRINT_ERROR();
    }
    
    data.data.shutdowns++;
    
    PRINT("Shutdowns now :%u", data.data.shutdowns);
    PRINT("Shutdowns now :%u", data.data.shutdowns);
    PRINT("Shutdowns now :%u", data.data.shutdowns);

    if ((rtn = checksumUpdate(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }
    else if ((rtn = eepromStorePut(&data)) != EEPROM_ERROR_OK)
    {
        return rtn;
    }
    else 
    {
        return EEPROM_ERROR_OK;
    }

    return EEPROM_ERROR_MISC;
}

#endif
