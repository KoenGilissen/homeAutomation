#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <gpiod.h>
#include "mcp23s17.h"
#include "homeAutomation.h"

#define __DEBUG 	1
#define debugPrint(x)  do { if ( __DEBUG ) { printf(x); }} while (0)

#define CONSUMER "WiZard"




typedef struct n{
	pthread_t * tid;
	outputDefinition_t *def;
	struct n *next;
} threadListElement;

pthread_mutex_t lock;
volatile int mcp23s17_fd;
ioBoard_t board0;
ioBoard_t board1;
ioBoard_t board2;
const int bus = 1;
const int chip_select = 0;

outputDefinition_t * newOutputDefinition(ioBoard_t *b, uint8_t gpioPort, uint8_t portValue);
void createNewThread(threadListElement **head, threadListElement **tail, outputDefinition_t **def);
void printThreadList(threadListElement **head, threadListElement **tail);
void cleanUpthreads(threadListElement **head, threadListElement **tail);
void cleanUpElement(threadListElement **element);

void* toggleOutput(void* arg);
  
  
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

    threadListElement *head = NULL;
    threadListElement *tail = NULL;
	
	mcp23s17_fd = mcp23s17_open(bus, chip_select);

	board0.outputHardwareAddress = 0;
	board0.inputHardwareAddress = 1;
	
	board1.outputHardwareAddress = 2;
	board1.inputHardwareAddress = 3;
	
	board2.outputHardwareAddress = 4;
	board2.inputHardwareAddress = 5;
	
	pthread_mutex_lock(&lock);
	initIoBoard(&board0, mcp23s17_fd);
	initIoBoard(&board1, mcp23s17_fd);
	initIoBoard(&board2, mcp23s17_fd);
	pthread_mutex_unlock(&lock);

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
			createNewThread(&head, &tail, &a4);
			createNewThread(&head, &tail, &a5);
			sleep(1);
		}
	}
	cleanUpthreads(&head, &tail);*/
	
	while(1)
	{
		interruptValues = 0; // 0011 1111
		interruptValues =  (gpiod_line_get_value(intB2B) << 5) | (gpiod_line_get_value(intB2A) <<4) | \
							(gpiod_line_get_value(intB1B) << 3) | (gpiod_line_get_value(intB1A) << 2) | \
							(gpiod_line_get_value(intB0B) << 1) | (gpiod_line_get_value(intB0A));
		if(interruptValues > 0)
			printf("interruptValues = %d\n", interruptValues);
		
		usleep(20000);
	}  
    return 0;
}

outputDefinition_t * newOutputDefinition(ioBoard_t *b, uint8_t gpioPort, uint8_t portValue)
{
	outputDefinition_t *od = (outputDefinition_t *) malloc(sizeof(outputDefinition_t));
    if(od == NULL)
    {
    	printf("MALLOC failed!\n");
    	return NULL;
    }
	od->threadRunning = 0;
    od->board = b;
    od->gpioPort = gpioPort;
    od->portValue = portValue;  //A4
	return od;
}

void createNewThread(threadListElement **head, threadListElement **tail, outputDefinition_t **def )
{
	threadListElement* newThreadEl = (threadListElement*) malloc(sizeof(threadListElement));
	pthread_t *newThread = (pthread_t * ) malloc(sizeof(pthread_t));

	if(newThreadEl == NULL || newThread == NULL)
	{
		printf("%s\n", "Malloc failed");
		return;
	}
	printf("New thread element @ %p\n", (void*) newThreadEl);
	newThreadEl->def = *def;
	newThreadEl->tid = newThread;

	if(*head == NULL)
	{
		(*def)->threadRunning = 1;
		pthread_create(newThreadEl->tid, NULL, &toggleOutput, (void*) newThreadEl->def);
		*head = newThreadEl;
		*tail = newThreadEl;
		newThreadEl->next = NULL;
	}
	else
	{
		(*def)->threadRunning = 1;
		pthread_create(newThreadEl->tid, NULL, &toggleOutput, (void*) newThreadEl->def);	
		(*tail)->next = newThreadEl;
		*tail = newThreadEl;
		newThreadEl->next = NULL;
	}
}

void printThreadList(threadListElement **head, threadListElement **tail)
{
	threadListElement *temp = *head;
	while(temp != NULL)
	{
		printf("thread element at %p tid = %ld\n", (void*) temp, *(temp->tid));
		temp = temp->next;
	}
}

void cleanUpthreads(threadListElement **head, threadListElement **tail)
{
	threadListElement *temp = *head;
	threadListElement *prev = *head;

	if(temp != NULL && temp->def->threadRunning == 0) //HEAD
	{
		temp = (*head)->next;
		prev = (*head)->next;
		cleanUpElement(head);
		*head = prev;
	}

	while(temp != NULL) 
	{
		if(temp->def->threadRunning == 1) //iterate through the list
		{
			prev = temp;
			temp = temp->next;
		}
		else
		{
			if(temp == *tail)
			{
				*tail = prev;
				cleanUpElement(&temp);
				break;
			}
			else
			{
				prev->next = temp->next;
				cleanUpElement(&temp);
				temp = prev->next;
			}
		}
	}
}

void cleanUpElement(threadListElement **element)
{
	printf("Removing Thread @ %p\n", (void*) *element);
	free((*element)->def);
	free((*element)->tid);
	free(*element);
	*element = NULL;
	return;
	
}


void* toggleOutput(void* arg)
{
	int error = 0;
	uint8_t currentPortValue = 0;
	uint8_t newPortValue = 0;
	error = pthread_detach(pthread_self());
	if(error != 0)
		printf("\n%s\n", strerror(error));

	outputDefinition_t * od = arg;
	if(od == NULL || od->board == NULL)
	{
		printf("%s\n", "[ERROR] outputDefinition NULL value");
		pthread_exit(NULL);
	}
	pthread_mutex_lock(&lock);
	currentPortValue = mcp23s17_read_reg(od->gpioPort, od->board->outputHardwareAddress, mcp23s17_fd);
	newPortValue = currentPortValue | od->portValue;
	debugPrint("<Thread X Toggle B>\n");
	mcp23s17_write_reg(newPortValue, od->gpioPort, od->board->outputHardwareAddress, mcp23s17_fd);
	pthread_mutex_unlock(&lock);
	usleep(100000); //100ms
	pthread_mutex_lock(&lock);
	currentPortValue = mcp23s17_read_reg(od->gpioPort, od->board->outputHardwareAddress, mcp23s17_fd);
	newPortValue = ~od->portValue & currentPortValue;
	mcp23s17_write_reg(newPortValue, od->gpioPort, od->board->outputHardwareAddress, mcp23s17_fd);
	debugPrint("</Thread X Toggle B>\n");
	pthread_mutex_unlock(&lock);
	od->threadRunning = 0;
    pthread_exit(NULL);
}
