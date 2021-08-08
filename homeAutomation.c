#include "homeAutomation.h"

extern pthread_mutex_t spiLock;
extern volatile int mcp23s17_fd;

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

void* toggleOutput(void* arg)
{
	int error = 0;
	uint8_t currentPortValue = 0;
	uint8_t newPortValue = 0;
	error = pthread_detach(pthread_self());
	if(error != 0)
		handle_error("Toggle output thread: Failed to detach thread\n");

	threadInstance * tI = arg;
	if(tI == NULL || tI->gpio == NULL)
	{
		perror("Toggle output thread: NULL arg\n");
		pthread_exit(NULL);
	}
	pthread_mutex_lock(&spiLock);
	currentPortValue = mcp23s17_read_reg(tI->gpio->gpioPort, tI->gpio->board->outputHardwareAddress, mcp23s17_fd);
	newPortValue = currentPortValue | tI->gpio->bit;
	mcp23s17_write_reg(newPortValue, tI->gpio->gpioPort, tI->gpio->board->outputHardwareAddress, mcp23s17_fd);
	pthread_mutex_unlock(&spiLock);
	usleep(100000); //100ms
	pthread_mutex_lock(&spiLock);
	currentPortValue = mcp23s17_read_reg(tI->gpio->gpioPort, tI->gpio->board->outputHardwareAddress, mcp23s17_fd);
	newPortValue = ~tI->gpio->bit & currentPortValue;
	mcp23s17_write_reg(newPortValue, tI->gpio->gpioPort, tI->gpio->board->outputHardwareAddress, mcp23s17_fd);
	pthread_mutex_unlock(&spiLock);
	tI->threadRunning = 0;
    pthread_exit(NULL);
}

void createNewThread(threadInstance **head, void* (*func)(void *), ioLoc * args)
{
	threadInstance* temp = *head;
	threadInstance* newThreadEl = (threadInstance*) malloc(sizeof(threadInstance));
	pthread_t *newThread = (pthread_t * ) malloc(sizeof(pthread_t));

	if(newThreadEl == NULL || newThread == NULL)
		handle_error("Create new thread: Memory allocation failed\n");
	newThreadEl->tid = newThread;
	newThreadEl->gpio = args;
	newThreadEl->next = (*head);
	(*head) = newThreadEl;
	newThreadEl->threadRunning = 1;
	pthread_create(newThreadEl->tid, NULL, func, (void*) newThreadEl);
}

void removeFinishedThread(threadInstance **head)
{
	threadInstance *temp = *head;
	threadInstance *prev = *head;
	if(temp != NULL && temp->threadRunning == 0) //HEAD must be removed
	{
		(*head) = (*head)->next;
		cleanUpThreadItem(&prev);
		return;		
	}

	while(temp != NULL && temp->threadRunning == 1)
	{
		prev = temp;
		temp = temp->next;
	}

	if(temp == NULL)
		return;

	prev->next = temp->next;
	cleanUpThreadItem(&temp);
}

void cleanUpThreadItem(threadInstance **el)
{
	printf("Cleaning thread item: %p\n", *el);
	if(el == NULL || (*el)->tid == NULL || (*el)->gpio->board == NULL || (*el)->gpio == NULL)
	{
		perror("Clean up NULL pointer");
		return;
	}
	free((*el)->tid);
	//free((*el)->gpio->board);
	free((*el)->gpio);
	free((*el));
	*el = NULL;
	return;
}

ioLoc * newIoLoc(ioBoard_t *board, uint8_t port, uint8_t bit)
{
	ioLoc *nIoLoc = malloc(sizeof(ioLoc));
	if(nIoLoc == NULL)
		handle_error("Create new io location: Memory allocation failed\n");
	nIoLoc->board = board;
	nIoLoc->gpioPort = port;
	nIoLoc->bit = bit;
	return nIoLoc;
}

void * dummyFunc(void *d)
{
	debugPrint("dummy Func!\n");
	int error = pthread_detach(pthread_self());
	if(error != 0)
		printf("\n%s\n", strerror(error));
	if(d == NULL)
	{
		printf("%s\n", "Error: received thread instance in dummy func is NULL");
	}
	threadInstance *tI = (threadInstance*) d;
	sleep(5);
	tI->threadRunning = 0;
	pthread_exit(NULL);
	return NULL;
}

void printOutputThreadList(threadInstance **head)
{
	threadInstance *temp = *head;
	while(temp != NULL)
	{
		printf("thread element at %p tid = %ld\n", (void*) temp, *(temp->tid));
		temp = temp->next;
	}
}