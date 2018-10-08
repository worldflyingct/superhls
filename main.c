#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <microhttpd.h>
#include "datacontroller.h"

#define INSTRUCTION    "This is jsmpegserver, power by https://www.worldflying.cn, if you have some question, you can send a email to jevian_ma@worldflying.cn"
#define TOPICCONFLICT  "{\"errcode\":-1, \"errmsg\":\"topic exist\"}"
#define TRANSFERFINISH "{\"errcode\":0, \"errmsg\":\transfer finish\"}"

struct STRING {
    char *string;
    size_t len;
};

int kv_cb (void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
    if (!strcmp (key, MHD_HTTP_HEADER_HOST) && kind == MHD_HEADER_KIND) {
        struct STRING* string = (struct STRING*)malloc(sizeof(struct STRING));
        string->len = strlen(value) + 1;
        string->string = (char*)malloc(string->len);
        strcpy(string->string, value);
        *((struct STRING**)cls) = string;
        return MHD_YES;
    }
    return MHD_NO;  
}

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
#ifdef DEBUG
                printf("topic exist, in %s, at %d\n", __FILE__, __LINE__);
#endif
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
#ifdef DEBUG
        printf("url:%s, in %s, at %d\n", url, __FILE__, __LINE__);
#endif
        size_t size = strlen(url);
        if (!strcmp(url + size-3, ".ts")) {
            char *topic = (char*)malloc(size-4); // "-x.ts"共5个字符,不减5只减4是为了\0
            memcpy(topic, url, size-5);
            topic[size-5] = '\0';
            size_t id;
            if('a' <= url[size-4] && url[size-4] <= 'f') {
                id = url[size-4] - 'a' + 10;
            } else {
                id = url[size-4] - '0';
            }
            size_t len;
#ifdef DEBUG
            printf("topic:%s,id:%02x, in %s, at %d\n", topic, id, __FILE__, __LINE__);
#endif
            char *html = gettsfile (topic, id, &len);
            free (topic);
#ifdef DEBUG
            printf("len:%d, in %s, at %d\n", len, __FILE__, __LINE__);
#endif
            response = MHD_create_response_from_buffer(len, html, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Content-Type", "video/mp2t");
        } else if (!strcmp(url + size-5, ".m3u8")) {
            struct STRING* httphost;
            MHD_get_connection_values (connection, MHD_HEADER_KIND, &kv_cb, &httphost);
            char *topic = (char*)malloc(size-4); // 不减5只减4是为了\0
            memcpy(topic, url, size-5);
            topic[size-5] = '\0';
            size_t len;
            char *html = createm3u8file (topic, httphost->string, httphost->len, &len);
            free (topic);
            free (httphost->string);
            free (httphost);
            response = MHD_create_response_from_buffer(len, html, MHD_RESPMEM_MUST_FREE);
            MHD_add_response_header(response, "Content-Type", "application/vnd.apple.mpegurl");
        } else {
#ifdef DEBUG
            printf("in %s, at %d\n", __FILE__, __LINE__);
#endif
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
    signal(SIGALRM, createtsfile);
    struct itimerval itv;
    itv.it_value.tv_sec = itv.it_interval.tv_sec = 1;
    itv.it_value.tv_usec = itv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY, 8001, NULL, NULL, &connectionHandler, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        printf("run http server fail, in %s, at %d\n", __FILE__, __LINE__);
        return -1;
    }
    getchar();
    MHD_stop_daemon(daemon);
    return 0;
}