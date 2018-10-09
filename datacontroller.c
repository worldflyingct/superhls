#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "datacontroller.h"
#include "config.h"

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
#define EXTM3UDATA   "#EXTINF:0."
#define EXTM3UFOOT   "#EXT-X-ENDLIST\r"

struct TOPICLIST *addtopictolist (const char* topic) {
    struct TOPICLIST *topiclist = (struct TOPICLIST*)malloc(sizeof(struct TOPICLIST));
    size_t len = strlen(topic);
    topiclist->topic = (char*)malloc(len + 1);
    strcpy (topiclist->topic, topic);
    topiclist->topiclen = len;
    struct CONFIG* config = getconfig ();
    topiclist->tsdatabuff = (char*)malloc(config->tsdatabuffsize);
    topiclist->buffusesize = 0;
    topiclist->tsdatalisthead = NULL;
    topiclist->tsdatalistdesc = NULL;
    topiclist->tsdatalisttail = NULL;
    topiclist->tsdatanum = 0;
    topiclist->tsdatastep = 0;
    topiclist->m3u8 = (char*)malloc(sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT));
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
        free (tmp->data);
        free (tmp);
    }
    free (topiclist->topic);
    free (topiclist->tsdatabuff);
    free (topiclist->m3u8);
    free (topiclist);
}

void addtsdatatobuff (struct TOPICLIST *topiclist, const char *data, size_t len) {
    memcpy(topiclist->tsdatabuff + topiclist->buffusesize, data, len);
    topiclist->buffusesize += len;
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
    free (topiclist->m3u8);
    size_t size = sizeof(EXTM3UHEAD) - 1 + numbersize  + 1 + 3*(sizeof(EXTM3UDATA) - 1 + 13 + config->httphostlen + topiclist->topiclen) + 1 ;
    topiclist->m3u8 = (char*)malloc(size);
    struct TSDATALIST* tsdatalist0 = topiclist->tsdatalistdesc;
    struct TSDATALIST* tsdatalist1 = tsdatalist0->tail;
    struct TSDATALIST* tsdatalist2 = tsdatalist1->tail;
    topiclist->m3u8len = sprintf(topiclist->m3u8, EXTM3UHEAD"%u\r"EXTM3UDATA"%06u,\r%s%s%x.ts\r"EXTM3UDATA"%06u,\r%s%s%x.ts\r"EXTM3UDATA"%06u,\r%s%s%x.ts\r",
            topiclist->tsdatastep - 3,
            config->tstimelong, config->httphost, topiclist->topic, tsdatalist0->id,
            config->tstimelong, config->httphost, topiclist->topic, tsdatalist1->id,
            config->tstimelong, config->httphost, topiclist->topic, tsdatalist2->id
        );
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
            createm3u8file (topiclist);
        } else {
            struct TSDATALIST *tmp = topiclist->tsdatalisthead;
            topiclist->tsdatalisthead = topiclist->tsdatalisthead->tail;
            free(tmp->data);
            free(tmp);
            topiclist->tsdatalistdesc = topiclist->tsdatalistdesc->tail;
            topiclist->tsdatalisttail->tail = tsdatalist;
            topiclist->tsdatalisttail = topiclist->tsdatalisttail->tail;
            createm3u8file (topiclist);
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
    return "";
}
