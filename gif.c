#include "gif.h"

#include <stdlib.h>


#define WRITE(ptr, sz) \
    gif->write_fn (gif->write_ud, ptr, sz)


struct gif {
    void *write_ud;
    gif_write write_fn;
};


static void
write_bytes (struct gif *gif, void *ptr, size_t sz)
{
    gif->write_fn (gif->write_ud, ptr, sz);
}


static void
write_byte (struct gif *gif, gif_byte b)
{
    write_bytes (gif, &b, 1);
}


static void
write_word (struct gif *gif, gif_word  w)
{
    write_bytes (gif, &w, 2);
}


struct gif *
gif_begin (struct gif_desc const *desc)
{
    struct gif *gif;

    gif = (struct gif *) malloc (sizeof (*gif));
    gif->write_fn = desc->write_fn;
    gif->write_ud = desc->write_ud;

    write_bytes (gif, "GIF", 3);
    write_bytes (gif, "89a", 3);
    write_word (gif, desc->w);
    write_word (gif, desc->h);
    /* Global color flag, 1bit; 8bits color palette
       (256 colors), 3bit; color table is not sorted, 1bit;
       size of the global color table (256), 3bit. */
    write_byte (gif, 0xf7);
    write_byte (gif, 0);    /* Take the first color in the gct
                               as the background color. */
    write_byte (gif, 0);    /* Do not give pixel aspect ratio
                               information. */
    write_bytes (gif, desc->gct, 256 * 3);

    return gif;
}


void
gif_add_frame (struct gif *gif, struct gif_frame *f)
{

}


void
gif_end (struct gif **gif)
{
    write_byte (*gif, 0x3b);
    free (*gif);
    *gif = NULL;
}