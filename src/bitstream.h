/**
 *  Copyright (c) 2015, Russell
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-----------------------------------------------------------------------------
 * Minimal MSB-first bit stream writer and reader.
 *
 * Independent implementation (replaces the LGPL FFmpeg get_bits.h /
 * put_bits.h helpers formerly used by bits.c).  Bit-exact equivalent to the
 * replaced code within the range it was used, i.e. for field widths of
 * 1..13 bits and field values that fit in the field width:
 *
 *   - Bits are packed MSB-first: the first bit written ends up in bit 7 of
 *     byte 0, the next one in bit 6, and so on.
 *   - g729_put_bits() appends the low n bits of value, most significant of
 *     those n bits first.  Bits of value above bit n-1 are masked off.
 *     Precondition: 1 <= n <= 32.
 *   - g729_flush_bits() zero-pads the stream to the next byte boundary.
 *     Bits of the final byte that were never written read as zero; bytes
 *     past the flushed position are left untouched (provide a zeroed
 *     buffer if the whole output must be defined).
 *   - g729_get_bits() returns the next n bits as an unsigned integer whose
 *     most significant bit is the first bit read, and advances the read
 *     position by n.  Bits beyond the end of the buffer read as zero.
 *
 * All buffer accesses are byte-wise, so there are no alignment or byte
 * order assumptions.  The caller must provide a buffer large enough for the
 * bits it writes; writing past the end is silently dropped.
 *----------------------------------------------------------------------------
 */

#ifndef __G729_BITSTREAM_H__
#define __G729_BITSTREAM_H__

#include "g729a_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    G729_UWord8 *buf;        /* output buffer, one byte per element */
    unsigned int size_bits;  /* buffer capacity in bits             */
    unsigned int bit_pos;    /* index of the next bit to write      */
} G729_BitWriter;

typedef struct
{
    const G729_UWord8 *buf;  /* input buffer, one byte per element  */
    unsigned int size_bits;  /* buffer capacity in bits             */
    unsigned int bit_pos;    /* index of the next bit to read       */
} G729_BitReader;

/**
 *  @brief  Initialize a writer over a buffer of size_bytes bytes.
 */
static inline void g729_bit_writer_init(G729_BitWriter *bw, G729_UWord8 *buf,
                                        unsigned int size_bytes)
{
    bw->buf       = buf;
    bw->size_bits = size_bytes * 8u;
    bw->bit_pos   = 0;
}

/**
 *  @brief  Append the low n bits of value, most significant bit first.
 */
static inline void g729_put_bits(G729_BitWriter *bw, unsigned int n,
                                 G729_UWord32 value)
{
    while (n > 0 && bw->bit_pos < bw->size_bits)
    {
        G729_UWord8 mask = (G729_UWord8)(1u << (7u - (bw->bit_pos & 7u)));

        if ((value >> (n - 1u)) & 1u)
            bw->buf[bw->bit_pos >> 3] |= mask;
        else
            bw->buf[bw->bit_pos >> 3] &= (G729_UWord8)~mask;

        bw->bit_pos++;
        n--;
    }
}

/**
 *  @brief  Zero-pad the stream up to the next byte boundary.
 */
static inline void g729_flush_bits(G729_BitWriter *bw)
{
    while ((bw->bit_pos & 7u) != 0 && bw->bit_pos < bw->size_bits)
    {
        bw->buf[bw->bit_pos >> 3] &=
            (G729_UWord8)~(1u << (7u - (bw->bit_pos & 7u)));
        bw->bit_pos++;
    }
}

/**
 *  @brief  Initialize a reader over a buffer of size_bytes bytes.
 */
static inline void g729_bit_reader_init(G729_BitReader *br,
                                        const G729_UWord8 *buf,
                                        unsigned int size_bytes)
{
    br->buf       = buf;
    br->size_bits = size_bytes * 8u;
    br->bit_pos   = 0;
}

/**
 *  @brief  Read the next n bits, first bit read becomes the most
 *          significant bit of the result.
 */
static inline G729_UWord32 g729_get_bits(G729_BitReader *br, unsigned int n)
{
    G729_UWord32 value = 0;

    while (n > 0)
    {
        unsigned int bit = 0;

        if (br->bit_pos < br->size_bits)
            bit = (br->buf[br->bit_pos >> 3] >> (7u - (br->bit_pos & 7u))) & 1u;

        value = (value << 1) | bit;
        br->bit_pos++;
        n--;
    }
    return value;
}

#ifdef __cplusplus
}
#endif

#endif  /* __G729_BITSTREAM_H__ */
/* end of file */
