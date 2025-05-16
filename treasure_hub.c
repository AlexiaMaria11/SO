#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>

typedef struct{
  float latitude, longitude;
}GPS;

typedef struct{
  int id;
  char user[32];
  GPS gps;
  char clue[64];
  int value;
}TREASURE;

int running=0, stopping=0, mpid=-1;

void read_from_pipe_and_print(int fd) 
{
  char buffer[256];
  int bytesRead;
  while ((bytesRead = read(fd, buffer, 255)) > 0)
  {
    buffer[bytesRead] = '\0';
    printf("%s", buffer);
  }
  if (bytesRead == -1)
  {
    perror("Error reading from pipe");
    exit(EXIT_FAILURE);
  }
}

//counts the treasures from the specified hunt
int countTreasures(char *hunt)
{
  char file[128];
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  struct stat st;
  if(lstat(hunt, &st))
  {
    perror("Error at finding the path\n");
    exit(EXIT_FAILURE);
  }
  else
  if(S_ISDIR(st.st_mode)==0)
    {
	    perror("It isn't a directory\n");
	    exit(EXIT_FAILURE);
    }
  int f=open(file, O_RDONLY);
  if(f==-1)
  {
    perror("Open");
    exit(EXIT_FAILURE);
  }
  int b, c=0;
  TREASURE t;
  while((b=read(f, &t, sizeof(TREASURE)))==sizeof(TREASURE))
  {
    c++;
  }
  if (b == -1) 
  {
    perror("Read");
    if(close(f)==-1)
    {
      perror("Close");
      exit(EXIT_FAILURE);
    }
  }
  if(close(f)==-1)
  {
    perror("Close");
    exit(EXIT_FAILURE);
  }
  return c;
}

//list all the hunts and shows the treasures count in each hunt
void list_hunts()
{
  DIR *dir=opendir(".");
  if(dir==NULL)
  {
    perror("Open directory");
    exit(EXIT_FAILURE);
  }
  struct dirent *d;
  int c=0;
  while((d=readdir(dir)))
  {
    if(strcmp(d->d_name, ".")==0 || strcmp(d->d_name, "..")==0 || strcmp(d->d_name, ".git")==0)
      continue;
    if(d->d_type == DT_DIR)
    {
      c++;
      int nr=countTreasures(d->d_name);
      printf("Name: %s\tNumber of treasures: %d\n", d->d_name, nr);
    }
  }
  if(c==0)
    printf("No hunt found\n");
  closedir(dir);
}

//handler for SIGUSR1 (different actions based on the input from the terminal)
void handler1(int signal)
{
  char aux[128];
  int f=open("command.txt", O_RDONLY);
  if(f==-1)
  {
    perror("Open");
    exit(EXIT_FAILURE);
  }
  int b=read(f, aux, sizeof(aux));
  if(b==-1)
  {
    perror("Read");
    if(close(f)==-1)
    {
      perror("Close");
      exit(EXIT_FAILURE);
    }
    exit(EXIT_FAILURE);
  }
  else if(b==0)
  {
    perror("End of file\n");
    if(close(f)==-1)
    {
      perror("Close");
      exit(EXIT_FAILURE);
    }
    return;
  }
  aux[b]='\0';
  if(close(f)==-1)
  {
    perror("Close");
    exit(EXIT_FAILURE);
  }
  char *p=strtok(aux, " \n");
  if(p==NULL)
    return;
  if(strcmp(p, "list_hunts")==0)
    list_hunts();
  else
  {
    if(strncmp(p, "list_treasures", 14)==0)
    {
      char *hunt=strtok(NULL, "\n");
      if(hunt)
      {
        int pf[2];
        if(pipe(pf)==-1)
        {
          perror("Pipe");
          exit(EXIT_FAILURE);
        }
        int pid=fork();
        if(pid<0)
        {
          perror("Error at creating a child");
          exit(EXIT_FAILURE);
        }
        else if(pid==0)
        {
          close(pf[0]);
          dup2(pf[1], 1);
          close(pf[1]);
          execl("./treasure_manager", "treasure_manager", "--list", hunt, NULL);
          perror("Error at execl");
          exit(EXIT_FAILURE);
        }
        else
        {
          close(pf[1]);
          read_from_pipe_and_print(pf[0]);
          close(pf[0]);
          waitpid(pid, NULL, 0);
        }
      }
      else printf("You need to introduce an hunt\n");
    }
    else
    {
      if(strncmp(p, "view_treasure", 13)==0)
      {
        char *hunt=strtok(NULL, " ");
        char *ID=strtok(NULL, "\n");
        if(hunt && ID) 
        {
          int pf[2];
          if(pipe(pf)==-1)
          {
            perror("Pipe");
            exit(EXIT_FAILURE);
          }
          int pid=fork();
          if(pid<0)
          {
            perror("Error at creating a child");
            exit(EXIT_FAILURE);
          }
          else if(pid==0)
          {
            close(pf[0]);
            dup2(pf[1], 1);
            close(pf[1]);
            execl("./treasure_manager", "treasure_manager", "--view", hunt, ID, NULL);
            perror("Error at execl");
            exit(EXIT_FAILURE);
          }
          else
          {
            close(pf[1]);
            read_from_pipe_and_print(pf[0]);
            close(pf[0]);
            waitpid(pid, NULL, 0);
          }
        }
        else printf("You need to introduce an hunt and a treasure id\n");
      }
      else
      {
        if(strcmp(p, "calculate_score")==0)
        {
          int pf[2];
          if(pipe(pf)==-1)
          {
            perror("Pipe");
            exit(EXIT_FAILURE);
          }
          int pid=fork();
          if(pid<0)
          {
            perror("Error at creating a child");
            exit(EXIT_FAILURE);
          }
          else if(pid==0)
          {
            close(pf[0]);
            dup2(pf[1], 1);
            close(pf[1]);
            execl("./calculate_score", "calculate_score", NULL);
            perror("Error at execl");
            exit(EXIT_FAILURE);
          }
          else
          {
            close(pf[1]);
            read_from_pipe_and_print(pf[0]);
            close(pf[0]);
            waitpid(pid, NULL, 0);
          }
        }
      }
    }
  }
}

//handler for SIGUSR2 (stopping the monitor)
void handler2(int signal)
{
  printf("Monitor is stopping\n");
  usleep(10000000);
  exit(0);
}

//handler for SICHLD (detects if the monitor ended)
void handler3(int signal)
{
  int status, pid;
  while((pid = waitpid(-1, &status, 0)) != -1)   //-1 for any child
  {
    if (WIFEXITED(status)) 
    {
      int code = WEXITSTATUS(status);
      if (pid == mpid) 
      {
        printf("Monitor ended with code: %d\n", code);
        running = 0;
        mpid = -1;
        stopping = 0;
      }
      else 
      {
        if (code == 0)
          printf("treasure_manager or calculate_score ended successfully\n");
        else
          printf("treasure_manager or calculate_score ended with error code: %d\n", code);
      }
    }
  }
}


//starts the monitor and handles the commands with signals
void start_monitor()
{
  if(running)
  {
    printf("The monitor is already running\n");
    return;
  }
  int pid=fork();
  if(pid<0)
  {
    perror("Error at creating a child");
    exit(EXIT_FAILURE);
  }
  else
  {
    if(pid==0)
    {
      struct sigaction sa1, sa2;
      sa1.sa_handler=handler1;
      sa1.sa_flags=0;
      sigaction(SIGUSR1, &sa1, NULL);
      sa2.sa_handler=handler2;
      sa2.sa_flags=0;
      sigaction(SIGUSR2, &sa2, NULL);
      while(1) pause();
      exit(0);
    }
    else
    {
      mpid=pid;
      running=1;
      stopping=0;
      printf("Monitor started running with pid=%d\n", mpid);
    }
  }
}

//write a command to an auxiliar file "commnad.txt"
void command(char *command_name)
{
  int f=open("command.txt", O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if(f==-1)
  {
    perror("Open");
    exit(EXIT_FAILURE);
  }
  write(f, command_name, strlen(command_name));
  kill(mpid, SIGUSR1);
  sleep(5);
  close(f);
}

int main(void)
{
  struct sigaction s;
  s.sa_handler=handler3;
  s.sa_flags=0;
  sigaction(SIGCHLD, &s, NULL);
  char input[128];
  while(1)
  {
    if(fgets(input, sizeof(input), stdin)==NULL)
      continue;
    input[strcspn(input,"\n")]='\0';
    if(strcmp(input, "start_monitor")==0)
      start_monitor();
    else
    {
      if(strcmp(input, "list_hunts")==0)
      {
        if(running==0) printf("You need to start the monitor first\n");
        else if(stopping==1) printf("Monitor is stopping and you can't give any commands\n");
        else command(input);
      }
      else
      {
        if(strncmp(input, "list_treasures", 14)==0)
        {
          if(running==0) printf("You need to start the monitor first\n");
          else if(stopping==1) printf("Monitor is stopping and you can't give any commands\n");
          else command(input);
        }
        else
        {
          if(strncmp(input, "view_treasure", 13)==0)
          {
            if(running==0) printf("You need to start the monitor first\n");
            else if(stopping==1) printf("Monitor is stopping and you can't give any commands\n");
            else command(input);
          }
          else
          {
            if(strcmp(input, "stop_monitor")==0)
            {
              if(running==0) printf("The monitor is not running\n");
              else
              {
                kill(mpid, SIGUSR2);
                sleep(1);
                stopping=1;
              }
            }
            else
            {
              if(strcmp(input, "exit")==0)
              {
                if(running) printf("Monitor is running\n");
                else break;
              }
              else 
              {
                if(strcmp(input, "calculate_score")==0)
                {
                  if(running==0) printf("You need to start the monitor first\n");
                  else if(stopping==1) printf("Monitor is stopping and you can't give any commands\n");
                  else command(input);
                }
                else
                  printf("The command is not correct\n");
              }
            }
          }
        }
      }
    }
  }
  return 0;
}
