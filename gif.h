/* Declares a simple gif encoder.
   See UNLICENSE for copyright notice. */

#ifndef GIF_H
#define GIF_H

#include <stddef.h>


#define GIF_STRINGIFY(x)                        #x
#define GIF_MACRO_STRINGIFY(x)                  GIF_STRINGIFY (x)
#define GIF_STRING_JOIN_IMMEDIATE(arg1, arg2)   arg1 ## arg2
#define GIF_STRING_JOIN_DELAY(arg1, arg2)       GIF_STRING_JOIN_IMMEDIATE (arg1, arg2)
#define GIF_STRING_JOIN(arg1, arg2)             GIF_STRING_JOIN_DELAY (arg1, arg2)
#define GIF_UNIQUE_NAME(name)                   GIF_STRING_JOIN (name, __LINE__)

#define GIF_STATIC_ASSERT(cond)    \
    typedef char GIF_UNIQUE_NAME (gif_dummy_array)[(cond) ? 1 : -1]


struct gif;

typedef unsigned char   gif_u8;
typedef unsigned short  gif_u16;

GIF_STATIC_ASSERT (sizeof (gif_u8) == 1);
GIF_STATIC_ASSERT (sizeof (gif_u16) == 2);


typedef void (*gif_read) (void *ud, void *ptr, size_t sz);
typedef void (*gif_write) (void *ud, void const *ptr, size_t sz);


struct gif_desc {
    gif_u16 w, h;
    gif_u16 ti;
    gif_u8 gct[256][3];

    void *write_ud;
    gif_write write_fn;
};


struct gif_frame {
    gif_u8 (*lct)[3];
    gif_u8 *cols;
};


struct gif *gif_begin (struct gif_desc const *desc);
void gif_add_frame (struct gif *gif, struct gif_frame const *frm);
void gif_end (struct gif **gif);

#endif
