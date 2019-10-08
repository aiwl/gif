#ifndef GIF_H
#define GIF_H


#include <stddef.h>


typedef unsigned char   gif_byte;
typedef unsigned short  gif_word;


/* TODO Static assert the size of gif_byte and gif_word. */


typedef void (*gif_write) (void *ud, void const *ptr, size_t sz);


struct gif;


struct gif_desc {
    gif_word w, h;
    gif_word ti;
    gif_byte gct[256][3];

    void *write_ud;
    gif_write write_fn;
};


struct gif_frame {
    gif_byte (*lct)[3];  
    gif_byte *cols;
};


struct gif *gif_begin (struct gif_desc const *desc);
void gif_add_frame (struct gif const *gif, struct gif_frame const *f);
void gif_end (struct gif **gif);


#endif