/* Declarations for a simple gif encoder.
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



typedef unsigned char   gif_u8;
typedef unsigned short  gif_u16;

GIF_STATIC_ASSERT (sizeof (gif_u8) == 1);
GIF_STATIC_ASSERT (sizeof (gif_u16) == 2);


typedef void (*gif_write) (void *ud, void const *ptr, size_t sz);


struct gif;
struct gif_ct { gif_u8 ct[256][3]; };


struct gif *gif_begin (gif_u16 w, gif_u16 h, gif_u16 ti,
                       struct gif_ct const *gct,
                       void *write_ud, gif_write write_fn);
void gif_end (struct gif **gif);
void gif_begin_frame (struct gif *gif, struct gif_ct const *lct);
void gif_end_frame (struct gif *gif);
void gif_set_pixel (struct gif *gif, gif_u16 i, gif_u16 j, gif_u8 idx);


#endif
