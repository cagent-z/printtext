/*
 *    Copyright (C) 2016 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *Cagent Zhang, Oct,2016
 *
 *
 * description: queue functions library
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fifo.h"

//initialize custom fifo
void fifo_init(FIFO *fifo)
{
    fifo->read = 0;
    fifo->write = 0;
}

//read a line of string from custom fifo
char *fifo_read(FIFO *fifo)
{
    if (fifo->read == fifo->write){
        return NULL;
    }

    char *line_string = (char *)malloc(MAX_CHAR_PER_LINE);
    memset(line_string, 0, MAX_CHAR_PER_LINE);

    strcpy(line_string, fifo->data[fifo->read]);
    memset(fifo->data[fifo->read], 0, MAX_CHAR_PER_LINE);

   fifo->read = (fifo->read + 1) % MAX_LINE;

   return line_string;
}

//write a line of string into custom fifo
int fifo_write(FIFO *fifo, const char *data)
{
    if ((fifo->write + 1) % MAX_LINE == fifo->read){
        return -1;
    }

    strcpy(fifo->data[fifo->write], data);
    fifo->write = (fifo->write + 1) % MAX_LINE;

    return 0;
}

// get the numbers of elements in the fifo
int fifo_get_lines(FIFO *fifo)
{
    return (fifo->write + MAX_LINE - fifo->read) % MAX_LINE;
}
