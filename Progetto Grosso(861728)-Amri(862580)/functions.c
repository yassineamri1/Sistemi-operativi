#include "header.h"

parameters read_file(parameters par) {
    FILE *file;
    int i;
    int j=0;
    char **parameters = (char **)calloc(10, sizeof(char *));
    char *line = (char *)calloc( 40, sizeof(char));
    char *tok;

    if ((file = fopen("parametri.txt", "r")) == NULL)
        fprintf(stderr, "Can't open file\n");

    while(fscanf(file,"%[^\n]", line ) != EOF) {   
        getc(file);
        tok = strtok(line, "=");
        while(tok != NULL) {
            i=0;
            while(i<2) {
                switch(i) {
                    case 0:
                        break;
                        
                    case 1:
                        parameters[j] = strcpy((char*)calloc(strlen(tok) +1, sizeof(char)), tok);
                        j++;
                        break;
                }
                tok = strtok(NULL, "=");
                i++;
            }       
        }

    }

    fclose(file);

    par.SO_HOLES = atoi(parameters[0]);
    par.SO_TOP_CELLS = atoi(parameters[1]);
    par.SO_CAP_MAX= atoi(parameters[2]);
    par.SO_CAP_MIN = atoi(parameters[3]);
    par.SO_TAXI = atoi(parameters[4]);
    par.SO_DURATION  = (long) atoi(parameters[5]);
    par.SO_TIMEOUT = (long) atoi(parameters[6]);
    par.SO_TIMENSEC_MIN= atoi(parameters[7]);
    par.SO_TIMENSEC_MAX = atoi(parameters[8]);
    par.SO_SOURCES = atoi(parameters[9]);

    return par;
}

int set_sem(int sem_id, int index, int sem_val) {
    return semctl(sem_id, index, SETVAL, sem_val);
}


void dec_sem(int sem_id, int index) {
    struct sembuf sops;
    sops.sem_num =(unsigned short) index;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    semop(sem_id, &sops, 1);
}


int dec_sem_no_wait(int sem_id, int index) {
    struct sembuf sops;
    sops.sem_num = (unsigned short) index;
    sops.sem_op = -1;
    sops.sem_flg = IPC_NOWAIT;
    return semop(sem_id, &sops, 1);
}

void rel_sem(int sem_id, int index) {
    struct sembuf sops;
    sops.sem_num =(unsigned short) index;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    semop(sem_id, &sops, 1);
}

void wait_sem_zero(int sem_id, int index) {
    struct sembuf sops;
    sops.sem_num = (unsigned short) index;
    sops.sem_op = 0;
    sops.sem_flg = 0;
    semop(sem_id, &sops, 1);
}

int get_sem_val(int sem_id, int x, int y) {
    return semctl(sem_id, get_pos(x, y), GETVAL,0);
}
