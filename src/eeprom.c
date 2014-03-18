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

#define EEPROM_BASE     0x08080000
#define EEPROM_LAST     0x08081FFF
#define EEPROM_SIZE     (EEPROM_LAST - EEPROM_BASE + 1)

#define compile_time_assert(X)                                              \
    extern int (*compile_assert(void))[sizeof(struct {                      \
                unsigned int compile_assert_failed : (X) ? 1 : -1; })]

typedef struct {
    uint32_t lastShutdownError; /* True or False */
    uint32_t unresponsiveShutdowns;
} eepromData;

typedef struct {
    eepromData data;
    uint32_t checksum;
} eepromStore;

compile_time_assert(sizeof(eepromData) == 8); /* We don't want any padding */

#if 0
static eepromStore * const pStore = (eepromStore *)EEPROM_BASE;
static eepromData * const  pData = &(((eepromStore*)EEPROM_BASE)->data);
#else 
static eepromStore store;
static eepromStore * const pStore = &store;
static eepromData * const pData = &(pStore->data);
#endif

uint32_t generateChecksum(const void * data, uint32_t bytes)
{
    uint32_t checksum = 0;
    uint8_t index = 0;
    uint8_t * pChk = (uint8_t *)&checksum;
    const uint8_t * pD = (uint8_t *)data;

    while(bytes > 0)
    {
        printf("xor: %u and %u\n", *(pChk + index % 4), *(pD + index));
        *(pChk + index % 4) ^= *(pD + index);

        index++;
        bytes--;
    }

    return checksum;
}

bool checksumUpdate(void)
{
    pStore->checksum = generateChecksum(pData, sizeof(*pData));
    return true;
}

bool checksumOk(void)
{
    uint32_t generatedChecksum = generateChecksum(pData, sizeof(*pData));

    if (generatedChecksum == pStore->checksum)
    {
        return true;
    }
    return false;
}


int main(void)
{
#include <stdio.h>
    memset(pStore, 0, sizeof(*pStore));
    pStore->data.lastShutdownError = 0x55;
    pStore->data.unresponsiveShutdowns = 0x53;
    printf("generated checksum: %x\n", generateChecksum(pData, sizeof(*pData)));
    checksumUpdate();
    printf("generated ok?: %d\n", checksumOk());

    return 0;
}

bool chibios_test_eeprom(void)
{
    memset(pStore, 0, sizeof(*pStore));
    pStore->data.lastShutdownError = 0x55;
    pStore->data.unresponsiveShutdowns = 0x53;
    chprintf("generated checksum: %x\n", generateChecksum(pData, sizeof(*pData)));
    checksumUpdate();
    chprintf("generated ok?: %d\n", checksumOk());

    pStore->data.unresponsiveShutdowns = 0x50;
    chprintf("generated ok?: %d\n", checksumOk());

}

