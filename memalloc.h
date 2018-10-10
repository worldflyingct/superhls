#ifndef __MEMALLOC_H__
#define __MEMALLOC_H__

void* memalloc (size_t num, char* filename, int line);
void memfree (void* p);
void showmallocnum ();

#endif