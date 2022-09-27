#include "header.h"

#include <stdio.h>

int shm_map_id, shm_pg_id, shm_stat_id;

int msg_id, msg_starv_id;

int sem_taxi_id, sem_cap_id, sem_update_stats_id, sem_source_id;
int end = 0;
pid_t *pg;
pid_t pid_map;
pid_t pid_starv;
map *shm_map;
stats *statistics;

msgbuf_s starv;
parameters par;
struct sigaction sa;
sigset_t set;
char buf_shm_map_id[10];
char buf_msg_id[10];
char buf_shm_pg_id[10];
char buf_sem_source_id[10];
char buf_sem_taxi_id[10];
char buf_sem_cap_id[10];
char buf_shm_stat_id[10];
char buf_sem_update_stats_id[10];
char buf_msg_starv_id[10];

void init_map();
void init_holes();
void init_taxi();
void init_source();
void set_cap_cells();
void print_map();
void print_stats();
void top_cells();
int outstanding_requests();
void deallocate();
void end_game();

void handle_signal(int signal);

int main(int argc, char *argv[]) {
    par = read_file(par);

    sa.sa_handler = handle_signal;
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    shm_map_id = shmget(IPC_PRIVATE, sizeof(map), IPC_CREAT | IPC_EXCL | 0666);
    shm_pg_id = shmget(IPC_PRIVATE, sizeof(pid_t), IPC_CREAT | IPC_EXCL | 0666);
    shm_stat_id = shmget(IPC_PRIVATE, sizeof(stats), IPC_CREAT | IPC_EXCL | 0666);   
    msg_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0666);
    msg_starv_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0666);
    sem_taxi_id = semget(IPC_PRIVATE, 1, 0666);
    sem_source_id = semget(IPC_PRIVATE, 1, 0666);
    sem_cap_id = semget(IPC_PRIVATE, SO_WIDTH * SO_HEIGHT, 0666);
    sem_update_stats_id = semget(IPC_PRIVATE, 1, 0666);

    shm_map = shmat(shm_map_id, NULL, 0);
    pg = shmat(shm_pg_id, NULL, 0); 
    *pg = 0;  
    statistics = shmat(shm_stat_id, NULL, 0);

    init_map();

    set_sem(sem_taxi_id, 0, par.SO_TAXI);
    set_sem(sem_source_id, 0, par.SO_SOURCES);

    set_sem(sem_update_stats_id, 0, 1);

    set_cap_cells();

    init_holes();
    
    snprintf(buf_shm_map_id, 10, "%d", shm_map_id);
    snprintf(buf_msg_id, 10, "%d", msg_id);
    snprintf(buf_shm_pg_id, 10, "%d", shm_pg_id);
    snprintf(buf_sem_source_id, 10, "%d", sem_source_id);
    snprintf(buf_sem_taxi_id, 10, "%d", sem_taxi_id);
    snprintf(buf_sem_cap_id, 10, "%d", sem_cap_id);
    snprintf(buf_shm_stat_id, 10, "%d", shm_stat_id);
    snprintf(buf_sem_update_stats_id, 10, "%d", sem_update_stats_id);
    snprintf(buf_msg_starv_id, 10, "%d", msg_starv_id);

    init_source();
    wait_sem_zero(sem_source_id, 0);

    init_taxi();
    wait_sem_zero(sem_taxi_id, 0);
 
    printf("Inizio simulazione... \n");
    alarm((unsigned int)par.SO_DURATION);

    switch (pid_map = fork()) {
    case -1:
        printf("Error occured in fork (2)\n");
        exit(EXIT_FAILURE);
        break;
    case 0:
        while (!end) {
            printf("\n");
            printf("Stato mappa:\n");
            print_map();
            sleep(1);
        }
        break;
    default:
        break;
    }

    while (!end) {
        if (msgrcv(msg_starv_id, &starv, sizeof(int), 1, 0) >= 0) {
            switch (pid_starv = fork()) {
            case -1:
                TEST_ERROR
                exit(EXIT_FAILURE);
                break;

            case 0: {
                if (execlp(TAXI_PATH, buf_shm_map_id, buf_msg_id, buf_shm_pg_id, buf_sem_taxi_id, buf_sem_cap_id, buf_shm_stat_id, buf_sem_update_stats_id, buf_msg_starv_id, NULL) == -1){
                    perror("Error occured in EXECLP(2)\n");
                    TEST_ERROR
                    exit(EXIT_FAILURE);
                }
                break;
            }

            default:
                break;
            }
        }
    }

    return 0;
}

void init_map() {
    int x, y;
    for (x = 0; x < SO_HEIGHT; x++) {
        for (y = 0; y < SO_WIDTH; y++) {
            shm_map->map[x][y].x = x;
            shm_map->map[x][y].y = y;
            shm_map->map[x][y].hole = 0;
            shm_map->map[x][y].s_pid = 0;
            shm_map->map[x][y].sources = 0;
            shm_map->map[x][y].capacity = (rand() % (par.SO_CAP_MAX - par.SO_CAP_MIN +1)) + par.SO_CAP_MIN;
            shm_map->map[x][y].n_taxi = 0;
            shm_map->map[x][y].crossing_time = (rand() % (par.SO_TIMENSEC_MAX - par.SO_TIMENSEC_MIN + 1)) + par.SO_TIMENSEC_MIN;
            shm_map->map[x][y].crossing_counter = 0;
            shm_map->map[x][y].top_cell = 0;
        }
    }
    return;
}

void set_cap_cells() {
    int i, j;
    for (i = 0; i < SO_HEIGHT; i++) {
        for (j = 0; j < SO_WIDTH; j++) {
            if (shm_map->map[i][j].hole == 1) {
                set_sem(sem_cap_id, get_pos(i, j), 0);
            }
            else {
                set_sem(sem_cap_id, get_pos(i, j), shm_map->map[i][j].capacity);
            }
        }
    }
}

void init_holes() {
    int h;
    for(h = par.SO_HOLES; h > 0; h--) {
        int x = (rand() % (SO_HEIGHT));
        int y = (rand() % (SO_WIDTH));

       if (shm_map->map[x][y].hole == 0) {
            if (x == 0) {
                if (y == 0) {
                    if (shm_map->map[1][0].hole == 0 && shm_map->map[0][1].hole == 0 && shm_map->map[1][1].hole == 0) {
                        shm_map->map[0][0].hole = 1;
                    }
                }
                if (y == SO_WIDTH - 1) {
                    if (shm_map->map[x][y-1].hole == 0 && shm_map->map[x+1][y].hole == 0 && shm_map->map[x-1][y-1].hole == 0) {
                        shm_map->map[x][y].hole = 1;
                    }
                }
            } else if (x == SO_HEIGHT - 1) {
                if (y == 0) {
                    if (shm_map->map[x-1][y].hole == 0 && shm_map->map[x][y+1].hole == 0 && shm_map->map[x-1][y+1].hole == 0) {
                        shm_map->map[x][y].hole = 1;
                    }
                }
                if (y == SO_WIDTH - 1) {
                    if (shm_map->map[x-1][y].hole == 0 && shm_map->map[x][y-1].hole == 0 && shm_map->map[x-1][y-1].hole == 0) {
                        shm_map->map[x][y].hole = 1;
                    }
                }
            } else {
                if (shm_map->map[x][y].hole == 0 && shm_map->map[x+1][y].hole == 0 && shm_map->map[x-1][y].hole == 0 && shm_map->map[x][y+1].hole == 0 && shm_map->map[x][y-1].hole == 0 &&
                 shm_map->map[x-1][y-1].hole == 0 && shm_map->map[x-1][y+1].hole == 0 && shm_map->map[x+1][y+1].hole == 0 && shm_map->map[x+1][y-1].hole == 0) {
                    shm_map->map[x][y].hole = 1;
                }
            }
        }
    }
}

void init_source() {
    int i, j;
    printf("Generazione dei source...\n");
    for (i = 0; i < par.SO_SOURCES; i++) {
        switch (fork()) {
        case -1:
            printf("Error occured in fork (1)\n");
            exit(EXIT_FAILURE);
            break;
        case 0: {

            if (execlp(SOURCE_PATH, buf_shm_map_id, buf_msg_id, buf_shm_pg_id, buf_sem_source_id, NULL) == -1) {
                perror("Error occured in EXECLP(1)\n");
                TEST_ERROR
                exit(EXIT_FAILURE);
            }

            break;
        }
        default:
            break;
        }
    }  
}

void init_taxi() {
    int i, j;
    printf("Generazione dei taxi...\n");
    for (i = 0; i < par.SO_TAXI; i++) {
        switch (fork()) {
        case -1:
            printf("Error occured in fork (2)\n");
            exit(EXIT_FAILURE);
            break;
        case 0: {
            if (execlp(TAXI_PATH, buf_shm_map_id, buf_msg_id,
                       buf_shm_pg_id, buf_sem_taxi_id, buf_sem_cap_id, buf_shm_stat_id, buf_sem_update_stats_id, buf_msg_starv_id, NULL) == -1) {
                perror("Error occured in EXECLP(2)\n");
                TEST_ERROR
                exit(EXIT_FAILURE);
            }
            break;
        }
        default:
            break;
        }
    }
}

void print_map() {
    int x, y;
    for (x = 0; x < SO_HEIGHT; x++) {
        for (y = 0; y < SO_WIDTH; y++) {
            if (shm_map->map[x][y].hole == 1) {
                printf("[H");
                
            }
            else if (shm_map->map[x][y].n_taxi == 0) {
                printf("[-");
            }
            else {
                printf("[%d", shm_map->map[x][y].n_taxi);
            }
            printf("]");
        }
        printf("\n");
    }
}

void print_stats() {
    int i, j;
    printf("TOP CELLS: %d\n", par.SO_TOP_CELLS);
    top_cells();
    for (i = 0; i < SO_HEIGHT; i++) {
        for (j = 0; j < SO_WIDTH; j++) {
            if (shm_map->map[i][j].hole == 1) 
                printf("[H]");
            else {
               if (shm_map->map[i][j].top_cell == 1) {
                   if (shm_map->map[i][j].s_pid > 0) 
                       printf("[\033[4m" "S" "\033[24m]");
                   else
                       printf("[T]");
               } else if (shm_map->map[i][j].s_pid > 0) 
                   printf("[S]");
               else
                   printf("[-]");
            }
            
        }
        printf("\n");
    }
    

    printf("\n");
    printf("STATISTISCHE FINALI:\n");
    printf("Richieste effettuate con successo: %d\n", statistics->travel_success);
    printf("Richieste effettuate abortite: %d\n", statistics->travel_aborted);
    printf("Richieste inevase: %d\n", outstanding_requests());
    printf("Taxi che ha raccolto piu' richieste: %d (richieste: %d)\n", statistics->pid_max_request, statistics->max_request);
    printf("Taxi che ha attraversato più celle: %d (celle: %d)\n", statistics->pid_max_cells, statistics->max_cells);
    printf("Taxi che ha fatto il viaggio più lungo: %d (tempo: %.3f sec)\n", statistics->pid_max_time_travel, ((float)statistics->max_time_travel / 1000000000));
    printf("\n");
}

void top_cells() {
    int i, j, c;
    cell temp[SO_HEIGHT * SO_WIDTH];
    cell tmp;

    for (i = 0; i < SO_HEIGHT; i++) {
        for (j = 0; j < SO_WIDTH; j++) {
            temp[get_pos(i, j)] = shm_map->map[i][j];
        }
    }


    for (i = 0; i < SO_HEIGHT * SO_WIDTH; i++) {
        tmp = temp[i];
        c = i - 1;
        while (c >= 0 && temp[c].crossing_counter < tmp.crossing_counter) {
            temp[c + 1] = temp[c];
            c--;
        }
        temp[c + 1] = tmp;
    }

    for (i = 0; i < par.SO_TOP_CELLS; i++) {
        shm_map->map[temp[i].x][temp[i].y].top_cell = 1;
    }
}

void handle_signal(int signal) {
    if (signal == SIGINT) { 
        end = 1;
        sleep(1);
        print_stats();
        end_game();
    } else if(signal == SIGALRM) {
        end = 1;
        sleep(1);
        print_stats();
        end_game();
    } else if(signal == SIGTERM)
        exit(EXIT_SUCCESS);
    
}

int outstanding_requests() {
    struct msqid_ds buf;
    if (msgctl(msg_id, IPC_STAT, &buf) == -1) {
        TEST_ERROR
    }
    return (int)buf.msg_qnum;
}

void end_game() {
    if (killpg(*pg, SIGKILL) == -1) {
        perror("Error occured in killpg()\n");
    }
    deallocate();
    kill(pid_map, SIGTERM);
    kill(pid_starv, SIGTERM);
    exit(EXIT_SUCCESS);
}

void deallocate() {
    shmctl(shm_map_id, IPC_RMID, 0);
    shmctl(shm_pg_id, IPC_RMID, 0);
    shmctl(shm_stat_id, IPC_RMID, 0);
    msgctl(msg_id, IPC_RMID, 0);
    msgctl(msg_starv_id, IPC_RMID, 0);
    semctl(sem_cap_id, IPC_RMID, 0);
    semctl(sem_taxi_id, IPC_RMID, 0);
    semctl(sem_source_id, IPC_RMID, 0);
    semctl(sem_update_stats_id, IPC_RMID, 0);
}
