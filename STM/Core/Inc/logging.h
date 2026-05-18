#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "main.h"

// SPI commands for SD/MMC cards, taken from:
// https://github.com/kiwih/cubeide-sd-card/blob/master/cubeide-sd-card/FATFS/Target/user_diskio_spi.c

/* MMC/SD command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC		0x01		/* MMC ver 3 */
#define CT_SD1		0x02		/* SD ver 1 */
#define CT_SD2		0x04		/* SD ver 2 */
#define CT_SDC		(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK	0x08		/* Block addressing */

// these are for setting the CS pin on the sd card to high or low
#define CS_HIGH()	{HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);}
#define CS_LOW()	{HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);}


/*
 * We write in 512 byte blocks, thus a 4GB card has 8,388,608 blocks, approximately 1 day at 60Hz
 * After that we start overwriting old data.
 * The card is formatted, such that the first block contains the write position, i.e. the position where the next block would go.
 * All other data is written in blocks, if a block's last byte in \0 this block is the last block of an entry and the next block start a new entry.
 * Ideally we therefore want our entry to be shorter than 512 bytes.
 */

struct msdCard {
    SPI_HandleTypeDef* _spi;
    uint32_t _first_block;
    uint32_t _num_blocks;
    uint32_t _write_pos;
};

bool makeCard(SPI_HandleTypeDef* spi, struct msdCard* card);
void freeCard(struct msdCard* card);
bool checkComm(struct msdCard* card);
bool log_str(struct msdCard* card, const char* str);

#endif
