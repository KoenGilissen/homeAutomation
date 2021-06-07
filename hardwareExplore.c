#include <stdio.h>
#include <unistd.h>
#include "mcp23s17.h"

typedef struct {
	uint8_t outputHardwareAddress;
	uint8_t inputHardwareAddress;
	
} ioBoard_t;

uint8_t initIoBoard(ioBoard_t *board, int mcp23s17_fd);


int main(void)
{
    const int bus = 1;
    const int chip_select = 0;

    uint8_t inputB0A = 0;
	uint8_t inputB0B = 0;
	
	uint8_t inputB1A = 0;
	uint8_t inputB1B = 0;
	
    uint8_t inputB2A = 0;
	uint8_t inputB2B = 0;
	
	ioBoard_t board0;
	ioBoard_t board1;
	ioBoard_t board2;
	
	board0.outputHardwareAddress = 0;
	board0.inputHardwareAddress = 1;
	
	board1.outputHardwareAddress = 2;
	board1.inputHardwareAddress = 3;
	
	board2.outputHardwareAddress = 4;
	board2.inputHardwareAddress = 5;

    int mcp23s17_fd = mcp23s17_open(bus, chip_select);

	initIoBoard(&board0, mcp23s17_fd);
	initIoBoard(&board1, mcp23s17_fd);
	initIoBoard(&board2, mcp23s17_fd);

	while(1)
	{	
		inputB0A = mcp23s17_read_reg(GPIOA,  board0.inputHardwareAddress, mcp23s17_fd);
		inputB0B = mcp23s17_read_reg(GPIOB,  board0.inputHardwareAddress, mcp23s17_fd);
		
		
		inputB1A = mcp23s17_read_reg(GPIOA,  board1.inputHardwareAddress, mcp23s17_fd);
		inputB1B = mcp23s17_read_reg(GPIOB,  board1.inputHardwareAddress, mcp23s17_fd);
		
		
		inputB2A = mcp23s17_read_reg(GPIOA,  board2.inputHardwareAddress, mcp23s17_fd);
		inputB2B = mcp23s17_read_reg(GPIOB,  board2.inputHardwareAddress, mcp23s17_fd);
		
		printf("inputB0A: 0x%x\tinputB0B: 0x%x\t", inputB0A, inputB0B);
		printf("inputB1A: 0x%x\tinputB1B: 0x%x\t", inputB1A, inputB1B);
		printf("inputB2A: 0x%x\tinputB2B: 0x%x\n", inputB2A, inputB2B);
		
		for(int i = 0; i < 5; i++)
		{
			sleep(1);
			printf(".");
		}
		printf("\n");
	}
    close(mcp23s17_fd);
}

uint8_t initIoBoard(ioBoard_t *board, int mcp23s17_fd)
{
	// Output config MCP23S17
    // config register
	const uint8_t ioconfigOutput = 	BANK_OFF | \
									INT_MIRROR_OFF | \
									SEQOP_OFF | \
									DISSLW_OFF | \
									HAEN_ON | \
									ODR_OFF | \
									INTPOL_LOW;
	
    mcp23s17_write_reg(ioconfigOutput, IOCON, board->outputHardwareAddress, mcp23s17_fd);
	mcp23s17_write_reg(0x00, IODIRA, board->outputHardwareAddress, mcp23s17_fd);
    mcp23s17_write_reg(0x00, IODIRB, board->outputHardwareAddress, mcp23s17_fd);
	
	//Input MCP23S17
    const uint8_t ioconfigInput = 	BANK_OFF | \
									SEQOP_OFF | \
									INT_MIRROR_ON | \
									DISSLW_OFF | \
									HAEN_ON | \
									ODR_OFF | \
									INTPOL_HIGH;
	mcp23s17_write_reg(ioconfigInput, IOCON, board->inputHardwareAddress, mcp23s17_fd);
    mcp23s17_write_reg(0xFF, IODIRA, board->inputHardwareAddress, mcp23s17_fd);
    mcp23s17_write_reg(0xFF, IODIRB, board->inputHardwareAddress, mcp23s17_fd);
	mcp23s17_write_reg(0x00, GPPUA, board->inputHardwareAddress, mcp23s17_fd);
    mcp23s17_write_reg(0x00, GPPUB, board->inputHardwareAddress, mcp23s17_fd);
	
	//config interrupts
	
	//Set DEFVALn Register: generates an interrupt if a mismatch occurs between the DEFVAL register and the port
	mcp23s17_write_reg(0x00, DEFVALA, board->inputHardwareAddress, mcp23s17_fd);
	mcp23s17_write_reg(0x00, DEFVALB, board->inputHardwareAddress, mcp23s17_fd);
	
	//Enable interrupt by compare (not by change)
	/*The INTCON register controls how the associated pin
	value is compared for the interrupt-on-change feature.
	If a bit is set, the corresponding I/O pin is compared
	against the associated bit in the DEFVAL register. If a
	bit value is clear, the corresponding I/O pin is compared
	against the previous value*/
	mcp23s17_write_reg(0xFF, INTCONA, board->inputHardwareAddress, mcp23s17_fd);
	mcp23s17_write_reg(0xFF, INTCONB, board->inputHardwareAddress, mcp23s17_fd);
	
	// Enable Interrupt on all pins
	mcp23s17_write_reg(0xFF, GPINTENA, board->inputHardwareAddress, mcp23s17_fd);
	mcp23s17_write_reg(0xFF, GPINTENB, board->inputHardwareAddress, mcp23s17_fd);
	
	return 1;
}