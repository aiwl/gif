/* This is free and unencumbered software released into the public domain.

   Anyone is free to copy, modify, publish, use, compile, sell, or
   distribute this software, either in source code form or as a compiled
   binary, for any purpose, commercial or non-commercial, and by any
   means.

   In jurisdictions that recognize copyright laws, the author or authors
   of this software dedicate any and all copyright interest in the
   software to the public domain. We make this dedication for the benefit
   of the public at large and to the detriment of our heirs and
   successors. We intend this dedication to be an overt act of
   relinquishment in perpetuity of all present and future rights to this
   software under copyright law.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to <http://unlicense.org/> */

#ifndef GIF_H
#define GIF_H


#include <stddef.h>


struct gif;


typedef unsigned char   gif_u8;
typedef unsigned short  gif_u16;
/* TODO Static assert the size of gif_u8 and gif_u16. */


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
