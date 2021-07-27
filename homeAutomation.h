#ifndef _HOMEAUTOMATION_H
#define _HOMEAUTOMATION_H

#include <stdio.h>
#include <unistd.h>
#include "mcp23s17.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>


#define __DEBUG 	1
#define debugPrint(x)  do { if ( __DEBUG ) { printf(x); }} while (0)

#define CONSUMER "WiZard"

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

typedef struct {
	uint8_t threadRunning;
	ioBoard_t *board;
	uint8_t gpioPort;
	uint8_t portValue;
}  inputDefinition_t;

typedef struct n{
	pthread_t * tid;
	outputDefinition_t *def;
	struct n *next;
} outputThreadInstance;

typedef struct i{
	pthread_t *tid;
	inputDefinition_t *def;
	struct i *next;  
		
} inputThreadInstance;

typedef struct x{
	pthread_t *tid;
	uint8_t threadRunning;
	void *arguments;
	struct x *next;
} threadInstance;

uint8_t initIoBoard(ioBoard_t *board, int mcp23s17_fd);

outputDefinition_t * newOutputDefinition(ioBoard_t *b, uint8_t gpioPort, uint8_t portValue);
void createOutputThread(outputThreadInstance **head, outputThreadInstance **tail, outputDefinition_t **def);
void printOutputThreadList(outputThreadInstance **head, outputThreadInstance **tail);
void cleanUpOuputThreads(outputThreadInstance **head, outputThreadInstance **tail);
void cleanUpOutputThreadItem(outputThreadInstance **element);
void* toggleOutput(void* arg);

void createNewThread(threadInstance **head, threadInstance **tail, void *args, void (*thingToDo)());
void cleanUpThreads(threadInstance **head, threadInstance **tail);
void dummyFunc();
void cleanUpThreadItem(threadInstance **el);


#endif