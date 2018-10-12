#ifndef __DATACONTROLLER_H__
#define __DATACONTROLLER_H__

struct TSDATALIST {
    size_t id;
    char* data;
    size_t size;
    struct TSDATALIST* head;
    struct TSDATALIST* tail;
};

struct TOPICLIST {
    char* topic;
    size_t topiclen;
    char* tsdatabuff;
    size_t buffusedsize; // 这里记录有多少的tsdatabuff已经被使用
    struct TSDATALIST* tsdatalisthead;
    struct TSDATALIST* tsdatalisttail;
    unsigned int tsdatastep;
    char* m3u8;
    size_t m3u8len;
    struct TOPICLIST* head;
    struct TOPICLIST* tail;
};

struct TOPICLIST *gettopiclist (const char* topic);
struct TOPICLIST *addtopictolist (const char* topic);
void removetopicfromlist (struct TOPICLIST *topiclist);
void addtsdatatobuff (struct TOPICLIST *topiclist, const char *data, size_t len);
char *getm3u8file (char *topic, size_t* len);
char *gettsfile (char *topic, size_t id, size_t *len);
char* getlatesttsfile (char *topic, size_t *len);
void createalltsfile ();

#endif