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

int safe_open(char *path, int flags, mode_t mode) 
{
  int fd = open(path, flags, mode);
  if (fd == -1) 
  {
    perror(path);
    exit(EXIT_FAILURE);
  }
  return fd;
}

void safe_close(int fd) 
{
  if (close(fd) == -1) 
  {
    perror("Close");
    exit(EXIT_FAILURE);
  }
}

void ensure_type(char *path, int type) 
{
    struct stat st;
    if (lstat(path, &st) == -1) 
    {
        perror(path);
        exit(EXIT_FAILURE);
    }
    int is_type = 0;
    switch(type) 
    {
      case S_IFDIR:
        is_type = S_ISDIR(st.st_mode);
        break;
      case S_IFREG:
        is_type = S_ISREG(st.st_mode);
        break;
      case S_IFLNK:
        is_type = S_ISLNK(st.st_mode);
        break;
      default:
        fprintf(stderr, "Unsupported type\n");
        exit(EXIT_FAILURE);
    }
    if (!is_type) 
    {
      fprintf(stderr, "%s is not a valid %s\n", path,
        (type == S_IFDIR) ? "directory" :
        (type == S_IFREG) ? "regular file" :
        (type == S_IFLNK) ? "symbolic link" : "type");
      exit(EXIT_FAILURE);
    }
}

//removes the \n character if it exists
//retunrs the length of the string
int read_line(char *text, int size) 
{
  if (fgets(text, size, stdin) == NULL) 
  {
    text[0] = '\0';
    return 0;
  }
  int len = strlen(text);
  if (len > 0 && text[len - 1] == '\n') 
  {
    text[len - 1] = '\0';
    len--;
  }
  return len;
}

//creates a new treasure and validates the input
TREASURE* newTreasure(char *hunt)
{
  TREASURE* new = (TREASURE*)malloc(sizeof(TREASURE));
  if(new==NULL)
  {
    perror("Error allocating space for a new treasure");
    exit(EXIT_FAILURE);
  }
  memset(new, 0, sizeof(TREASURE));
  char buff_in[64];
  
  printf("Treasure ID: ");
  read_line(buff_in, sizeof(buff_in));
  char *endptr;
  long val = strtol(buff_in, &endptr, 10);
  if (*endptr != '\0') 
  {
    fprintf(stderr, "Invalid treasure ID\n");
    free(new);
    exit(EXIT_FAILURE);
  }

  //unique ID check
  TREASURE t={0};
  char file[128];
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  int f=safe_open(file, O_RDONLY, 0777);
  int b=0;
  while((b=read(f, &t, sizeof(TREASURE)))>0)
  {
    if(b==-1)
    {
      perror("Read error");
      exit(EXIT_FAILURE);
    }
    if(b==0) break;
    if(b!=sizeof(TREASURE))
    {
      fprintf(stderr, "Partial read\n");
      exit(EXIT_FAILURE);
    }
    if(t.id==val)
    {
      fprintf(stderr, "This treasure ID is already in the file\n");
      free(new);
      safe_close(f);
      exit(EXIT_FAILURE);
    }
  }
  safe_close(f);
  new->id=val;
  memset(buff_in, 0, sizeof(buff_in));

  printf("Username: ");
  read_line(buff_in, sizeof(buff_in));
  if(strlen(buff_in)==0)
  {
    fprintf(stderr, "Username is empty\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  strcpy(new->user, buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  printf("Latitude: ");
  read_line(buff_in, sizeof(buff_in));
  float valf = strtof(buff_in, &endptr);
  if (*endptr != '\0') 
  {
    fprintf(stderr, "Invalid latitude\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  new->gps.latitude=valf;
  if (new->gps.latitude < -90.0 || new->gps.latitude > 90.0) 
  {
    fprintf(stderr, "Latitude must be between -90 and 90\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  memset(buff_in, 0, sizeof(buff_in));

  printf("Longitude: ");
  read_line(buff_in, sizeof(buff_in));
  valf = strtof(buff_in, &endptr);
  if (*endptr != '\0') 
  {
    fprintf(stderr, "Invalid longitude\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  new->gps.longitude=valf;
  if (new->gps.longitude < -180.0 || new->gps.longitude > 180.0) 
  {
    fprintf(stderr, "Longitude must be between -180 and 180\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  memset(buff_in, 0, sizeof(buff_in));

  printf("Clue: ");
  read_line(buff_in, sizeof(buff_in));
  if(strlen(buff_in)==0)
  {
    fprintf(stderr, "Clue is empty\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  strcpy(new->clue, buff_in);
  memset(buff_in, 0, sizeof(buff_in));

  printf("Value: ");
  read_line(buff_in, sizeof(buff_in));
  val = strtol(buff_in, &endptr, 10);
  if (*endptr != '\0') {
    fprintf(stderr, "Invalid value\n");
    free(new);
    exit(EXIT_FAILURE);
  }
  new->value=val;
  memset(buff_in, 0, sizeof(buff_in));

  return new;
}

//adds a new treasure to the hunt
//creates the directory if it does not exist
//logs the operation in logged_hunt
//creates the symbolic link if it does not exist
void add(char *hunt)
{
  char dir[128], file[128], log[128], link[128];
  snprintf(dir, sizeof(dir), "%s", hunt);
  struct stat st;
  if(lstat(dir, &st)==0 && S_ISDIR(st.st_mode))
  {
    printf("Directory already exists\n");
  }
  else
  {
    if(mkdir(dir, 0777)==-1)
    {
      perror("Couldn't create a new directory");
      exit(EXIT_FAILURE);
    }
  }
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
  snprintf(link, sizeof(link), "logged_hunt-%s", hunt);
  int f=safe_open(file, O_WRONLY | O_CREAT | O_APPEND, 0777);
  TREASURE *new = newTreasure(hunt);
  if(write(f, new, sizeof(*new))==-1)
  {
    perror("Error at writing a new feature");
    free(new);
    safe_close(f);
    exit(EXIT_FAILURE);
  }
  safe_close(f);
  int lo=safe_open(log, O_WRONLY | O_CREAT | O_APPEND, 0777);
  char info[256];
  snprintf(info, sizeof(info), "--add: ID:%d User:%s Latitude:%f Longitude:%f Clue:%s Value:%d\n", new->id, new->user, new->gps.latitude, new->gps.longitude, new->clue, new->value);
  if(write(lo, info, strlen(info))==-1)
  {
    perror("Error writing to log file");
    free(new);
    safe_close(f);
    exit(EXIT_FAILURE);
  }
  safe_close(lo);
  free(new);
  if(lstat(link, &st)==-1)
  {
    if(symlink(log, link)==-1)
	  {
	    perror("Couldn't create a symlink");
	    exit(EXIT_FAILURE);
	  }
  }
  else
  {
    if (!S_ISLNK(st.st_mode)) 
    {
      if (unlink(link) == -1) 
      {
        perror("Couldn't remove non-symlink file");
        exit(EXIT_FAILURE);
      }
      if (symlink(log, link) == -1) 
      {
        perror("Couldn't create a symlink");
        exit(EXIT_FAILURE);
      }
      printf("Non-symlink file replaced with symbolic link\n");
    } 
    else 
    {
        printf("Symbolic link already exists\n");
    }
  }
  printf("Treasure added\n");
}

//lists all the treasures from the hunt, the total size of the hunt and the last modification
//logs the operation in logged_hunt
void list(char *hunt)
{
  char file[128], log[128];
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
  struct stat st,st1, st2;
  ensure_type(hunt, S_IFDIR);
  printf("Name: %s\n", hunt);
  ensure_type(file, S_IFREG);
  ensure_type(log, S_IFREG);
  lstat(hunt, &st);
  lstat(file, &st1);
  lstat(log, &st2);
  printf("Total size: %ld\n", st1.st_size+st2.st_size);
  printf("Last modification: %s", ctime(&st.st_mtime));
  TREASURE t={0};
  int f=safe_open(file, O_RDONLY, 0777);
  int b;
  while((b=read(f, &t, sizeof(TREASURE)))>0)
  {
    if(b==-1)
    {
      perror("Read error");
      exit(EXIT_FAILURE);
    }
    if(b==0) break;
    if(b!=sizeof(TREASURE))
    {
      fprintf(stderr, "Partial read\n");
      exit(EXIT_FAILURE);
    }
    printf("ID: %d - User: %s - Latitude: %f - Longitude: %f - Clue: %s - Value: %d\n", t.id, t.user, t.gps.latitude, t.gps.longitude, t.clue, t.value);
  }
  safe_close(f);
  char aux[128];
  snprintf(aux, sizeof(aux), "--list: listed all the treasures from %s\n", hunt);
  int lo=safe_open(log, O_WRONLY | O_CREAT | O_APPEND, 0777);
  if(write(lo, aux, strlen(aux))==-1)
  {
    perror("Error writing to log file");
    safe_close(f);
    exit(EXIT_FAILURE);
  }
  printf("%s listed\n", hunt);
  safe_close(lo);
}

//view the treasure with the specified id from the hunt
//logs the operation in logged_hunt
void view(char *hunt, char *id)
{
  char *endptr;
  long ID = strtol(id, &endptr, 10);
  if (*endptr != '\0') 
  {
    fprintf(stderr, "Not a valid id");
    exit(EXIT_FAILURE);
  } 
  char file[128];
  ensure_type(hunt, S_IFDIR);
  int found_id=0;
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  int f=safe_open(file, O_RDONLY, 0777);
  TREASURE t={0};
  int b=0;
  while((b=read(f, &t, sizeof(TREASURE)))>0 && !found_id)
  {
    if(b==-1)
    {
      perror("Read error");
      exit(EXIT_FAILURE);
    }
    if(b==0) break;
    if(b!=sizeof(TREASURE))
    {
      fprintf(stderr, "Partial read\n");
      exit(EXIT_FAILURE);
    }
    if(t.id==ID)
    {
      found_id=1;
      printf("ID: %d - User: %s - Latitude: %f - Longitude: %f - Clue: %s - Value: %d\n", t.id, t.user, t.gps.latitude, t.gps.longitude, t.clue, t.value);
    }
  }
  safe_close(f);
  if(found_id==0)
  {
    printf("The treasure with this id was not found\n");
    exit(EXIT_FAILURE);
  }
  char log[128], aux[128];
  snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
  snprintf(aux, sizeof(aux), "--view: viewed the treasure with the %ld id\n", ID);
  int lo=safe_open(log, O_WRONLY | O_CREAT | O_APPEND, 0777);
  if(write(lo, aux, strlen(aux))==-1)
  {
    perror("Error writing to log file");
    safe_close(f);
    exit(EXIT_FAILURE);
  }
  printf("Viewed the treasure with the id=%ld in the %s\n", ID, hunt);
  safe_close(lo);
}

//removes the treasure with the specified id from the hunt(rewrites the treasures.dat file)
//logs the operation in logged_hunt
void remove_treasure(char *hunt, char *id)
{
  char *endptr;
  long ID = strtol(id, &endptr, 10);
  if (*endptr != '\0') 
  {
    fprintf(stderr, "Not a valid id");
    exit(EXIT_FAILURE);
  }
  char file[128], aux[128];
  ensure_type(hunt, S_IFDIR);
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  snprintf(aux, sizeof(aux), "%s/aux.dat", hunt);
  int f=safe_open(file, O_RDONLY, 0777);
  int a=safe_open(aux, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  TREASURE t={0};
  int found_id=0, b=0;
  while((b=read(f, &t, sizeof(TREASURE)))>0)
  {
    if(b==-1)
    {
      perror("Read error");
      exit(EXIT_FAILURE);
    }
    if(b==0) break;
    if(b!=sizeof(TREASURE))
    {
      fprintf(stderr, "Partial read\n");
      exit(EXIT_FAILURE);
    }
    if(t.id==ID)
    {
      found_id=1;
      continue;
    }
    if(write(a, &t, sizeof(TREASURE))==-1)
    {
      perror("Error writing to log file");
      safe_close(f);
      exit(EXIT_FAILURE);
    }
  }
  safe_close(f);
  safe_close(a);
  if(found_id)
  {
    if(unlink(file))
    {
      perror("Couldn't remove the old file");
      exit(EXIT_FAILURE);
    }
    if(rename(aux, file))
    {
      perror("Couldn't rename the auxiliary file");
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    if(unlink(aux))
    {
      perror("Couldn't remove the auxiliary file");
      exit(EXIT_FAILURE);
    }
    printf("The treasure with this id was not found\n");
    return;
  }
  char log[128], info[128];
  snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
  snprintf(info, sizeof(info), "--remove_treasure: removed the treasure with the %ld id from the %s hunt\n", ID, hunt);
  int lo=safe_open(log, O_WRONLY | O_CREAT | O_APPEND, 0777);
  if(write(lo, info, strlen(info))==-1)
  {
    perror("Error writing to log file");
    safe_close(f);
    exit(EXIT_FAILURE);
  }
  printf("Removed the treasure with the id=%ld from the %s\n", ID, hunt);
  safe_close(lo);
}

//remove the hunt and all the files
void remove_hunt(char *hunt)
{
  char file[128], log[128], link[128];
  snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
  snprintf(log, sizeof(log), "%s/logged_hunt", hunt);
  snprintf(link, sizeof(link), "logged_hunt-%s", hunt);
  ensure_type(hunt, S_IFDIR);
  ensure_type(file, S_IFREG);
  if(unlink(file))
  {
    perror("Error at deleting the file");
    exit(EXIT_FAILURE);
  }
  ensure_type(link, S_IFLNK);
  if(unlink(link))
  {
    perror("Error at deleting the link");
    exit(EXIT_FAILURE);
  }
  ensure_type(log, S_IFREG);
  if(unlink(log))
  {
    perror("Error at deleting the log file");
    exit(EXIT_FAILURE);
  }
  if(rmdir(hunt))
  {
    perror("Error at deleting the directory");
    exit(EXIT_FAILURE);
  }
  printf("%s removed\n", hunt);
}

int main(int argc, char *argv[])
{
  if(argc<2)
  {
    fprintf(stderr, "Not enough arguments\n");
    exit(EXIT_FAILURE);
  }
  if(strcmp(argv[1], "--add")==0)
  {
    if(argc<3)
    {
      fprintf(stderr, "You need to introduce a hunt id\n");
      exit(EXIT_FAILURE);
    }
    else 
    {
      if(argc==3)
        add(argv[2]);
      else
      {
        fprintf(stderr, "Too many arguments\n");
        exit(EXIT_FAILURE);
      }
    }
  }
  else
    {
      if(strcmp(argv[1], "--list")==0)
	    {
	      if(argc<3)
        {
	        fprintf(stderr, "You need to introduce a hunt id\n");
	        exit(EXIT_FAILURE);
	      }
        else
        {
          if(argc==3)
	            list(argv[2]);
          else
          {
            fprintf(stderr, "Too many arguments\n");
            exit(EXIT_FAILURE);
          }
        }
	    }
      else
      {
        if(strcmp(argv[1], "--view")==0)
        {
          if(argc<4)
          {
            fprintf(stderr, "You need to introduce a hunt id AND an id\n");
            exit(EXIT_FAILURE);
          }
          else
          {
            if(argc==4)
              view(argv[2], argv[3]);
            else
            {
              fprintf(stderr, "Too many arguments\n");
              exit(EXIT_FAILURE);
            }
          }
        }
        else
        {
          if(strcmp(argv[1], "--remove_treasure")==0)
          {
            if(argc<4)
            {
              fprintf(stderr, "You need to introduce a hunt id AND an id\n");
              exit(EXIT_FAILURE);
            }
            else
            {
              if(argc==4)
                remove_treasure(argv[2], argv[3]);
              else
              {
                fprintf(stderr, "Too many arguments\n");
                exit(EXIT_FAILURE);
              }
            }
          }
          else
          {
            if(strcmp(argv[1], "--remove_hunt")==0)
            {
              if(argc<3)
              {
                fprintf(stderr, "You need to introduce a hunt id\n");
                exit(EXIT_FAILURE);
              }
              else 
              {
                if(argc==3)
                remove_hunt(argv[2]);
                else
                {
                  fprintf(stderr, "Too many arguments\n");
                  exit(EXIT_FAILURE);
                }
              }
            }
            else
            {
              fprintf(stderr, "Invalid argument\n");
            }
          }
        }
      }
    }
  return 0;
}
