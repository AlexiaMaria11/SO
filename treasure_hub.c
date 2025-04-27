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

void start()
{
    if(running)
    {
        printf("The monitor is already running\n");
        return;
    }
    int pid=fork();
    if(pid<0)
    {
        perror("Error at creating a child\n");
        exit(EXIT_FAILURE);
    }
    else{
        if(pid==0)
        {
            //nu stiu ce vine aici
            exit(0);
        }
        else{
            mpid=pid;
            running=1;
            stopping=0;
            printf("Monitor started running\n");
        }
    }
}

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
        perror("Couldn't open the treasures file\n");
        exit(EXIT_FAILURE);
    }
    int b, c=0;
    TREASURE t;
    while((b=read(f, &t, sizeof(TREASURE)))==sizeof(TREASURE))
    {
      c++;
    }
    close(f);
    return c;
}

void list_hunts()
{
    DIR *dir=opendir(".");
    if(dir==NULL)
    {
        perror("Error at opening the directory\n");
        exit(EXIT_FAILURE);
    }
    struct dirent *d;
    int c=0;
    while((d=readdir(dir)))
    {
        if(strcmp(d->d_name, ".")==0 || strcmp(d->d_name, "..")==0)
            continue;
        if(d->d_type == DT_DIR)
        {
            c++;
            int nr=countTreasures(d->d_name);
            printf("Name: %s\nNumber of treasures: %d\n", d->d_name, nr);
        }
    }
    if(c==0)
        printf("No hunt found\n");
    closedir(dir);
}

int main(int argc, char *argv[])
{

    return 0;
}