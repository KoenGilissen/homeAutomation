#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <gpiod.h>
#include "mcp23s17.h"
#include "homeAutomation.h"




pthread_mutex_t spiLock;
volatile int mcp23s17_fd;
ioBoard_t board0;
ioBoard_t board1;
ioBoard_t board2;
const int bus = 1;
const int chip_select = 0;
outputThreadInstance *head = NULL;
outputThreadInstance *tail = NULL;

void creatNewInputWatchThread(void);
void printSomething(void);
void inputWatchFunc(void);
  
int main(void)
{
	int returnValue = 0;
	int interruptValues = 0;

	/*
		https://github.com/starnight/libgpiod-example/blob/master/libgpiod-input/main.c
		$ sudo gpioinfo
		P9_11 --> gpiochip0 line 30
		P9_12 --> gpiochip1 line 28
		P9_13 --> gpiochip0 line 31
		P9_14 --> gpiochip1 line 18
		P9_15 --> gpiochip1 line 16
		P9_16 --> gpiochip1 line 19
	*/

	const unsigned int lineNumberB0A = 30;
	const unsigned int lineNumberB0B = 28;
	const unsigned int lineNumberB1A = 31;
	const unsigned int lineNumberB1B = 18;
	const unsigned int lineNumberB2A = 16;
	const unsigned int lineNumberB2B = 19;


	char *chipname0 = "gpiochip0";
	char *chipname1 = "gpiochip1";
	struct gpiod_chip *chip0;
	struct gpiod_chip *chip1;

	struct gpiod_line *intB0A;
	struct gpiod_line *intB0B;
	struct gpiod_line *intB1A;
	struct gpiod_line *intB1B;
	struct gpiod_line *intB2A;
	struct gpiod_line *intB2B;

    
	
	mcp23s17_fd = mcp23s17_open(bus, chip_select);

	board0.outputHardwareAddress = 0;
	board0.inputHardwareAddress = 1;
	
	board1.outputHardwareAddress = 2;
	board1.inputHardwareAddress = 3;
	
	board2.outputHardwareAddress = 4;
	board2.inputHardwareAddress = 5;
	
	pthread_mutex_lock(&spiLock);
	initIoBoard(&board0, mcp23s17_fd);
	initIoBoard(&board1, mcp23s17_fd);
	initIoBoard(&board2, mcp23s17_fd);
	pthread_mutex_unlock(&spiLock);

	chip0 = gpiod_chip_open_by_name(chipname0);
	chip1 = gpiod_chip_open_by_name(chipname1);
	if (!chip0 || !chip1) {
		perror("Open chip failed\n");
		exit(EXIT_FAILURE);
	}

	intB0A = gpiod_chip_get_line(chip0, lineNumberB0A);
	intB0B = gpiod_chip_get_line(chip1, lineNumberB0B);
	intB1A = gpiod_chip_get_line(chip0, lineNumberB1A);
	intB1B = gpiod_chip_get_line(chip1, lineNumberB1B);
	intB2A = gpiod_chip_get_line(chip1, lineNumberB2A);
	intB2B = gpiod_chip_get_line(chip1, lineNumberB2B);

	if (!intB0A || !intB0B || !intB1A || !intB1B || !intB2A || !intB2B) {
		perror("Get line failed\n");
		gpiod_chip_close(chip0);
		gpiod_chip_close(chip1);
		exit(EXIT_FAILURE);
	}

	returnValue = gpiod_line_request_input(intB0A, CONSUMER); //0 on success, -1 on failure
	returnValue = returnValue + gpiod_line_request_input(intB0B, CONSUMER);
	returnValue = returnValue + gpiod_line_request_input(intB1A, CONSUMER);
	returnValue = returnValue + gpiod_line_request_input(intB1B, CONSUMER);
	returnValue = returnValue + gpiod_line_request_input(intB2A, CONSUMER);
	returnValue = returnValue + gpiod_line_request_input(intB2B, CONSUMER);

	if (returnValue < 0) {
		perror("Request line as input failed\n");
		gpiod_line_release(intB0A);
		gpiod_line_release(intB0B);
		gpiod_line_release(intB1A);
		gpiod_line_release(intB1B);
		gpiod_line_release(intB2A);
		gpiod_line_release(intB2B);
		gpiod_chip_close(chip0);
		gpiod_chip_close(chip1);
		exit(EXIT_FAILURE);
	}

  	
/*	outputDefinition_t *a4 = NULL;
	outputDefinition_t *a5 = NULL;
	for(int i = 0; i < 10; i++)
	{
		a4 = newOutputDefinition(&board2, GPIOA, 0x10);
		a5 = newOutputDefinition(&board2, GPIOA, 0x20);
		if(a4 != NULL || a5 != NULL)
		{
			createOutputThread(&head, &tail, &a4);
			createOutputThread(&head, &tail, &a5);
			sleep(1);
		}
	}
	cleanUpOuputThreads(&head, &tail);*/

	creatNewInputWatchThread();
	
	while(1)
	{
		interruptValues = 0; // 0011 1111
		interruptValues =  (gpiod_line_get_value(intB2B) << 5) | (gpiod_line_get_value(intB2A) <<4) | \
							(gpiod_line_get_value(intB1B) << 3) | (gpiod_line_get_value(intB1A) << 2) | \
							(gpiod_line_get_value(intB0B) << 1) | (gpiod_line_get_value(intB0A));
		if(interruptValues > 0)
		{
			uint8_t gpioA = 0;
			uint8_t gpioB = 0;
			printf("interruptValues = 0x%.2X\n", interruptValues);
			switch(interruptValues)
			{
				case 0x3: // board 0 port A and B
					debugPrint("Interrupt from board 0\n");
					gpioA = mcp23s17_read_reg(GPIOA, board0.inputHardwareAddress, mcp23s17_fd);
					gpioB = mcp23s17_read_reg(GPIOB, board0.inputHardwareAddress, mcp23s17_fd);
					break;
				case 0xC: // board 1 port A and B
					debugPrint("Interrupt from board 1\n");
					gpioA = mcp23s17_read_reg(GPIOA, board1.inputHardwareAddress, mcp23s17_fd);
					gpioB = mcp23s17_read_reg(GPIOB, board1.inputHardwareAddress, mcp23s17_fd);
					break;
				case 0x30: // board 2 port A and B
					debugPrint("Interrupt from board 2\n");
					gpioA = mcp23s17_read_reg(GPIOA, board2.inputHardwareAddress, mcp23s17_fd);
					gpioB = mcp23s17_read_reg(GPIOB, board2.inputHardwareAddress, mcp23s17_fd);
					break;
				default:
					debugPrint("Unknown interrupt values\n");
					gpioA = mcp23s17_read_reg(GPIOA, board0.inputHardwareAddress, mcp23s17_fd);
					gpioB = mcp23s17_read_reg(GPIOB, board0.inputHardwareAddress, mcp23s17_fd);
					gpioA = mcp23s17_read_reg(GPIOA, board1.inputHardwareAddress, mcp23s17_fd);
					gpioB = mcp23s17_read_reg(GPIOB, board1.inputHardwareAddress, mcp23s17_fd);
					gpioA = mcp23s17_read_reg(GPIOA, board2.inputHardwareAddress, mcp23s17_fd);
					gpioB = mcp23s17_read_reg(GPIOB, board2.inputHardwareAddress, mcp23s17_fd);
					break;
			}
		}
		
		usleep(100000); //100ms 
	}  
    return 0;
}

void printSomething(void)
{
	printf("Hello This is me %p\n", printSomething);
	outputDefinition_t *a4 = NULL;
	outputDefinition_t *a5 = NULL;
	for(int i = 0; i < 10; i++)
	{
		a4 = newOutputDefinition(&board2, GPIOA, 0x10);
		a5 = newOutputDefinition(&board2, GPIOA, 0x20);
		if(a4 != NULL || a5 != NULL)
		{
			createOutputThread(&head, &tail, &a4);
			createOutputThread(&head, &tail, &a5);
			sleep(1);
		}
	}
	cleanUpOuputThreads(&head, &tail);
}

void inputWatchFunc(void)
{
	debugPrint("Hello from input watch function\n");
	void (*funcPtr)(void) = &printSomething;
	(*funcPtr)();
}

void creatNewInputWatchThread(void)
{
	pthread_t *newThread = (pthread_t * ) malloc(sizeof(pthread_t));
	if(newThread == NULL)
	{
		printf("%s\n", "Malloc failed");
		return;
	}
	debugPrint("Creating new input watch thread\n");
	pthread_create(newThread, NULL, &inputWatchFunc, NULL);
}

