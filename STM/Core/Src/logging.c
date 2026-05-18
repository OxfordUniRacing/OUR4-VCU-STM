#include <string.h>
#include <stdbool.h>


#include "main.h"
#include "logging.h"


uint8_t send_cmd (SPI_HandleTypeDef* spi, uint8_t cmd,	uint32_t arg);
bool read_block (struct msdCard* card, uint8_t* buffer, uint32_t sector);
bool write_block (struct msdCard* card, const uint8_t* buffer);
bool wait_ready (SPI_HandleTypeDef* spi, uint32_t wt);

inline void fclk_fast(SPI_HandleTypeDef* spi);
inline void fclk_slow(SPI_HandleTypeDef* spi);

uint8_t xchg_spi (SPI_HandleTypeDef* spi, uint8_t dat);
void despiselect (SPI_HandleTypeDef* spi);
int spiselect(SPI_HandleTypeDef* spi);

bool makeCard(SPI_HandleTypeDef* spi, struct msdCard* card)
{

    if (card == NULL) {
        return NULL;
    }
    card->_spi = spi;
    // block 0 is used to retain the write position between power cycles
    card->_first_block = 1;

    // query the card for the number of blocks
    uint8_t buffer[512];
    // send read command
    if (send_cmd(spi, CMD17, 0) != 0) {
        return false;
    }
    // receive the data block
    HAL_StatusTypeDef res = HAL_SPI_Receive(spi, buffer, 16, 100);
    if (res != HAL_OK) {
        return false;
    }
    // get the number of blocks
    uint8_t n;
    uint32_t csize;
    if ((buffer[0] >> 6) == 1) {	/* SDC ver 2.00 */
        csize = buffer[9] + ((uint16_t)buffer[8] << 8) + ((uint32_t)(buffer[7] & 63) << 16) + 1;
        card->_num_blocks = csize << 10;
    } else {					/* SDC ver 1.XX or MMC ver 3 */
        n = (buffer[5] & 15) + ((buffer[10] & 128) >> 7) + ((buffer[9] & 3) << 1) + 2;
        csize = (buffer[8] >> 6) + ((uint16_t)buffer[7] << 2) + ((uint16_t)(buffer[6] & 3) << 10) + 1;
        card->_num_blocks = csize << (n - 9);
    }


    // read the first block
    if (!read_block(card, buffer, 0)) {
        return false;
    }
    card->_write_pos = *(uint32_t*)buffer;

    return card;
}

void freeCard(struct msdCard* card) {
    // write current write position to block 0
    uint32_t buffer[512 / 4];
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = card->_write_pos;
    write_block(card, (uint8_t*)buffer);
    wait_ready(card->_spi, 100);

}

bool checkComm(struct msdCard* card) {
    // write a test pattern to the card
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));

    /*	//Want to  keep it to just ascii
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i;
    }
    */

    memcpy(buffer, "TEST-text, followed by numbers:", 32);
    if (!write_block(card, buffer)) {
        return false;
    }

    // read the block back
    uint8_t buffer2[512];
    if (!read_block(card, buffer2, card->_write_pos - 1)) {
        return false;
    }
    // compare the two buffers
    for (int i = 0; i < sizeof(buffer); i++) {
        if (buffer[i] != buffer2[i]) {
            return false;
        }
    }
    return true;
}

bool log_str(struct msdCard* card, const char* str) {
    // get string length
    char* p = str;
    uint32_t len = strlen(str) + 1; // +1 for null terminator
    uint8_t buffer[512];

    while (len > 0) {
        memset(buffer, 0, sizeof(buffer));
        // copy the string to the buffer
        uint32_t copy_len = len > sizeof(buffer) ? sizeof(buffer) : len;
        memcpy(buffer, str, copy_len);
        // write the buffer to the card
        if (!write_block(card, buffer)) {
            return false;
        }
        // update the string pointer and length
        p += copy_len;
        len -= copy_len;
    }
    return true;
}

/* --------------------------------------------------------------------- */
/*                 Base SD card commands                                 */
/* --------------------------------------------------------------------- */


/**
 * @brief  Initializes the micro SD card
 * 
 * This function implements the startup sequence for the SD card. It was written following general advice from
 * https://elm-chan.org/docs/mmc/mmc_e.html and the arduino code from https://github.com/kiwih/cubeide-sd-card/blob/master/cubeide-sd-card/FATFS/Target/user_diskio_spi.c
 * 
 * regular SD cards are NOT supported!! To support them more cases must be caught.
 * 
 * @param  card: pointer to the SD card structure
 */
bool initializeCard(struct msdCard* card) {
    // short initial delay
    HAL_Delay(10);

    // Go to low speed mode and set the CS pin high
    fclk_slow(card->_spi);
    CS_HIGH();

    // initially send at least 74 clocks with CS high
    for (int n = 10; n; n--) xchg_spi(card->_spi, 0xFF);

    // send CMD0 to reset the card
    if (send_cmd(card->_spi, CMD0, 0) != 1) {
        return false;
    }

    // if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
    //     for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);	/* Get 32 bit return value of R7 resp */
    //     if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* Is the card supports vcc of 2.7-3.6V? */
    //         while (SPI_Timer_Status() && send_cmd(ACMD41, 1UL << 30)) ;	/* Wait for end of initialization with ACMD41(HCS) */
    //         if (SPI_Timer_Status() && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
    //             for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);
    //             ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* Card id SDv2 */
    //         }
    //     }
    // } else {	/* Not SDv2 card */
    //     if (send_cmd(ACMD41, 0) <= 1) 	{	/* SDv1 or MMC? */
    //         ty = CT_SD1; cmd = ACMD41;	/* SDv1 (ACMD41(0)) */
    //     } else {
    //         ty = CT_MMC; cmd = CMD1;	/* MMCv3 (CMD1(0)) */
    //     }
    //     while (SPI_Timer_Status() && send_cmd(cmd, 0)) ;		/* Wait for end of initialization */
    //     if (!SPI_Timer_Status() || send_cmd(CMD16, 512) != 0)	/* Set block length: 512 */
    //         ty = 0;
    // }

    uint8_t ocr[4] = {0, 0, 0, 0};  
    int time; // time for waiting for initialization
    uint8_t ty = 0; // card type

    // send CMD8 to check for SDv2
    if (send_cmd(card->_spi, CMD8, 0x1AA) == 1) {
        // card is SDv2
        for (int n = 0; n < 4; n++) ocr[n] = xchg_spi(card->_spi, 0xFF);
        if (ocr[2] != 0x01 || ocr[3] != 0xAA) {
            // card does not support vcc of 2.7-3.6V
            return false;
        }
        time = HAL_GetTick();
        // wait for the card to be ready, this can take a bit (1s timeout)
        while (true)
        {
            if (send_cmd(card->_spi, ACMD41, 1UL << 30) == 0) {
                break;
            }
            if (HAL_GetTick() - time > 1000) {
                return false;
            }
        }
        // check the CCS bit in the OCR
        if (send_cmd(card->_spi, CMD58, 0) == 0) {
            for (int n = 0; n < 4; n++) ocr[n] = xchg_spi(card->_spi, 0xFF);
            ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; // check if byte or block mode
        }
    } else if (send_cmd(card->_spi, ACMD41, 0) == 1) {
        // card is SDv1
        ty = CT_SD1;
        time = HAL_GetTick();
        // wait for the card to be ready, this can take a bit (1s timeout)
        while (true)
        {
            if (send_cmd(card->_spi, ACMD41, 1UL << 30) == 0) {
                break;
            }
            if (HAL_GetTick() - time > 1000) {
                return false;
            }
        }
    } else {
        // card is MMC
        ty = CT_MMC;
        time = HAL_GetTick();
        // wait for the card to be ready, this can take a bit (1s timeout)
        while (true)
        {
            if (send_cmd(card->_spi, CMD1, 0) == 0) {
                break;
            }
            if (HAL_GetTick() - time > 1000) {
                return false;
            }
        }
    }

    // set the block length to 512 bytes
    if (send_cmd(card->_spi, CMD16, 512) != 0) {
        return false;
    }
    

    // If we reach here, the card is initialized successfully
    fclk_fast(card->_spi);
    return true;
}


bool write_block (struct msdCard* card, const uint8_t* buffer)
{
    // wait for the card to be ready
    // we do this before writing as the last write operation does not need to finish before we move on doing other stuff
    if (!wait_ready(card->_spi, 100)) {
        return false;
    }

    uint32_t sector = card->_write_pos;

    // send CMD24 to write a single block
    if (send_cmd(card->_spi, CMD24, sector) != 0) {
        return false;
    }

    // send data token
    HAL_SPI_Transmit(card->_spi, (uint8_t[]){0xFE}, 1, 100);
    // send the data block
    HAL_SPI_Transmit(card->_spi, buffer, 512, 100);
    // send dummy CRC
    HAL_SPI_Transmit(card->_spi, (uint8_t[]){0xFF, 0xFF}, 2, 100);

    // increment the write position
    card->_write_pos++;
    // check if we need to wrap around
    if (card->_write_pos >= card->_num_blocks) {
        card->_write_pos = card->_first_block;
    }

    return true;
}

bool read_block (struct msdCard* card, uint8_t* buffer, uint32_t sector)
{
    // sanity check sector in range
    //if (sector < card->_first_block || sector >= card->_num_blocks) {
	if (sector >= card->_num_blocks) { // Used for when sector = 0
        return false;
    }

    // prepare the buffer
    memset(buffer, 0, 512);

    // wait for the card to be ready
    if (!wait_ready(card->_spi, 100)) {
        return false;
    }

    // send CMD17 to read a single block
    if (send_cmd(card->_spi, CMD17, sector) != 0) {
        return false;
    }

    // wait for the data token, with a timeout of 100ms
    uint8_t token;
    uint32_t time = HAL_GetTick();

    do {
        token = xchg_spi(card->_spi, 0xFF);
    } while (token == 0xFF && (HAL_GetTick() - time < 100));

    if (token != 0xFE) {
        return false;
    }

    // read the data block
    HAL_SPI_Receive(card->_spi, buffer, 512, 100);

    return true;
}


/* --------------------------------------------------------------------- */
/*                 Underlying SPI functions                              */
/* --------------------------------------------------------------------- */


/**
 * @brief  Exchanges a byte with the SD card
 * 
 * This function sends a byte to the SD card and receives a byte back. It is used for sending commands and data.
 * 
 * @param  dat: byte to send
 * @return received byte
 */
uint8_t xchg_spi (SPI_HandleTypeDef* spi, uint8_t dat)
{
	uint8_t rxDat;
    HAL_SPI_TransmitReceive(spi, &dat, &rxDat, 1, 50);
    return rxDat;
}


/* Send a command to the SD card */
/**
 * @brief  Sends a command to the SD card
 * 
 * This function sends a command to the SD card and receives the response. It is used for sending commands to the SD card.
 * 
 * @param  cmd: command index
 * @param  arg: argument for the command
 * @return response from the SD card
 */
uint8_t send_cmd (SPI_HandleTypeDef* spi, uint8_t cmd,	uint32_t arg)
{
	uint8_t n, res;

	if (cmd & 0x80) {	/* Send a CMD55 prior to ACMD<n> */
		cmd &= 0x7F;
		res = send_cmd(spi, CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card and wait for ready except to stop multiple block read */
	if (cmd != CMD12) {
		despiselect(spi);
		if (!spiselect(spi)) return 0xFF;
	}

	/* Send command packet */
	xchg_spi(spi, 0x40 | cmd);				/* Start + command index */
	xchg_spi(spi, (uint8_t)(arg >> 24));		/* Argument[31..24] */
	xchg_spi(spi, (uint8_t)(arg >> 16));		/* Argument[23..16] */
	xchg_spi(spi, (uint8_t)(arg >> 8));		/* Argument[15..8] */
	xchg_spi(spi, (uint8_t)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	xchg_spi(spi, n);

	/* Receive command resp */
	if (cmd == CMD12) xchg_spi(spi, 0xFF);	/* Discard following one byte when CMD12 */
	n = 10;								/* Wait for response (10 bytes max) */
	do {
		res = xchg_spi(spi, 0xFF);
	} while ((res & 0x80) && --n);

	return res;							/* Return received response */
}


/**
 * @brief Send a command until the expected result or timeout
 * 
 * Send the command until the response matches the specified result. If the respond doesn't match the specified result
 * in timeout many ms the function aborts and return false.
 * 
 * @param spi The SPI handler to use
 * @param cmd The command to send
 * @param arg The command arguements
 * @param res The expected result
 * @param timeout The mximum time to wait for the expected result
 * @return true if the expected result is returned, false if the timeout is reached
 */
bool send_command_until_timeout(SPI_HandleTypeDef* spi, uint8_t cmd, uint32_t arg, uint8_t res, int timeout)
{
    int time = HAL_GetTick();
    while (true)
    {
        if (send_cmd(spi, cmd, arg) == res) {
            break;
        }
        if (HAL_GetTick() - time > 1000) {
            return false;
        }
    }
    return true;
}


/**
 * @brief  Waits for the SD card to be ready
 * 
 * This function waits for the SD card to be ready for the next command.
 * 
 * @param  card: pointer to the SD card structure
 * @param  wt: timeout in milliseconds
 * @return true if ready, false if timeout
 */
bool wait_ready (SPI_HandleTypeDef* spi, uint32_t wt)
{
	uint8_t d;
	uint32_t waitSpiTimerTickStart = HAL_GetTick();

	do {
		d = xchg_spi(spi, 0xFF);
	} while (d != 0xFF && ((HAL_GetTick() - waitSpiTimerTickStart) < wt));	/* Wait for card goes ready or timeout */

	return (d == 0xFF) ? true : false;
}


/**
 * @brief  Deselects the SD card and releases the SPI bus
 */
void despiselect (SPI_HandleTypeDef* spi)
{
	CS_HIGH();		/* Set CS# high */
	xchg_spi(spi, 0xFF);	/* Dummy clock (force DO hi-z for multiple slave SPI) */

}


/**
 * @brief  Selects the SD card and waits for it to be ready
 */
int spiselect(SPI_HandleTypeDef* spi)	/* 1:OK, 0:Timeout */
{
	CS_LOW();		/* Set CS# low */
	xchg_spi(spi, 0xFF);     /* Dummy clock (force DO enabled) */
	if (wait_ready(spi, 500)) return 1;	/* Wait for card ready */

    // timeout
	despiselect(spi);
	return 0;
}


/**
 * @brief  Set SCLK = slow, approx 280 KBits/s
 */
inline void fclk_slow(SPI_HandleTypeDef* spi) {
    MODIFY_REG(spi->Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_128);
}

/**
 * @brief  Set SCLK = fast, approx 4.5 MBits/s
 */
inline void fclk_fast(SPI_HandleTypeDef* spi) {
    MODIFY_REG(spi->Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_8);
}
