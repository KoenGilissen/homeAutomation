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

typedef struct n{
	pthread_t * tid;
	outputDefinition_t *def;
	struct n *next;
} threadListElement;


pthread_t * tid[20];
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
  
int main(void)
{
    threadListElement *head = NULL;
    threadListElement *tail = NULL;


 
    
	/*outputDefinition_t *test = (outputDefinition_t *) malloc(sizeof(outputDefinition_t));
    if(test == NULL)
    {
    	printf("MALLOC failed!\n");
    	return -1;
    }
	test->threadRunning = 0;
    test->board = &board2;
    test->gpioPort = GPIOA;
    test->portValue = 0b00010000;  //A4*/
	
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
  
	
  	/*dynamicThread = (pthread_t * ) malloc(sizeof(pthread_t));
  	if(dynamicThread == NULL)
    {
    	printf("MALLOC failed!\n");
    	return -1;
    }


  	pthread_create(dynamicThread, NULL, &toggleOutput, (void*) test);*/
	outputDefinition_t *test = NULL;
	for(int i = 0; i < 10; i++)
	{
		test = newOutputDefinition(&board2, GPIOA, 0x10);
		if(test != NULL)
		{
			createNewThread(&head, &tail, &test);
			sleep(2);
			cleanUpthreads(&head, &tail);
		}
	}
	


/*    while (i < 1) {
        error = pthread_create(&(tid[i]), NULL, &toggleOutput, (void*) test);
        if (error != 0)
            printf("\nThread can't be created : [%s]", strerror(error));
        i++;
    }*/
	
	
	
	while(1)
	{
		
		pthread_mutex_lock(&lock);
		debugPrint("<Main Thread Toggle A>\n");
		mcp23s17_write_reg(0x20, GPIOA, board2.outputHardwareAddress, mcp23s17_fd); //A5
		pthread_mutex_unlock(&lock);
		usleep(100000); //100ms
		pthread_mutex_lock(&lock);
		mcp23s17_write_reg(0x00, GPIOA, board2.outputHardwareAddress, mcp23s17_fd); //A5
		debugPrint("</Main Thread Toggle A>\n");
		pthread_mutex_unlock(&lock);
		
		sleep(2);
	}  
    return 0;
}

	ioBoard_t *board;
	uint8_t gpioPort;
	uint8_t portValue;

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
