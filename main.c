/*
 *    Copyright (C) 2016 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    after compiled, you can use it to print your text as you want.
 *    Although, the functions is simple, you can also extend its function.
 *
 *Cagent Zhang, Oct,2016
 *
 *
 * Implements:
 * 1. support concurrent access fifo
 * 2. start children thread(read and print)
 * 3. the main thread
 *    3.1 open file
 *    3.2 receive user command on while loop.
 *    3.3 check children thread status.
 * 4. on read thread:
 *    4.1 get the numbers of elements in the fifo, if less than 5, execute 4.4-4.5
 *        if the queue is full,sleep(notify to print thread by condition variables)
 *    4.2 check user command flag, read a line of string.
 *    4.3 write fifo
 *    4.4 notify print thread by condition variables( the queue have space)
 *    4.5 return 4.1
 *    4.6 meet EOF,thread exit
 * 5. on print thread:
 *    5.1 if the queue is empty, sleep. if it not empty, exceute 5.2
 *    5.2 read queue, and print message to terminal
 *    5.3 sleep specified time interval(usleep)
 *    5.4 return 5.1
 *    5.5 read EOF, thread exit
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>

/* According to POSIX.1-2001 */
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>

#include "fifo.h"
#include "printtext.h"

static struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"debug", no_argument, NULL, 'd'},
    {"print", required_argument, NULL, 'p'},
    {"time", required_argument, NULL, 't'},
    {"first", required_argument, NULL, 'f'},
    {"last", required_argument, NULL, 'l'},
    {NULL, 0, NULL, 0}
};

struct printtext_args printtext_args = {NULL, 0, 0, 0};

void usage_printtext(void)
{
    fprintf(stderr,
        "Usage: printtext [OPTIONS]... [VARIABLE]...\n"
        "Print text as you like mode\n"
        "\n"
        " -h, --help       print this help\n"
        " -d, --debug      enable debugging\n"
        " -p, --print      need print text\n"
        " -t, --time       time interval of outputting every line\n"
        " -f, --first      print text from first line to last line\n"
        " -l, --last       print text from last line to first line\n"
        "\n");
}

int parse_printtext_args(int argc, char *argv[])
{
    int c;

    while ((c = getopt_long(argc, argv, "p:t:f:l:hd", long_options, NULL)) !=
        EOF) {
        switch (c) {
        case 'h': /* 'help' options */
            usage_printtext();
            exit(EXIT_SUCCESS);
            break;
        case 'd': /* 'debug' options */
            debug = 1;
            break;
        case 'p':
            printtext_args.text_name = optarg;
            break;
        case 't':
            printtext_args.interval_time = atoi(optarg);
            break;
        case 'f':
            printtext_args.first_line = atoi(optarg);
            break;
        case 'l':
            printtext_args.last_line = atoi(optarg);
            break;
        default: /* ignore uknown options */
            //fprintf(stderr,"printtext: invalid option -- '%c'\n", c);
            fprintf(stderr, "Try `printtext -h' for more information.\n");
            exit(EXIT_FAILURE);
            break;
        }
    }

    //non-option arguments
    if (optind < argc) {
        //output debug information: parameters
        if (debug){
            bzero(msg, sizeof(msg));
            sprintf(msg, "non-option ARGV-elements: %s", argv[argc-1]);
            write_debug(msg);
        }
        printtext_args.text_name = argv[argc-1];
    } else if (argc == 1){
        fprintf(stderr,"No printed text\n");
        exit(EXIT_SUCCESS);
    }

    return 0;
}

/* one handler for both signals.
 * when a thread received SIGUSR1 and SIGUSR2, will call it.
 */
static void handle_sig(int sig_num)
{
    if (sig_num == SIGUSR1){
        //output debug information: catch SIGUSR1
        if (debug){
            bzero(msg, sizeof(msg));
            sprintf(msg, "%s", "Received SIGUSR1");
            write_debug(msg);
        }
        SIGUSR1_flag = 1;
    }
    else if (sig_num == SIGUSR2){
        //output debug information: catch SIGUSR2
        if (debug){
            bzero(msg, sizeof(msg));
            sprintf(msg, "%s", "Received SIGUSR2");
            write_debug(msg);
        }
        SIGUSR2_flag = 1;
    }
    return;
}

void print_text(void *time)
{
    int t_time = *((int *)time);
    char *pointer;

    while (1) {
        if (fifo_get_lines(fifo) == 0) {
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
        } else {
            //output debug information: line number
            if (debug){
                bzero(msg, sizeof(msg));
                sprintf(msg, "Line1: %d", fifo_get_lines(fifo));
                write_debug(msg);
            }

            pointer = fifo_read(fifo);
            if (strncasecmp(pointer, "EOF", 3) == 0) {
                pthread_exit(NULL);
            }
            printf("%s", pointer);
            free(pointer);

            usleep(t_time);
        }
    }
}

// function to read last n lines from the file
// at any point without reading the entire file
int my_tail(FILE *stream, int n)
{
    int ret = 0;
    int count = 0; // To count '\n' characters

    // unsigned long long pos (stores upto 2^64 – 1 chars)
    // assuming that long long int takes 8 bytes
    unsigned long long pos;

    if ( n < 0)
        return 1;
    // Go to End of file
    if (fseek(stream, 0, SEEK_END)){
        perror("failed to fseek");
        ret = 1;
    } else {
        pos = ftell(stream);
        // search for '\n' characters
        while (pos)
        {
            // Move 'pos' away from end of file.
            if (!fseek(stream, --pos, SEEK_SET))
            {
                if (fgetc(stream) == '\n')
                    if (count++ == n)
                        break;
            } else {
                perror("failed to fseek");
                ret = 1;
            }
        }
    }

    return ret;
}

int skip_line(FILE *fp, int line)
{
    int n = 0;
    char buf[1024] = {0};

    while( n < (line-1) && fgets(buf,sizeof(buf),fp) != NULL){
        if (buf[strlen(buf)-1] == '\n')
            n++;
    }

    return 0;
}

int get_line(FILE *fp)
{
    int line = 0;
    char buf[1024] = {0};

    // Go to head of file
    if (fseek(fp, 0, SEEK_SET)){
        perror("failed to fseek");
        return -1;
    }
    while(fgets(buf,sizeof(buf),fp) != NULL){
        if (buf[strlen(buf)-1] == '\n')
            line++;
    }

    return line;
}

void read_text(void *file_pointer)
{
    FILE *fp = (FILE*)file_pointer;
    char buf[MAX_CHAR_PER_LINE] = { 0 };

    //install signal function
    if (signal(SIGUSR1, handle_sig) == SIG_ERR){
        perror("failed to signal");
        pthread_exit(NULL);
    }
    if (signal(SIGUSR2, handle_sig) == SIG_ERR){
        perror("failed to signal");
        pthread_exit(NULL);
    }

    //according to different arguments, adjust the sequence to print
    if (printtext_args.first_line != 0){
        skip_line(fp, printtext_args.first_line);
    } else if ( printtext_args.last_line !=0) {
        my_tail(fp, printtext_args.last_line);
    }

    while (1) {
        /* when SIGUSR1 or SIGUSR2 is triggered, run the following statements
         * if 'SIGUSR1_flag = 1' or 'SIGUSR2_flag = 1' is true,
         * don't forget to set them as zero later
         */
        if (SIGUSR1_flag == 1){
            system("clear");
            //clear FIFO
            memset(fifo, 0, sizeof(FIFO));
            //let file pointer locate head of file
            fseek(fp, 0, SEEK_SET);
            //set flag for zero
            SIGUSR1_flag = 0;
        } else if (SIGUSR2_flag == 1){
            system("clear");
            //clear FIFO
            memset(fifo, 0, sizeof(FIFO));
            //let file pointer locate middle of file
            my_tail(fp, (get_line(fp)/2));
            //set flag for zero
            SIGUSR2_flag = 0;
        }

        if (fifo_get_lines(fifo) < 5) {

            //output debug information: line number
            if (debug){
                bzero(msg, sizeof(msg));
                sprintf(msg, "Line: %d", fifo_get_lines(fifo));
                write_debug(msg);
            }

            bzero(buf, MAX_CHAR_PER_LINE);
            if (fgets(buf, sizeof(buf), fp) == NULL) {
                //end of file
                fifo_write(fifo, "EOF");
                break;
            }
            fifo_write(fifo, buf);

            //output debug information: buf
            if (0){
                bzero(msg, sizeof(msg));
                sprintf(msg,"read characters:%s", buf);
                write_debug(msg);
            }

        } else if (fifo_get_lines(fifo) == 5) {
            //waiting for established condition
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&cond, &mutex);
            pthread_mutex_unlock(&mutex);
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    //if define DEBUG, enable debugging
#ifdef DEBUG
    debug = 1;
#endif

    int ret = 0;
    int t = 0;
    FILE *fp = NULL;
    char command = 0;
    char *_cmdname = NULL;
    fd_set fd_table;
    struct timeval time;

    pthread_t tid = 0, tid1 = 0;

    _cmdname = *argv;
    if (strrchr(_cmdname, '/') != NULL)
        _cmdname = strrchr(_cmdname, '/') + 1;
    strcpy(argv[0], _cmdname);

    //parse command line parameters
    parse_printtext_args(argc, argv);

    fp = fopen(printtext_args.text_name, "r");
    if (fp == NULL) {
        if (debug) {
            perror("Fail to fopen");
        }
        fprintf(stderr, "printtext: cannot access %s: No such file or directory\n",
            printtext_args.text_name);
        exit(EXIT_FAILURE);
    }

    //initialize a fifo
    fifo = (FIFO *) malloc(sizeof(FIFO));

    //initialize cond and mutex
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);

    //clear screen
    system("clear");

    //this thread is used to print text
    t = printtext_args.interval_time;
    ret = pthread_create(&tid, NULL, (void *)&print_text, (void *)&t);
    if (ret != 0) {
        handle_error("Fail to pthread_create\n");
    }

    //this thread is used to read text
    ret = pthread_create(&tid1, NULL, (void *)&read_text, (void *)fp);
    if (ret != 0) {
        handle_error("Fail to pthread_create\n");
    }

    /* use select() to monitor standard input file descriptor, the timeout
     * argument specified the interval that select() should block waiting for
     * a file descriptor to become ready. when a timeout occurrs, the process won't block.
     */
    time.tv_sec = 2;
    time.tv_usec = 0;

    FD_ZERO(&fd_table);
    FD_SET(0, &fd_table);

    while (1) {
        //initialize these parameters for every loop
        FD_SET(0, &fd_table);
        time.tv_sec = 2;
        time.tv_usec = 0;

        ret = select(1, &fd_table, NULL, NULL, &time);
        if (ret){
            //wait user command
            scanf("%c", &command);
            getchar();
        } else {
            //output debug information: call select(),timeout occurrs
            if (debug){
                bzero(msg, sizeof(msg));
                sprintf(msg,"%s", "call select, timeout occurrs");
                write_debug(msg);
            }
        }

        switch (command) {
        case 'q': /* user exit */
            //clean thread
            pthread_cancel(tid);
            pthread_cancel(tid1);
            //free resource
            free(fifo);
            exit(0);
            break;
        case 'f': /* from head to tail, print text again */
            //The signal is asynchronously directed to thread
            pthread_kill(tid1, SIGUSR1);
            command = '\0';
            break;
        case 'l': /* from tail to head, print text again */
            pthread_kill(tid, SIGUSR2);
            command = '\0';
            break;
        default: /* ignore uknown command */
            break;
        }

       /* this can  be  used  to check for the existence of a thread ID.
        * if read or print thread is dead, the loop exit
       */
        if (pthread_kill(tid, 0) == ESRCH)
            break;
        else if (pthread_kill(tid1, 0) == ESRCH)
            break;
    }

    //clean
    pthread_join(tid, NULL);
    pthread_join(tid1, NULL);
    free(fifo);

    return 0;
}
