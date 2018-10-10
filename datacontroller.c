#include <stdio.h>
#include <string.h>
#include "datacontroller.h"
#include "config.h"
#include "memalloc.h"

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

#define EXTM3UHEAD   "#EXTM3U\r#EXT-X-VERSION:3\r#EXT-X-TARGETDURATION:1\r#EXT-X-MEDIA-SEQUENCE:"
#define EXTM3UDATA   "#EXTINF:1.000000\r"
#define EXTM3UFOOT   "#EXT-X-ENDLIST\r"

struct TOPICLIST *addtopictolist (const char* topic) {
    struct TOPICLIST *topiclist = (struct TOPICLIST*)memalloc(sizeof(struct TOPICLIST), __FILE__, __LINE__);
    size_t len = strlen(topic);
    topiclist->topic = (char*)memalloc(len + 1, __FILE__, __LINE__);
    memcpy (topiclist->topic, topic, len + 1);
    topiclist->topiclen = len;
    struct CONFIG* config = getconfig ();
    topiclist->tsdatabuff = (char*)memalloc(config->tsdatabuffsize, __FILE__, __LINE__);
    topiclist->buffusesize = 0;
    struct TSDATALIST* tsdatalisthead = NULL;
    struct TSDATALIST* tsdatalisttail = NULL;
    for (int i = 0 ; i < 15 ; i++) {
        struct TSDATALIST* tsdatalist = (struct TSDATALIST*)memalloc(sizeof(struct TSDATALIST), __FILE__, __LINE__);
        tsdatalist->id = i;
        tsdatalist->data = NULL;
        tsdatalist->size = 0;
        tsdatalist->tail == NULL;
        if (tsdatalisthead == NULL) {
            tsdatalist->head == NULL;
            tsdatalisthead = tsdatalist;
            tsdatalisttail = tsdatalisthead;
        } else {
            tsdatalist->head = tsdatalisttail;
            tsdatalisttail->tail = tsdatalist;
            tsdatalisttail = tsdatalisttail->tail;
        }
    }
    topiclist->tsdatalisthead = tsdatalisthead;
    topiclist->tsdatalisttail = tsdatalisttail;
    topiclist->tsdatastep = 0;
    topiclist->m3u8 = (char*)memalloc(sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT), __FILE__, __LINE__);
    memcpy(topiclist->m3u8, EXTM3UHEAD"0\r"EXTM3UFOOT, sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT));
    topiclist->m3u8len = sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT) - 1;
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
    } else {
        topiclist->tail->head = topiclist->head;
    }
    struct TSDATALIST *tsdatalist = topiclist->tsdatalisthead;
    while (tsdatalist != NULL) {
        struct TSDATALIST *tmp = tsdatalist;
        tsdatalist = tsdatalist->tail;
        if (tmp->data) {
            memfree (tmp->data);
        }
        memfree (tmp);
    }
    memfree (topiclist->topic);
    memfree (topiclist->tsdatabuff);
    memfree (topiclist->m3u8);
    memfree (topiclist);
}

void createm3u8file (struct TOPICLIST *topiclist) {
/*
    sizeof(unsigned int)为1时最大值为2^8=256,需3个字节表示
    sizeof(unsigned int)为2时最大值为2^16=65536,需5个字节表示
    sizeof(unsigned int)为4时最大值为2^32=4294967296,需10个字节表示
    sizeof(unsigned int)为8时最大值为2^64=18446744073709551616,需20个字节表示
    sizeof(unsigned int)为16时最大值为2^128=340282366920938463463374607431768211456,需39个字节表示
*/
    size_t numbersize;
    if (sizeof(unsigned int) == 1) {
        numbersize = 3;
    } else if (sizeof(unsigned int) == 2) {
        numbersize = 5;
    } else if (sizeof(unsigned int) == 4) {
        numbersize = 10;
    } else if (sizeof(unsigned int) == 8) {
        numbersize = 20;
    } else if (sizeof(unsigned int) == 16) {
        numbersize = 39;
    }
    struct CONFIG* config = getconfig ();
    memfree (topiclist->m3u8);
    size_t size = sizeof(EXTM3UHEAD) - 1 + numbersize  + 1 + 15*(sizeof(EXTM3UDATA) - 1 + 5 + config->httphostlen + topiclist->topiclen) + 1 ;
    topiclist->m3u8 = (char*)memalloc(size, __FILE__, __LINE__);
    struct TSDATALIST* tsdatalist0 = topiclist->tsdatalisthead;
    struct TSDATALIST* tsdatalist1 = tsdatalist0->tail;
    struct TSDATALIST* tsdatalist2 = tsdatalist1->tail;
    struct TSDATALIST* tsdatalist3 = tsdatalist2->tail;
    struct TSDATALIST* tsdatalist4 = tsdatalist3->tail;
    struct TSDATALIST* tsdatalist5 = tsdatalist4->tail;
    struct TSDATALIST* tsdatalist6 = tsdatalist5->tail;
    struct TSDATALIST* tsdatalist7 = tsdatalist6->tail;
    struct TSDATALIST* tsdatalist8 = tsdatalist7->tail;
    struct TSDATALIST* tsdatalist9 = tsdatalist8->tail;
    struct TSDATALIST* tsdatalist10 = tsdatalist9->tail;
    struct TSDATALIST* tsdatalist11 = tsdatalist10->tail;
    struct TSDATALIST* tsdatalist12 = tsdatalist11->tail;
    struct TSDATALIST* tsdatalist13 = tsdatalist12->tail;
    struct TSDATALIST* tsdatalist14 = tsdatalist13->tail;
    topiclist->m3u8len = sprintf(topiclist->m3u8, EXTM3UHEAD"%u\r"EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r"
                                                                  EXTM3UDATA"%s%s%x.ts\r",
            topiclist->tsdatastep,
            config->httphost, topiclist->topic, tsdatalist0->id,
            config->httphost, topiclist->topic, tsdatalist1->id,
            config->httphost, topiclist->topic, tsdatalist2->id,
            config->httphost, topiclist->topic, tsdatalist3->id,
            config->httphost, topiclist->topic, tsdatalist4->id,
            config->httphost, topiclist->topic, tsdatalist5->id,
            config->httphost, topiclist->topic, tsdatalist6->id,
            config->httphost, topiclist->topic, tsdatalist7->id,
            config->httphost, topiclist->topic, tsdatalist8->id,
            config->httphost, topiclist->topic, tsdatalist9->id,
            config->httphost, topiclist->topic, tsdatalist10->id,
            config->httphost, topiclist->topic, tsdatalist11->id,
            config->httphost, topiclist->topic, tsdatalist12->id,
            config->httphost, topiclist->topic, tsdatalist13->id,
            config->httphost, topiclist->topic, tsdatalist14->id
        );
}

void createtsfile (struct TOPICLIST *topiclist) {
    char *file = topiclist->tsdatabuff;
    size_t endpoint = 0;
    for (size_t pos = 0 ; pos < topiclist->buffusesize ; pos += 188) {
        if (file[pos] == 0x47) {
            if (file[pos+1] == 0x40 && file[pos+2] == 0x00 && (file[pos+10] & 0x01)) { // 寻找ts结构中的pat包
                endpoint = pos;
            }
        } else {
            printf ("packet is bad, topic:%s, in %s, at %d\n", topiclist->topic, __FILE__, __LINE__);
            topiclist->buffusesize = 0;
            return;
        }
    }
    if (endpoint == 0) {
        printf ("not find pat packet, topic:%s, in %s, at %d\n", topiclist->topic, __FILE__, __LINE__);
        topiclist->buffusesize = 0;
        return;
    }
    struct TSDATALIST *tsdatalist = (struct TSDATALIST*)memalloc(sizeof(struct TSDATALIST), __FILE__, __LINE__);
    tsdatalist->id = topiclist->tsdatastep & 0x0f;
    tsdatalist->data = (char*)memalloc(endpoint, __FILE__, __LINE__);
    memcpy(tsdatalist->data, topiclist->tsdatabuff, endpoint);
    tsdatalist->size = endpoint;
    tsdatalist->tail = NULL;
    topiclist->tsdatastep++;
    tsdatalist->head = topiclist->tsdatalisttail;
    struct TSDATALIST *tmp = topiclist->tsdatalisthead;
    topiclist->tsdatalisthead = topiclist->tsdatalisthead->tail;
    topiclist->tsdatalisthead->head = NULL;
    memfree(tmp);
    tmp = topiclist->tsdatalisttail->head;
    if (tmp->data != NULL) {
        memfree(tmp->data);
        tmp->data = NULL;
    }
    tmp->size = 0;
    topiclist->tsdatalisttail->tail = tsdatalist;
    topiclist->tsdatalisttail = topiclist->tsdatalisttail->tail;
    createm3u8file (topiclist);
    topiclist->buffusesize -= endpoint;
    if (topiclist->buffusesize < endpoint) {
        memcpy(topiclist->tsdatabuff, topiclist->tsdatabuff + endpoint, topiclist->buffusesize);
    } else {
        for (size_t pos = 0 ; pos < topiclist->buffusesize ; pos++) {
            topiclist->tsdatabuff[pos] = topiclist->tsdatabuff[pos+endpoint];
        }
    }
}

void createalltsfile () {
    struct TOPICLIST *topiclist = topiclisthead;
    while (topiclist != NULL) {
        createtsfile (topiclist);
        topiclist = topiclist->tail;
    }
}

char* getm3u8file (char *topic, size_t* len) {
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL) {
        *len = sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT) - 1;
        return EXTM3UHEAD"0\r"EXTM3UFOOT;
    }
    *len = topiclist->m3u8len;
    return topiclist->m3u8;
}

char* gettsfile (char *topic, size_t id, size_t *len) {
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL) {
        *len = 0;
        return "";
    }
    struct TSDATALIST* tsdatalist = topiclist->tsdatalisttail;
    while (tsdatalist != NULL) {
        if (tsdatalist->id == id) {
            *len = tsdatalist->size;
            return tsdatalist->data;
        }
        tsdatalist = tsdatalist->head;
    }
    *len = 0;
    return "";
}

void addtsdatatobuff (struct TOPICLIST *topiclist, const char *data, size_t size) {
    struct CONFIG* config = getconfig ();
    if (topiclist->buffusesize + size > config->tsdatabuffsize) {
        printf ("buffer is not enough, at %s, in %d\n" __FILE__, __LINE__);
        createtsfile (topiclist);
        return;
    }
    memcpy(topiclist->tsdatabuff + topiclist->buffusesize, data, size);
    topiclist->buffusesize += size;
}
