#include <string.h>
#include <unistd.h>
#include <jansson.h>
#include "config.h"
#include "memalloc.h"

struct CONFIG config;

struct CONFIG* getconfig () {
    return &config;
}

struct CONFIG* initconfig () {
    if(!access("config.json", F_OK)) {
        json_error_t error;
        json_t *obj = json_load_file("config.json", 0, &error);
        json_t* httphostobj = json_object_get(obj, "http_host");
        size_t len = json_string_length(httphostobj);
        char *httphost = (char*)memalloc(len + 1, __FILE__, __LINE__);
        memcpy(httphost, json_string_value(httphostobj), len + 1);
        config.httphost = httphost;
        config.httphostlen = len;
        config.tsdatabuffsize = json_integer_value(json_object_get(obj, "tsdatabuffsize"));
        config.tstimelong = json_integer_value(json_object_get(obj, "tstimelong"));
        json_decref(obj);
    } else {
        json_t *obj = json_object();
        json_object_set_new(obj, "http_host", json_string("http://localhost"));
        json_object_set_new(obj, "tsdatabuffsize", json_integer(1024*1024));
        json_object_set_new(obj, "tstimelong", json_integer(666666));
        json_dump_file(obj, "config.json", 0);
        json_decref(obj);
        config.httphost = "http://localhost";
        config.httphostlen = sizeof("http://localhost") - 1;
        config.tsdatabuffsize = 512*1024;
        config.tstimelong = 666666;
    }
    return &config;
}