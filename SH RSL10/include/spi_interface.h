/*
 * spi_interface.h
 *
 *  Created on: 27.7.2017
 *      Author: zbdjyq
 */

#ifndef SPI_INTERFACE_H_
#define SPI_INTERFACE_H_

#include "stdint.h"


#define SPI_IF_SPI_ID                  ((uint32_t) 1)

#define SPI_IF_PIN_CLK                 ((uint32_t)12)
#define SPI_IF_PIN_CS                  ((uint32_t) 9)
#define SPI_IF_PIN_MOSI                ((uint32_t)10) // seri
#define SPI_IF_PIN_MISO                ((uint32_t)11) // sero
#define SPI_IF_PIN_INT                 ((uint32_t)6)


#define SPI_IF_CMD_RECV                ((uint32_t)0xF0000001)
#define SPI_IF_CMD_SEND                ((uint32_t)0xF0000002)
#define SPI_IF_CMD_DONE                ((uint32_t)0xF0000004)
#define SPI_IF_CMD_ERROR               ((uint32_t)0xF0000010)


typedef enum
{
	/** No request from master. */
	SPI_IF_STATE_IDLE = 0,
	/** Read request for received message. */
	SPI_IF_STATE_RECV_REQ,
	/** Write request for sending message. */
	SPI_IF_STATE_SEND_REQ
} SPI_IF_State;


typedef struct
{
	SPI_IF_State state;
	uint8_t *tx_buf;
	uint8_t *rx_buf;
	size_t tx_msg_len;
	size_t rx_msg_len;
	size_t tx_msg_pos;
	size_t buf_len;

} SPI_IF_Type;


extern void SPI_IF_Init(void);

extern size_t SPI_IF_MessagePending(void);

extern void SPI_IF_SetMessage(const char *msg, size_t msg_len);

extern uint32_t SPI_IF_GetMessage(char *msg, uint16_t *msg_len);

extern void SPI1_RX_IRQHandler(void);

extern void SPI1_ERROR_IRQHandler(void);

extern void DIO0_IRQHandler(void);


#endif /* SPI_INTERFACE_H_ */
