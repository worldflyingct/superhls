#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "datacontroller.h"

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
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        }
    } else {
        size_t size = strlen(url);
        if (!strcmp(url + size-3, ".ts")) {
            char *topic = (char*)malloc(size-7); // "-xxxx.ts"共8个字符,不减8只减7是为了\0
            strncpy(topic, url, size-8);
            int id = 0;
            if('a' <= url[size-4] && url[size-4] <= 'f') {
                id += url[size-4] - 'a' + 10;
            } else {
                id += url[size-4] - '0';
            }
            if('a' <= url[size-5] && url[size-5] <= 'f') {
                id += (url[size-5] - 'a' + 10) << 4;
            } else {
                id += (url[size-5] - '0') << 4;
            }
            if('a' <= url[size-6] && url[size-6] <= 'f') {
                id += (url[size-6] - 'a' + 10) << 8;
            } else {
                id += (url[size-6] - '0') << 8;
            }
            if('a' <= url[size-7] && url[size-7] <= 'f') {
                id += (url[size-7] - 'a' + 10) << 12;
            } else {
                id += (url[size-7] - '0') << 12;
            }
            size_t len;
            printf("topic:%s,id:%04x, in %s, at %d\n", topic, id, __FILE__, __LINE__);
            char *html = gettsfile (topic, id, &len);
            free (topic);
            printf("len:%d, in %s, at %d\n", len, __FILE__, __LINE__);
            response = MHD_create_response_from_buffer(len, html, MHD_RESPMEM_PERSISTENT);
            // MHD_add_response_header(response, "Content-Type", "application/vnd.apple.mpegurl");
        } else if (!strcmp(url + size-5, ".m3u8")) {
            char *topic = (char*)malloc(size-4); // 不减5只减4是为了\0
            strncpy(topic, url, size-5);
            size_t len;
            char *html = createm3u8file (topic, &len);
            free (topic);
            response = MHD_create_response_from_buffer(len, html, MHD_RESPMEM_MUST_FREE);
            MHD_add_response_header(response, "Content-Type", "application/vnd.apple.mpegurl");
        } else {
            response = MHD_create_response_from_buffer(sizeof(INSTRUCTION)-1, INSTRUCTION, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Content-Type", "text/plain");
        }
    }
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

int main(int argc, char *argv[]) {
    struct MHD_Daemon *d = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY, 8001, NULL, NULL, &connectionHandler, NULL, MHD_OPTION_END);
    if (d == NULL) {
        return -1;
    }
    getchar();
    return 0;
}