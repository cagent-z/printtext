#
#    Copyright (C) 2016 Freescale Semiconductor, Inc. All Rights Reserved.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    after compiled, you can use it to print your text as you want.
#    Although, the functions is simple, you can also extend its function.
#
# Cagent Zhang, Oct,2016
#

# Declare the contents of the .PHONY variable as phony.
.PHONY: clean

# Make variables (CC, etc...)
CC       = gcc
LD       = ld
CFLAGS   = -Wall -lpthread
#CFLAGS   += -g -DDEBUG

#object files
OBJS     = main.o fifo.o

#add a target to generate binary
TARGET = printtext

#install
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)


# TODO: clean files
clean:
	rm -rf $(TARGET) *.o
