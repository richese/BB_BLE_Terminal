/*
 * spi_interface.c
 *
 *  Created on: 27.7.2017
 *      Author: zbdjyq
 */

#include "rsl10.h"

#include "../include/app.h"
#include "../include/spi_interface.h"


volatile SPI_IF_Type spi_if;


extern void SPI_IF_Init(void)
{
	spi_if.state = SPI_IF_STATE_IDLE;
	// TODO: set buffer pointers from init arguments instead
	spi_if.tx_buf = app_env.spi1_tx_value;
	spi_if.rx_buf = app_env.spi1_rx_value;
	spi_if.tx_msg_len = 0;
	spi_if.rx_msg_len = 0;
	spi_if.tx_msg_pos = 0;
	spi_if.buf_len = 60;

	// Configure pins for use by SPI1.
	Sys_SPI_DIOConfig(SPI_IF_SPI_ID,
			          SPI1_SELECT_SLAVE,
				      DIO_LPF_DISABLE | DIO_WEAK_PULL_UP,
				      SPI_IF_PIN_CLK,
				      SPI_IF_PIN_CS,
				      SPI_IF_PIN_MOSI,
				      SPI_IF_PIN_MISO);

	// Configure SPI1 peripheral.
	Sys_SPI_Config(SPI_IF_SPI_ID,
                   SPI1_SELECT_SLAVE | SPI1_ENABLE | SPI1_CLK_POLARITY_NORMAL |
                   SPI1_UNDERRUN_INT_ENABLE| SPI1_CONTROLLER_CM3 |
                   SPI1_MODE_SELECT_AUTO | SPI1_PRESCALE_16);

	// Configure SPI1 transfer parameters.
	Sys_SPI_TransferConfig(SPI_IF_SPI_ID,
			               SPI1_IDLE | SPI1_RW_DATA | SPI1_CS_1 | SPI1_WORD_SIZE_32);

	// Enable interrupt on CS pin.
	Sys_DIO_IntConfig(0,
			          DIO_SRC_DIO_9 | DIO_EVENT_TRANSITION | DIO_DEBOUNCE_DISABLE,
			          DIO_DEBOUNCE_SLOWCLK_DIV32,
			          SPI_IF_PIN_CS);

	// Force MISO pin to input mode before first transaction.
	Sys_DIO_Config(SPI_IF_PIN_MISO,
			       DIO_MODE_INPUT | DIO_WEAK_PULL_UP | DIO_LPF_DISABLE);

	// Set interrupt pin to signal master that message was received.
	Sys_DIO_Config(SPI_IF_PIN_INT,
			       DIO_WEAK_PULL_UP | DIO_MODE_GPIO_OUT_1);
	Sys_GPIO_Set_High(SPI_IF_PIN_INT);
}


size_t SPI_IF_MessagePending(void)
{
	return spi_if.tx_msg_len;
}

void SPI_IF_SetMessage(const char *msg, size_t msg_len)
{
	if (msg_len > spi_if.buf_len)
	{
		msg_len = spi_if.buf_len;
	}

	memcpy(spi_if.tx_buf, msg, msg_len);
	spi_if.tx_msg_len = msg_len;
	spi_if.tx_msg_pos = 0;
	// Signal Master that we have new message.
	Sys_GPIO_Set_Low(SPI_IF_PIN_INT);
}


uint32_t SPI_IF_GetMessage(char *msg, uint16_t *msg_len)
{
	uint32_t got_message = 0;

	if (msg != NULL && spi_if.state == SPI_IF_STATE_IDLE && spi_if.rx_msg_len > 0)
	{
		memcpy(msg, spi_if.rx_buf, spi_if.rx_msg_len);
		*msg_len = spi_if.rx_msg_len;
		spi_if.rx_msg_len = 0;
		got_message = 1;
	}

	return got_message;
}


static uint32_t SPI_IF_GetRecvByte(void)
{
	uint32_t val = SPI_IF_CMD_DONE;

	if (spi_if.tx_msg_pos < spi_if.tx_msg_len)
	{
		val = spi_if.tx_buf[spi_if.tx_msg_pos];
		spi_if.tx_msg_pos += 1;
	}

	if (val == SPI_IF_CMD_DONE)
	{
		spi_if.tx_msg_pos = 0;
		spi_if.tx_msg_len = 0;
	}

	return val;
}


void DIO0_IRQHandler(void)
{
	uint32_t cs_state = DIO->DATA & (1 << SPI_IF_PIN_CS);

	if (cs_state == 0)
	{
		// CS enabled -> Set as SPI serial output.
		Sys_DIO_Config(SPI_IF_PIN_MISO,
				       DIO_MODE_SPI1_SERO);
	}
	else
	{
		// CS disabled -> Set as input.
		Sys_DIO_Config(SPI_IF_PIN_MISO,
				       DIO_MODE_INPUT | DIO_WEAK_PULL_UP | DIO_LPF_DISABLE);

		spi_if.state = SPI_IF_STATE_IDLE;
	}
}


void SPI1_RX_IRQHandler(void)
{
	const uint32_t rx_data = SPI1->RX_DATA;

	switch (spi_if.state)
	{
		// Start new transaction.
		case SPI_IF_STATE_IDLE:
		{
			switch (rx_data)
			{
				// Start message transaction (Master -> RSL10).
				// Message to be sent over BLE.
				case SPI_IF_CMD_SEND:
				{
					spi_if.state = SPI_IF_STATE_SEND_REQ;
					// clear msg receive buffer
					memset(spi_if.rx_buf, 0, spi_if.buf_len);
					spi_if.rx_msg_len = 0;
					// echo back empty byte
					SPI1->TX_DATA = 0;
				} break;

				// Start message transaction (RSL10 -> Master).
				// Message received over BLE will be send to Master.
				case SPI_IF_CMD_RECV:
				{
					spi_if.state = SPI_IF_STATE_RECV_REQ;
					// Clear pending message signal.
					Sys_GPIO_Set_High(SPI_IF_PIN_INT);
					// Send first character of received message.
					const uint32_t tx_data = SPI_IF_GetRecvByte();
					SPI1->TX_DATA = tx_data;
					if (tx_data == SPI_IF_CMD_DONE)
					{
						// Go back to IDLE if reached end of message.
						spi_if.state = SPI_IF_STATE_IDLE;
					}
				} break;

				// Receiving any other characters in this state is considered an error.
				default:
					SPI1->TX_DATA = SPI_IF_CMD_ERROR;
			}
		} break;

		// Continue Master -> RSL10 transaction.
		case SPI_IF_STATE_SEND_REQ:
		{
			// Master signalizes end of message.
			if (rx_data == SPI_IF_CMD_DONE)
			{
				// Return to IDLE. App main loop will forward message to BLE.
				spi_if.state = SPI_IF_STATE_IDLE;
			}
			else
			{
				// check for buffer overflow and receive data
				if (spi_if.rx_msg_len < spi_if.buf_len)
				{
					spi_if.rx_buf[spi_if.rx_msg_len] = rx_data;
					spi_if.rx_msg_len += 1;
					// echo back received data
					SPI1->TX_DATA = rx_data;
				}
				else
				{
					// Silently ignore characters of oversized messages.
					// FIXME: Should signal error?
					SPI1->TX_DATA = rx_data;
				}
			}
		} break;

		// Continue RSL10 -> Master transaction.
		case SPI_IF_STATE_RECV_REQ:
		{
			// Master signalizes that that it cannot receive more characters.
			// Stop transaction but keep remaining data for next transaction (data stream).
			if (rx_data == SPI_IF_CMD_DONE)
			{
				spi_if.state = SPI_IF_STATE_IDLE;
				SPI1->TX_DATA = 0;
				// Signal Master that there are still characters left.
				Sys_GPIO_Set_Low(SPI_IF_PIN_INT);
			}
			else
			{
				const uint32_t tx_data = SPI_IF_GetRecvByte();
				SPI1->TX_DATA = tx_data;

				if (tx_data == SPI_IF_CMD_DONE)
				{
					// Go back to IDLE if reached end of message.
					spi_if.state = SPI_IF_STATE_IDLE;
					// Make sure INT signal is off.
					Sys_GPIO_Set_High(SPI_IF_PIN_INT);
				}
			}
		} break;
	}
}


void SPI1_ERROR_IRQHandler(void)
{

}
