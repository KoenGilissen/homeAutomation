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

pthread_t tid[2];
pthread_mutex_t lock;
volatile int mcp23s17_fd;
ioBoard_t board0;
ioBoard_t board1;
ioBoard_t board2;
const int bus = 1;
const int chip_select = 0;
  
void* toggleOutput(void* arg)
{
	pthread_mutex_lock(&lock);
	printf("<Thread X Toggle B>\n");
	mcp23s17_write_reg(0xFF, GPIOB, board2.outputHardwareAddress, mcp23s17_fd);
	pthread_mutex_unlock(&lock);
	usleep(100000); //100ms
	pthread_mutex_lock(&lock);
	mcp23s17_write_reg(0x00, GPIOB, board2.outputHardwareAddress, mcp23s17_fd);
	printf("</Thread X Toggle B>\n");
	pthread_mutex_unlock(&lock);
	pthread_detach(pthread_self());
    pthread_exit(NULL);
}
  
int main(void)
{
    int i = 0;
    int error;
	
	debugPrint("1\n");
	
	mcp23s17_fd = mcp23s17_open(bus, chip_select);
	
	debugPrint("2\n");
	
	board0.outputHardwareAddress = 0;
	board0.inputHardwareAddress = 1;
	
	board1.outputHardwareAddress = 2;
	board1.inputHardwareAddress = 3;
	
	board2.outputHardwareAddress = 4;
	board2.inputHardwareAddress = 5;
	
	debugPrint("3\n");
	pthread_mutex_lock(&lock);
	initIoBoard(&board0, mcp23s17_fd);
	initIoBoard(&board1, mcp23s17_fd);
	initIoBoard(&board2, mcp23s17_fd);
	
/*	debugPrint("main thread toggle\n");
	mcp23s17_write_reg(0xFF, GPIOB, board2.outputHardwareAddress, mcp23s17_fd);
	usleep(100000); //100ms
	mcp23s17_write_reg(0x00, GPIOB, board2.outputHardwareAddress, mcp23s17_fd);*/
	
	pthread_mutex_unlock(&lock);
  
	debugPrint("4\n");
  
    while (i < 1) {
        error = pthread_create(&(tid[i]), NULL, &toggleOutput, NULL);
        if (error != 0)
            printf("\nThread can't be created : [%s]", strerror(error));
        i++;
    }
	
	
	
	while(1)
	{
		
		pthread_mutex_lock(&lock);
		printf("<Main Thread Toggle A>\n");
		mcp23s17_write_reg(0xFF, GPIOA, board2.outputHardwareAddress, mcp23s17_fd);
		pthread_mutex_unlock(&lock);
		usleep(100000); //100ms
		pthread_mutex_lock(&lock);
		mcp23s17_write_reg(0x00, GPIOA, board2.outputHardwareAddress, mcp23s17_fd);
		printf("</Main Thread Toggle A>\n");
		pthread_mutex_unlock(&lock);
		
		sleep(2);
	}
  

  
    return 0;
}
