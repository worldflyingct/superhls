#ifndef __CONFIG_H__
#define __CONFIG_H__

struct CONFIG {
    unsigned int port;
    unsigned int tstimelong_sec;
    unsigned int tstimelong_usec;
};

struct CONFIG* getconfig ();
struct CONFIG* initconfig ();

#endif