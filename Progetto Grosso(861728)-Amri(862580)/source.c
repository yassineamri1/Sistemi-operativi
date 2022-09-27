#include "header.h"

int shm_map_id, shm_msq_id, shm_pg_id, sem_source_id;
map *shm_map;
pid_t *pg;

msgbuf msg;
int start_x, start_y;   
int dest_x, dest_y;
int source_stop = 1;

void handle_signal(int signal);
void set_process_group();

int main(int argc, char *argv[]) {    
    sigset_t my_mask;
    struct sigaction sa;

    sa.sa_handler = handle_signal;
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    shm_map_id = atoi(argv[0]);
    shm_msq_id = atoi(argv[1]);
    shm_pg_id = atoi(argv[2]);
    sem_source_id = atoi(argv[3]);

    shm_map = shmat(shm_map_id, NULL, 0);
    pg = shmat(shm_pg_id, NULL, 0);

    set_process_group();

    srand((unsigned int)getpid());

    do {
        start_x = rand() % SO_HEIGHT;
        start_y = rand() % SO_WIDTH;
    } while ((shm_map->map[start_x][start_y].hole) || shm_map->map[start_x][start_y].s_pid != 0);
    
    dec_sem(sem_source_id, 0);    
    shm_map->map[start_x][start_y].s_pid = getpid();
    shm_map->map[start_x][start_y].sources = 1;

    while (source_stop) {
        do {
            dest_x = rand() % SO_HEIGHT;
            dest_y = rand() % SO_WIDTH;
        } while (shm_map->map[dest_x][dest_y].hole || (dest_x == start_x && dest_y == start_x));

        msg.mtype = (long)getpid();
        msg.mtext.start_x = start_x;
        msg.mtext.start_y = start_y;
        msg.mtext.dest_x = dest_x;
        msg.mtext.dest_y = dest_y;
        msg.mtext.pid_s = getpid();

        msgsnd(shm_msq_id, &msg, sizeof(source), 0);
        sleep(1);
    }
}

void handle_signal(int signal) {
    if(signal == SIGINT) {
        source_stop = 0;
        exit(EXIT_SUCCESS);
    } else if(signal == SIGTERM) {
        source_stop = 0;
        exit(EXIT_SUCCESS);
    }
}

void set_process_group() {
    if (*pg == 0)
        *pg = getpid();
        
    if (setpgid(getpid(), *pg) == -1)
        perror("Error occured in setpgid()\n");
}
