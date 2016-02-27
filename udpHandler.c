#include "main.h"
#include "config/config.h"
#include "config.h"
#include <avr/boot.h>

extern volatile uint8_t gTimeout;
extern volatile uint8_t gLedOnTime;

extern void leaveBootloader();

#define OPCODE_RRQ		0x01	// uint16 - opcode, cstr - filename, cstr - mode (octet) 
#define OPCODE_WRQ		0x02
#define OPCODE_DATA		0x03	// uint16 - opcode, uint16 - block, bin - data
#define OPCODE_ACK		0x04	// uint16 - opcode, uint16 - block
#define OPCODE_ERR		0x05	// uint16 - opcode, uint16 - error code, cstr - error text

#define OPCODE_LEAVE	0x70	// uint16 - opcode	- leave bootloader
#define OPCODE_DELAY	0x71	// uint16 - opcode  - set timeout to 60s

#define ERROR_CODE_UNKNOWNOP 	0x01
#define ERROR_CODE_FULL	 		0x03
#define ERROR_CODE_ILLEGALOP	0x04

#define MAX_ADDR 				(1024 * (32 - 4))
#define TFTP_DATA_SIZE 			512

static uint16_t lLastBlock;
static uint8_t lFirstBlock[SPM_PAGESIZE];

void boot_program_page (uint16_t pageAddr, uint8_t *buf)
{
        uint8_t i;
        //uint8_t sreg;

        //sreg = SREG;
        //cli(); // interrupts dont have to be disabled, cuz we moved them into bootloader section

        eeprom_busy_wait();

        boot_page_erase_safe (pageAddr);
        boot_spm_busy_wait ();      // Wait until the memory is erased.

        for (i = 0; i < SPM_PAGESIZE; i+=2)
        {
            // Set up little-endian word.

            uint16_t w = *buf++;
            w += (*buf++) << 8;

            boot_page_fill (pageAddr + i, w);
        }

        boot_page_write (pageAddr);     // Store buffer in flash page.
        boot_spm_busy_wait();       // Wait until the memory is written.

        // Reenable RWW-section again. We need this if we want to jump back
        // to the application after bootloading.

        // boot_rww_enable_safe (); // it's done in leaveBootloader()

        // Re-enable interrupts (if they were ever enabled).

        //SREG = sreg;
		//sei();
}

void sendVal(uint8_t type, uint8_t code, const char* text, uint8_t l, const ip_address *to, uint16_t port)
{
	uint8_t *pkt = (uint8_t*)udp_get_buffer();
	pkt[0] = 0;
	pkt[1] = type;
	pkt[2] = 0;
	pkt[3] = code;
	pkt[4] = 0;
	udp_send_to(4+l, to, port);
}

void onUDP(uint8_t * data, uint16_t len, const ip_address *from, const uint16_t port)
{
	uint16_t *pOpcode = ((uint16_t*)data);
	uint16_t opcode = ntoh16(*pOpcode);

	//_delay_ms(10);
	
	switch (opcode)
	{
		case OPCODE_LEAVE:
		{
			sendVal(OPCODE_ACK, 0, NULL, 0, from, port);
			leaveBootloader();
			break;
		}
		case OPCODE_DELAY:
		{
			sendVal(OPCODE_ACK, 0, NULL, 0, from, port);
			gTimeout = 60;
			break;
		}
		case OPCODE_WRQ:
		{
			uint8_t * ptr = data + 2;
			while (*ptr)
			{
				ptr++;
			}

			ptr++;
			
			if (*ptr != 'o' && *ptr != 'O')
			{
				sendVal(OPCODE_ERR, ERROR_CODE_ILLEGALOP, NULL, 1, from, port);
				break;
			}
			
			gTimeout = 15;
			lLastBlock = 0;
			sendVal(OPCODE_ACK, 0, NULL, 0, from, port);
			break;
		}
		case OPCODE_DATA:
		{
			uint8_t startApp = 0;
			uint16_t block = ntoh16(*(pOpcode + 1));
			uint8_t *ptr = data + 4;
			uint16_t ptrLen = len - 4;
			
			if (block <= lLastBlock)
			{
				/* 	one of previous blocks? just ack it, maybe ACK
					dissapeared?
					*/
				sendVal(OPCODE_ACK, block, NULL, 0, from, port);
				return;
			}
			
			if (block != (lLastBlock + 1))
			{
				/*	blocks mus be sent in order, if they are not, 
					terminate transfer */
				sendVal(OPCODE_ERR, ERROR_CODE_ILLEGALOP, NULL, 1, from, port);
				return;
			}

			lLastBlock = block;

			uint16_t writeAddr = (block - 1) << 9; // Flash write address for this block
			if ((writeAddr + ptrLen) > MAX_ADDR) 
			{
				sendVal(OPCODE_ERR, ERROR_CODE_FULL, NULL, 1, from, port);
			} 
			else 
			{
				uint8_t s = 0;
				while (ptrLen % SPM_PAGESIZE) ptrLen++;
				
				if (writeAddr == 0) 
				{
					/* 	save first page in memory, flash at the transfer end
						bootloader will not exit if first page is empty / 
						transfer was not completed
						*/
					memcpy(lFirstBlock, ptr, SPM_PAGESIZE);
					s++;
					boot_page_erase_safe (0);
					boot_spm_busy_wait();
				}

				uint8_t pages = ptrLen / SPM_PAGESIZE;
				for (uint8_t p = s; p < pages; ++p)
				{
					uint16_t offset = (p * SPM_PAGESIZE);
					boot_program_page(offset + writeAddr, offset + ptr);
				}
								
				if (len - 4 < TFTP_DATA_SIZE) 
				{
					/* 	transfer is theoretically complete, flash first page, 
						ack block and bootup main program
						*/
					boot_program_page(0, lFirstBlock);
					startApp = 1;
				}
			}
			
			sendVal(OPCODE_ACK, block, NULL, 0, from, port);
			if (startApp)
			{
				leaveBootloader();
			}
			break;
		}
		//case OPCODE_RRQ:
		default:
		{
			sendVal(OPCODE_ERR, ERROR_CODE_UNKNOWNOP, NULL, 1, from, port);
			break;
		}
	}
	
}

