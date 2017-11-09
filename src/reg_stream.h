#ifndef _REG_STREAM_H_
#define _REG_STREAM_H_

#include <stddef.h>

struct reg_stream{
  const unsigned char* buff;
  size_t size;
  size_t pos;
};

struct reg_stream;

struct reg_stream* stream_new(const unsigned char* str, size_t size);
void stream_free(struct reg_stream* p);

unsigned char stream_char(struct reg_stream* p);
unsigned char stream_look(struct reg_stream* p, size_t idx);
int stream_end(struct reg_stream* p);
unsigned char stream_next(struct reg_stream* p);
unsigned char stream_back(struct reg_stream* p);
size_t stream_pos(struct reg_stream* p);

#endif
