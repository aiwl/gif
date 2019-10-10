/* http://paulbourke.net/fractals/mandelbrot/ */
#include "../gif.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PATH    "mandelbrot.gif"
#define W       256
#define H       256


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


static int
mandel (double x0, double y0, int imax)
{
    double x = 0, y = 0, xnew, ynew;
    int i;

    for (i = 0; i < imax; i++) {
        xnew = x * x - y * y + x0;
        ynew = 2 * x * y + y0;
        if (xnew * xnew + ynew * ynew > 4)
            return(i);
        x = xnew;
        y = ynew;
    }

    return imax;
}


static void
add_mandel_frame (struct gif *gif, double xc, double yc, double zoom)
{
    int i, j;

    gif_begin_frame (gif, NULL);
    for (i = 0; i < H; i++) {
        for (j = 0; j < W; j++) {
            double x, y;
            int k;

            x = 2.0 * (j + 0.5) / ((double) W) - 1.0;
            y = 2.0 * (i + 0.5) / ((double) H) - 1.0;

            x *= zoom;
            y *= zoom;

            x += xc;
            y += yc;

            k = mandel (x, y, 255);
            gif_set_pixel (gif, i, j, k);
        }
    }
    gif_end_frame (gif);
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
fill_gct_jet (struct gif_ct *gct)
{
    int i = 0;
    for (i = 0; i < 256; i++) {
        float v = (float) i / (float) 256;
        RGB rgb = jet (v, 0.0f, 1.0f);
        gct->ct[i][0] = 255.0f * rgb.r;
        gct->ct[i][1] = 255.0f * rgb.g;
        gct->ct[i][2] = 255.0f * rgb.b;
    }
}


int
main (int argc, char **argv)
{
    struct gif *gif;
    struct gif_ct gct;
    FILE *file;
    int i = 0;
    float zoom = 1.0f;

    file = fopen (PATH, "wb");
    fill_gct_jet (&gct);
    gif = gif_begin (W, H, 1, &gct, file, write);
    for (i = 0; i < 50; i++) {
        add_mandel_frame(gif, -0.74364, 0.1318259, zoom);
        zoom *= 0.75f;
    }
    gif_end (&gif);
    fclose (file);
    return 0;
}
