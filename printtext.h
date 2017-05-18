#ifndef __PRINTTEXT_H__
#define __PRINTTEXT_H__

#define write_debug(msg) \
    do { fprintf(stderr, "%s\n", msg); } while(0)

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while(0)

/* define two global variables for  user signal flags
 * set nonzero when signal occurs
 */
int SIGUSR1_flag;
int SIGUSR2_flag;

/* define a struct for parsing user command line parameters
 * when get some parameters, save them.
 */
struct printtext_args {
            char *text_name;
            int interval_time;
            int first_line;
            int last_line;
};
extern struct printtext_args printtext_args;

/*define two variables for debugging
 * debug variable as a flag for debugging
 * msg array save debugging output message
 */
int debug = 0;
char msg[100] = {0};

//define a global fifo pointer
FIFO *fifo;

/*define a mutex and a cond
 * mutex lock and condition variables let two children thread
 * good coordination and work
 */
pthread_mutex_t mutex;
pthread_cond_t cond;

/*
 * @brief          parse user command line
 * @param[in]      argc                    the numbers of parameters
 * @param[in]      argv                    a command line of string
 * @return         @li 0                   on success
 *                 @li !0                  on failure
 */
int parse_printtext_args(int argc, char *argv[]);

/*
 * @brief          read last n lines from file
 * @param[in]      stream                  standard file output stream pointer
 * @param[in]      n                       lines
 * @return         @li 0                   on success
 *                 @li 1                   on failure
 */
int my_tail(FILE *stream, int n);

/*
 * @brief          let file pointer move n lines from head of file
 * @param[in]      stream                  standard file output stream pointer
 * @param[in]      n                       lines
 * @return         @li 0                   on success
 *
 */
int skip_line(FILE *fp, int line);

/*
 * @brief          count lines of file
 * @param[in]      stream                  standard file output stream pointer
 * @return         @li int                 on success, the lines is returned
 *                 @li -1                  on failure
 */
int get_line(FILE *fp);

#endif // __PRINTTEXT_H__
