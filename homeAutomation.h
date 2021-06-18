#ifndef _HOMEAUTOMATION_H
#define _HOMEAUTOMATION_H

#include <stdio.h>
#include <unistd.h>
#include "mcp23s17.h"

typedef struct {
	uint8_t outputHardwareAddress;
	uint8_t inputHardwareAddress;
	
} ioBoard_t;

typedef struct {
	uint8_t threadRunning;
	ioBoard_t *board;
	uint8_t gpioPort;
	uint8_t portValue;
}  outputDefinition_t;

uint8_t initIoBoard(ioBoard_t *board, int mcp23s17_fd);


#endif