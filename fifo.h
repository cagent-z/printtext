#ifndef	__FIFO_H__
#define __FIFO_H__

#define MAX_LINE		6
#define MAX_CHAR_PER_LINE 512
typedef struct fifo_t {
    char data[MAX_LINE][MAX_CHAR_PER_LINE + 1];
    int read;
    int write;
} FIFO;

/*
 * @brief          initialize custom fifo
 * @param[in]      fifo                    FIFO struct for queue
 * @return         none
 */
void fifo_init(FIFO *fifo);

/*
 * @brief          read custom fifo
 * @param[in]      fifo                    FIFO struct for queue
 * @return         @li char*               on success, a line of string is returned
 *                 @li NULL                on failure, the queue is empty
 */
char *fifo_read(FIFO *fifo);

/*
 * @brief          write a line into the fifo
 * @param[in]      fifo                    FIFO struct for queue
 * @param[in]      data                    a line of string
 * @return         @li 0                   on success
 *                 @li -1                  on failure, the queue is full
 */
int fifo_write(FIFO *fifo, const char *data);

/*
 * @brief          get the numbers of elements in the fifo
 * @param[in]      fifo                    FIFO struct for queue
 * @return         @li >0                  on success, the result is returned
 *                 @li 0                   on failure, the queue is empty
 */
int fifo_get_lines(FIFO *fifo);

#endif // __FIFO_H__
