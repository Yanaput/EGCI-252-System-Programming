#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#define BUFSIZ 1024
#define MEM_SIZE 4096

struct shm_st{
  int written1;
  int written2;
  char data1[BUFSIZ];
  char data2[BUFSIZ];
};

void endChatHandler(int signum){
  if(signum == SIGTERM){
    exit(EXIT_SUCCESS);
  }
}

int main(int argc, char *argv[]){
  signal(SIGTERM, endChatHandler);
  if(argc != 2){
    fprintf(stderr, "Usage: %s <[1, 2]>\n", *argv);
    exit(EXIT_FAILURE);
  }
  int child;
  void *sh_mem = NULL;
  struct shm_st *sh_area;
  char buffer[BUFSIZ];


  int shmid = shmget((key_t) 6481145, MEM_SIZE, 0666 | IPC_CREAT);

  if (shmid == -1){
    fprintf(stderr, "shmget failed\n");
    exit(EXIT_FAILURE);
  }

  sh_mem = shmat(shmid, NULL, 0);


  if (sh_mem == (void *) -1){
    fprintf(stderr, "shmat failed\n");
    exit(EXIT_FAILURE);
  }

  sh_area = (struct shm_st *) sh_mem;

  if(!strcmp(argv[1], "1")){
    child = fork();
    switch(child){
      case -1 :
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
      case 0 : //child read from 2
        while(strncmp(sh_area->data2, "end chat",8)){
          if (sh_area->written2) {
            printf("%s", sh_area->data2);
            sh_area->written2 = 0;
          }
        }
        printf("out loop\n");
        kill(getppid(), SIGTERM);
        raise(SIGTERM);
        break;

      default :
        while(strncmp(buffer, "end chat", 8)){
          fgets(buffer, BUFSIZ, stdin);
          strcpy(sh_area->data1, buffer);
          sh_area->written1 = 1;
        }
        printf("out loop\n");
        kill(getpid(), SIGTERM);
        break;
    }
  }


  if(!strcmp(argv[1], "2")){
    child = fork();
    switch(child){
      case -1 :
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
      case 0 : //child read from 1
        while(strncmp(sh_area->data1, "end chat",8)){
          if (sh_area->written1) {
            printf("%s", sh_area->data1);
            sh_area->written1 = 0;
          }
        }
        printf("out loop\n");
        kill(child, SIGTERM);
        raise(SIGTERM);
        break;

      default :
        while(strncmp(buffer, "end chat",8)){
          fgets(buffer, BUFSIZ, stdin);
          strcpy(sh_area->data2, buffer);
          sh_area->written2 = 1;
        }
        kill(getpid(), SIGTERM);
        break;
    }
  }

  if (shmdt(sh_mem) == -1) {
      fprintf(stderr, "shmdt failed\n");
      exit(EXIT_FAILURE);
  }

  if (shmctl(shmid, IPC_RMID, 0) == -1) {
      fprintf(stderr, "shmctl(IPC_RMID) failed\n");
      exit(EXIT_FAILURE);
  }
}