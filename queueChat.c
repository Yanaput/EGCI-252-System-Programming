#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>

// struct myMsg init
struct myMsg{
  long int type;
  char data[BUFSIZ];  // 8192
};

int msgID;

void endChatHandler(int signo){
  if(signo == SIGTERM){
    msgctl(msgID, IPC_RMID, NULL);
    exit(EXIT_SUCCESS);
  }
}

int main(int argc, char* argv[]){
  int pid, running = 1;
  struct myMsg msg;

  if(argc != 2){ //if argc < 2 end program 
    fprintf(stderr, "Usage: %s <[1, 2]>\n", *argv);
    exit(EXIT_FAILURE);
  }

  msgID = msgget((key_t) 6481328, 0666 | IPC_CREAT);  // leader's ID
  if (msgID == -1){
    fprintf(stderr, "msgget failed\n");
    exit(EXIT_FAILURE);
  }

  signal(SIGTERM, endChatHandler); // set handler for SIGTERM to end chat

  if (!strcmp(argv[1], "1")) {
    pid = fork();  // fork child 1
    switch (pid) {
    case -1:
      perror("Forking failed\n");
      exit(EXIT_FAILURE);
    case 0: // child 1 receiving msg type 2
      while (strncmp(msg.data, "end chat\n", 8)) {
        if (msgrcv(msgID, (void *) &msg, BUFSIZ, 2, 0) == -1) {  // receiving msg_type 2
          fprintf(stderr, "msgrcv failed\n");
          exit(EXIT_FAILURE);
        }
        printf("%s", msg.data);
      }
      kill(getppid(), SIGTERM);  // terminate parent
      raise(SIGTERM);  // terminate child itself
      break;
    default: // parent 1 sending msg type 1
      while (strncmp(msg.data, "end chat\n", 8)) {
        fgets(msg.data, BUFSIZ, stdin);
        msg.type = 1;  // set msg type 1
        if (msgsnd(msgID, (void *) &msg, BUFSIZ, 0) == -1) {
          fprintf(stderr, "msgsnd failed\n");
          exit(EXIT_FAILURE);
        }
      }
      kill(pid, SIGTERM);  // terminate child
      raise(SIGTERM);  // terminate parent itself
      break;
    }
  }

  if(!strcmp(argv[1], "2")){
    pid = fork();  // fork child 2
    switch (pid) {
    case -1:
      perror("Forking failed\n");
      exit(EXIT_FAILURE);
    case 0: // child 2 receiving msg type 1
      while (strncmp(msg.data, "end chat\n", 8)) {
        if (msgrcv(msgID, (void *)&msg, BUFSIZ, 1, 0) == -1) {  // receivig msg type 1
          fprintf(stderr, "msgrcv failed\n");
          exit(EXIT_FAILURE);
        }
        printf("%s", msg.data);
      }
      kill(getppid(), SIGTERM);  // terminate parent
      raise(SIGTERM);  // terminate child itself
      break;
    default: // parent 2 sending msg type 2
      while (strncmp(msg.data, "end chat\n", 8)) {
        fgets(msg.data, BUFSIZ, stdin);
        msg.type = 2;  // set msg type to 2
        if (msgsnd(msgID, (void *)&msg, BUFSIZ, 0) == -1) {
          fprintf(stderr, "msgsnd failed\n");
          exit(EXIT_FAILURE);
        }
      }
      kill(pid, SIGTERM);  // terminate child
      raise(SIGTERM);  // terminate parent itself
      break;
    }
  }
  exit(EXIT_SUCCESS);
}