/**
 *
 * \file
 *
 * \brief WINC1500 configuration.
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef CONF_WINC_H_INCLUDED
#define CONF_WINC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "board.h"

/*
   ---------------------------------
   ---------- PIN settings ---------
   ---------------------------------
*/

//#define CONF_WINC_PIN_RESET				PIN_PA15
#define CONF_WINC_PIN_RESET				PIN_PA03
#define CONF_WINC_PIN_CHIP_ENABLE		PIN_PB09
#define CONF_WINC_PIN_WAKE				PIN_PB08

/*
   ---------------------------------
   ---------- SPI settings ---------
   ---------------------------------
*/

#define CONF_WINC_USE_SPI				(1)

/** SPI pin and instance settings. */
#define CONF_WINC_SPI_MODULE			SERCOM0
#define CONF_WINC_SPI_SERCOM_MUX		SPI_SIGNAL_MUX_SETTING_D
#define CONF_WINC_SPI_PINMUX_PAD0		PINMUX_PA04D_SERCOM0_PAD0 /* in */
#define CONF_WINC_SPI_PINMUX_PAD1		PINMUX_PA05D_SERCOM0_PAD1 /* cs driven from software */
#define CONF_WINC_SPI_PINMUX_PAD2		PINMUX_UNUSED /* out */
#define CONF_WINC_SPI_PINMUX_PAD3		PINMUX_PA07D_SERCOM0_PAD3 /* sck */
#define CONF_WINC_SPI_CS_PIN			PIN_PA06

/** SPI interrupt pin. */
//#define CONF_WINC_SPI_INT_PIN			PIN_PA14
/*
#define CONF_WINC_SPI_INT_PIN			PIN_PA14A_EIC_EXTINT14
#define CONF_WINC_SPI_INT_MUX			MUX_PA14A_EIC_EXTINT14
#define CONF_WINC_SPI_INT_EIC			(14)
*/
										
#define CONF_WINC_SPI_INT_PIN			PIN_PA16A_EIC_EXTINT0
#define CONF_WINC_SPI_INT_MUX			MUX_PA16A_EIC_EXTINT0
#define CONF_WINC_SPI_INT_EIC			(0)

/** SPI clock. */
#define CONF_WINC_SPI_CLOCK				(12000000)
//#define CONF_WINC_SPI_CLOCK				(6000000)

/*
   ---------------------------------
   --------- Debug Options ---------
   ---------------------------------
*/

#define CONF_WINC_DEBUG					(1)
#define CONF_WINC_PRINTF				printf

#ifdef __cplusplus
}
#endif

#endif /* CONF_WINC_H_INCLUDED */
