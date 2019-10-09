/* Defines a simple gif encoder.
   See UNLICENSE for copyright notice. */

#include "gif.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>


#define GIF_U16_MAX     0xFFFF

#define gif_true        1
#define gif_false       0

typedef unsigned int    gif_u32;
typedef int             gif_bool;

GIF_STATIC_ASSERT (sizeof (gif_u32) == 4);


#ifndef min
#   define min(a, b)    ((a) < (b) ? (a) : (b))
#endif


struct gif {
    gif_u16 w, h;
    gif_u16 ti;

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
write_word (struct gif *gif, gif_u16 w)
{
    write_bytes (gif, &w, 2);
}


/* ------------------------- utility buffer ------------------------- */

struct buf { gif_u8 *ptr; size_t sz, cap, cur; };


/* Returns the amount of bytes left to read from the buffer. */
#define buf_left(buf)                       \
    ((buf)->sz - (buf)->cur)


/* Returns a pointer to the byte a position `i` in `buf`. `i` must be
   smaller than the buffer's size. */
#define buf_byte_at(buf, i)                 \
    (assert (i < (buf)->sz), buf->ptr + i)


/* Writes a value, `val`, of a specific type to `buf`. Resizes the buffer
   if needed. */
#define buf_write_t(buf, T, val)                                \
    do {                                                        \
        buf_resize ((buf), (buf)->sz + sizeof (T));             \
        *((T *) offset_ptr ((buf)->ptr, (buf)->cur)) = (val);   \
        (buf)->cur += sizeof (T);                               \
    } while (0)
#define buf_write_byte(buf, val)                                  \
    buf_write_t ((buf), gif_u8, (val))


/* Reads from `buf` an value of a specific type and stores it into `val`
   if successful. `val` is a *pointer* of type `T`. Returns 0 on failure
   and sizeof (`T`) if successful. */
#define buf_read_t(buf, T, val)                                 \
    (buf_left (buf) < sizeof (T))                               \
     ? 0 : (*(val) = *((T *) buf_byte_at (buf, buf->cur)),      \
           buf->cur += sizeof (T), sizeof (T))
#define buf_read_byte(buf, val)                                   \
    buf_read_t (buf, gif_u8, val)


/* Creates a new buffer and returns a pointer to it. `sz` is the size
   of the buffer in bytes, if `sz` is zero and empty buffer is created.
   `ptr` points to memory used to initialize the buffer, if `ptr` is
   not NULL `sz` bytes are copied from it. */
static struct buf *
buf_open (void const *ptr, size_t sz)
{
    void *ptr0 = NULL;
    struct buf *buf = NULL;
    size_t cap;

    cap = sz ? sz : 1;
    ptr0 = malloc (cap);
    buf = malloc (sizeof (*buf));
    if (ptr && sz)
        memcpy (ptr0, ptr, sz);
    buf->ptr = ptr0;
    buf->sz = sz;
    buf->cap = cap;
    buf->cur = 0;
    return buf;
}


/* Closes the buffer pointed to by `buf` and releases all its
   resources. */
static void
buf_close (struct buf *buf)
{
    if (!buf)
        return;
    free (buf->ptr);
    free (buf);
}


static void
buf_resize (struct buf *buf, size_t newsz)
{
    if (buf->cap < newsz) {
        void *newptr;
        size_t newcap;

        newcap = buf->cap;
        while (newcap < newsz)
            newcap *= 2;
        newptr = realloc (buf->ptr, newcap);
        buf->ptr = newptr;
        buf->cap = newcap;
    }

    buf->sz = newsz;
    buf->cur = min (buf->cur, newsz);
}

/* ----------------------------- bit io ----------------------------- */

/* Helper structure for reading and writing of bits to a buffer. */
struct bit_io {

    /* Buffer to write/read. */
    struct buf *buf;

    /* Bit cursor. Offset to buffer head in bits. */
    size_t bitcur;
};


/* Gets the remaining bits for the byte `bitcur` points to. I.e. the
   number of bits not yet written/read. */
static gif_u8
calc_rem_bits (size_t bitcur)
{
    return (gif_u8) ((((bitcur) + 7) / 8) * 8 - (bitcur));
}


/* Takes `n` bits from `b` starting at `fb` and puts them to `a` starting
   from bit `fa`. Returns the resulting combination. */
static gif_u8
put_bits (gif_u8 a, gif_u8 b, gif_u8 fa, gif_u8 fb, gif_u8 n)
{
    gif_u8 m;  /* mask */

    assert ((fa + n) <= 8);
    assert ((fb + n) <= 8);

    m = ((gif_u8) ((gif_u16) (1 << n)) - 1) << fa;
    return (a & ~m) | (((b >> fb) << fa) & m);
}


static void
init_bit_io (struct bit_io *io, struct buf *buf)
{
    io->buf = buf;
    io->bitcur = 8 * buf->cur;
}


/* Write the first `nbits` stored in the `u` to the buffer. `nbits`
   should be in 0..16. */
static void
write_bits (struct bit_io *io, gif_u16 u, gif_u8 nbits)
{
    assert (nbits <= 16);

    if (!nbits)
         return;

    if (io->bitcur & 7) { /* remaining bits is last written byte? */
        gif_u8 rem_bits, *cur_byte, n;

        rem_bits = calc_rem_bits (io->bitcur);
        cur_byte = buf_byte_at (io->buf, io->bitcur / 8);
        n = min (nbits, rem_bits);
        *cur_byte = put_bits (*cur_byte, u & 0xFF, 8 - rem_bits, 0, n);
        io->bitcur += n;
        nbits -= n;
        u >>= n;
    }
    while (nbits) {
        gif_u8 n, byte;

        n = min (nbits, 8);
        byte = put_bits (0, u & 0xFF, 0, 0, n);
        buf_write_byte (io->buf, byte);
        io->bitcur += n;
        nbits -= n;
        u >>= n;
    }
}


/* Reads `nbits` bits from `io` and stores the result in `u`. Returns
   the number of bits actually read. */
static gif_u8
read_bits (struct bit_io *io, gif_u16 *u, gif_u8 nbits)
{
    gif_u8 br = 0;

    assert (nbits <= 16);

    *u = 0;
    if (!nbits)
        return 0;

    if (io->bitcur & 7) { /* remaining bits is last read byte? */
        gif_u8 rem_bits, *cur_byte, n;

        rem_bits = calc_rem_bits (io->bitcur);
        cur_byte = buf_byte_at (io->buf, io->bitcur / 8);
        n = min (nbits, rem_bits);
        *u = put_bits ((*u) & 0xFF, *cur_byte, 0, 8 - rem_bits, n);
        io->bitcur += n;
        nbits -= n;
        br += n;
    }
    while (nbits) {
        gif_u8 n, byte;
        gif_u16 utmp;
        gif_u32 nbytes;

        n = min (nbits, 8);
        nbytes = buf_read_byte (io->buf, &byte);
        if (!nbytes)
            return br;
        utmp = (gif_u16) put_bits (0, byte, 0, 0, n);
        *u = (*u) | (utmp << br);
        io->bitcur += n;
        nbits -= n;
        br += n;
    }
    return br;
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
patterns_match (gif_u8 const *str, struct pattern const *a,
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
clear_dict (struct dict *dict)
{
    gif_u32 i;
    for (i = 0; i < HASH_SIZE; i++)
        dict->first[i] = NIL_ID;
    dict->nentries = 0;
}


static gif_u16
add_pattern (struct dict *dict, gif_u8 const *in,
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
get_code (struct dict *dict, gif_u8 const *str,
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
get_pattern (struct dict *dict, gif_u16 code, struct pattern *pat)
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

/* ------------------------ lzw transcoding ------------------------- */

/* Compresses `nbytes` pointed to by `bytes` and returns the compressed
   bytes as a buffer. The caller is responsible to free the returned
   buffer. */
static struct buf *
lzw_compress (gif_u8 const *bytes, size_t nbytes)
{
    struct bit_io io;
    struct dict *dict = NULL;
    struct buf *buf = NULL;
    struct pattern pat;
    gif_u8 codebits;

    assert (nbytes);

    codebits = ALPH_BIT_SIZE + 1;
    dict = (struct dict *) malloc (sizeof (*dict));
    buf = buf_open (NULL, 0);
    init_bit_io (&io, buf);
    clear_dict (dict);
    pat.is_char = 0;
    pat.i = pat.j = 0;

    while (gif_true) {
        gif_u16 code, last_code = GIF_U16_MAX, new_code;

        while (get_code (dict, bytes, &pat, &code)) {
            last_code = code;
            pat.j++;

            if (pat.j == nbytes) {
                write_bits (&io, code, codebits);
                goto done;
            }
        }

        assert (dict->nentries < MAX_CODE_COUNT);
        write_bits (&io, last_code, codebits);
        new_code = add_pattern (dict, bytes, &pat);

        if (new_code == MAX_CODE) {
            write_bits (&io, CLEAR_CODE, codebits);
            clear_dict (dict);
            codebits = ALPH_BIT_SIZE + 1;
        }

        if (new_code == (1 << codebits)) {
            /* If the next available code does not fit in the current
               code bit size, increase it here, because it *may* be
               the next code we have to write. */
            codebits++;
        }

        pat.i = pat.j;
    }

done:
    write_bits (&io, STOP_CODE, codebits);
    free (dict);
    return buf;
}


static struct buf *
lzw_decompress (struct buf *in)
{
    /* This pattern used to update the dictionary during decoding. */
    struct pattern pat_dict;
    struct pattern code_pat = { 0 };
    struct pattern prev_code_pat = { 0 };
    struct bit_io io;
    struct dict *dict = NULL;
    struct buf *out;
    gif_u8 codebits;

    out = buf_open (NULL, 0);
    codebits = ALPH_BIT_SIZE + 1;
    dict = (struct dict *) malloc (sizeof (*dict));
    init_bit_io (&io, in);
    clear_dict (dict);
    pat_dict.is_char = 0;
    pat_dict.i = pat_dict.j = 0;
    while (gif_true) {
        gif_u16 code;
        gif_u8 nbits;

        /* (1) Read the code and update the output. */

read_code:
        nbits = read_bits (&io, &code, codebits);
        /* TODO: also stop when we cannot read any more bits from the
           input buffer. */
        assert (nbits == codebits);
        if (code == STOP_CODE) {
            goto done;
        }
        else if (code == CLEAR_CODE) {
            clear_dict (dict);
            codebits = ALPH_BIT_SIZE + 1;
            pat_dict.is_char = 0;
            pat_dict.i = pat_dict.j = (gif_u32) out->sz;
            goto read_code;
        }
        else if (get_pattern (dict, code, &code_pat)) {
            /* If the code is in the dict write its pattern to
               the output. */

            if (code_pat.is_char) {
                buf_write_byte (out, (gif_u8) code_pat.i);
            }
            else {
                gif_u32 k = 0;
                for (k = code_pat.i; k <= code_pat.j; k++) {
                    gif_u8 b = *buf_byte_at (out, k);
                    buf_write_byte (out, b);
                }
            }
            prev_code_pat = code_pat;
        }
        else {
            /* Special case in which the pattern for the code has not
               been inserted in the decoders dictionary, in which case
               we need to emit the last pattern + the first character
               of the last pattern. */

            gif_u32 k = 0;

            /* TODO: Comment this... */
            code_pat.is_char = gif_false;
            code_pat.i = (gif_u32) out->sz;
            if (prev_code_pat.is_char) {
                buf_write_byte (out, prev_code_pat.i);
                buf_write_byte (out, prev_code_pat.i);
            }
            else {
                gif_u8 b;
                for (k = prev_code_pat.i; k <= prev_code_pat.j; k++) {
                    b = *buf_byte_at (out, k);
                    buf_write_byte(out, b);
                }
                b = *buf_byte_at (out, prev_code_pat.i);
                buf_write_byte (out, b);
            }
            code_pat.j = (gif_u32) (out->sz - 1);
            prev_code_pat = code_pat;
        }

        /* (2) Having updated the output string, we need to search for
           new pattern and update the dictionary if we find one. */

        while (gif_true) {
            gif_u16 code;

            if (!get_code (dict, out->ptr, &pat_dict, &code)) {
                /* Found a new pattern. Add it to the dictionary
                   and stop for now. */

                gif_u16 new_code;

                new_code = add_pattern (dict, out->ptr, &pat_dict);
                if (new_code == ((1 << codebits) - 1)) {
                    /* The code for the added pattern is the last code for
                       the current code bit size. The next code could
                       represent a pattern which is not in the dictionary
                       yet, i.e. it does not fit into the current code
                       bit size. Hence, increase the bit size here. */
                    codebits++;
                }
                pat_dict.i = pat_dict.j;
                break; /* TODO: Delete this break statement? */
            }
            else if (pat_dict.j == (out->sz - 1)) {
                /* No new pattern found. Stop for now. */
                break;
            }
            else {
                pat_dict.j++;
            }
        }
    }

done:
    free (dict);
    return out;
}

/* -------------------------- gif encoding -------------------------- */

struct gif *
gif_begin (struct gif_desc const *desc)
{
    struct gif *gif;

    gif = (struct gif *) malloc (sizeof (*gif));
    gif->write_fn = desc->write_fn;
    gif->write_ud = desc->write_ud;
    gif->w = desc->w;
    gif->h = desc->h;
    gif->ti = desc->ti;

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


/* Writes `nbytes` of memory pointed to by `bytes` as a sequence of
   blocks of max. 255 bytes to the buffer. We use this function to
   write the compressed color indices of the gif's image data. */
static void
write_sub_blocks (struct gif *gif, gif_u8 const *bytes, size_t nbytes)
{
    /* Write LZW minimum code size. */
    write_byte (gif, 8);

    /* Write sequence of sub blocks. */
    while (nbytes) {
        gif_u8 n = (gif_u8) min (nbytes, 255);
        write_byte (gif, n);  /* block size */
        write_bytes (gif, bytes, n);
        bytes += n;
        nbytes -= n;
    }

    /* Write block terminator. */
    write_byte (gif, 0);
}


/* Writes `n` color indices, `cols`, of a gif image to the buffer. */
static void
write_color_indices (struct gif *gif, gif_u8 *cols, size_t n)
{
    struct buf *compr_cols = NULL;
    compr_cols = lzw_compress (cols, n);
    write_sub_blocks (gif, compr_cols->ptr, compr_cols->sz);
    buf_close (compr_cols);
}


void
gif_add_frame (struct gif *gif, struct gif_frame const *frm)
{
    /* Graphic control extension */
    write_byte (gif, 0x21);
    write_byte (gif, 0xf9);
    write_byte (gif, 4);
    write_byte (gif, 0);
    write_word (gif, gif->ti);
    write_byte (gif, 0);
    write_byte (gif, 0);

    /* Write image descriptor */
    write_byte (gif, 0x2c);
    write_word (gif, 0);
    write_word (gif, 0);
    write_word (gif, gif->w);
    write_word (gif, gif->h);

    if (frm->lct) {
        write_byte (gif, 0x83);
        write_bytes (gif, frm->lct, 256 * 3);
    }
    else {
        write_byte (gif, 0x00);
    }

    write_color_indices (gif, frm->cols, gif->w * gif->h);
}


void
gif_end (struct gif **gif)
{
    write_byte (*gif, 0x3b);
    free (*gif);
    *gif = NULL;
}


/* --------------------------- lzw tests ---------------------------- */

#ifdef GIF_LZW_TESTS


int
main (int argc, char **argv)
{
    return 0;
}


#endif
