#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

typedef struct {
    float latitude, longitude;
} GPS;

typedef struct {
    int id;
    char user[32];
    GPS gps;
    char clue[64];
    int value;
} TREASURE;

typedef struct {
    char user[32];
    int score;
} USER_SCORE;

//calculates the score for a hunt
void calculate_scores(char *hunt) 
{
    struct stat st;
    if(hunt==NULL)
    {
        fprintf(stderr, "Invalid hunt\n");
        exit(EXIT_FAILURE);
    }
    if (lstat(hunt, &st)==-1) 
    {
        perror(hunt);
        exit(EXIT_FAILURE);
    } 
    else 
    {
        if (S_ISDIR(st.st_mode) == 0) 
        {
            fprintf(stderr, "Not a directory\n");
            exit(EXIT_FAILURE);
        }
    }
    char file[128];
    snprintf(file, sizeof(file), "%s/treasures.dat", hunt);
    int f = open(file, O_RDONLY, 0777);
    if (f == -1) 
    {
        perror("Open file");
        exit(EXIT_FAILURE);
    }

    USER_SCORE *users = NULL;
    int count = 0, size = 0, b=0;
    TREASURE t;
    while ((b=read(f, &t, sizeof(TREASURE))) >0) 
    {
        if(b!=sizeof(TREASURE))
        {
            fprintf(stderr, "Partial read\n");
            exit(EXIT_FAILURE);
        }
        int found = 0;
        for (int i = 0; i < count; i++) 
        {
            if (strcmp(users[i].user, t.user) == 0) 
            {
                users[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (found == 0) 
        {
            if (count == size) 
            {
                if (size == 0) size = 8;
                else size *= 2;
                USER_SCORE *temp = realloc(users, size * sizeof(USER_SCORE));
                if (temp == NULL) 
                {
                    perror("Error at users realloc");
                    free(users); 
                    if (close(f) == -1) 
                    {
                        perror("Close file");
                    }
                    exit(EXIT_FAILURE);
                }
                users = temp;

            }
            strcpy(users[count].user, t.user);
            users[count].score = t.value;
            count++;
        }
    }
    if(b==-1)
    {
        perror("Read error");
        free(users);
        if (close(f) == -1) 
        {
            perror("Close file");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    if (close(f) == -1) 
    {
        perror("Close file");
        exit(EXIT_FAILURE);
    }

    printf("Name: %s\n", hunt);
    for (int i = 0; i < count; i++) {
        printf("User %s with score=%d\n", users[i].user, users[i].score);
    }
    if (count == 0) {
        printf("No treasures found.\n");
    }
    free(users);
}

int main(void)
{
    DIR *dir=opendir(".");
    if(dir==NULL)
    {
        perror("Open directory");
        exit(EXIT_FAILURE);
    }
    struct dirent *d;
    while((d=readdir(dir)))
    {
        if(strcmp(d->d_name, ".")==0 || strcmp(d->d_name, "..")==0 || strcmp(d->d_name, ".git")==0)
            continue;
        if(d->d_type == DT_DIR)
        {
            calculate_scores(d->d_name);
        }
    }
    if(closedir(dir)==-1)
    {
        perror("Close directory");
        exit(EXIT_FAILURE);
    }
    return 0;
}