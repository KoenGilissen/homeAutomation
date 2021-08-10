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
#define handle_error_en(en, msg) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define CONSUMER "WiZard"

typedef struct {
	uint8_t outputHardwareAddress;
	uint8_t inputHardwareAddress;
	
} ioBoard_t;

typedef struct{
	ioBoard_t *board;
	uint8_t gpioPort;
	uint8_t bit;
} ioLoc;

typedef struct x{
	pthread_t *tid;
	uint8_t threadRunning;
	ioLoc *gpio;
	struct x *next;
} threadInstance;

uint8_t initIoBoard(ioBoard_t *board, int mcp23s17_fd);
void* toggleOutput(void* arg);

void createNewThread(threadInstance **head, void* (*func)(void *), ioLoc * args);
void removeFinishedThread(threadInstance **head);
void * dummyFunc(void *d);
void cleanUpThreadItem(threadInstance **el);
ioLoc * newIoLoc(ioBoard_t *board, uint8_t port, uint8_t bit);
void printOutputThreadList(threadInstance **head);
void * inputWatch(void * arg);

uint8_t setBit(uint8_t num);


#endif