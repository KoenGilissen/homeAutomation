#@Author Dr. Ing. Koen Gilissen
#
#Make 
#	build: build  OUTPUT
#	run: run the  OUTPUT
#	clean: remove OUTPUT 
#	all: clean->build->run
#Make OUT=foo -->  an executable 'foo' will be build/run/cleaned
#Make S=code.cpp --> code.cpp will be build 
#


#--->> Build Settings<---#
OUTPUT:=program
BUILDDIR := .
SOURCE := main.c 
LIB := homeAutomation.c
#------------------------#

UNAME_S := $(shell uname -s)
PWD_S := $(shell pwd)

ifndef OUT
	OUTPUT:= program
else
	OUTPUT:= $(OUT)
endif 

# Compiler Settings
CC := gcc
CFLAGS:= -g -Wall -pedantic -lmcp23s17 -lpthread

#Tasks 

all: clean build run

build: $(SOURCE)
	@date
	@echo "[INFO]: Running in $(PWD_S)"
	@echo "[INFO]: Building for architecture $(UNAME_S)"
	@mkdir -p $(BUILDDIR)
	$(CC) -c $(LIB) -o homeAutomation.o $(CFLAGS)
	$(CC) $(SOURCE) homeAutomation.o -o $(OUTPUT) $(CFLAGS)

run: 
	./$(OUTPUT)

clean:
	@echo "[INFO]: Cleaning $(BUILDDIR)/$(OUTPUT)"
	rm -fr $(BUILDDIR)/$(OUTPUT)
