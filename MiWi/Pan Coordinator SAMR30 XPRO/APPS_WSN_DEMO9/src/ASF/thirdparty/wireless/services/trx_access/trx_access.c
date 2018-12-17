/*
 * @file trx_access.c
 *
 * @brief Performs interface functionalities between the PHY layer and ASF
 * drivers
 * Copyright (c) 2014-2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
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
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
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
 *
 */

/*
 * Copyright (c) 2014-2015 Atmel Corporation. All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

#include "board.h"
#if SAMD || SAMR21 || SAML21 || SAMR30
#include "spi.h"
#else
#include "spi_master.h"
#endif
#include "trx_access.h"
#include "delay.h"
#include "interrupt.h"
#include "conf_board.h"
#include "asf.h"
#include "framework/driver/drv_miwi_at86rf.h"

static irq_handler_t irq_hdl_trx = NULL;

#if SAMD || SAMR21 || SAML21 || SAMR30
struct spi_slave_inst_config slave_dev_config;
struct spi_config config;
struct spi_module master_1;
struct spi_slave_inst slave;
uint16_t dummy_read;
#else
struct spi_device SPI_AT86RFX_DEVICE = {
	/* ! Board specific select id */
	.id = AT86RFX_SPI_CS
};
#endif

#if SAMD || SAMR21 || SAML21 || SAMR30
void AT86RFX_ISR(void);

void AT86RFX_ISR(void)
#else
AT86RFX_ISR()
#endif

{
	/*Clearing the RF interrupt*/
	trx_irq_flag_clr();
	PHY_PacketReceieved();

}

void trx_spi_init(void)
{
	/* Initialize SPI in master_1 mode to access the transceiver */
#if SAMD || SAMR21 || SAML21 || SAMR30
	spi_slave_inst_get_config_defaults(&slave_dev_config);
	slave_dev_config.ss_pin = AT86RFX_SPI_CS;
	spi_attach_slave(&slave, &slave_dev_config);
	spi_get_config_defaults(&config);
	config.mux_setting = AT86RFX_SPI_SERCOM_MUX_SETTING;
	config.mode_specific.master.baudrate = AT86RFX_SPI_BAUDRATE;
	config.pinmux_pad0 = AT86RFX_SPI_SERCOM_PINMUX_PAD0;
	config.pinmux_pad1 = AT86RFX_SPI_SERCOM_PINMUX_PAD1;
	config.pinmux_pad2 = AT86RFX_SPI_SERCOM_PINMUX_PAD2;
	config.pinmux_pad3 = AT86RFX_SPI_SERCOM_PINMUX_PAD3;
	spi_init(&master_1, AT86RFX_SPI, &config);
	spi_enable(&master_1);

	struct extint_chan_conf eint_chan_conf;
	extint_chan_get_config_defaults(&eint_chan_conf);
	eint_chan_conf.gpio_pin = AT86RFX_IRQ_PIN;
	eint_chan_conf.gpio_pin_mux = AT86RFX_IRQ_PINMUX;
	eint_chan_conf.gpio_pin_pull      = EXTINT_PULL_DOWN;
	#if (SAML21 || SAMR30)
	eint_chan_conf.enable_async_edge_detection = false;
	#else
	eint_chan_conf.wake_if_sleeping    = true;
	#endif
	eint_chan_conf.filter_input_signal = false;
	eint_chan_conf.detection_criteria  = EXTINT_DETECT_RISING;
	extint_chan_set_config(AT86RFX_IRQ_CHAN, &eint_chan_conf);
	extint_register_callback(AT86RFX_ISR, AT86RFX_IRQ_CHAN,
			EXTINT_CALLBACK_TYPE_DETECT);

#else
	spi_master_1_init(AT86RFX_SPI);
	spi_master_1_setup_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE, SPI_MODE_0,
			AT86RFX_SPI_BAUDRATE, 0);
	spi_enable(AT86RFX_SPI);
	AT86RFX_INTC_INIT();
#endif
}

void PhyReset(void)
{
	/* Ensure control lines have correct levels. */
	RST_HIGH();
	SLP_TR_LOW();

	/* Wait typical time of timer TR1. */
	delay_us(330);

	RST_LOW();
	delay_us(10);
	RST_HIGH();
}

uint8_t trx_reg_read(uint8_t addr)
{
#if SAMD || SAMR21 || SAML21 || SAMR30
	uint16_t register_value = 0;
#else
	uint8_t register_value = 0;
#endif

	/*Saving the current interrupt status & disabling the global interrupt
	**/
	ENTER_TRX_CRITICAL_REGION();

	/* Prepare the command byte */
	addr |= READ_ACCESS_COMMAND;

#if SAMD || SAMR21 || SAML21 || SAMR30
	/* Start SPI transaction by pulling SEL low */
	spi_select_slave(&master_1, &slave, true);

	/* Send the Read command byte */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, addr);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, 0);
	while (!spi_is_write_complete(&master_1)) {
	}
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &register_value);

	/* Stop the SPI transaction by setting SEL high */
	spi_select_slave(&master_1, &slave, false);

#else
	/* Start SPI transaction by pulling SEL low */
	spi_select_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);

	/* Send the Read command byte */
	spi_write_packet(AT86RFX_SPI, &addr, 1);

	/* Do dummy read for initiating SPI read */
	spi_read_packet(AT86RFX_SPI, &register_value, 1);

	/* Stop the SPI transaction by setting SEL high */
	spi_deselect_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);
#endif

	/*Restoring the interrupt status which was stored & enabling the global
	 * interrupt */
	LEAVE_TRX_CRITICAL_REGION();

	return register_value;
}

void trx_reg_write(uint8_t addr, uint8_t data)
{
	/*Saving the current interrupt status & disabling the global interrupt
	**/
	ENTER_TRX_CRITICAL_REGION();

	/* Prepare the command byte */
	addr |= WRITE_ACCESS_COMMAND;

#if SAMD || SAMR21 || SAML21 || SAMR30
	/* Start SPI transaction by pulling SEL low */
	spi_select_slave(&master_1, &slave, true);

	/* Send the Read command byte */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, addr);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	/* Write the byte in the transceiver data register */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, data);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	/* Stop the SPI transaction by setting SEL high */
	spi_select_slave(&master_1, &slave, false);
#else
	/* Start SPI transaction by pulling SEL low */
	spi_select_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);

	/* Send the Read command byte */
	spi_write_packet(AT86RFX_SPI, &addr, 1);

	/* Write the byte in the transceiver data register */
	spi_write_packet(AT86RFX_SPI, &data, 1);

	/* Stop the SPI transaction by setting SEL high */
	spi_deselect_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);
#endif

	/*Restoring the interrupt status which was stored & enabling the global
	 * interrupt */
	LEAVE_TRX_CRITICAL_REGION();
}

void trx_irq_init(FUNC_PTR trx_irq_cb)
{
	/*
	 * Set the handler function.
	 * The handler is set before enabling the interrupt to prepare for
	 * spurious
	 * interrupts, that can pop up the moment they are enabled
	 */
	irq_hdl_trx = (irq_handler_t)trx_irq_cb;
}

uint8_t trx_bit_read(uint8_t addr, uint8_t mask, uint8_t pos)
{
	uint8_t ret;
	ret = trx_reg_read(addr);
	ret &= mask;
	ret >>= pos;
	return ret;
}

void trx_bit_write(uint8_t reg_addr, uint8_t mask, uint8_t pos,
		uint8_t new_value)
{
	uint8_t current_reg_value;
	current_reg_value = trx_reg_read(reg_addr);
	current_reg_value &= ~mask;
	new_value <<= pos;
	new_value &= mask;
	new_value |= current_reg_value;
	trx_reg_write(reg_addr, new_value);
}

void trx_frame_read(uint8_t *data, uint8_t length)
{
	/*Saving the current interrupt status & disabling the global interrupt
	**/
	ENTER_TRX_CRITICAL_REGION();

#if SAMD || SAMR21 || SAML21 || SAMR30
	uint16_t temp;
	/* Start SPI transaction by pulling SEL low */
	spi_select_slave(&master_1, &slave, true);

	temp = TRX_CMD_FR;

	/* Send the command byte */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, temp);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	while (length--) {
		while (!spi_is_ready_to_write(&master_1)) {
		}
		spi_write(&master_1, 0);
		while (!spi_is_write_complete(&master_1)) {
		}
		/* Dummy read since SPI RX is double buffered */
		while (!spi_is_ready_to_read(&master_1)) {
		}
		spi_read(&master_1, &temp);
		*data = temp;
		data++;
	}

	/* Stop the SPI transaction by setting SEL high */
	spi_select_slave(&master_1, &slave, false);
#else
	uint8_t temp;
	/* Start SPI transaction by pulling SEL low */
	spi_select_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);

	temp = TRX_CMD_FR;

	/* Send the command byte */
	spi_write_packet(AT86RFX_SPI, &temp, 1);

	spi_read_packet(AT86RFX_SPI, data, length);

	/* Stop the SPI transaction by setting SEL high */
	spi_deselect_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);
#endif

	/*Restoring the interrupt status which was stored & enabling the global
	 * interrupt */
	LEAVE_TRX_CRITICAL_REGION();
}

void trx_frame_write(uint8_t *data, uint8_t length)
{
	uint8_t temp;

	/*Saving the current interrupt status & disabling the global interrupt
	**/
	ENTER_TRX_CRITICAL_REGION();

#if SAMD || SAMR21 || SAML21 || SAMR30
	/* Start SPI transaction by pulling SEL low */
	spi_select_slave(&master_1, &slave, true);

	temp = TRX_CMD_FW;

	/* Send the command byte */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, temp);
	while (!spi_is_write_complete(&master_1)) {
	} 
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);
	while (length--) {
		while (!spi_is_ready_to_write(&master_1)) {
		}
		spi_write(&master_1, *data++);
		while (!spi_is_write_complete(&master_1)) {
		}
		/* Dummy read since SPI RX is double buffered */
		while (!spi_is_ready_to_read(&master_1)) {
		}
		spi_read(&master_1, &dummy_read);
	}

	/* Stop the SPI transaction by setting SEL high */
	spi_select_slave(&master_1, &slave, false);
#else
	/* Start SPI transaction by pulling SEL low */
	spi_select_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);

	temp = TRX_CMD_FW;

	/* Send the command byte */
	spi_write_packet(AT86RFX_SPI, &temp, 1);
	spi_write_packet(AT86RFX_SPI, data, length);

	/* Stop the SPI transaction by setting SEL high */
	spi_deselect_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);
#endif

	/*Restoring the interrupt status which was stored & enabling the global
	 * interrupt */
	LEAVE_TRX_CRITICAL_REGION();
}

/**
 * @brief Writes data into SRAM of the transceiver
 *
 * This function writes data into the SRAM of the transceiver
 *
 * @param addr Start address in the SRAM for the write operation
 * @param data Pointer to the data to be written into SRAM
 * @param length Number of bytes to be written into SRAM
 */
void trx_sram_write(uint8_t addr, uint8_t *data, uint8_t length)
{
	uint8_t temp;

	/*Saving the current interrupt status & disabling the global interrupt
	**/
	ENTER_TRX_CRITICAL_REGION();

#if SAMD || SAMR21 || SAML21 || SAMR30
	/* Start SPI transaction by pulling SEL low */
	spi_select_slave(&master_1, &slave, true);

	/* Send the command byte */
	temp = TRX_CMD_SW;

	/* Send the command byte */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, temp);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	/* Send the address from which the write operation should start */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, addr);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	while (length--) {
		while (!spi_is_ready_to_write(&master_1)) {
		}
		spi_write(&master_1, *data++);
		while (!spi_is_write_complete(&master_1)) {
		}
		/* Dummy read since SPI RX is double buffered */
		while (!spi_is_ready_to_read(&master_1)) {
		}
		spi_read(&master_1, &dummy_read);
	}

	/* Stop the SPI transaction by setting SEL high */
	spi_select_slave(&master_1, &slave, false);
#else
	/* Start SPI transaction by pulling SEL low */
	spi_select_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);

	/* Send the command byte */
	temp = TRX_CMD_SW;

	/* Send the command byte */
	spi_write_packet(AT86RFX_SPI, &temp, 1);
	while (!spi_is_tx_empty(AT86RFX_SPI)) {
	}

	/* Send the address from which the write operation should start */
	spi_write_packet(AT86RFX_SPI, &addr, 1);
	while (!spi_is_tx_empty(AT86RFX_SPI)) {
	}

	spi_write_packet(AT86RFX_SPI, data, length);

	/* Stop the SPI transaction by setting SEL high */
	spi_deselect_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);
#endif

	/*Restoring the interrupt status which was stored & enabling the global
	 * interrupt */
	LEAVE_TRX_CRITICAL_REGION();
}

/**
 * @brief Reads data from SRAM of the transceiver
 *
 * This function reads from the SRAM of the transceiver
 *
 * @param[in] addr Start address in SRAM for read operation
 * @param[out] data Pointer to the location where data stored
 * @param[in] length Number of bytes to be read from SRAM
 */
void trx_sram_read(uint8_t addr, uint8_t *data, uint8_t length)
{
	delay_us(1); /* wap_rf4ce */

	/*Saving the current interrupt status & disabling the global interrupt
	**/
	ENTER_TRX_CRITICAL_REGION();
#if SAMD || SAMR21 || SAML21 || SAMR30
	uint16_t temp;
	/* Start SPI transaction by pulling SEL low */
	spi_select_slave(&master_1, &slave, true);

	temp = TRX_CMD_SR;

	/* Send the command byte */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, temp);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	/* Send the address from which the read operation should start */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, addr);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	/* Upload the received byte in the user provided location */
	while (length--) {
		while (!spi_is_ready_to_write(&master_1)) {
		}
		spi_write(&master_1, 0);
		while (!spi_is_write_complete(&master_1)) {
		}
		/* Dummy read since SPI RX is double buffered */
		while (!spi_is_ready_to_read(&master_1)) {
		}
		spi_read(&master_1, &temp);
		*data = temp;
		data++;
	}

	/* Stop the SPI transaction by setting SEL high */
	spi_select_slave(&master_1, &slave, false);
#else
	uint8_t temp;
	/* Start SPI transaction by pulling SEL low */
	spi_select_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);

	temp = TRX_CMD_SR;

	/* Send the command byte */
	spi_write_packet(AT86RFX_SPI, &temp, 1);
	while (!spi_is_tx_empty(AT86RFX_SPI)) {
	}

	/* Send the command byte */
	spi_write_packet(AT86RFX_SPI, &addr, 1);
	while (!spi_is_tx_empty(AT86RFX_SPI)) {
	}

	/* Send the address from which the read operation should start */
	/* Upload the received byte in the user provided location */
	spi_read_packet(AT86RFX_SPI, data, length);

	/* Stop the SPI transaction by setting SEL high */
	spi_deselect_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);
#endif

	/*Restoring the interrupt status which was stored & enabling the global
	 * interrupt */
	LEAVE_TRX_CRITICAL_REGION();
}

/**
 * @brief Writes and reads data into/from SRAM of the transceiver
 *
 * This function writes data into the SRAM of the transceiver and
 * simultaneously reads the bytes.
 *
 * @param addr Start address in the SRAM for the write operation
 * @param idata Pointer to the data written/read into/from SRAM
 * @param length Number of bytes written/read into/from SRAM
 */
void trx_aes_wrrd(uint8_t addr, uint8_t *idata, uint8_t length)
{
	uint8_t *odata;
#if SAMD || SAMR21 || SAML21 || SAMR30
	uint16_t odata_var = 0;
#endif
	uint8_t temp;

	delay_us(1); /* wap_rf4ce */

	ENTER_TRX_REGION();

#ifdef NON_BLOCKING_SPI
	while (spi_state != SPI_IDLE) {
		/* wait until SPI gets available */
	}
#endif
#if SAMD || SAMR21 || SAML21 || SAMR30
	/* Start SPI transaction by pulling SEL low */
	spi_select_slave(&master_1, &slave, true);

	/* Send the command byte */
	temp = TRX_CMD_SW;

	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, temp);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	/* write SRAM start address */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, addr);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	/* now transfer data */
	odata = idata;

	/* write data byte 0 - the obtained value in SPDR is meaningless */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, *idata++);
	while (!spi_is_write_complete(&master_1)) {
	}
	/* Dummy read since SPI RX is double buffered */
	while (!spi_is_ready_to_read(&master_1)) {
	}
	spi_read(&master_1, &dummy_read);

	/* Reading Spi Data for the length specified */
	while (length > 0) {
		while (!spi_is_ready_to_write(&master_1)) {
		}
		spi_write(&master_1, *idata++);
		while (!spi_is_write_complete(&master_1)) {
		}
		while (!spi_is_ready_to_read(&master_1)) {
		}

#if SAMD || SAMR21 || SAML21 || SAMR30
		spi_read(&master_1, &odata_var);
		*odata++ = (uint8_t)odata_var;
#else
		spi_read(&master_1, (uint16_t *)odata++);
#endif
		length--;
	}

	/* To get the last data byte, write some dummy byte */
	while (!spi_is_ready_to_write(&master_1)) {
	}
	spi_write(&master_1, 0);
	while (!spi_is_write_complete(&master_1)) {
	}
	while (!spi_is_ready_to_read(&master_1)) {
	}
#if SAMD || SAMR21 || SAML21 || SAMR30
	spi_read(&master_1, &odata_var);
	*odata = (uint8_t)odata_var;
#else
	spi_read(&master_1, (uint16_t *)odata);
#endif

	/* Stop the SPI transaction by setting SEL high */
	spi_select_slave(&master_1, &slave, false);
#else
	/* Start SPI transaction by pulling SEL low */
	spi_select_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);

	/* Send the command byte */
	temp = TRX_CMD_SW;
	spi_write_packet(AT86RFX_SPI, &temp, 1);
	while (!spi_is_tx_empty(AT86RFX_SPI)) {
	}

	/* write SRAM start address */
	spi_write_packet(AT86RFX_SPI, &addr, 1);
	while (!spi_is_tx_empty(AT86RFX_SPI)) {
	}

	/* now transfer data */
	odata = idata;

	/* write data byte 0 - the obtained value in SPDR is meaningless */
	spi_write_packet(AT86RFX_SPI, idata++, 1);

	/* Reading Spi Data for the length specified */
	while (length > 0) {
		spi_write_packet(AT86RFX_SPI, idata++, 1);
		while (!spi_is_tx_empty(AT86RFX_SPI)) {
		}
		spi_read_single(AT86RFX_SPI, odata++);
		length--;
	}

	/* To get the last data byte, write some dummy byte */
	spi_read_packet(AT86RFX_SPI, odata, 1);

	/* Stop the SPI transaction by setting SEL high */
	spi_deselect_device(AT86RFX_SPI, &SPI_AT86RFX_DEVICE);
#endif
	LEAVE_TRX_REGION();
}

