#include "../gif.h"

#include <stdio.h>


#define PATH    "mandelbrot.gif"
#define W       256
#define H       256


static void
write (void *ud, void const *ptr, size_t sz)
{
    fwrite (ptr, sz, 1, (FILE *) ud);
}


int
main (int argc, char **argv)
{
    struct gif *gif;
    struct gif_desc desc;
    struct gif_frame frm;
    gif_byte cols[W * H];
    FILE *file;

    file = fopen (PATH, "w");

    desc.w = W;
    desc.h = H;
    desc.ti = 0;
    desc.write_fn = write;
    desc.write_ud = file;

    frm.lct = NULL;
    frm.cols = cols;

    gif = gif_begin (&desc);
    gif_add_frame(gif, &frm);
    gif_add_frame(gif, &frm);
    gif_add_frame(gif, &frm);
    gif_end (&gif);

    fclose (file);
    return 0;
}