#include <stdio.h>
#include <string.h>
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

#define EXTM3UHEAD   "#EXTM3U\r#EXT-X-VERSION:3\r#EXT-X-TARGETDURATION:1\r#EXT-X-MEDIA-SEQUENCE:"
#define EXTM3UDATA   "#EXTINF:1.000000\r"
#define EXTM3UFOOT   "#EXT-X-ENDLIST\r"

#define MAXTSPACKAGE 14

struct TOPICLIST *addtopictolist (const char* topic) {
    struct TOPICLIST *topiclist = (struct TOPICLIST*)memalloc(sizeof(struct TOPICLIST), __FILE__, __LINE__);
    size_t len = strlen(topic);
    topiclist->topic = (char*)memalloc(len + 1, __FILE__, __LINE__);
    memcpy (topiclist->topic, topic, len + 1);
    topiclist->topiclen = len;
    struct CONFIG* config = getconfig ();
    topiclist->tsdatabuff = (char*)memalloc(config->tsdatabuffsize, __FILE__, __LINE__);
    topiclist->buffusedsize = 0;
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
    topiclist->m3u8 = (char*)memalloc(sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT), __FILE__, __LINE__);
    memcpy(topiclist->m3u8, EXTM3UHEAD"0\r"EXTM3UFOOT, sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT));
    topiclist->m3u8len = sizeof(EXTM3UHEAD"0\r"EXTM3UFOOT) - 1;
    topiclist->mp4 = (char*)memalloc(1, __FILE__, __LINE__); // 为了防止后面free出现内存异常
    topiclist->mp4len = 0;
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
    for (int i = 0 ; i < MAXTSPACKAGE ; i++) {
        struct TSDATALIST *tmp = tsdatalist;
        tsdatalist = tsdatalist->tail;
        memfree (tmp->data);
        memfree (tmp);
    }
    memfree (topiclist->topic);
    memfree (topiclist->tsdatabuff);
    memfree (topiclist->m3u8);
    memfree (topiclist->mp4);
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
    size_t size = sizeof(EXTM3UHEAD) - 1 + numbersize  + 1 + 14*(sizeof(EXTM3UDATA) - 1 + 5 + topiclist->topiclen) -1 + 1 ;
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
    topiclist->m3u8len = sprintf(topiclist->m3u8, EXTM3UHEAD"%u\r"EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r"
                                                                  EXTM3UDATA"%s%x.ts\r",
            topiclist->tsdatastep,
            topiclist->topic+1, tsdatalist0->id,
            topiclist->topic+1, tsdatalist1->id,
            topiclist->topic+1, tsdatalist2->id,
            topiclist->topic+1, tsdatalist3->id,
            topiclist->topic+1, tsdatalist4->id,
            topiclist->topic+1, tsdatalist5->id,
            topiclist->topic+1, tsdatalist6->id,
            topiclist->topic+1, tsdatalist7->id,
            topiclist->topic+1, tsdatalist8->id,
            topiclist->topic+1, tsdatalist9->id,
            topiclist->topic+1, tsdatalist10->id,
            topiclist->topic+1, tsdatalist11->id,
            topiclist->topic+1, tsdatalist12->id,
            topiclist->topic+1, tsdatalist13->id
        );
}

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

void createtsfile (struct TOPICLIST *topiclist) {
    char cmd[192];
    char inputfile[64];
    char outputfile[64];
    char *file = topiclist->tsdatabuff;
    struct TSDATALIST *tsdatalist = (struct TSDATALIST*)memalloc(sizeof(struct TSDATALIST), __FILE__, __LINE__);
    tsdatalist->id = topiclist->tsdatastep & 0x0f;
    tsdatalist->data = (char*)memalloc(topiclist->buffusedsize, __FILE__, __LINE__);
    memcpy(tsdatalist->data, topiclist->tsdatabuff, topiclist->buffusedsize);
    tsdatalist->size = topiclist->buffusedsize;
    tsdatalist->tail = NULL;
    tsdatalist->head = topiclist->tsdatalisttail;
    struct TSDATALIST *tmp = topiclist->tsdatalisthead;
    topiclist->tsdatalisthead = topiclist->tsdatalisthead->tail;
    topiclist->tsdatalisthead->head = NULL;
    memfree(tmp->data);
    memfree(tmp);
    topiclist->tsdatastep++;
    topiclist->tsdatalisttail->tail = tsdatalist;
    topiclist->tsdatalisttail = topiclist->tsdatalisttail->tail;
    createm3u8file (topiclist);
    sprintf(inputfile, "%s%s.ts", "/dev/shm", topiclist->topic);
    sprintf(outputfile, "%s%s.mp4", "/dev/shm", topiclist->topic);
    int fd1 = open(inputfile, O_WRONLY|O_CREAT|O_TRUNC, 0777);
    write (fd1, tsdatalist->data, tsdatalist->size);
    close (fd1);
    sprintf(cmd, "%s -i %s -codec copy -y %s", "ffmpeg", inputfile, outputfile);
    printf("cmd:%s\n", cmd);
    system (cmd);
    struct stat statbuff;
    stat(outputfile, &statbuff);
    char* mp4file = (char*)memalloc(statbuff.st_size, __FILE__, __LINE__);
    int fd2 = open(inputfile, O_RDONLY, 0777);
    read (fd2, mp4file, statbuff.st_size);
    close (fd2);
    unlink (inputfile);
    unlink (outputfile);
    memfree (topiclist->mp4);
    topiclist->mp4 = mp4file;
    topiclist->mp4len = statbuff.st_size;
    topiclist->buffusedsize = 0;
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

char* getmp4file (char *topic, size_t *len) {
    struct TOPICLIST *topiclist = gettopiclist (topic);
    if (topiclist == NULL) {
        *len = 0;
        return "";
    }
    *len = topiclist->mp4len;
    return topiclist->mp4;
}

void addtsdatatobuff (struct TOPICLIST *topiclist, const char *data, size_t size) {
    struct CONFIG* config = getconfig ();
    if (topiclist->buffusedsize + size > config->tsdatabuffsize) {
        printf ("buffer is not enough, at %s, in %d\n", __FILE__, __LINE__);
        return;
    }
    memcpy(topiclist->tsdatabuff + topiclist->buffusedsize, data, size);
    topiclist->buffusedsize += size;
}
