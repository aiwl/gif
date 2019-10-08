#include "gif.h"

#include <stdlib.h>
#include <assert.h>


#define GIF_U16_MAX     0xFFFF


typedef unsigned int    gif_u32;
typedef int             gif_bool;


#define gif_true        1
#define gif_false       0


struct gif {
    void *write_ud;
    gif_write write_fn;
};


static void
write_bytes (struct gif *gif, void const *ptr, size_t sz)
{
    gif->write_fn (gif->write_ud, ptr, sz);
}


static void
write_byte (struct gif *gif, gif_u8 b)
{
    write_bytes (gif, &b, 1);
}


static void
write_word (struct gif *gif, gif_u16  w)
{
    write_bytes (gif, &w, 2);
}

/* ---------------------- dictionary patterns -----------------------
   Definitions for a dictionary which is used to map pattern to codes
   and vice versa. */

#define NIL_ID              GIF_U16_MAX
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
    gif_u32 is_char : 1;

    /* By convention, if the pattern is stored as a single
       character, we store that character in `i`. */
    gif_u32 i : 31;
    gif_u32 j;
};



static gif_u32
get_pattern_length (struct pattern const *pat)
{
    assert (pat->i <= pat->j);
    return pat->is_char ? 1 : pat->j - pat->i + 1;
}


static gif_bool
patterns_match (gif_u8 const *str,
                struct pattern const *a,
                struct pattern const *b)
{
    gif_u32 n;
    n = get_pattern_length (a);
    if (n != get_pattern_length (b)) {
        return gif_false;
    }
    else if (a->is_char) {
        assert (b->is_char);
        return a->i == b->i;
    }
    else {
        gif_u32 i = 0;
        for (i = 0; i < n; i++)
            if (str[a->i + i] != str[b->i + i])
                return gif_false;
         return gif_true;
    }
}


static gif_u32
hash (gif_u8 const *str, struct pattern const *pat)
{
    gif_u32 i, h;
    assert (!pat->is_char);
    h = 0;
    for (i = pat->i; i <= pat->j; i++)
        h = 37 * h + (gif_u32) str[i];
    return h;
}


struct entry {
    struct pattern pat;
    gif_u16 next;
};


struct dict {
    gif_u16 first[HASH_SIZE];
    struct entry entries[MAX_CODE_COUNT];
    gif_u32 nentries;
};


/* Converts a identifier of a pattern within the dictionary
   to the actual LZW code for that pattern. */
#define dict_id_to_code(id)             \
    ((id) + FIRST_COMPR_CODE)
#define code_to_dict_id(code)           \
    (assert (code >= FIRST_COMPR_CODE), \
     (code) - FIRST_COMPR_CODE)


static struct entry
make_entry (struct pattern const *pat, gif_u16 next)
{
    struct entry e;
    e.pat = *pat;
    e.next = next;
    return e;
}


static void
dict_clear (struct dict *dict)
{
    gif_u32 i;
    for (i = 0; i < HASH_SIZE; i++)
        dict->first[i] = NIL_ID;
    dict->nentries = 0;
}


static gif_u16
dict_add_pattern (struct dict *dict, gif_u8 const *in,
                  struct pattern const *pat)
{
    gif_u32 h;
    struct entry e;

    if (pat->is_char)
        return (gif_u16) pat->i;

    h = hash (in, pat) % HASH_SIZE;
    e = make_entry (pat, dict->first[h]);
    dict->entries[dict->nentries] = e;
    dict->first[h] = dict->nentries++;
    return FIRST_COMPR_CODE + dict->nentries - 1;
}


static gif_bool
dict_get_code (struct dict *dict, gif_u8 const *str,
               struct pattern const *pat, gif_u16 *code)
{
    if (get_pattern_length (pat) == 1) {
        *code = (gif_u16) str[pat->i];
        return gif_true;
    }
    else {
        gif_u32 h;
        gif_u16 id;

        h = hash (str, pat) % HASH_SIZE;
        id = dict->first[h];
        while (id != NIL_ID) {
            struct entry *e;

            e = dict->entries + id;
            if (patterns_match (str, pat, &e->pat)) {
                *code = dict_id_to_code (id);
                return gif_true;
            }
            id = e->next;
        }
        return gif_false;
    }
}


static gif_bool
dict_get_pattern (struct dict *dict, gif_u16 code, struct pattern *pat)
{
    gif_u16 id;

    if (code < CLEAR_CODE) {
        pat->is_char = 1;
        pat->i = (gif_u32) code;
        return gif_true;
    }
    id = code_to_dict_id (code);
    if (dict->nentries <= id) {
        return gif_false;
    }
    else {
        *pat = dict->entries[id].pat;
        return gif_true;
    }
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
gif_add_frame (struct gif const *gif, struct gif_frame const *f)
{

}


void
gif_end (struct gif **gif)
{
    write_byte (*gif, 0x3b);
    free (*gif);
    *gif = NULL;
}
