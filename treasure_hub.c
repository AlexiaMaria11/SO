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

void list_treasures(char *hunt)
{
  char file[128], log[128];
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
  struct stat st,st1, st2;
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
  write(1, "Hunt: ", 6);
  write(1, hunt, strlen(hunt));
  if(lstat(file, &st1))
    {
      perror("Error at finding the treasure file\n");
      exit(EXIT_FAILURE);
    }
  else
    if(S_ISREG(st1.st_mode)==0)
      {
	      perror("It isn't a regular file\n");
	      exit(EXIT_FAILURE);
      }
  if(lstat(log, &st2))
    {
      perror("Error at finding the treasure file\n");
      exit(EXIT_FAILURE);
    }
  else
    if(S_ISREG(st2.st_mode)==0)
      {
	      perror("It isn't a regular file\n");
	      exit(EXIT_FAILURE);
      }
  char size[64];
  
  snprintf(size, sizeof(size), "\nTotal size: %ld\n", st1.st_size+st2.st_size);
  write(1, size, strlen(size));
  char modification[64];
  snprintf(modification, sizeof(modification), "Last modification: %s", ctime(&st.st_mtime));
  write(1, modification, strlen(modification));
  TREASURE t;
  int f=open(file, O_RDONLY, 0777);
  if(f==-1)
    {
      perror("Error at opening a treasure file\n");
      exit(EXIT_FAILURE);
    }
  int b;
  while((b=read(f, &t, sizeof(TREASURE)))==sizeof(TREASURE))
    {
      char info[256];
      snprintf(info, sizeof(info), "ID: %d - User: %s - Latitude: %f - Longitude: %f - Clue: %s - Value: %d\n", t.id, t.user, t.gps.latitude, t.gps.longitude, t.clue, t.value);
      write(1, info, strlen(info));
    }
  close(f);
  char aux[128];
  snprintf(aux, sizeof(aux), "--list: listed all the treasures from %s\n", hunt);
  int lo=open(log, O_WRONLY | O_CREAT | O_APPEND, 0777);
  if(lo==-1)
    {
      perror("Error at opening log file\n");
      exit(EXIT_FAILURE);
    }
  write(lo, aux, strlen(aux));
  close(lo);
}

void view_treasure(char *hunt, char *id)
{
    if(atoi(id)==0)
    {
        perror("Not a valid id\n");
        exit(EXIT_FAILURE);
    }
    int ID=atoi(id);
    char file[128];
    struct stat st;
    if(lstat(hunt, &st))
    { 
        perror("Error at finding the path\n");
        exit(EXIT_FAILURE);
    }
    else 
    {
        if(S_ISDIR(st.st_mode)==0)
        {
        perror("Not a directory\n");
        exit(EXIT_FAILURE);
        }
    }
    int found_id=0;
    snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
    int f=open(file, O_RDONLY, 0777);
    if(f==-1)
    {
        perror("Couldn't open the file\n");
        exit(EXIT_FAILURE);
    }
    TREASURE t;
    while(read(f, &t, sizeof(TREASURE))==sizeof(TREASURE) && !found_id)
    {
        if(t.id==ID)
        {
          found_id=1;
          char info[256];
          snprintf(info, sizeof(info), "ID: %d - User: %s - Latitude: %f - Longitude: %f - Clue: %s - Value: %d\n", t.id, t.user, t.gps.latitude, t.gps.longitude, t.clue, t.value);
          write(1, info, strlen(info));
        }
    }
    close(f);
    if(found_id==0)
    {
        write(1, "ID not found\n", 14);
    }
    char log[128], aux[128];
    snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
    snprintf(aux, sizeof(aux), "--view: viewed the treasure with the %d id\n", ID);
    int lo=open(log, O_WRONLY | O_CREAT | O_APPEND, 0777);
    if(lo==-1)
    {
        perror("Error at opening log file\n");
        exit(EXIT_FAILURE);
    }
    write(lo, aux, strlen(aux));
    close(lo);
}

int main(int argc, char *argv[])
{

    return 0;
}