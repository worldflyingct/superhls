#ifndef __CONFIG_H__
#define __CONFIG_H__

struct CONFIG {
    unsigned int port;
    unsigned int tstimelong;
};

struct CONFIG* getconfig ();
struct CONFIG* initconfig ();

#endif