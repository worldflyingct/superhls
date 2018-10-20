#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "datacontroller.h"
#include "config.h"
#include "memalloc.h"

static struct TOPICLIST *topiclisthead = NULL;
static struct TOPICLIST *topiclisttail = NULL;

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

#define EXTM3UHEAD1  "#EXTM3U\r#EXT-X-VERSION:3\r#EXT-X-TARGETDURATION:"
#define EXTM3UHEAD2  "#EXT-X-MEDIA-SEQUENCE:"
#define EXTM3UDATA   "#EXTINF:"
#define EXTM3UFOOT   "#EXT-X-ENDLIST\r"

#define MAXTSPACKAGE 16

struct TOPICLIST *addtopictolist (const char* topic) {
    struct TOPICLIST *topiclist = (struct TOPICLIST*)memalloc(sizeof(struct TOPICLIST), __FILE__, __LINE__);
    size_t len = strlen(topic);
    topiclist->topic = (char*)memalloc(len + 1, __FILE__, __LINE__);
    memcpy (topiclist->topic, topic, len + 1);
    topiclist->topiclen = len;
    topiclist->tstempdatahead = NULL;
    topiclist->buffusedsize = 0;
    topiclist->emptytime = 0;
    pthread_mutex_init(&topiclist->mutex, NULL);
    struct TSDATALIST* tsdatalisthead = NULL;
    struct TSDATALIST* tsdatalisttail = NULL;
    for (int i = 0 ; i < MAXTSPACKAGE ; i++) {
        struct TSDATALIST* tsdatalist = (struct TSDATALIST*)memalloc(sizeof(struct TSDATALIST), __FILE__, __LINE__);
        tsdatalist->id = i;
        tsdatalist->data = (char*)memalloc(1, __FILE__, __LINE__); // 为了防止后面free出现内存异常
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
    topiclist->tsdatastep = MAXTSPACKAGE;
    topiclist->m3u8 = (char*)memalloc(sizeof(EXTM3UHEAD1"1\r"EXTM3UHEAD2"0\r"EXTM3UFOOT), __FILE__, __LINE__);
    memcpy(topiclist->m3u8, EXTM3UHEAD1"1\r"EXTM3UHEAD2"0\r"EXTM3UFOOT, sizeof(EXTM3UHEAD1"1\r"EXTM3UHEAD2"0\r"EXTM3UFOOT));
    topiclist->m3u8len = sizeof(EXTM3UHEAD1"1\r"EXTM3UHEAD2"0\r"EXTM3UFOOT) - 1;
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
    memfree (topiclist->topic);
    struct TSDATALIST *tsdatalist = topiclist->tsdatalisthead;
    for (int i = 0 ; i < MAXTSPACKAGE ; i++) {
        struct TSDATALIST *tmp = tsdatalist;
        tsdatalist = tsdatalist->tail;
        memfree (tmp->data);
        memfree (tmp);
    }
    pthread_mutex_destroy (&topiclist->mutex);
    struct TSTEMPDATA* tstempdata = topiclist->tstempdatahead;
    while (tstempdata != NULL) {
        struct TSTEMPDATA* tmp = tstempdata;
        tstempdata = tstempdata->tail;
        memfree(tmp->data);
        memfree(tmp);
    }
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
    memfree (topiclist->m3u8);
    struct CONFIG* config = getconfig ();
    unsigned int duration;
    if (config->tstimelong_usec == 0)
        duration = config->tstimelong_sec;
    else
        duration = config->tstimelong_sec + 1;
    size_t size = sizeof(EXTM3UHEAD1) - 1 + 2 + sizeof(EXTM3UHEAD2) - 1 + numbersize  + 1 + 5*(sizeof(EXTM3UDATA) - 1 + 9 + 5 + topiclist->topiclen) -1 + 1;
    topiclist->m3u8 = (char*)memalloc(size, __FILE__, __LINE__);
    struct TSDATALIST* tsdatalist4 = topiclist->tsdatalisttail;
    struct TSDATALIST* tsdatalist3 = tsdatalist4->head;
    struct TSDATALIST* tsdatalist2 = tsdatalist3->head;
    struct TSDATALIST* tsdatalist1 = tsdatalist2->head;
    struct TSDATALIST* tsdatalist0 = tsdatalist1->head;
    topiclist->m3u8len = sprintf(topiclist->m3u8, EXTM3UHEAD1"%d\r"EXTM3UHEAD2"%u\r"
                                                    EXTM3UDATA"%d.%06d\r%s%x.ts\r"
                                                    EXTM3UDATA"%d.%06d\r%s%x.ts\r"
                                                    EXTM3UDATA"%d.%06d\r%s%x.ts\r"
                                                    EXTM3UDATA"%d.%06d\r%s%x.ts\r"
                                                    EXTM3UDATA"%d.%06d\r%s%x.ts\r",
            duration, topiclist->tsdatastep,
            config->tstimelong_sec, config->tstimelong_usec, topiclist->topic+1, tsdatalist0->id,
            config->tstimelong_sec, config->tstimelong_usec, topiclist->topic+1, tsdatalist1->id,
            config->tstimelong_sec, config->tstimelong_usec, topiclist->topic+1, tsdatalist2->id,
            config->tstimelong_sec, config->tstimelong_usec, topiclist->topic+1, tsdatalist3->id,
            config->tstimelong_sec, config->tstimelong_usec, topiclist->topic+1, tsdatalist4->id
        );
}

void createtsfile (struct TOPICLIST *topiclist) {
    struct TSDATALIST *tsdatalist = (struct TSDATALIST*)memalloc(sizeof(struct TSDATALIST), __FILE__, __LINE__);
    tsdatalist->id = topiclist->tsdatastep & 0x0f;
    tsdatalist->data = (char*)memalloc(topiclist->buffusedsize, __FILE__, __LINE__);
    struct TSTEMPDATA* tstempdata = topiclist->tstempdatahead;
    size_t pos = 0;
    while (tstempdata != NULL) {
        memcpy(tsdatalist->data + pos, tstempdata->data, tstempdata->size);
        pos += tstempdata->size;
        struct TSTEMPDATA* tmp1 = tstempdata;
        tstempdata = tstempdata->tail;
        memfree(tmp1->data);
        memfree(tmp1);
    }
    tsdatalist->size = topiclist->buffusedsize;
    tsdatalist->tail = NULL;
    tsdatalist->head = topiclist->tsdatalisttail;
    struct TSDATALIST *tmp2 = topiclist->tsdatalisthead;
    topiclist->tsdatalisthead = topiclist->tsdatalisthead->tail;
    topiclist->tsdatalisthead->head = NULL;
    memfree(tmp2->data);
    memfree(tmp2);
    topiclist->tsdatastep++;
    topiclist->tsdatalisttail->tail = tsdatalist;
    topiclist->tsdatalisttail = topiclist->tsdatalisttail->tail;
    createm3u8file (topiclist);
    topiclist->tstempdatahead = NULL;
    topiclist->buffusedsize = 0;
}

void createalltsfile () {
    struct TOPICLIST *topiclist = topiclisthead;
    while (topiclist != NULL) {
        if (topiclist->buffusedsize == 0) {
            topiclist->emptytime++;
            if (topiclist->emptytime >= 30) {
                struct TOPICLIST *tmp = topiclist;
                topiclist = topiclist->tail;
                removetopicfromlist (tmp);
            }
        } else {
            topiclist->emptytime = 0;
            createtsfile (topiclist);
            topiclist = topiclist->tail;
        }
    }
}

char* getm3u8file (char *topic, size_t* len) {
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL) {
        *len = sizeof(EXTM3UHEAD1"1\r"EXTM3UHEAD2"0\r"EXTM3UFOOT) - 1;
        return EXTM3UHEAD1"1\r"EXTM3UHEAD2"0\r"EXTM3UFOOT;
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

char* getlatesttsfile (char *topic, size_t *len) {
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL) {
        *len = 0;
        return "";
    }
    *len = topiclist->tsdatalisttail->size;
    return topiclist->tsdatalisttail->data;
}

void addtsdatatobuff (struct TOPICLIST *topiclist, const char *data, size_t size) {
    struct TSTEMPDATA* tstempdata = (struct TSTEMPDATA*)memalloc(topiclist->buffusedsize, __FILE__, __LINE__);
    tstempdata->data = (char*)memalloc(size, __FILE__, __LINE__);
    memcpy(tstempdata->data, data, size);
    tstempdata->size = size;
    tstempdata->tail = NULL;
    pthread_mutex_lock (&topiclist->mutex);
    if (topiclist->tstempdatahead == NULL) {
        topiclist->tstempdatahead = tstempdata;
        topiclist->tstempdatatail = topiclist->tstempdatahead;
    } else {
        topiclist->tstempdatatail->tail = tstempdata;
        topiclist->tstempdatatail = topiclist->tstempdatatail->tail;
    }
    topiclist->buffusedsize += size;
    pthread_mutex_unlock (&topiclist->mutex);
}
