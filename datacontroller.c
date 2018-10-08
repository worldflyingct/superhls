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
#ifdef DEBUG
        printf("topic:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d, in %s, at %d\n",
                topiclist->topic[0], topiclist->topic[1], topiclist->topic[2], topiclist->topic[3], topiclist->topic[4], topiclist->topic[5], topiclist->topic[6], topiclist->topic[7], topiclist->topic[8],
                topiclist->topic[9], topiclist->topic[10], topiclist->topic[11], topiclist->topic[12], topiclist->topic[13], topiclist->topic[14],
            __FILE__, __LINE__);
        printf("topic2:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d, in %s, at %d\n",
            topic[0], topic[1], topic[2], topic[3], topic[4], topic[5], topic[6], topic[7], topic[8],
            topic[9], topic[10], topic[11], topic[12], topic[13], topic[14],
             __FILE__, __LINE__);
#endif
        if (!strcmp(topiclist->topic, topic)) {
#ifdef DEBUG
            printf("the same\n");
#endif
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
#ifdef DEBUG
        printf("in %s, at %d\n", __FILE__, __LINE__);
#endif
        char *file = topiclist->tsdatabuff;
        if (file[0] != 0x47) {
            size_t realhead = -1;
            for (size_t pos = 0 ; pos < topiclist->buffusesize ; pos ++) {
                if (file[pos] == 0x47) {
                    realhead = pos;
                    break;
                }
            }
            if (realhead == -1) {
#ifdef DEBUG
                printf("package is bad,in %s, at %d\n", __FILE__, __LINE__);
#endif
                topiclist->buffusesize = 0;
                continue;
            }
            topiclist->buffusesize -= realhead;
            if (topiclist->buffusesize < realhead) {
#ifdef DEBUG
                printf("in %s, at %d\n", __FILE__, __LINE__);
#endif
                memcpy(topiclist->tsdatabuff, topiclist->tsdatabuff + realhead, topiclist->buffusesize);
            } else {
#ifdef DEBUG
                printf("in %s, at %d\n", __FILE__, __LINE__);
#endif
                for (size_t pos = 0 ; pos < topiclist->buffusesize ; pos++) {
                    topiclist->tsdatabuff[pos] = topiclist->tsdatabuff[pos+realhead];
                }
            }
        }
        size_t endpoint;
        for (size_t pos = 0 ; pos < topiclist->buffusesize ; pos += 188) {
            if (file[pos] != 0x47) {
#ifdef DEBUG
                printf("package is bad,in %s, at %d\n", __FILE__, __LINE__);
#endif
                topiclist->buffusesize = 0;
                continue;
            }
            if (file[pos+1] == 0x40 && file[pos+2] == 0x00 && (file[pos+10] & 0x01)) { // 寻找ts结构中的pat包
                endpoint = pos;
            }
        }
        struct TSDATALIST *tsdatalist = (struct TSDATALIST*)malloc(sizeof(struct TSDATALIST));
        tsdatalist->id = topiclist->tsdatastep & 0x0f;
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
#ifdef DEBUG
            printf("in %s, at %d\n", __FILE__, __LINE__);
#endif
            memcpy(topiclist->tsdatabuff, topiclist->tsdatabuff + endpoint, topiclist->buffusesize);
        } else {
#ifdef DEBUG
            printf("in %s, at %d\n", __FILE__, __LINE__);
#endif
            for (size_t pos = 0 ; pos < topiclist->buffusesize ; pos++) {
                topiclist->tsdatabuff[pos] = topiclist->tsdatabuff[pos+endpoint];
            }
        }
        topiclist = topiclist->tail;
    }
}

#define EXTM3UHEAD   "#EXTM3U\r#EXT-X-VERSION:3\r#EXT-X-TARGETDURATION:1\r#EXT-X-MEDIA-SEQUENCE:"
#define EXTM3UDATA   "#EXTINF:1.000000,\rhttp://"
#define EXTM3UFOOT   "#EXT-X-ENDLIST\r"

char *createm3u8file (char *topic, char* httphost, size_t httphostlen, size_t* len) {
#ifdef DEBUG
    printf("in %s, at %d\n", __FILE__, __LINE__);
#endif
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL || topiclist->tsdatanum < 3) {
        char *html = (char*)malloc(sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT));
        strcpy(html, EXTM3UHEAD"0\r"EXTM3UFOOT);
        *len = sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT) - 1;
        return html;
    }
    size_t size = sizeof(EXTM3UHEAD) - 1 + 20  + 1 + 4*(sizeof(EXTM3UDATA) - 1 + 6 + httphostlen - 1 + topiclist->topicsize - 1) + 1 ;
#ifdef DEBUG
    printf("size:%d,in %s, at %d\n", size, __FILE__, __LINE__);
#endif
    struct TSDATALIST* tsdatalist0 = topiclist->tsdatalistdesc;
    struct TSDATALIST* tsdatalist1 = tsdatalist0->tail;
    struct TSDATALIST* tsdatalist2 = tsdatalist1->tail;
    char *html = (char*)malloc(size);
    *len = sprintf(html, EXTM3UHEAD"%u\r"EXTM3UDATA"%s%s-%x.ts\r"EXTM3UDATA"%s%s-%x.ts\r"EXTM3UDATA"%s%s-%x.ts\r",
            topiclist->tsdatastep - 3,
            httphost, topic, tsdatalist0->id,
            httphost, topic, tsdatalist1->id,
            httphost, topic, tsdatalist2->id
        );
#ifdef DEBUG
    printf("len:%d,in %s, at %d\n", *len, __FILE__, __LINE__);
#endif
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
#ifdef DEBUG
        printf("id:%04x, in %s, at %d\n", id, __FILE__, __LINE__);
#endif
        if (tsdatalist->id == id) {
            *len = tsdatalist->len;
            return tsdatalist->data;
        }
        tsdatalist = tsdatalist->tail;
    }
    *len = 0;
    return NULL;
}
