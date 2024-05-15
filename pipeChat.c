#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define FIFO_1 "./fifo1to2"
#define FIFO_2 "./fifo2to1"
#define MAX_RBUF 80

int FIFO_FD1, FIFO_FD2;

void endChatHandler(int signo) {//signal handler for SIGTERM
    if (signo == SIGTERM){
      unlink(FIFO_1);
      unlink(FIFO_2);
      exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]){
  int child, nbytes;
  char rbuf[MAX_RBUF]= "";

  if(argc != 2){//if argc < 2 end program 
    fprintf(stderr, "Usage: %s <[1, 2]>\n", *argv);
    exit(EXIT_FAILURE);
  }
  if(access(FIFO_1, F_OK) == -1){ 
    FIFO_FD1 = mkfifo(FIFO_1, 0777);
    if(FIFO_FD1){
      fprintf(stderr, "Could not create fifo %s\n", FIFO_1);
      exit(EXIT_FAILURE);
    }
  }
  if(access(FIFO_2, F_OK) == -1){
    FIFO_FD2 = mkfifo(FIFO_2, 0777);
    if(FIFO_FD2){
      fprintf(stderr, "Could not create fifo %s\n", FIFO_2);
      exit(EXIT_FAILURE);
    }
  }

  FIFO_FD1 = open(FIFO_1, O_RDWR);   
  FIFO_FD2 = open(FIFO_2, O_RDWR);

  argv++;

  if(strcmp(*argv, "1") == 0){ // if chat 1 do this
    child = fork(); // fork child to receive msg from pipe and to print to stdout
    signal(SIGTERM, endChatHandler);// set signal handler for SIGTERM
    switch(child){
      case -1: 
        perror("Forking failed");
        exit(EXIT_FAILURE);
      case 0://child 1
        while(strncmp(rbuf, "end chat\n", 8)){ // check if msg is end chat or not 
          memset(rbuf, '\0', MAX_RBUF); // reset buffer
          nbytes = read(FIFO_FD2, rbuf, MAX_RBUF); //read from pipe 2to 1
          write(1, rbuf, nbytes); // write stdout
        }
        close(FIFO_FD2);
        kill(getppid(), SIGTERM); // send signal to parent
        break;
      default://parent 1
        while(strncmp(rbuf, "end chat\n",8) ){ 
          memset(rbuf, '\0', MAX_RBUF); 
          nbytes = read(0, rbuf, MAX_RBUF); // read from stdin
          write(FIFO_FD1, rbuf, nbytes);// write to pipe 1to2
        }
        close(FIFO_FD1);
        kill(child, SIGTERM); // kill it's child becuase why not
        raise(SIGTERM); // kill itself becuase life is sad and it kill it own child
        break;
    }
  }

  else if(strcmp(*argv, "2") == 0){ // if chat 2 do this (alomost repeat above but change pipe)
    child = fork();
    signal(SIGTERM, endChatHandler);
    nbytes = sizeof(rbuf);
    switch (child){
      case -1:
        perror("Forking failed");
        exit(EXIT_FAILURE);
      case 0: //child 2
        while(strncmp(rbuf, "end chat\n", 8)) {
            memset(rbuf, '\0', MAX_RBUF);
            nbytes = read(FIFO_FD1, rbuf, MAX_RBUF);
            write(1, rbuf, nbytes);
        }
        close(FIFO_FD1);
        kill(getppid(), SIGTERM);
        break;
      default://parent 2
        while(strncmp(rbuf, "end chat\n", 8)){
          memset(rbuf, '\0', MAX_RBUF);
          nbytes = read(0, rbuf, MAX_RBUF);
          write(FIFO_FD2, rbuf, nbytes);
        }
        close(FIFO_FD2);
        kill(child, SIGTERM);
        raise(SIGTERM);
        break;
    }
  }
  if(FIFO_FD1 != -1) close(FIFO_FD1);
  if(FIFO_FD2 != -1) close(FIFO_FD2);

  exit(EXIT_SUCCESS);
}