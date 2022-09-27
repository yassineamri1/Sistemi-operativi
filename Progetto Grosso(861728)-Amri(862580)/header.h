#ifndef header_h

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>

#define SO_WIDTH 60
#define SO_HEIGHT 20
#define SOURCE_PATH "./source"
#define TAXI_PATH "./taxi"

#define get_pos(x, y) ((x * (SO_WIDTH)) + y)
#define TEST_ERROR                                                                \
    if (errno)                                                                    \
    {                                                                             \
        fprintf(stderr,                                                           \
                "%s:%d: PID:%d: Error %d (%s)\n", 				  \
                __FILE__,                                                         \
                __LINE__,                                                         \
                getpid(),                                                         \
                errno,                                                            \
                strerror(errno));                                                 \
    };

typedef struct {
    pid_t s_pid;
    int x;
    int y;
    int hole;
    int sources;
    int capacity;
    int n_taxi;
    int top_cell;
    int crossing_counter;
    long crossing_time;

} cell;

typedef struct {
    int start_x;
    int start_y;
    int dest_x;
    int dest_y;
    pid_t pid_s;

} source;

typedef struct {
    long mtype;
    source mtext;

} msgbuf;


typedef struct {
    long mtype;
    int mtext;

} msgbuf_s;

typedef struct {
    cell map[SO_HEIGHT][SO_WIDTH];
} map;

typedef struct {
    int travel_success;
    int travel_aborted;
    int max_cells;
    int max_request;
    long max_time_travel;
    pid_t pid_max_cells;
    pid_t pid_max_request;
    pid_t pid_max_time_travel;
    
} stats;

typedef struct {
    int SO_HOLES;
    int SO_TOP_CELLS;
    int SO_CAP_MAX;
    int SO_CAP_MIN;
    int SO_TAXI;
    int SO_DURATION;
    long SO_TIMEOUT;
    long SO_TIMENSEC_MIN;
    long SO_TIMENSEC_MAX;
    int SO_SOURCES;

} parameters;

parameters read_file(parameters par);

int set_sem(int sem_id, int index, int sem_val);

void reset_sem_set(int sem_id, int dim, int init_val);

void dec_sem(int sem_id, int index);

int dec_sem_no_wait(int sem_id, int index);

void rel_sem(int sem_id, int index);

void wait_sem_zero(int sem_id, int index);

void reset_sem_set(int sem_id, int dim, int init_val);

int get_sem_val(int sem_id, int x, int y);

#endif
