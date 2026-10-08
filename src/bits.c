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

/**
 *  Portions of this file are derived from the following ITU notice:
 *
 *  ITU-T G.729 Software Package Release 2 (November 2006)
 *
 *  ITU-T G.729A Speech Coder    ANSI-C Source Code
 *  Version 1.1    Last modified: September 1996
 *
 *  Copyright (c) 1996,
 *  AT&T, France Telecom, NTT, Universite de Sherbrooke
 *  All rights reserved.
 */

/*****************************************************************************/
/* bit stream manipulation routines                                          */
/*****************************************************************************/
#include "g729a_typedef.h"
#include "ld8a.h"
#include "tab_ld8a.h"

#include "dtx.h"
#include "octet.h"

#include "bitstream.h"

/* prototypes for local functions */
static void  int2bin(G729_Word16 value, G729_Word16 no_of_bits, G729_Word16 *bitstream);
static G729_Word16   bin2int(G729_Word16 no_of_bits, const G729_Word16 *bitstream);

/*----------------------------------------------------------------------------
 * g729_prm2bits_ld8k -converts encoder parameter vector into vector of serial bits
 * g729_bits2prm_ld8k - converts serial received bits to  encoder parameter vector
 *
 * The transmitted parameters are:
 *
 *     LPC:     1st codebook           7+1 bit
 *              2nd codebook           5+5 bit
 *
 *     1st subframe:
 *          pitch period                 8 bit
 *          parity check on 1st period   1 bit
 *          codebook index1 (positions) 13 bit
 *          codebook index2 (signs)      4 bit
 *          pitch and codebook gains   4+3 bit
 *
 *     2nd subframe:
 *          pitch period (relative)      5 bit
 *          codebook index1 (positions) 13 bit
 *          codebook index2 (signs)      4 bit
 *          pitch and codebook gains   4+3 bit
 *----------------------------------------------------------------------------
 */
void g729_prm2bits_ld8k(
        G729_Word16   prm[],           /* input : encoded parameters + frame type
                                          prm[0] = 1: voice, 2: SID, 0: not transmitted */
        G729_Word16 bits[]            /* output: serial bits, bits[0] = SYNC word,
                                         bits[1] = number of bits in this frame */
        )
{
    G729_Word16 i;
    *bits++ = SYNC_WORD;     /* bit[0], at receiver this bits indicates BFI */

    switch(prm[0]){

    case 1 : {
        *bits++ = RATE_8000;
        for (i = 0; i < PRM_SIZE; i++)
        {
            int2bin(prm[i+1], g729_bitsno[i], bits);
            bits += g729_bitsno[i];
        }
        break;
    }

    case 2 : {
        /* OCTET_TX_MODE: an extra zero bit is packed at the end of a SID
           bit stream (15 bits -> 16 bits) */
        *bits++ = RATE_SID_OCTET;
        for (i = 0; i < 4; i++)
        {
            int2bin(prm[i+1], g729_bitsno2[i], bits);
            bits += g729_bitsno2[i];
        }
        *bits = BIT_0;
        break;
    }

    /* not transmitted */
    default : {
        *bits = RATE_0;
        break;
    }

    }

    return;
}

/*----------------------------------------------------------------------------
 * int2bin convert integer to binary and write the bits bitstream array
 *----------------------------------------------------------------------------
 */
static void int2bin(
        G729_Word16 value,             /* input : decimal value         */
        G729_Word16 no_of_bits,        /* input : number of bits to use */
        G729_Word16 *bitstream         /* output: bitstream             */
        )
{
    G729_Word16 *pt_bitstream;
    G729_Word16   i, bit;

    pt_bitstream = bitstream + no_of_bits;

    for (i = 0; i < no_of_bits; i++)
    {
        bit = value & (G729_Word16)0x0001;      /* get lsb */
        if (bit == 0)
            *--pt_bitstream = BIT_0;
        else
            *--pt_bitstream = BIT_1;
        value >>= 1;
    }
}

/*----------------------------------------------------------------------------
 *  g729_bits2prm_ld8k - converts serial received bits to  encoder parameter vector
 *----------------------------------------------------------------------------
 */
void g729_bits2prm_ld8k(
        const G729_Word16 bits[],      /* input : serial bits, bits[0] = number of
                                          bits in this frame                       */
        G729_Word16   prm[]            /* output: prm[1] = frame type (1 voice,
                                          2 SID, 0 not transmitted), then the
                                          decoded parameters                     */
        )
{
    G729_Word16 i;
    G729_Word16 nb_bits;

    nb_bits = *bits++;        /* Number of bits in this frame       */

    if(nb_bits == RATE_8000) {
        prm[1] = 1;
        for (i = 0; i < PRM_SIZE; i++)
        {
            prm[i+2] = bin2int(g729_bitsno[i], bits);
            bits  += g729_bitsno[i];
        }
    }
    else if(nb_bits == RATE_SID_OCTET) {
        /* the last bit of the SID bit stream under octet mode is discarded */
        prm[1] = 2;
        for (i = 0; i < 4; i++)
        {
            prm[i+2] = bin2int(g729_bitsno2[i], bits);
            bits += g729_bitsno2[i];
        }
    }
    else {
        prm[1] = 0;
    }
    return;

}

/*----------------------------------------------------------------------------
 * bin2int - read specified bits from bit array  and convert to integer value
 *----------------------------------------------------------------------------
 */
static G729_Word16 bin2int(       /* output: decimal value of bit pattern */
        G729_Word16 no_of_bits,          /* input : number of bits to read       */
        const G729_Word16 *bitstream     /* input : array containing bits        */
        )
{
    G729_Word16   value, i;
    G729_Word16 bit;

    value = 0;
    for (i = 0; i < no_of_bits; i++)
    {
        value <<= 1;
        bit = *bitstream++;
        if (bit == BIT_1)  value += 1;
    }
    return(value);
}

G729_Word16 g729_prm2bits_ld8k_compressed(
    G729_Word16 prm[],            /* input : encoded parameters + frame type
                                     prm[0] = 1: voice, 2: SID, 0: not transmitted */
    G729_UWord8 bits[]            /* output: serial bits                        */
)
{
    G729_BitWriter bw;
    int i;

    switch(prm[0]){

    case 1 : {
        g729_bit_writer_init(&bw, bits, 10);  /* 80 bits = 10 bytes */
        for (i = 0; i < PRM_SIZE; ++i)
        {
            g729_put_bits(&bw, (unsigned int)g729_bitsno[i], (G729_UWord32)prm[i+1]);
        }
        g729_flush_bits(&bw);
        return 10;
    }

    case 2 : {
        /* OCTET_TX_MODE: 15 bits SID + 1 zero pad bit = 2 bytes */
        g729_bit_writer_init(&bw, bits, 2);
        for (i = 0; i < 4; ++i)
        {
            g729_put_bits(&bw, (unsigned int)g729_bitsno2[i], (G729_UWord32)prm[i+1]);
        }
        g729_flush_bits(&bw);
        return 2;
    }

    /* not transmitted */
    default : {
        return 0;
    }

    }
}

void g729_bits2prm_ld8k_compressed(
    const G729_UWord8  bits[],    /* input : serial bits                        */
    G729_Word16  prm[],           /* output: prm[1] = frame type (1 voice,
                                     2 SID, 0 not transmitted), then the
                                     decoded parameters                        */
    G729_Word16  nb_bytes         /* input : number of bytes in this frame      */
)
{
    G729_BitReader br;
    int i;

    if (nb_bytes == 10)
    {
        prm[1] = 1;
        g729_bit_reader_init(&br, bits, 10);
        for (i = 0; i < PRM_SIZE; ++i)
        {
            prm[i+2] = (G729_Word16)g729_get_bits(&br, (unsigned int)g729_bitsno[i]);
        }
    }
    else if (nb_bytes == 2)
    {
        /* the last bit of the SID bit stream under octet mode is discarded */
        prm[1] = 2;
        g729_bit_reader_init(&br, bits, 2);
        for (i = 0; i < 4; ++i)
        {
            prm[i+2] = (G729_Word16)g729_get_bits(&br, (unsigned int)g729_bitsno2[i]);
        }
    }
    else
    {
        prm[1] = 0;
    }
}
