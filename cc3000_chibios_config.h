/** @file
    @brief Contains all configuration to use this library with ChibiOS/RT.
    @details This should be copied into the user project directory and included
             in the build process from there, allowing multiple projects to use
             this library with different configurations. */
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

/** @defgroup config ChibiOS/RT Configuration
 *  @brief Configuration settings to setup the CC3000 library to use in
 *         your ChibiOS/RT project. See documentation for
 *         cc3000_chibios_config.h.
 *  @{
 */

#ifndef __CHIBIOS_CC3000_CONFIG__
#define __CHIBIOS_CC3000_CONFIG__

#include "ch.h"
#include "hal.h"

/**** For Driver control ****/
/** @brief Sets whether the EXT driver is used by anything other than the
 *         CC3000 driver.
 *  @details This is used to permit the driver to be stopped when not
 *           in use. */
#define CHIBIOS_CC3000_EXT_EXCLUSIVE        TRUE

/** @brief Sets whether the SPI driver is used by anything other than the
 *         CC3000 driver.
 *  @details When FALSE, ChibiOS SPI mutex support will be utilised to 
 *           safely acquire the bus before altering the configuration / 
 *           communicating with the CC3000. This obviously depends on the user
 *           correctly protecting the SPI driver alterations external to this
 *           library.
 *  @warning Use when FALSE has not currently been tested.
 *           This is likely to be an unpopular option however
 *           since I believe you would need a buffer between the MCU and CC3000,
 *           due to the CC3000 seemingly not being designed to share a bus. See
 *           Table 1 CC3000 Module Pins Description in the CC3000
 *           datasheet for more information. */
#define CHIBIOS_CC3000_SPI_EXCLUSIVE        TRUE

/**** Interrupt pin ****/
/** @brief Port being used for interrupt pin monitoring. */
#define CHIBIOS_CC3000_IRQ_PORT             GPIOB
/** @brief EXT mode being used for interrupt pin monitoring. */
#define CHIBIOS_CC3000_IRQ_EXT_MODE         EXT_MODE_GPIOB
/** @brief Pin being used for interrupt pin monitoring. */
#define CHIBIOS_CC3000_IRQ_PAD              9

/**** WLAN Pin / VBAT_SW_EN ****/
/** @brief Port being used for WLAN Enable control. */
#define CHIBIOS_CC3000_WLAN_EN_PORT         GPIOB
/** @brief Pin being used for WLAN Enable control. */
#define CHIBIOS_CC3000_WLAN_EN_PAD          8

/**** SPI_DRIVER ****/
/** @brief Port being used for communications to CC3000. */
#define CHIBIOS_CC3000_PORT                 GPIOB
/** @brief Pin used for SPI chip select of CC3000. */
#define CHIBIOS_CC3000_NSS_PAD              12
/** @brief Pin used for SPI clock */
#define CHIBIOS_CC3000_SCK_PAD              13
/** @brief Pin used for SPI MISO aka CC3000 DOUT. */
#define CHIBIOS_CC3000_MISO_PAD             14
/** @brief Pin used for SPI MOSI aka CC3000 DIN. */
#define CHIBIOS_CC3000_MOSI_PAD             15

/**** Size of the IRQ thread ****/
/** @brief Working area size of the IRQ thread. */
#define CHIBIOS_CC3000_IRQ_THD_AREA         512
/** @brief Priority of the IRQ thread. Should be higher than the thread
 *         using the CC3000 API. */
#define CHIBIOS_CC3000_IRQ_THD_PRIO         (NORMALPRIO + 1)

/*****************************************************************************/
/* Under ordinary circumstances, below here should not need to be altered.   */
/*****************************************************************************/

/**** Debug Helpers  ****/
/**@brief Set to TRUE to enable basic debug print from the SPI Driver. */
#define CHIBIOS_CC3000_DBG_PRINT_ENABLED    FALSE

/**@brief Serial driver to use for debug print.
 * @details This must be configured before calling #cc3000ChibiosWlanInit().
 *          Only required if #CHIBIOS_CC3000_DBG_PRINT_ENABLED is TRUE. */
#define CHIBIOS_CC3000_DBG_PRINT_DRIVER     SD1

/**@brief Sets if #CHIBIOS_CC3000_DBG_PRINT_MTX is used to protect chprintf's.
 * @details  Only required if #CHIBIOS_CC3000_DBG_PRINT_ENABLED is TRUE. */
#define CHIBIOS_CC3000_DBG_PRINT_USE_MTX    TRUE

/**@brief A mutex variable to protect debug print statements.
 * @details This mutex is needed to prevent occasional muddled text due to
 *          threading. 
 * @warning It must be globally available i.e. must not be declared static and 
 *          must be initialised before calling any CC3000 function.
 *          This same mutex should protect all chprintf statements using the 
 *          driver specified by #CHIBIOS_CC3000_DBG_PRINT_DRIVER. */
#define CHIBIOS_CC3000_DBG_PRINT_MTX        printMtx

/** @def CHIBIOS_CC3000_DBG_PRINT_MTX_LK
 *  @brief Locks the mutex protecting chprintf messages.
 *  @details Uses #CHIBIOS_CC3000_DBG_PRINT_MTX if 
 *            #CHIBIOS_CC3000_DBG_PRINT_USE_MTX is TRUE. */

/** @def CHIBIOS_CC3000_DBG_PRINT_MTX_UNLK
 *  @brief Unlocks the mutex protecting chprintf messages.
 *  @details Uses #CHIBIOS_CC3000_DBG_PRINT_MTX if 
 *            #CHIBIOS_CC3000_DBG_PRINT_USE_MTX is TRUE. */

/** @def CHIBIOS_CC3000_DBG_PRINT
 *  @brief Debug message print.
 *  @details Only if #CHIBIOS_CC3000_DBG_PRINT_ENABLED is TRUE. Uses the serial
 *           driver specified in #CHIBIOS_CC3000_DBG_PRINT_DRIVER.
 *  @param fmt Formatted string, appropriate for chprintf().
 *  @param ... Values for placeholders in @p fmt. */

/** @def CHIBIOS_CC3000_DBG_PRINT_HEX
 *  @brief Debug hex print.
 *  @details Only if #CHIBIOS_CC3000_DBG_PRINT_ENABLED is TRUE. Uses the serial
 *           driver specified in #CHIBIOS_CC3000_DBG_PRINT_DRIVER.
 *  @param DATA Memory address of first element to print.
 *  @param LEN  Length of data to print. */

#if CHIBIOS_CC3000_DBG_PRINT_USE_MTX == TRUE
    /** @brief Allows the use of the mutex defined by 
     *         #CHIBIOS_CC3000_DBG_PRINT_MTX.
     *  @details Only if CHIBIOS_CC3000_DBG_PRINT_USE_MTX is TRUE. */
    extern Mutex CHIBIOS_CC3000_DBG_PRINT_MTX;
#endif

#if CHIBIOS_CC3000_DBG_PRINT_USE_MTX == FALSE            
    #define CHIBIOS_CC3000_DBG_PRINT_MTX_LK()       
    #define CHIBIOS_CC3000_DBG_PRINT_MTX_UNLK()     
#else
    #define CHIBIOS_CC3000_DBG_PRINT_MTX_LK()       \
        chMtxLock(&CHIBIOS_CC3000_DBG_PRINT_MTX)
    #define CHIBIOS_CC3000_DBG_PRINT_MTX_UNLK()     \
        chMtxUnlock()
#endif

#if CHIBIOS_CC3000_DBG_PRINT_ENABLED == TRUE
    #include "chprintf.h"

    #define CHIBIOS_CC3000_DBG_PRINT(fmt, ...)                              \
        CHIBIOS_CC3000_DBG_PRINT_MTX_LK();                                  \
        chprintf((BaseSequentialStream*)&CHIBIOS_CC3000_DBG_PRINT_DRIVER,   \
                 "(%s:%d) " fmt "\n\r", __FILE__, __LINE__, __VA_ARGS__);   \
        CHIBIOS_CC3000_DBG_PRINT_MTX_UNLK();

    #define CHIBIOS_CC3000_DBG_PRINT_HEX(DATA, LEN)                           \
    {                                                                         \
        int i = 0;                                                            \
        CHIBIOS_CC3000_DBG_PRINT_MTX_LK();                                    \
        while (i++ != LEN)                                                    \
        {                                                                     \
            chprintf((BaseSequentialStream*)&CHIBIOS_CC3000_DBG_PRINT_DRIVER, \
                    "0x%02x ", DATA[i]);                                      \
        }                                                                     \
        chprintf((BaseSequentialStream*)&CHIBIOS_CC3000_DBG_PRINT_DRIVER,     \
                  "\r\n");                                                    \
        CHIBIOS_CC3000_DBG_PRINT_MTX_UNLK();                                  \
    }
#else
    #define CHIBIOS_CC3000_DBG_PRINT(fmt, ...)
    #define CHIBIOS_CC3000_DBG_PRINT_HEX(DATA, LEN)
#endif

/**** Check Config ****/
/* This library requires SPI to be blocking. */
#if (!defined(SPI_USE_WAIT)) || (SPI_USE_WAIT == FALSE)
    #error "SPI_USE_WAIT should be defined as TRUE."
#endif

/* If the SPI bus is not exclusive, we need mutex protection enabled. */
#if (CHIBIOS_CC3000_SPI_EXCLUSIVE == FALSE)
    #if (!defined(SPI_USE_MUTUAL_EXCLUSION)) || (SPI_USE_MUTUAL_EXCLUSION == FALSE)
    #error "If the SPI bus is not exclusive, mutexs should be used."
    #endif
#endif


/** @} */


#endif /*__CHIBIOS_CC3000_CONFIG__*/

