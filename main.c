#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <microhttpd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include "datacontroller.h"
#include "config.h"
#include "memalloc.h"

static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define INSTRUCTION    "This is jsmpegserver, power by https://www.worldflying.cn, if you have some question, you can send a email to jevian_ma@worldflying.cn"
#define TOPICCONFLICT  "{\"errcode\":-1, \"errmsg\":\"topic exist\"}"
#define TRANSFERFINISH "{\"errcode\":0, \"errmsg\":\transfer finish\"}"

int connectionHandler(void *cls,
            struct MHD_Connection *connection,
            const char *url,
            const char *method,
            const char *version,
            const char *upload_data,
            size_t *upload_data_size,
            void ** ptr) {
    struct MHD_Response *response;
    if (!strcmp(method, "POST")) {
        if (*ptr == NULL) {
            pthread_rwlock_rdlock(&rwlock);
            struct TOPICLIST* topiclist = gettopiclist (url);
            if (topiclist == NULL) {
                pthread_mutex_lock(&mutex);
                topiclist = addtopictolist (url);
                pthread_mutex_unlock(&mutex);
                if (*upload_data_size != 0) {
                    addtsdatatobuff(topiclist, upload_data, *upload_data_size);
                    pthread_rwlock_unlock(&rwlock);
                } else {
                    pthread_rwlock_unlock(&rwlock);
                }
                *ptr = topiclist;
                return MHD_YES;
            } else {
                pthread_rwlock_unlock(&rwlock);
                response = MHD_create_response_from_buffer(sizeof(TOPICCONFLICT)-1, TOPICCONFLICT, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Content-Type", "text/plain");
            }
        } else {
            struct TOPICLIST* topiclist = *ptr;
            if (*upload_data_size != 0) {
                pthread_rwlock_rdlock(&rwlock);
                addtsdatatobuff(topiclist, upload_data, *upload_data_size);
                pthread_rwlock_unlock(&rwlock);
                *upload_data_size = 0;
                return MHD_YES;
            }
            response = MHD_create_response_from_buffer(sizeof(TRANSFERFINISH)-1, TRANSFERFINISH, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Content-Type", "text/plain");
            MHD_add_response_header(response, "Access-Control-Allow-Headers", "*");
        }
    } else {
        size_t size = strlen(url);
        if (!strcmp(url + size-3, ".ts")) {
            char *topic = (char*)memalloc(size-3, __FILE__, __LINE__); // "x.ts"共4个字符,不减4只减3是为了\0
            memcpy(topic, url, size-4);
            topic[size-4] = '\0';
            size_t id;
            if (url[size-4] >= 'a') {
                id = url[size-4] - 'a' + 10;
            } else {
                id = url[size-4] - '0';
            }
            size_t len;
            pthread_rwlock_rdlock(&rwlock);
            char* html = gettsfile (topic, id, &len);
            pthread_rwlock_unlock(&rwlock);
            memfree (topic);
            response = MHD_create_response_from_buffer(len, html, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Content-Type", "video/mp2t");
        } else if (!strcmp(url + size-5, ".m3u8")) {
            char* topic = (char*)memalloc(size-4, __FILE__, __LINE__); // ".m3u8"共5个字符,不减5只减4是为了\0
            memcpy(topic, url, size-5);
            topic[size-5] = '\0';
            size_t len;
            pthread_rwlock_rdlock(&rwlock);
            char* html = getm3u8file (topic, &len);
            pthread_rwlock_unlock(&rwlock);
            memfree (topic);
            response = MHD_create_response_from_buffer(len, html, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Content-Type", "application/vnd.apple.mpegurl");
        } else {
            response = MHD_create_response_from_buffer(sizeof(INSTRUCTION)-1, INSTRUCTION, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Content-Type", "text/plain");
        }
    }
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

void request_completed (void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe) {
    struct TOPICLIST* topiclist = *con_cls;
    if (topiclist != NULL) {
        topiclist->willdelete = 1;
    }
}

void signalarmhandle () {
    pthread_rwlock_wrlock(&rwlock);
    createalltsfile ();
    pthread_rwlock_unlock(&rwlock);
}

int main (int argc, char *argv[]) {
    int isdeamon = 1;
    if (argc > 1) {
        if (strcmp(argv[1], "--version") == 0) {
            printf("%s %s\n", __DATE__, __TIME__);
            return 0;
        } else if (strcmp(argv[1], "--run") == 0) {
            isdeamon = 0;
        } else {
            printf("unknown paramater, in %s, at %d\n", __FILE__, __LINE__);
            return -1;
        }
    }
    if (isdeamon) {
        int pid = fork();
        if (pid < 0) {
            printf("create hide thread fail\n");
            return -1;
        } else if(pid > 0) {
            return 0;
        } else {
            setsid();
        }
    }
    struct CONFIG* config = initconfig ();
    signal(SIGALRM, signalarmhandle);
    struct itimerval itv;
    itv.it_value.tv_sec = itv.it_interval.tv_sec = config->tstimelong_sec;
    itv.it_value.tv_usec = itv.it_interval.tv_usec = config->tstimelong_usec;
    setitimer(ITIMER_REAL, &itv, NULL);
    int cpunum = get_nprocs();
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY, config->port, NULL, NULL, &connectionHandler, NULL, MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL, MHD_OPTION_THREAD_POOL_SIZE, cpunum, MHD_OPTION_END);
    if (daemon == NULL) {
        printf("run http server fail, in %s, at %d\n", __FILE__, __LINE__);
        return -1;
    }
    if (access("fifo", F_OK) != 0) {
        mkfifo("fifo", 0777);
    }
    char* fifobuf = (char*)memalloc(16, __FILE__, __LINE__);
    while (1) {
        int fd = open("fifo", O_RDONLY);
        read (fd, fifobuf, 16);
        if (!strncmp (fifobuf, "exit\n", 5)) {
            close (fd);
            break;
        }
        close (fd);
    }
    memfree (fifobuf);
    MHD_stop_daemon(daemon);
    return 0;
}