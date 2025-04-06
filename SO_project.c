#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef struct{
  float latitude, longitude;
}GPS;

typedef struct{
  int id;
  char user[30];
  GPS gps;
  char clue[60];
  int value;
}TREASURE;

TREASURE* newTreasure()
{
  TREASURE* new = (TREASURE*)malloc(sizeof(TREASURE));
  if(new==NULL)
    {
      perror("Error allocating space for a treasure\n");
      exit(EXIT_FAILURE);
    }
  char buff_in[10], buff_out[120];
  
  strcpy(buff_out, "ID: ");
  write(1, buff_out, strlen(buff_out));
  read(0, buff_in, sizeof(buff_in));
  new->id=atoi(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Username: ");
  write(1, buff_out, strlen(buff_out));
  read(0, new->user, sizeof(new->user));
  new->user[strcspn(new->user, "\n")]='\0';

  strcpy(buff_out, "Latitude: ");
  write(1, buff_out, strlen(buff_out));
  read(0, buff_in, sizeof(buff_in));
  new->gps.latitude=atof(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Longitude: ");
  write(1, buff_out, strlen(buff_out));
  read(0, buff_in, sizeof(buff_in));
  new->gps.longitude=atof(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Clue: ");
  write(1, buff_out, strlen(buff_out));
  read(0, new->clue, sizeof(new->clue));
  new->clue[strcspn(new->clue, "\n")]='\0';

  strcpy(buff_out, "Value: ");
  write(1, buff_out, strlen(buff_out));
  read(0, buff_in, sizeof(buff_in));
  new->value=atoi(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  return new;
}

int main(int argc, char *argv[])
{
  if(argc<2){
    perror("Not enough arguments\n");
    exit(EXIT_FAILURE);
  }
  if(strcmp(argv[1], "--add")==0){
    if(argc<3){
      perror("You need to introduce a hunt id\n");
      exit(EXIT_FAILURE);
    }
    char dir[100];
    strncpy(dir, argv[2], sizeof(dir));
    struct stat stat_buff;
    if(lstat(dir, &stat_buff)==0 && S_ISDIR(stat_buff.st_mode))
      {
	write(1, "Directory already exists\n", strlen("Directory already exists\n"));
      }
    else
      {
	if(mkdir(dir, 0777)==-1)
	  {
	    perror("Couldn't create a new directory\n");
	    exit(EXIT_FAILURE);
	  }
      }
    char file[216], log[216];
    strncpy(file, argv[2], sizeof(file));
    strcat(file, "/treasures_");
    strcat(file, argv[2]);
    strcat(file, ".txt");
    strcpy(log, "logs_");
    strcat(log, argv[2]);
    strcat(log, ".txt");
    int treasure_file=open(file, O_RDWR | O_CREAT | O_APPEND, 0777);
    if(treasure_file==-1)
      {
	perror("Error at opening treasure file\n");
	exit(EXIT_FAILURE);
      }
    int log_file=open(log, O_RDWR | O_CREAT | O_APPEND, 0777);
    if(log_file==-1)
      {
	perror("Error at opening log file\n");
	close(treasure_file);
	exit(EXIT_FAILURE);
      }
    TREASURE* new = newTreasure();
    if(write(treasure_file, new, sizeof(TREASURE*))==-1)
      {
	perror("Error at writing a new feature");
	exit(EXIT_FAILURE);
      }
    free(new);
    close(treasure_file);
    close(log_file);
  }
  return 0;
}
