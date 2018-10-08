#ifndef __DATACONTROLLER_H__
#define __DATACONTROLLER_H__

#define TSDATABUFFSIZE   128*1024

struct TSDATALIST {
    size_t id;
    char* data;
    size_t len;
    struct TSDATALIST* tail;
};

struct TOPICLIST {
    char* topic;
    size_t topicsize;
    char tsdatabuff[TSDATABUFFSIZE];
    size_t buffusesize; // 这里记录有多少的tsdatabuff已经被使用
    struct TSDATALIST* tsdatalisthead;
    struct TSDATALIST* tsdatalistdesc;
    struct TSDATALIST* tsdatalisttail;
    size_t tsdatanum;
    size_t tsdatastep;
    struct TOPICLIST* head;
    struct TOPICLIST* tail;
};

struct TOPICLIST *gettopiclist (const char* topic);
struct TOPICLIST *addtopictolist (const char* topic);
void removetopicfromlist (struct TOPICLIST *topiclist);
void addtsdatatobuff (struct TOPICLIST *topiclist, const char *data, size_t len);
char *createm3u8file (char *topic, char* httphost, size_t httphostlen, size_t* len);
char *gettsfile (char *topic, int id, size_t *len);

#endif