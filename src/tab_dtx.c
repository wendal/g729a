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
 *  ITU-T G.729A Annex B     ANSI-C Source Code
 *  Version 1.3    Last modified: August 1997
 *
 *  Copyright (c) 1996,
 *  France Telecom, Rockwell International, Universite de Sherbrooke
 *  All rights reserved.
 */

/*********************************************************************/
/******             Tables used for VAD/DTX/CNG                 ******/
/*********************************************************************/

#include "g729a_typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "ld8a.h"
#include "tab_ld8a.h"

#include "vad.h"
#include "dtx.h"
#include "tab_dtx.h"

#include "g729a_encoder.h"

/* VAD constants */
const G729_Word16 g729_lbf_corr[NP+1] = {
  7869, 7011, 4838, 2299, 321, -660, -782, -484, -164, 3, 39, 21, 4};
const G729_Word16 g729_shift_fx[33] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                         2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5,0};
const G729_Word16 g729_factor_fx[33] = {32767, 16913, 17476, 18079, 18725, 19418,
                          20165, 20972, 21845, 22795, 23831, 24966,
                          26214, 27594, 29127, 30840, 32767, 17476,
                          18725, 20165, 21845, 23831, 26214, 29127,
                          32767, 18725, 21845, 26214, 32767, 21845,
                          32767, 32767,0};

/* Quantization of SID gain */
const G729_Word16 g729_fact[NB_GAIN+1] = {410, 26, 13};
const G729_Word16 g729_marg[NB_GAIN+1] = {0, 0, 1};
const G729_Word16 g729_tab_Sidgain[32] = {
    2,    5,    8,   13,   20,   32,   50,   64,
   80,  101,  127,  160,  201,  253,  318,  401,
  505,  635,  800, 1007, 1268, 1596, 2010, 2530,
 3185, 4009, 5048, 6355, 8000,10071,12679,15962 };

/* Quantization of LSF vector */
const G729_Word16 g729_noise_fg_sum[MODE][M] = {
  {7798, 8447, 8205, 8293, 8126, 8477, 8447, 8703, 9043, 8604},
  {10514, 12402, 12833, 11914, 11447, 11670, 11132, 11311, 11844, 11447}
};

const G729_Word16 g729_noise_fg_sum_inv[MODE][M] = {
  {17210, 15888, 16357, 16183, 16516, 15833, 15888, 15421, 14840, 15597},
  {12764, 10821, 10458, 11264, 11724, 11500, 12056, 11865, 11331, 11724}
};

const G729_Word16 g729_PtrTab_1[32] = {96,52,20,54,86,114,82,68,36,121,48,92,18,120,
                         94,124,50,125,4,100,28,76,12,117,81,22,90,116,
                         127,21,108,66};
const G729_Word16 g729_PtrTab_2[2][16]= {{31,21,9,3,10,2,19,26,4,3,11,29,15,27,21,12},
                           {16,1,0,0,8,25,22,20,19,23,20,31,4,31,20,31}};

const G729_Word16 g729_Mp[MODE] = {8644, 16572};

/*---------------------------------------------------------------------------*
 * Function  g729_Init_lsfq_noise                                            *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~                                                 *
 *                                                                           *
 * -> Initialization of variables for the lsf quantization in the SID        *
 *                                                                           *
 * The ITU reference keeps noise_fg[][][] as a run-time-initialized global   *
 * table (Init_lsfq_noise() in dec_sid.c). Here the table is per-instance    *
 * state; this function fills the caller-provided array.                     *
 *                                                                           *
 *---------------------------------------------------------------------------*/
void g729_Init_lsfq_noise(G729_Word16 noise_fg[MODE][MA_NP][M])
{
    G729_Word16 i, j;
    G729_Word32 acc0;

    /* initialize the noise_fg */
    for (i=0; i<4; i++)
        g729_Copy(g729_fg[0][i], noise_fg[0][i], M);

    for (i=0; i<4; i++)
        for (j=0; j<M; j++){
            acc0 = g729_L_mult(g729_fg[0][i][j], 19660);
            acc0 = g729_L_mac(acc0, g729_fg[1][i][j], 13107);
            noise_fg[1][i][j] = g729_extract_h(acc0);
        }
}
