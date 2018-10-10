#ifndef __CONFIG_H__
#define __CONFIG_H__

struct CONFIG {
    char *httphost;
    size_t httphostlen;
    size_t tsdatabuffsize;
};

struct CONFIG* getconfig ();
struct CONFIG* initconfig ();

#endif