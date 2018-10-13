#ifndef __CONFIG_H__
#define __CONFIG_H__

struct CONFIG {
    size_t tsdatabuffsize;
    unsigned int port;
    unsigned int tstimelong;
};

struct CONFIG* getconfig ();
struct CONFIG* initconfig ();

#endif