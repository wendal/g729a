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
 *  Version 1.4    Last modified: November 2000
 *
 *  Copyright (c) 1996,
 *  France Telecom, Rockwell International, Universite de Sherbrooke
 *  All rights reserved.
 */

/*
**
** File:            "dec_sid.c"
**
** Description:     Comfort noise generation
**                  performed at the decoder part
**
*/
/**** Fixed point version ***/

#include "g729a_typedef.h"
#include "basic_op.h"
#include "ld8a.h"
#include "tab_ld8a.h"

#include "vad.h"
#include "dtx.h"
#include "sid.h"
#include "tab_dtx.h"

#include "g729a_decoder.h"

/* Initial value for lspSid (ITU static initializer) */
static const G729_Word16 init_lspSid[M] = {
  31441,  27566,  21458,  13612,   4663,
  -4663, -13612, -21458, -27566, -31441};

/*
**
** Function:        g729_Init_Dec_cng()
**
** Description:     Initialize dec_cng state variables
**
**
*/
void g729_Init_Dec_cng(g729a_cng_state * state)
{

  state->cur_gain = 0;
  g729_Copy(init_lspSid, state->lspSid, M);
  state->sid_gain = g729_tab_Sidgain[0];

  /* Initialize the noise LSF predictor coefficients */
  g729_Init_lsfq_noise(state->noise_fg);

  return;
}

/*-----------------------------------------------------------*
 * procedure g729_Dec_cng:                                   *
 *           ~~~~~~~~~~~~                                    *
 *                     Receives frame type                   *
 *                     0  :  for untransmitted frames        *
 *                     2  :  for SID frames                  *
 *                     Decodes SID frames                    *
 *                     Computes current frame excitation     *
 *                     Computes current frame LSPs
 *-----------------------------------------------------------*/
void g729_Dec_cng(
  g729a_cng_state * state,
  G729_Word16 past_ftyp,     /* (i)   : past frame type                      */
  G729_Word16 sid_sav,       /* (i)   : energy to recover SID gain           */
  G729_Word16 sh_sid_sav,    /* (i)   : corresponding scaling factor         */
  G729_Word16 *parm,         /* (i)   : coded SID parameters                 */
  G729_Word16 *exc,          /* (i/o) : excitation array                     */
  G729_Word16 *lsp_old,      /* (i/o) : previous lsp                         */
  G729_Word16 *A_t,          /* (o)   : set of interpolated LPC coefficients */
  G729_Word16 *seed,         /* (i/o) : random generator seed                */
  G729_Word16 freq_prev[MA_NP][M]
                        /* (i/o) : previous LPS for quantization        */
)
{
  G729_Word16 temp, ind;
  G729_Word16 dif;

  dif = g729_sub(past_ftyp, 1);

  /* SID Frame */
  /*************/
  if(parm[0] != 0) {

    state->sid_gain = g729_tab_Sidgain[(int)parm[4]];

    /* Inverse quantization of the LSP */
    g729_sid_lsfq_decode(state, &parm[1], state->lspSid, freq_prev);

  }

  /* non SID Frame */
  /*****************/
  else {

    /* Case of 1st SID frame erased : quantize-decode   */
    /* energy estimate stored in sid_gain         */
    if(dif == 0) {
      g729_Qua_Sidgain(&sid_sav, &sh_sid_sav, 0, &temp, &ind);
      state->sid_gain = g729_tab_Sidgain[(int)ind];
    }

  }

  if(dif == 0) {
    state->cur_gain = state->sid_gain;
  }
  else {
    state->cur_gain = g729_mult_r(state->cur_gain, A_GAIN0);
    state->cur_gain = g729_add(state->cur_gain, g729_mult_r(state->sid_gain, A_GAIN1));
  }

  g729_Calc_exc_rand(state->cur_gain, exc, seed, FLAG_DEC, 0);

  /* Interpolate the Lsp vectors */
  g729_Int_qlpc(lsp_old, state->lspSid, A_t);
  g729_Copy(state->lspSid, lsp_old, M);

  return;
}


void g729_sid_lsfq_decode(
                     g729a_cng_state * state, /* (i) : CNG state (noise_fg)  */
                     G729_Word16 *index,             /* (i) : quantized indices    */
                     G729_Word16 *lspq,              /* (o) : quantized lsp vector */
                     G729_Word16 freq_prev[MA_NP][M] /* (i) : memory of predictor  */
                     )
{
  G729_Word32 acc0;
  G729_Word16 i, j, k, lsfq[M], tmpbuf[M];

  /* get the lsf error vector */
  g729_Copy(g729_lspcb1[g729_PtrTab_1[index[1]]], tmpbuf, M);
  for (i=0; i<M/2; i++)
    tmpbuf[i] = g729_add(tmpbuf[i], g729_lspcb2[g729_PtrTab_2[0][index[2]]][i]);
  for (i=M/2; i<M; i++)
    tmpbuf[i] = g729_add(tmpbuf[i], g729_lspcb2[g729_PtrTab_2[1][index[2]]][i]);

  /* guarantee minimum distance of 0.0012 (~10 in Q13) between tmpbuf[j]
     and tmpbuf[j+1] */
  for (j=1; j<M; j++){
    acc0 = g729_L_mult(tmpbuf[j-1], 16384);
    acc0 = g729_L_mac(acc0, tmpbuf[j], -16384);
    acc0 = g729_L_mac(acc0, 10, 16384);
    k = g729_extract_h(acc0);

    if (k > 0){
      tmpbuf[j-1] = g729_sub(tmpbuf[j-1], k);
      tmpbuf[j] = g729_add(tmpbuf[j], k);
    }
  }

  /* compute the quantized lsf vector */
  g729_Lsp_prev_compose(tmpbuf, lsfq, state->noise_fg[index[0]], freq_prev,
                   g729_noise_fg_sum[index[0]]);

  /* update the prediction memory */
  g729_Lsp_prev_update(tmpbuf, freq_prev);

  /* lsf stability check */
  g729_Lsp_stability(lsfq);

  /* convert lsf to lsp */
  g729_Lsf_lsp2(lsfq, lspq, M);

}
