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

typedef struct{
    char user[32];
    int score;
}USER_SCORE;

int main(int argc, char *argv[])
{
    if(argc!=2)
    {
        perror("Not the right number of arguments\n");
        exit(EXIT_FAILURE);
    }
    struct stat st;
    if(lstat(argv[1], &st))
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
    char file[128];
    snprintf(file, sizeof(file), "%s/treasures.dat", argv[1]);
    int f=open(file, O_RDONLY, 0777);
    if(f==-1)
    {
        perror("Couldn't open the file\n");
        exit(EXIT_FAILURE);
    }
    USER_SCORE *users=NULL;
    int count=0, size=0;
    TREASURE t;
    while(read(f, &t, sizeof(TREASURE))==sizeof(TREASURE))
    {
        int found=0;
        for(int i=0; i<count; i++)
        {
            if(strcmp(users[i].user, t.user)==0)
            {
                users[i].score+=t.value;
                found=1;
                break;
            }
        }
        if(found==0)
        {
            if(count==size)
            {
                if(size==0) size=8;
                else size*=2;
                users=realloc(users, size*sizeof(USER_SCORE));
                if(users==NULL)
                {
                    perror("Error at users realloc\n");
                    exit(EXIT_FAILURE);
                    if(close(f)==-1)
                    {
                        perror("Error closing the file\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            strcpy(users[count].user, t.user);
            users[count].score=t.value;
            count++;  
        }
    }
    if(close(f)==-1)
    {
      perror("Error closing the file\n");
      exit(EXIT_FAILURE);
    }
    for(int i=0; i<count; i++)
        printf("User %s with score=%d\n", users[i].user, users[i].score);
    free(users);
    return 0;
}
