#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "datacontroller.h"

#ifdef SAVEFILE
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

struct TOPICLIST *topiclisthead = NULL;
struct TOPICLIST *topiclisttail = NULL;

struct TOPICLIST *gettopiclist (const char* topic) {
    struct TOPICLIST *topiclist = topiclisthead;
    while (topiclist != NULL) {
        if (!strcmp(topiclist->topic, topic)) {
            return topiclist;
        }
        topiclist = topiclist->tail;
    }
    return NULL;
}

struct TOPICLIST *addtopictolist (const char* topic) {
    struct TOPICLIST *topiclist = (struct TOPICLIST*)malloc(sizeof(struct TOPICLIST));
    size_t len = strlen(topic) + 1;
    topiclist->topic = (char*)malloc(len);
    strcpy (topiclist->topic, topic);
    topiclist->topicsize = len;
    topiclist->buffusesize = 0;
    topiclist->tsdatalisthead = NULL;
    topiclist->tsdatalistdesc = NULL;
    topiclist->tsdatalisttail = NULL;
    topiclist->tsdatanum = 0;
    topiclist->tsdatastep = 0;
    topiclist->tail = NULL;
    if (topiclisthead == NULL) {
        topiclist->head = NULL;
        topiclisthead = topiclist;
        topiclisttail = topiclisthead;
    } else {
        topiclist->head = topiclisttail;
        topiclisttail->tail = topiclist;
        topiclisttail = topiclisttail->tail;
    }
    return topiclist;
}

void removetopicfromlist (struct TOPICLIST *topiclist) {
    if(topiclist->head == NULL) {
        topiclisthead = topiclist->tail;
    } else {
        topiclist->head->tail = topiclist->tail;
    }
    if (topiclist->tail == NULL) {
        topiclisttail = topiclist->head;
    }
    struct TSDATALIST *tsdatalist = topiclist->tsdatalisthead;
    while (tsdatalist != NULL) {
        struct TSDATALIST *tmp = tsdatalist;
        tsdatalist = tsdatalist->tail;
        free (tmp->data);
        free (tmp);
    }
    free (topiclist->topic);
    free (topiclist);
}

void addtsdatatobuff (struct TOPICLIST *topiclist, const char *data, size_t len) {
    memcpy(topiclist->tsdatabuff + topiclist->buffusesize, data, len);
    topiclist->buffusesize += len;
}

void createtsfile () {
    struct TOPICLIST *topiclist = topiclisthead;
    while (topiclist != NULL) {
        char *file = topiclist->tsdatabuff;
        size_t endpoint;
        for (size_t pos = 0 ; pos < topiclist->buffusesize ; pos += 188) {
            if (file[pos+1] == 0x40 && file[pos+2] == 0x00 && (file[pos+10] & 0x01)) { // 寻找ts结构中的pat包
                endpoint = pos;
            }
        }
        struct TSDATALIST *tsdatalist = (struct TSDATALIST*)malloc(sizeof(struct TSDATALIST));
        tsdatalist->id = topiclist->tsdatastep % 10;
        tsdatalist->data = (char*)malloc(endpoint);
        memcpy(tsdatalist->data, topiclist->tsdatabuff, endpoint);
        tsdatalist->len = endpoint;
        tsdatalist->tail = NULL;
#ifdef SAVEFILE
        char filename[64];
        if (access("store", F_OK) == 0) {
            mkdir ("store", 0777);
        }
        sprintf(filename, "store/%s-%04x.ts", topiclist->topic + 1, tsdatalist->id);
        int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0777);
        write(fd, tsdatalist->data, endpoint);
        close (fd);
#endif
        topiclist->tsdatastep++;
        if (topiclist->tsdatanum == 0) {
            topiclist->tsdatalisthead = tsdatalist;
            topiclist->tsdatalistdesc = topiclist->tsdatalisthead;
            topiclist->tsdatalisttail = topiclist->tsdatalisthead;
            topiclist->tsdatanum++;
        } else if (topiclist->tsdatanum < 3) {
            topiclist->tsdatalisttail->tail = tsdatalist;
            topiclist->tsdatalisttail = topiclist->tsdatalisttail->tail;
            topiclist->tsdatanum++;
        } else if (topiclist->tsdatanum < 6) {
            topiclist->tsdatalistdesc = topiclist->tsdatalistdesc->tail;
            topiclist->tsdatalisttail->tail = tsdatalist;
            topiclist->tsdatalisttail = topiclist->tsdatalisttail->tail;
            topiclist->tsdatanum++;
        } else {
            struct TSDATALIST *tmp = topiclist->tsdatalisthead;
            topiclist->tsdatalisthead = topiclist->tsdatalisthead->tail;
            free(tmp->data);
            free(tmp);
            topiclist->tsdatalistdesc = topiclist->tsdatalistdesc->tail;
            topiclist->tsdatalisttail->tail = tsdatalist;
            topiclist->tsdatalisttail = topiclist->tsdatalisttail->tail;
        }
        topiclist->buffusesize -= endpoint;
        if (topiclist->buffusesize < endpoint) {
            memcpy(topiclist->tsdatabuff, topiclist->tsdatabuff + endpoint, topiclist->buffusesize);
        } else {
            for (size_t pos = 0 ; pos < topiclist->buffusesize ; pos++) {
                topiclist->tsdatabuff[pos] = topiclist->tsdatabuff[pos+endpoint];
            }
        }
        topiclist = topiclist->tail;
    }
}

#define EXTM3UHEAD   "#EXTM3U\r#EXT-X-VERSION:3\r#EXT-X-TARGETDURATION:1\r#EXT-X-MEDIA-SEQUENCE:"
#define EXTM3UDATA   "#EXTINF:0.666666,\rhttp://"
#define EXTM3UFOOT   "#EXT-X-ENDLIST\r"

char *createm3u8file (char *topic, char* httphost, size_t httphostlen, size_t* len) {
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL || topiclist->tsdatanum < 3) {
        char *html = (char*)malloc(sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT));
        strcpy(html, EXTM3UHEAD"0\r"EXTM3UFOOT);
        *len = sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT) - 1;
        return html;
    }
    // sizeof(size_t)为2时最大值为65536,需5个字节表示,sizeof(size_t)为4时最大值为4294967296,需10个字节表示,sizeof(size_t)为8时最大值为18446744073709551616,需20个字节表示
    size_t size = sizeof(EXTM3UHEAD) - 1 + 5 * sizeof(size_t) / 2  + 1 + 4*(sizeof(EXTM3UDATA) - 1 + 5 + httphostlen - 1 + topiclist->topicsize - 1) + 1 ;
    struct TSDATALIST* tsdatalist0 = topiclist->tsdatalistdesc;
    struct TSDATALIST* tsdatalist1 = tsdatalist0->tail;
    struct TSDATALIST* tsdatalist2 = tsdatalist1->tail;
    char *html = (char*)malloc(size);
    *len = sprintf(html, EXTM3UHEAD"%u\r"EXTM3UDATA"%s%s%x.ts\r"EXTM3UDATA"%s%s%x.ts\r"EXTM3UDATA"%s%s%x.ts\r",
            topiclist->tsdatastep - 3,
            httphost, topic, tsdatalist0->id,
            httphost, topic, tsdatalist1->id,
            httphost, topic, tsdatalist2->id
        );
    return html;
}

char *gettsfile (char *topic, size_t id, size_t *len) {
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL) {
        *len = 0;
        return NULL;
    }
    struct TSDATALIST* tsdatalist = topiclist->tsdatalisthead;
    while (tsdatalist != NULL) {
        if (tsdatalist->id == id) {
            *len = tsdatalist->len;
            return tsdatalist->data;
        }
        tsdatalist = tsdatalist->tail;
    }
    *len = 0;
    return NULL;
}
