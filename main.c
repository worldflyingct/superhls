#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <microhttpd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "datacontroller.h"
#include "config.h"
#include "memalloc.h"

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
            struct TOPICLIST* topiclist = gettopiclist (url);
            if (topiclist == NULL) {
                topiclist = addtopictolist (url);
                *ptr = topiclist;
                return MHD_YES;
            } else {
                response = MHD_create_response_from_buffer(sizeof(TOPICCONFLICT)-1, TOPICCONFLICT, MHD_RESPMEM_PERSISTENT);
                MHD_add_response_header(response, "Content-Type", "text/plain");
            }
        } else {
            struct TOPICLIST* topiclist = *ptr;
            if (*upload_data_size != 0) {
                addtsdatatobuff (topiclist, upload_data, *upload_data_size);
                *upload_data_size = 0;
                return MHD_YES;
            }
            removetopicfromlist (topiclist);
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
            if (url[size-4] > 'a') {
                id = url[size-4] - 'a' + 10;
            } else {
                id = url[size-4] - '0';
            }
            size_t len;
            char *html = gettsfile (topic, id, &len);
            memfree (topic);
            response = MHD_create_response_from_buffer(len, html, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Content-Type", "video/mp2t");
        } else if (!strcmp(url + size-5, ".m3u8")) {
            char *topic = (char*)memalloc(size-4, __FILE__, __LINE__); // ".m3u8"共4个字符,不减5只减4是为了\0
            memcpy(topic, url, size-5);
            topic[size-5] = '\0';
            size_t len;
            char *html = getm3u8file (topic, &len);
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

int main(int argc, char *argv[]) {
    if (argc != 2 || strcmp(argv[1], "--run") != 0) {
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
    uint16_t port = 0;
    for (size_t pos = config->httphostlen-1 ; pos != 0 ; pos--) {
        if (config->httphost[pos] == ':') {
            pos++;
            while (config->httphost[pos] != '\0') {
                port *= 10;
                port += config->httphost[pos] - '0';
                pos++;
            }
            break;
        }
    }
    signal(SIGALRM, createalltsfile);
    struct itimerval itv;
    itv.it_value.tv_sec = itv.it_interval.tv_sec = 1;
    itv.it_value.tv_usec = itv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY, port, NULL, NULL, &connectionHandler, NULL, MHD_OPTION_END);
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