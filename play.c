#include <ao/ao.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <mpg123.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#define BITS 8

struct list {
    char *name;
    struct list *prev, *next;
};

struct list* head = NULL;

static const char *dirpath = "/home/elknhns/Downloads/";
pthread_t tid[1];
pthread_mutex_t lock;
int count;

void *play_song(void *argv) {
    if(head != NULL) {
        struct list *temp;
        mpg123_handle *mh;
        unsigned char *buffer;
        char fpath[1000];
        size_t buffer_size, done;
        int err, i;

        int driver;
        ao_device *dev;

        ao_sample_format format;
        int channels, encoding;
        long rate;

        /* initializations */
        temp = head;
        sprintf(fpath,"%s%s",dirpath,temp->name);
        ao_initialize();
        driver = ao_default_driver_id();
        mpg123_init();
        mh = mpg123_new(NULL, &err);
        buffer_size = mpg123_outblock(mh);
        buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

        /* open the file and get the decoding format */
        mpg123_open(mh, fpath);
        mpg123_getformat(mh, &rate, &channels, &encoding);

        /* set the output format and open the output device */
        format.bits = mpg123_encsize(encoding) * BITS;
        format.rate = rate;
        format.channels = channels;
        format.byte_format = AO_FMT_NATIVE;
        format.matrix = 0;
        dev = ao_open_live(driver, &format, NULL);

        /* decode and play */
        while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK) {
            pthread_mutex_lock(&lock);
            pthread_mutex_unlock(&lock);
            ao_play(dev, buffer, done);
        }

        /* clean up */
        free(buffer);
        ao_close(dev);
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        ao_shutdown();
    }
}

void add(char *filename) {
    struct list* temp = head;
    struct list* newFile = (struct list*)malloc(sizeof(struct list));

    newFile->name = filename;
    newFile->prev = NULL;
    newFile->next = NULL;

    if(head == NULL) head = newFile;
    else {
        while(temp->next != NULL) temp = temp->next;
        temp->next = newFile;
        newFile->prev = temp;
    }
}

int mygetch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int main(int argc, char *argv[]) {
    int error, play = 1;
    char input, filename[1000], cmd[1000], debug[1000];
    FILE *in;

    count = argc;

    if(argc < 2) exit(0);

    sprintf(cmd, "ls %s > list.txt", dirpath);
    if(system(cmd)) printf("Command processor exists\n");
    else printf("Command processor doesn't exist\n");

    in = fopen("list.txt", "r");
    while(fscanf(in, "%[^\n]s", filename) != EOF && strrchr(filename, '.') == ".mp3") {
        add(filename);
    }

    if(pthread_mutex_init(&lock, NULL) != 0) { 
        printf("\nMutex init has failed\n"); 
        return 1; 
    }

    error = pthread_create(&(tid[0]), NULL, play_song, (void*)argv);
    if (error != 0) printf("\nThread can't be created :[%s]", strerror(error));

    pthread_join(tid[0], NULL);

    while(mygetch() != 's') {
        input = mygetch();
        if(input == ' ') {
            if(play == 1) {
                pthread_mutex_lock(&lock);
                play = 0;
            }
            else {
                pthread_mutex_unlock(&lock);
                play = 1;
            }
        }
    }

    pthread_mutex_destroy(&lock);

    return 0;
}