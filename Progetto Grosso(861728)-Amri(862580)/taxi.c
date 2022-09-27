#include "header.h"

void move_up();
void move_right();
void move_left();
void move_down();
void move(int end_x, int end_y);
void start_simulation();
void handle_signal(int signal);
void set_process_group();
void update_stats(int outcome);

int shm_map_id, shm_msq_id, shm_pg_id, shm_sem_taxi, shm_sem_cap_taxi, shm_sem_update_stats, shm_stat_data_id, shm_msq_starv_id;
map *shm_map;
pid_t *pg;

int taxi_x;
int taxi_y;
int taxi_stop = 1;
msgbuf msg;
msgbuf_s starv;
stats *statistics;
parameters par;
struct timeval;

int tot_request = 0;
int tot_abort_request = 0;
int tot_out_request = 0;
int tot_n_cells_cross = 0;
long tot_n_crossing_time = 0;

int main(int argc, char *argv[]) {
    struct sigaction sa;
    int x, y, placed;
    par = read_file(par);

    sa.sa_handler = handle_signal;
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    shm_map_id = atoi(argv[0]);
    shm_msq_id = atoi(argv[1]);
    shm_pg_id = atoi(argv[2]);
    shm_sem_taxi = atoi(argv[3]);
    shm_sem_cap_taxi = atoi(argv[4]);
    shm_stat_data_id = atoi(argv[5]);
    shm_sem_update_stats = atoi(argv[6]);
    shm_msq_starv_id = atoi(argv[7]);

    shm_map = shmat(shm_map_id, NULL, 0);
    pg = shmat(shm_pg_id, NULL, 0);
    statistics = shmat(shm_stat_data_id, NULL, 0);

    set_process_group();

    srand((unsigned int)getpid());


    placed = 1;
    do {
        x = rand() % SO_HEIGHT;
        y = rand() % SO_WIDTH;
        if (get_sem_val(shm_sem_cap_taxi, x, y) > 0 && shm_map->map[x][y].hole == 0) {          
            placed = 0;
        }

    } while (placed);
    dec_sem(shm_sem_cap_taxi, get_pos(x, y));

    shm_map->map[x][y].n_taxi++;
    shm_map->map[x][y].crossing_counter++;
    taxi_x = x;
    taxi_y = y;
    dec_sem_no_wait(shm_sem_taxi, 0);
    wait_sem_zero(shm_sem_taxi, 0);

    start_simulation();
}

void handle_signal(int signum) {
    if(signum == SIGINT) {
        taxi_stop = 0;
        exit(EXIT_SUCCESS);
    } else if(signum == SIGTERM) {
        taxi_stop = 0;
        exit(EXIT_SUCCESS);
    } else if(signum ==  SIGALRM) {
        taxi_stop = 0;
        rel_sem(shm_sem_cap_taxi, get_pos(taxi_x, taxi_y));
        shm_map->map[taxi_x][taxi_y].n_taxi--;
        tot_abort_request++;
        update_stats(1);
        starv.mtext = getpid();
        starv.mtype = 1;
        msgsnd(shm_msq_starv_id, &starv, sizeof(int), IPC_NOWAIT);
        exit(EXIT_SUCCESS);
    }
}

void start_simulation() {
    while (taxi_stop) {
        if (shm_map->map[taxi_x][taxi_y].s_pid != 0 ) {
	    if (msgrcv(shm_msq_id, &msg, sizeof(source), shm_map->map[taxi_x][taxi_y].s_pid, 0) == -1) {
	        TEST_ERROR
	        exit(EXIT_FAILURE);
	    }
	    tot_request++;
	    move(msg.mtext.dest_x, msg.mtext.dest_y);
	    update_stats(0);
	    tot_n_crossing_time = 0;
	}
	else {
	    if (msgrcv(shm_msq_id, &msg, sizeof(source), 0, 0) == -1) {
	        TEST_ERROR
	        exit(EXIT_FAILURE);
	    }     
	    move(msg.mtext.start_x, msg.mtext.start_y);
	    tot_request++;
	    move(msg.mtext.dest_x, msg.mtext.dest_y);
	    update_stats(0);

	    tot_n_crossing_time = 0;
	}
    }
}

void set_process_group() {
    if (*pg == 0) {
        *pg = getpid();
    }
    if (setpgid(getpid(), *pg) == -1)
        perror("Error occured in setpgid()\n");
}

void move_up() {
    struct timespec timeout, rem;

    alarm((unsigned int)par.SO_TIMEOUT);
    dec_sem(shm_sem_cap_taxi, (unsigned short)get_pos((taxi_x - 1), taxi_y));
    shm_map->map[taxi_x - 1][taxi_y].n_taxi++;

    timeout.tv_nsec = shm_map->map[taxi_x - 1][taxi_y].crossing_time;
    timeout.tv_sec = 0;
    nanosleep(&timeout, &rem);
    alarm(0);
    rel_sem(shm_sem_cap_taxi, get_pos(taxi_x, taxi_y));

    shm_map->map[taxi_x][taxi_y].n_taxi--;
    taxi_x--;
    shm_map->map[taxi_x][taxi_y].crossing_counter++;
    tot_n_cells_cross++;
    tot_n_crossing_time = shm_map->map[taxi_x][taxi_y].crossing_time + tot_n_crossing_time;
}

void move_down() {

    struct timespec timeout, rem;
    alarm((unsigned int)par.SO_TIMEOUT);
    dec_sem(shm_sem_cap_taxi, (unsigned short)get_pos((taxi_x + 1), taxi_y));
    shm_map->map[taxi_x + 1][taxi_y].n_taxi++;

    timeout.tv_nsec = shm_map->map[taxi_x + 1][taxi_y].crossing_time;
    timeout.tv_sec = 0;
    nanosleep(&timeout, &rem);
    alarm(0);

    rel_sem(shm_sem_cap_taxi, get_pos(taxi_x, taxi_y));

    shm_map->map[taxi_x][taxi_y].n_taxi--;
    taxi_x++;
    shm_map->map[taxi_x][taxi_y].crossing_counter++;
    tot_n_cells_cross++;
    tot_n_crossing_time = shm_map->map[taxi_x][taxi_y].crossing_time + tot_n_crossing_time;
}

void move_left() {

    struct timespec timeout, rem;
    alarm((unsigned int)par.SO_TIMEOUT);
    dec_sem(shm_sem_cap_taxi, (unsigned short)get_pos(taxi_x, (taxi_y - 1)));
    shm_map->map[taxi_x][taxi_y - 1].n_taxi++;

    timeout.tv_nsec = shm_map->map[taxi_x][taxi_y - 1].crossing_time;
    timeout.tv_sec = 0;
    nanosleep(&timeout, &rem);
    alarm(0);

    rel_sem(shm_sem_cap_taxi, get_pos(taxi_x, taxi_y));

    shm_map->map[taxi_x][taxi_y].n_taxi--;
    taxi_y--;
    shm_map->map[taxi_x][taxi_y].crossing_counter++;
    tot_n_cells_cross++;
    tot_n_crossing_time = shm_map->map[taxi_x][taxi_y].crossing_time + tot_n_crossing_time;
}

void move_right() {   

    struct timespec timeout, rem;
    alarm((unsigned int)par.SO_TIMEOUT);
    dec_sem(shm_sem_cap_taxi, (unsigned short)get_pos(taxi_x, taxi_y + 1));
    shm_map->map[taxi_x][taxi_y + 1].n_taxi++;

    timeout.tv_nsec = shm_map->map[taxi_x][taxi_y + 1].crossing_time;
    timeout.tv_sec = 0;
    nanosleep(&timeout, &rem);
    alarm(0);

    rel_sem(shm_sem_cap_taxi, get_pos(taxi_x, taxi_y));

    shm_map->map[taxi_x][taxi_y].n_taxi--;
    taxi_y++;
    shm_map->map[taxi_x][taxi_y].crossing_counter++;
    tot_n_cells_cross++;
    tot_n_crossing_time = shm_map->map[taxi_x][taxi_y].crossing_time + tot_n_crossing_time;
}

void move(int end_x, int end_y) {
    if(taxi_x != end_x){
        while (taxi_x != end_x) {
            if (taxi_x < end_x) {
                move_down();
            }
            else if (taxi_x > end_x) {
                move_up();
            }     
        }
    }  
    if(taxi_y != end_y) {
    	while (taxi_y != end_y) {
            if (taxi_y < end_y) {
                move_right();
            }
            else if (taxi_y > end_y) {
                move_left();
            }
        }
    }
}

void update_stats(int outcome) {
    switch (outcome) {
    case 0:
        dec_sem(shm_sem_update_stats, 0);
        statistics->travel_success++;
        if (tot_request > statistics->max_request) {
            statistics->max_request = tot_request;
            statistics->pid_max_request = getpid();
        }

        if (tot_n_cells_cross > statistics->max_cells) {
            statistics->max_cells = tot_n_cells_cross;
            statistics->pid_max_cells = getpid();
        }
        if (tot_n_crossing_time > statistics->max_time_travel) {
            statistics->max_time_travel = tot_n_crossing_time;
            statistics->pid_max_time_travel = getpid();
        }
        rel_sem(shm_sem_update_stats, 0);
        break;
    case 1:
        dec_sem(shm_sem_update_stats, 0);
        statistics->travel_aborted++;
        rel_sem(shm_sem_update_stats, 0);
        break;
    }
}
