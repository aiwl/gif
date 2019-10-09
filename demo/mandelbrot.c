#include "../gif.h"

#include <stdio.h>
#include <stdlib.h>


#define PATH    "mandelbrot.gif"
#define W       1024
#define H       512


#define at(frm, i, j) frm->cols[(i) * W + (j)]


typedef struct {
    float r; /* In 0..1 */
    float g; /* In 0..1 */
    float b; /* In 0..1 */
} RGB;


static void
write (void *ud, void const *ptr, size_t sz)
{
    fwrite (ptr, sz, 1, (FILE *) ud);
}


static void
add_mandel_frame (struct gif *gif, struct gif_frame *frm, gif_u8 idx)
{
    int i, j;
    for (i = 0; i < H; i++) {
        for (j = 0; j < W; j++) {
            float x0, y0;
            float x, y;
            int k;

            x = y = 0.0f;
            x0 = (j + 0.5f) / ((float) W);
            y0 = (i + 0.5f) / ((float) H);
            x0 = -2.5f + x0 * 3.5f;
            y0 = -1.0f + y0 * 2.0f;
            for (k = 0; k < 50; k++) {
                float x1;

                x1 = x * x - y * y + x0;
                y = 2 * x * y + y0;
                x = x1;
                if ((x * x + y * y) > (2 * 2))
                    break;
            }
            at (frm, i, j) = k * 5;
        }
    }
    gif_add_frame (gif, frm);
}


static RGB 
jet (float v, float vmin, float vmax)
{
    RGB c = { 1.0f, 1.0f, 1.0f };
    float dv;

    if (v < vmin) v = vmin;
    if (v > vmax) v = vmax;
    dv = vmax - vmin;
    if (v < (vmin + 0.25f * dv)) {
        c.r = 0.0f;
        c.g = 4.0f * (v - vmin) / dv;
    } 
    else if (v < (vmin + 0.5f * dv)) {
        c.r = 0.0f;
        c.b = 1.0f + 4.0f * (vmin + 0.25f * dv - v) / dv;
    } 
    else if (v < (vmin + 0.75f * dv)) {
        c.r = 4.0f * (v - vmin - 0.5f * dv) / dv;
        c.b = 0.0f;
    } 
    else {
        c.g = 1.0f + 4.0f * (vmin + 0.75f * dv - v) / dv;
        c.b = 0.0f;
    }
    return c;
}


static void
fill_gct_jet (gif_u8 (*gct)[3])
{
    int i = 0;
    for (i = 0; i < 256; i++) {
        float v = (float) i / (float) 256;
        RGB rgb = jet (v, 0.0f, 1.0f);
        gct[i][0] = 255.0f * rgb.r;
        gct[i][1] = 255.0f * rgb.g;
        gct[i][2] = 255.0f * rgb.b;
    }
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
    desc.ti = 1;
    desc.write_fn = write;
    desc.write_ud = file;
    fill_gct_jet (desc.gct);

    frm.lct = NULL;
    frm.cols = (gif_u8 *) malloc (W * H);

    gif = gif_begin (&desc);
    add_mandelbrot_frame (gif, &frm, 0);
//    add_frame (gif, &frm, 100);
//    add_frame (gif, &frm, 200);
    gif_end (&gif);

    free (frm.cols);
    fclose (file);
    return 0;
}
