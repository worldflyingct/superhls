#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// #define STOREMALLOCNUM

#ifdef STOREMALLOCNUM
static int mallocnum = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void* memalloc (size_t num, char* filename, int line) {
    void* p = (void*) malloc (num);
    if (p == NULL) {
        printf ("mem alloc fail, at %s, in %d\n", filename, line);
        exit(-1);
        return NULL;
    }
#ifdef STOREMALLOCNUM
    pthread_mutex_lock(&mutex);
    mallocnum++;
    pthread_mutex_unlock(&mutex);
#endif
    return p;
}

void memfree (void* p) {
#ifdef STOREMALLOCNUM
    pthread_mutex_lock(&mutex);
    mallocnum--;
    pthread_mutex_unlock(&mutex);
#endif
    free (p);
}

void showmallocnum (char* filename, int line) {
#ifdef STOREMALLOCNUM
    printf ("mallocnum:%d, at %s, in %d\n", mallocnum, filename, line);
#else
    printf ("you need build this again and open STOREMALLOCNUM switch and then show malloc num, at %s, in %d\n", filename, line);
#endif
    fflush (stdout);
}

