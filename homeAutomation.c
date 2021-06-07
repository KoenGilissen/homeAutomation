#include "homeAutomation.h"

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