#include "gif.h"

#include <stdlib.h>
#include <assert.h>


typedef int             gif_bool;
typedef unsigned int    gif_uint;


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



/* ----------------------- dictionary pattern -----------------------
   Definitions for a dictionary which is used to map pattern to codes
   and vice versa. */

#define NIL_ID              UINT16_MAX
#define HASH_SIZE           8192

#define ALPH_BIT_SIZE       8
#define CLEAR_CODE          (1 << ALPH_BIT_SIZE)
#define STOP_CODE           (CLEAR_CODE + 1)
#define FIRST_COMPR_CODE    (STOP_CODE + 1)
#define MAX_CODE            0xFFF
#define MAX_CODE_COUNT      (MAX_CODE + 1)


/* A pattern is either a single character or a substring
   represented by two markers `i`, `j`, which point to the
   start and end of the pattern in some string data. */
struct pattern {

    /* Note, we store information identifying if the pattern
       is stored as either a single character or substring
       within the `i` index using bitfields. This might be
       slow? But we don't care that much about performance
       here anyways .. */
    int is_char : 1;

    /* By convention, if the pattern is stored as a single
       character, we store that character in `i`. */
    int i : 31;
    int j;
};


#define get_pattern_len(pat)                    \
    (assert ((pat)->i <= (pat)->j),             \
     (pat)->is_char ? 1 : (pat)->j - (pat)->i + 1)


static gif_bool
patterns_match (gif_byte const *str,
                struct pattern const *a,
                struct pattern const *b)
{
    gif_uint n;
    n = patlen (a);
    if (n != patlen (b)) {
        return false;
    }
    else if (a->is_char) {
        expect (b->is_char);
        return a->i == b->i;
    }
    else {
        gif_uint i = 0;
        for (i = 0; i < n; i++)
            if (str[a->i + i] != str[b->i + i])
                return false;
         return true;
    }
}


static gif_uint
hash (gif_byte const *str, struct pattern const *pat)
{
    gif_uint i, h;
    assert (!pat->is_char);
    h = 0;
    for (i = pat->i; i <= pat->j; i++)
        h = 37 * h + (gif_uint) str[i];
    return h;
}

/* -------------------------- gif encoding -------------------------- */

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
