#include <string.h>
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
        config.tsdatabuffsize = json_integer_value(json_object_get(obj, "tsdatabuffsize"));
        config.port = json_integer_value(json_object_get(obj, "port"));
        config.tstimelong = json_integer_value(json_object_get(obj, "tstimelong"));
        json_decref(obj);
    } else {
        json_t *obj = json_object();
        json_object_set_new(obj, "tsdatabuffsize", json_integer(1024*1024));
        json_object_set_new(obj, "port", json_integer(8002));
        json_object_set_new(obj, "tstimelong", json_integer(500000));
        json_dump_file(obj, "config.json", 0);
        json_decref(obj);
        config.tsdatabuffsize = 512*1024;
        config.port = 8002;
        config.tstimelong = 500000;
    }
    return &config;
}