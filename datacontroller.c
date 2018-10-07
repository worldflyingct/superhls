#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "datacontroller.h"

struct TOPICLIST *topiclisthead = NULL;
struct TOPICLIST *topiclisttail = NULL;

struct TOPICLIST *gettopiclist (const char* topic) {
    struct TOPICLIST *topiclist = topiclisthead;
    if (topiclist == NULL) {
         printf("topiclist is null, in %s, at %d\n", __FILE__, __LINE__);
        return NULL;
    }
    while (topiclist != NULL) {
        printf("topic:%c,%c,%c,%c,%c,%c, in %s, at %d\n",
                topiclist->topic[0], topiclist->topic[1], topiclist->topic[2], topiclist->topic[3], topiclist->topic[4], topiclist->topic[5], topiclist->topic[6],
            __FILE__, __LINE__);
        printf("topic2:%c,%c,%c,%c,%c,%c, in %s, at %d\n",
            topic[0], topic[1], topic[2], topic[3], topic[4], topic[5], topic[6],
             __FILE__, __LINE__);
        if (!strcmp(topiclist->topic, topic)) {
            printf("the same\n");
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
    if (topiclist->buffusesize > TSDATABUFFSIZE / 2) { //缓存足够，开始生成ts文件
        printf("in %s, at %d\n", __FILE__, __LINE__);
        char *file = topiclist->tsdatabuff;
        int endpoint;
        for (int pos = 0 ; pos < topiclist->buffusesize ; pos += 188) {
            if (file[pos+1] == 0x40 && file[pos+2] == 0x00 && (file[pos+10] & 0x01)) { // 寻找ts结构中的pat包
                endpoint = pos;
            }
        }
        struct TSDATALIST *tsdatalist = (struct TSDATALIST*)malloc(sizeof(struct TSDATALIST));
        tsdatalist->id = topiclist->tsdatastep;
        tsdatalist->data = (char*)malloc(endpoint);
        memcpy(tsdatalist->data, topiclist->tsdatabuff, endpoint);
        tsdatalist->len = endpoint;
        tsdatalist->tail = NULL;
        topiclist->tsdatastep++;
        if (topiclist->tsdatastep & 0x8000) { // 大于32768就重新设置为0
            topiclist->tsdatastep = 0;
        }
        if (topiclist->tsdatanum == 0) {
            topiclist->tsdatalisthead = tsdatalist;
            topiclist->tsdatalistdesc = topiclist->tsdatalisthead;
            topiclist->tsdatalisttail = topiclist->tsdatalisthead;
            topiclist->tsdatanum++;
        } else if (topiclist->tsdatanum < 5) {
            topiclist->tsdatalisttail->tail = tsdatalist;
            topiclist->tsdatalisttail = topiclist->tsdatalisttail->tail;
            topiclist->tsdatanum++;
        } else if (topiclist->tsdatanum < 256) {
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
        memcpy(topiclist->tsdatabuff, topiclist->tsdatabuff + endpoint, topiclist->buffusesize);
    }
}

#define EXTM3UHEAD   "#EXTM3U\r#EXT-X-VERSION:3\r#EXT-X-TARGETDURATION:10\r#EXT-X-MEDIA-SEQUENCE:0\r"
#define EXTM3UDATA   "#EXTINF:1.000000,\r%s-%04x.ts\r"
#define EXTM3UFOOT   "#EXT-X-ENDLIST\r"

char *createm3u8file (char *topic, size_t* len) {
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL || topiclist->tsdatanum < 5) {
        char *html = (char*)malloc(sizeof(EXTM3UHEAD""EXTM3UFOOT));
        strcpy(html, EXTM3UHEAD""EXTM3UFOOT);
        *len = sizeof(EXTM3UHEAD""EXTM3UFOOT) - 1;
        return html;
    }
    char *html = (char*)malloc(sizeof(EXTM3UHEAD""EXTM3UFOOT) + 5*(sizeof("#EXTINF:1.000000,") - 1 + topiclist->topicsize - 1 + 1 + 4 + 3 + 2)); // #EXTINF:1.000000,的长度+topic的长度+间隔符长度+id的长度+".ts"的长度+换行
    struct TSDATALIST *tsdatalist1 = topiclist->tsdatalistdesc;
    struct TSDATALIST *tsdatalist2 = tsdatalist1->tail;
    struct TSDATALIST *tsdatalist3 = tsdatalist2->tail;
    struct TSDATALIST *tsdatalist4 = tsdatalist3->tail;
    struct TSDATALIST *tsdatalist5 = tsdatalist4->tail;
    *len = sprintf(html, EXTM3UHEAD""EXTM3UDATA""EXTM3UDATA""EXTM3UDATA""EXTM3UDATA""EXTM3UDATA""EXTM3UFOOT,
            topic+1, tsdatalist1->id,
            topic+1, tsdatalist2->id,
            topic+1, tsdatalist3->id,
            topic+1, tsdatalist4->id,
            topic+1, tsdatalist5->id
        );
    return html;
}

char *gettsfile (char *topic, int id, size_t *len) {
    printf("in %s, at %d\n", id, __FILE__, __LINE__);
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL) {
        printf("id:%04x, in %s, at %d\n", id, __FILE__, __LINE__);
        *len = 0;
        return NULL;
    }
    struct TSDATALIST* tsdatalist = topiclist->tsdatalisthead;
    while (tsdatalist != NULL) {
        printf("id:%04x, in %s, at %d\n", id, __FILE__, __LINE__);
        if (tsdatalist->id == id) {
            *len = tsdatalist->len;
            return tsdatalist->data;
        }
        tsdatalist = tsdatalist->tail;
    }
    *len = 0;
    return NULL;
}
