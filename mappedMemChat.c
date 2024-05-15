#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#define FILE_LENGTH 0x100

void* file;

struct mmchatFlag{
  int write2;
  int write1;
  char data1[256];
  char data2[256];
};

void endChatHandler(int signo){
  if(signo == SIGTERM)
    munmap(file, FILE_LENGTH);
    system("rm -f chat_log");
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]){
  if(argc != 2){
    printf("need 2 args\n");
    exit(EXIT_FAILURE);
  }
  
  
  signal(SIGTERM, endChatHandler);

  char buffer[256];
  int fd = open("chat_log", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  lseek (fd, FILE_LENGTH+1, SEEK_SET);
  write (fd, "", 1);
  lseek (fd, 0, SEEK_SET);
  file = mmap (0, FILE_LENGTH, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
  close (fd);

  int run = 1;
  
  struct mmchatFlag* file_memory = (struct mmchatFlag*)file;
  
  if(atoi(argv[1]) == 1){
    int child = fork();
    switch(child){
      case -1 :
        printf("Fork failed\n");
        exit(EXIT_FAILURE);
        break;
      case 0 : // child
        while (run) {
          if(file_memory->write2 == 1){
            strcpy(buffer,file_memory->data2);
            if(strncmp(buffer, "end chat", 8) == 0){
              run = 0;
            }
            printf("%s", buffer);
            file_memory->write2 = 0;
            memset(buffer, '\0', 256);
          }
        }
        // munmap(file, FILE_LENGTH);
        kill(getppid(), SIGTERM);
        raise(SIGTERM);
        break;
      default : // parent
        while(run){
          fgets(buffer, 256, stdin);
          if(strncmp(buffer, "end chat", 8) == 0){
            run = 0;
          }
          sprintf(file_memory->data1, "%s", buffer);
          fflush(stdin);
          file_memory->write1 = 1;
        }
        // munmap(file, FILE_LENGTH);
        kill(child, SIGTERM);
        raise(SIGTERM);
        break;
    }
  }

  if(atoi(argv[1])==2){
    int child = fork();
    switch(child){
      case -1 :
        printf("Fork failed\n");
        exit(EXIT_FAILURE);
        break;
      case 0 : // child
        while(run){
          if(file_memory->write1 == 1){
            strcpy(buffer, file_memory->data1);
            if(strncmp(buffer, "end chat", 8) == 0){
              run = 0;
            }
            printf("%s", buffer);
            file_memory->write1 = 0;
            memset(buffer, '\0', 256);
          }
        }
        // munmap(file, FILE_LENGTH);
        kill(getppid(), SIGTERM);
        raise(SIGTERM);
        break;
      default : // parent
        while(run){
          fgets(buffer, 256, stdin);
          if(strncmp(buffer, "end chat", 8) == 0){
            run = 0;
          }
          sprintf(file_memory->data2, "%s", buffer);
          fflush(stdin);
          file_memory->write2 = 1;
        }
        // munmap(file, FILE_LENGTH);
        kill(child, SIGTERM);
        raise(SIGTERM);
        break;
    }
  }
}