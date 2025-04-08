#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
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

int read_line(char *text, int size)
{
  int len=read(0, text, size-1);
  if(len>0)
    {
      if(text[len-1]=='\n')
	{
	  text[len-1]='\0';
	  len--;
	}
      else
	text[len]='\0';
    }
  else
    text[0]='\0';
  return len;
}

int isValidFloat(char* str) {
    int i = 0;
    int hasDot=0;
    int hasDigit = 0;
    while (str[i] != '\0') {
        if (isdigit(str[i])) {
            hasDigit = 1;
        } else if (str[i] == '.' && !hasDot) {
            hasDot = 1;
        } else {
            return 0;
        }
        i++;
    }
    return hasDigit;
}

TREASURE* newTreasure()
{
  TREASURE* new = (TREASURE*)malloc(sizeof(TREASURE));
  if(new==NULL)
    {
      perror("Error allocating space for a treasure\n");
      exit(EXIT_FAILURE);
    }
  char buff_in[16], buff_out[256];

  //intrebare daca se introduc date gresite asteptam date corecte sau iesim din program
  
  strcpy(buff_out, "ID: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(atoi(buff_in)<=0)
    {
      perror("Invalid id\n");
      free(new);
      exit(EXIT_FAILURE);
    }
  new->id=atoi(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Username: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(strlen(buff_in)==0)
    {
      perror("Username is empty\n");
      exit(EXIT_FAILURE);
    }
  strcpy(new->user, buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Latitude: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(atoi(buff_in))
    new->gps.latitude=atoi(buff_in);
  else
    if(isValidFloat(buff_in)==0)
      {
	perror("Invalid latitude(float)\n");
	exit(EXIT_FAILURE);
      }
    else
      new->gps.latitude=atof(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Longitude: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(atoi(buff_in))
    new->gps.longitude=atoi(buff_in);
  else
    if(isValidFloat(buff_in)==0)
      {
	perror("Invalid longitude(float)\n");
	exit(EXIT_FAILURE);
      }
    else
      new->gps.longitude=atof(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Clue: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(strlen(buff_in)==0)
    {
      perror("Clue is empty\n");
      exit(EXIT_FAILURE);
    }
  strcpy(new->clue, buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Value: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(atoi(buff_in)<=0)
    {
      perror("Invalid value\n");
      free(new);
      exit(EXIT_FAILURE);
    }
  new->value=atoi(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  return new;
}

void add(char *hunt)
{
  char dir[128], file[128], log[128], link[128];
  strncpy(dir, hunt, sizeof(dir));
  snprintf(dir, sizeof(dir), "%s", hunt);
  struct stat st;
  if(lstat(dir, &st)==0 && S_ISDIR(st.st_mode))
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
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  snprintf(log, sizeof(log), "%s/logged", hunt);
  snprintf(link, sizeof(link), "logged_%s", hunt);
  int f=open(file, O_RDWR | O_CREAT | O_APPEND, 0777);
  if(f==-1)
    {
      perror("Error at opening treasure file\n");
      exit(EXIT_FAILURE);
    }
  TREASURE* new = newTreasure();
  if(write(f, new, sizeof(TREASURE*))==-1)
    {
      perror("Error at writing a new feature");
      close(f);
      exit(EXIT_FAILURE);
    }
  close(f);
  int lo=open(log, O_WRONLY | O_CREAT | O_APPEND, 0777);
  if(lo==-1)
    {
      perror("Error at opening log file\n");
      exit(EXIT_FAILURE);
    }
  char info[256];
  sprintf(info,  "--add: ID:%d User:%s Latitude:%f Longitude:%f Clue:%s Value:%d\n", new->id, new->user, new->gps.latitude, new->gps.longitude, new->clue, new->value);
  write(lo, info, strlen(info));
  close(lo);
  free(new);
  if(lstat(link, &st)==-1)
    {
      if(symlink(log, link)==-1)
	{
	  perror("couldn't create a symlink\n");
	  exit(EXIT_FAILURE);
	}
    }
  else
    {
      if(S_ISLNK(st.st_mode))
	{
	  write(1, "Symbolic link already exists\n", strlen("Symbolic link already exists\n"));
	}
    }
  write(1, "Treasure added\n", 16);
}

void list(char *hunt)
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
  write(1, "Hunt: ", 6);
  write(1, hunt, strlen(hunt));
  if(lstat(file, &st))
    {
      perror("Error at finding the treasure file\n");
      exit(EXIT_FAILURE);
    }
  else
    if(S_ISREG(st.st_mode)==0)
      {
	perror("It isn't a regular file\n");
	exit(EXIT_FAILURE);
      }
  char size[64];

  //intrebare putem folosi snprintf
  
  snprintf(size, sizeof(size), "\nTotal size: %ld\n", st.st_size);
  write(1, size, strlen(size));
  char modification[64];
  snprintf(modification, sizeof(modification), "Last modification: %s", ctime(&st.st_mtime));
  write(1, modification, strlen(modification));
  TREASURE t;
  int f=open(file, O_RDONLY);
  if(f==-1)
    {
      perror("Error at opening a treasure file\n");
      exit(EXIT_FAILURE);
    }
  int b;
  while((b=read(f, &t, sizeof(TREASURE)))>0)
    {
      char info[256];
      //nu afiseaza bine
      sprintf(info,  "ID:%d User:%s Latitude:%f Longitude:%f Clue:%s Value:%d\n", t.id, t.user, t.gps.latitude, t.gps.longitude, t.clue, t.value);
      write(1, info, strlen(info));
    }
  close(f);
  char log[128], aux[128];
  snprintf(log, sizeof(log), "%s/logged", hunt);
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
    add(argv[2]);
  }
  else
    {
      if(strcmp(argv[1], "--list")==0)
	{
	   if(argc<3){
	     perror("You need to introduce a hunt id\n");
	     exit(EXIT_FAILURE);
	   }
	   list(argv[2]);
	}
    }
  return 0;
}
