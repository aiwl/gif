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
void gif_add_frame (struct gif *gif, struct gif_frame const *f);
void gif_end (struct gif **gif);


#endif
