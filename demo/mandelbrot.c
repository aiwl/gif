#include "../gif.h"

#include <stdio.h>
#include <stdlib.h>


#define PATH    "mandelbrot.gif"
#define W       256
#define H       256


static void
write (void *ud, void const *ptr, size_t sz)
{
    fwrite (ptr, sz, 1, (FILE *) ud);
}


static void
add_frame (struct gif *gif, struct gif_frame *frm, gif_u8 idx)
{
    int i = 0;

    for (i = 0; i < W * H; i++)
        frm->cols[i] = idx;
    gif_add_frame (gif, frm);
}


int
main (int argc, char **argv)
{
    struct gif *gif;
    struct gif_desc desc;
    struct gif_frame frm;
    FILE *file;

    file = fopen (PATH, "w");

    desc.w = W;
    desc.h = H;
    desc.ti = 0;
    desc.write_fn = write;
    desc.write_ud = file;

    desc.gct[0][0] = 255;
    desc.gct[0][1] = 0;
    desc.gct[0][2] = 255;

    desc.gct[1][0] = 255;
    desc.gct[1][1] = 255;
    desc.gct[1][2] = 0;

    desc.gct[2][0] = 255;
    desc.gct[2][1] = 0;
    desc.gct[2][2] = 0;

    frm.lct = NULL;
    frm.cols = (gif_u8 *) malloc (W * H);

    gif = gif_begin (&desc);
    gif_add_frame (gif, &frm);
    gif_add_frame (gif, &frm);
    gif_add_frame (gif, &frm);
    gif_end (&gif);

    fclose (file);
    return 0;
}
