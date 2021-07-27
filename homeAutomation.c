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

void createOutputThread(outputThreadInstance **head, outputThreadInstance **tail, outputDefinition_t **def )
{
	outputThreadInstance* newThreadEl = (outputThreadInstance*) malloc(sizeof(outputThreadInstance));
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

void printOutputThreadList(outputThreadInstance **head, outputThreadInstance **tail)
{
	outputThreadInstance *temp = *head;
	while(temp != NULL)
	{
		printf("thread element at %p tid = %ld\n", (void*) temp, *(temp->tid));
		temp = temp->next;
	}
}

void cleanUpOuputThreads(outputThreadInstance **head, outputThreadInstance **tail)
{
	outputThreadInstance *temp = *head;
	outputThreadInstance *prev = *head;

	if(temp != NULL && temp->def->threadRunning == 0) //HEAD
	{
		temp = (*head)->next;
		prev = (*head)->next;
		cleanUpOutputThreadItem(head);
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
				cleanUpOutputThreadItem(&temp);
				break;
			}
			else
			{
				prev->next = temp->next;
				cleanUpOutputThreadItem(&temp);
				temp = prev->next;
			}
		}
	}
}

void cleanUpOutputThreadItem(outputThreadInstance **element)
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
	pthread_mutex_lock(&spiLock);
	currentPortValue = mcp23s17_read_reg(od->gpioPort, od->board->outputHardwareAddress, mcp23s17_fd);
	newPortValue = currentPortValue | od->portValue;
	debugPrint("<Thread X Toggle B>\n");
	mcp23s17_write_reg(newPortValue, od->gpioPort, od->board->outputHardwareAddress, mcp23s17_fd);
	pthread_mutex_unlock(&spiLock);
	usleep(100000); //100ms
	pthread_mutex_lock(&spiLock);
	currentPortValue = mcp23s17_read_reg(od->gpioPort, od->board->outputHardwareAddress, mcp23s17_fd);
	newPortValue = ~od->portValue & currentPortValue;
	mcp23s17_write_reg(newPortValue, od->gpioPort, od->board->outputHardwareAddress, mcp23s17_fd);
	debugPrint("</Thread X Toggle B>\n");
	pthread_mutex_unlock(&spiLock);
	od->threadRunning = 0;
    pthread_exit(NULL);
}

void createNewThread(threadInstance **head, threadInstance **tail, void *args, void (*thingToDo)())
{
	threadInstance* newThreadEl = (threadInstance*) malloc(sizeof(threadInstance));
	pthread_t *newThread = (pthread_t * ) malloc(sizeof(pthread_t));

	if(newThreadEl == NULL || newThread == NULL)
	{
		printf("%s\n", "Malloc failed");
		return;
	}
	printf("New thread element @ %p\n", (void*) newThreadEl);
	newThreadEl->arguments = (int*) *args;
	newThreadEl->tid = newThread;

	if(*head == NULL)
	{
		newThreadEl->threadRunning = 1;
		pthread_create(newThreadEl->tid, NULL, (void*) &thingToDo, (void*) newThreadEl->arguments);
		*head = newThreadEl;
		*tail = newThreadEl;
		newThreadEl->next = NULL;
	}
	else
	{
		newThreadEl->threadRunning = 1;
		pthread_create(newThreadEl->tid, NULL, (void*) &thingToDo, (void*) newThreadEl->arguments);
		(*tail)->next = newThreadEl;
		*tail = newThreadEl;
		newThreadEl->next = NULL;
	}
}

void cleanUpThreads(threadInstance **head, threadInstance **tail)
{
	threadInstance *temp = *head;
	threadInstance *prev = *head;

	if(temp != NULL && temp->threadRunning == 0) //HEAD
	{
		temp = (*head)->next;
		prev = (*head)->next;
		cleanUpThreadItem(head);
		*head = prev;
	}

	while(temp != NULL) 
	{
		if(temp->threadRunning == 1) //iterate through the list
		{
			prev = temp;
			temp = temp->next;
		}
		else
		{
			if(temp == *tail)
			{
				*tail = prev;
				cleanUpThreadItem(&temp);
				break;
			}
			else
			{
				prev->next = temp->next;
				cleanUpThreadItem(&temp);
				temp = prev->next;
			}
		}
	}

}

void cleanUpThreadItem(threadInstance **el)
{
	printf("Removing Thread @ %p\n", (void*) *el);
	free((*el)->arguments);
	free((*el)->tid);
	free(*el);
	*el = NULL;
	return;
}

void dummyFunc(void)
{

}
