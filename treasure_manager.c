#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
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
    int hasDot = 0;
    int hasDigit = 0;

    while (str[i] != '\0') {
        if (isdigit(str[i])) {
            hasDigit = 1; 
        } else if (str[i] == '.' && !hasDot && hasDigit) {
            hasDot = 1; 
        } else if (str[i] == '-' && i == 0) {
        } else {
            return 0; 
        }
        i++;
    }
    return hasDigit && (hasDot || i > 0);
}

TREASURE* newTreasure(char *hunt)
{
  TREASURE* new = (TREASURE*)malloc(sizeof(TREASURE));
  if(new==NULL)
    {
      perror("Error allocating space for a new treasure\n");
      exit(EXIT_FAILURE);
    }
  char buff_in[32], buff_out[256];
  
  strcpy(buff_out, "Treasure ID: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(atoi(buff_in)==0)
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
      free(new);
      exit(EXIT_FAILURE);
    }
  TREASURE t;
  char file[128];
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  int f=open(file, O_RDONLY, 0777);
  if(f==-1)
    {
      perror("Error at opening treasure file\n");
      free(new);
      exit(EXIT_FAILURE);
    }
  while(read(f, &t, sizeof(TREASURE))==sizeof(TREASURE))
  {
    if(strcmp(buff_in, t.user)==0)
    {
      perror("This user name is already in the file\n");
      free(new);
      close(f);
      exit(EXIT_FAILURE);
    }
  }
  close(f);
  strcpy(new->user, buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Latitude: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(isValidFloat(buff_in)==0)
  {
    perror("Invalid latitude\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  new->gps.latitude=atof(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Longitude: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(isValidFloat(buff_in)==0)
  {
    perror("Invalid longitude\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  new->gps.longitude=atof(buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Clue: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(strlen(buff_in)==0)
    {
      perror("Clue is empty\n");
      free(new);
      exit(EXIT_FAILURE);
    }
  strcpy(new->clue, buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  strcpy(buff_out, "Value: ");
  write(1, buff_out, strlen(buff_out));
  read_line(buff_in, sizeof(buff_in));
  if(atoi(buff_in)==0)
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
  snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
  snprintf(link, sizeof(link), "logged_hunt-%s", hunt);
  int f=open(file, O_WRONLY | O_CREAT | O_APPEND, 0777);
  if(f==-1)
    {
      perror("Error at opening treasure file\n");
      exit(EXIT_FAILURE);
    }
  TREASURE *new = newTreasure(hunt);
  if(write(f, new, sizeof(*new))==-1)
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
      close(f);
      free(new);
      exit(EXIT_FAILURE);
    }
  char info[256];
  sprintf(info,  "--add: ID:%d User:%s Latitude:%f Longitude:%f Clue:%s Value:%d\n", new->id, new->user, new->gps.latitude, new->gps.longitude, new->clue, new->value);
  if(write(lo, info, strlen(info))==-1)
  {
    perror("Error writing to log file\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  close(lo);
  free(new);
  if(lstat(link, &st)==-1)
    {
      if(symlink(log, link)==-1)
	    {
	      perror("Couldn't create a symlink\n");
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

void view(char *hunt, char *id)
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

void remove_treasure(char *hunt, char *id)
{
    if(atoi(id)==0)
    {
        perror("Not a valid id\n");
        exit(EXIT_FAILURE);
    }
    int ID=atoi(id);
    char file[128], aux[128];
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
    snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
    snprintf(aux, sizeof(aux), "%s/aux.dat", hunt);
    int f=open(file, O_RDONLY, 0777);
    if(f==-1)
    {
        perror("Couldn't open the file\n");
        exit(EXIT_FAILURE);
    }
    int a=open(aux, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if(a==-1)
    {
        perror("Couldn't open the auxiliar file\n");
        exit(EXIT_FAILURE);
    }
    TREASURE t;
    int found_id=0;
    while(read(f, &t, sizeof(TREASURE))==sizeof(TREASURE))
    {
        if(t.id==ID)
        {
          found_id=1;
          continue;
        }
        write(a, &t, sizeof(TREASURE));
    }
    close(f);
    close(a);
    if(found_id)
    {
      if(unlink(file))
      {
        perror("Couldn't remove the old file\n");
        exit(EXIT_FAILURE);
      }
      if(rename(aux, file))
      {
        perror("Couldn't rename the auxiliary file\n");
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      if(unlink(aux))
      {
        perror("Couldn't remove the auxiliary file\n");
        exit(EXIT_FAILURE);
      }
      write(1, "The treasure with this id was not found\n", 40);
      return;
    }
   char log[128], info[128];
   snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
   snprintf(info, sizeof(info), "--remove_treasure: removed the treasure with the %d id from the %s hunt\n", ID, hunt);
   int lo=open(log, O_WRONLY | O_CREAT | O_APPEND, 0777);
   if(lo==-1)
    {
        perror("Error at opening log file\n");
        exit(EXIT_FAILURE);
    }
   write(lo, info, strlen(info));
   close(lo);
   write(1, "Treasure removed\n", 18);
}

void remove_hunt(char *hunt)
{
  char file[128], log[128], link[128];
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
  snprintf(link, sizeof(link), "logged_hunt-%s", hunt);
  struct stat st;
  if(lstat(hunt, &st))
  {
    perror("Error at finding the directory path\n");
    exit(EXIT_FAILURE);
  }
  if(S_ISDIR(st.st_mode)==0)
  {
    perror("Not a directory\n");
    exit(EXIT_FAILURE);
  }
  if(lstat(file, &st))
  {
    perror("Error at finding the file path\n");
    exit(EXIT_FAILURE);
  }
  if(S_ISREG(st.st_mode)==0)
  {
    perror("Not a regular file\n");
    exit(EXIT_FAILURE);
  }
  if(unlink(file))
  {
    perror("Error at deleting the file\n");
    exit(EXIT_FAILURE);
  }
  if(lstat(link, &st))
  {
    perror("Error at finding the link path\n");
    exit(EXIT_FAILURE);
  }
  if(S_ISLNK(st.st_mode)==0)
  {
    perror("Not a link\n");
    exit(EXIT_FAILURE);
  }
  if(unlink(link))
  {
    perror("Error at deleting the link\n");
    exit(EXIT_FAILURE);
  }
  if(lstat(log, &st))
  {
    perror("Error at finding the log path\n");
    exit(EXIT_FAILURE);
  }
  if(S_ISREG(st.st_mode)==0)
  {
    perror("Not a regular file\n");
    exit(EXIT_FAILURE);
  }
  if(unlink(log))
  {
    perror("Error at deleting the log file\n");
    exit(EXIT_FAILURE);
  }
  if(rmdir(hunt))
  {
    perror("Error at deleting the directory\n");
    exit(EXIT_FAILURE);
  }
  write(1, "Hunt removed\n", 13);
}

int main(int argc, char *argv[])
{
  if(argc<2){
    perror("Not enough arguments\n");
    exit(EXIT_FAILURE);
  }
  if(strcmp(argv[1], "--add")==0){
    if(argc<3)
    {
      perror("You need to introduce a hunt id\n");
      exit(EXIT_FAILURE);
    }
    else {
      if(argc==3)
            add(argv[2]);
      else
      {
        perror("Too many arguments\n");
        exit(EXIT_FAILURE);
      }
    }
  }
  else
    {
      if(strcmp(argv[1], "--list")==0)
	    {
	      if(argc<3){
	        perror("You need to introduce a hunt id\n");
	        exit(EXIT_FAILURE);
	      }
        else 
          if(argc==3)
	            list(argv[2]);
          else
          {
            perror("Too many arguments\n");
            exit(EXIT_FAILURE);
          }
	    }
      else
      {
        if(strcmp(argv[1], "--view")==0)
        {
          if(argc<4)
          {
            perror("You need to introduce a hunt id AND an id\n");
            exit(EXIT_FAILURE);
          }
          else
            if(argc==4)
              view(argv[2], argv[3]);
            else
            {
              perror("Too many arguments\n");
              exit(EXIT_FAILURE);
            }
        }
        else
        {
            if(strcmp(argv[1], "--remove_treasure")==0)
            {
                if(argc<4)
                {
                    perror("You need to introduce a hunt id AND an id\n");
                    exit(EXIT_FAILURE);
                }
                else
                    if(argc==4)
                        remove_treasure(argv[2], argv[3]);
                    else
                    {
                        perror("Too many arguments\n");
                        exit(EXIT_FAILURE);
                    }
            }
            else{
              if(strcmp(argv[1], "--remove_hunt")==0)
              {
                if(argc<3){
                  perror("You need to introduce a hunt id\n");
                  exit(EXIT_FAILURE);
                }
                else if(argc==3)
                  remove_hunt(argv[2]);
                  else
                  {
                    perror("Too many arguments\n");
                    exit(EXIT_FAILURE);
                  }
            }
            else
            {
              perror("Invalid argument\n");
            }
        }
      }
      }
    }
  return 0;
}
