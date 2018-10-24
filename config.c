#include <unistd.h>
#include <jansson.h>
#include "config.h"
#include "memalloc.h"

static struct CONFIG config;

struct CONFIG* getconfig () {
    return &config;
}

struct CONFIG* initconfig () {
    if(!access("config.json", F_OK)) {
        json_error_t error;
        json_t *obj = json_load_file("config.json", 0, &error);
        json_t *port = json_object_get(obj, "port");
        if (port == NULL) {
            config.port = 8002;
        } else {
            config.port = json_integer_value(port);
        }
        json_t *tstimelong = json_object_get(obj, "tstimelong");
        if (tstimelong == NULL) {
            config.tstimelong_sec = 1;
            config.tstimelong_usec = 0;
        } else {
            unsigned int usec = json_integer_value(tstimelong);
            if (usec > 9000000) {
                config.tstimelong_sec = 9;
                config.tstimelong_usec = 000000;
            } else {
                config.tstimelong_sec = usec / 1000000;
                config.tstimelong_usec = usec % 1000000;
            }
        }
        json_decref(obj);
    } else {
        json_t *obj = json_object();
        json_object_set_new(obj, "port", json_integer(8002));
        json_object_set_new(obj, "tstimelong", json_integer(1000000));
        json_dump_file(obj, "config.json", 0);
        json_decref(obj);
        config.port = 8002;
        config.tstimelong_sec = 1;
        config.tstimelong_usec = 0;
    }
    return &config;
}