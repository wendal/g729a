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

/*--------------------------------------------------------------------------*
 * VAD.C                                                                    *
 * ~~~~~                                                                    *
 * Voice Activity Detection (G.729 Annex B).                                *
 *--------------------------------------------------------------------------*/

#include "g729a_typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "ld8a.h"
#include "tab_ld8a.h"

#include "vad.h"
#include "dtx.h"
#include "tab_dtx.h"

#include "g729a_encoder.h"

/* local function */
static G729_Word16 MakeDec(
               G729_Word16 dSLE,    /* (i)  : differential low band energy */
               G729_Word16 dSE,     /* (i)  : differential full band energy */
               G729_Word16 SD,      /* (i)  : differential spectral distortion */
               G729_Word16 dSZC     /* (i)  : differential zero crossing rate */
);

/*---------------------------------------------------------------------------*
 * Function  g729_Init_Vad                                                   *
 * ~~~~~~~~~~~~~~~~~~                                                        *
 *                                                                           *
 * -> Initialization of variables for voice activity detection               *
 *                                                                           *
 *---------------------------------------------------------------------------*/
void g729_Init_Vad(g729a_vad_state * state)
{
    /* Static vectors to zero */
    g729_Set_zero(state->MeanLSF, M);
    g729_Set_zero(state->Min_buffer, 16);

    /* Initialize VAD parameters */
    state->MeanSE = 0;
    state->MeanSLE = 0;
    state->MeanE = 0;
    state->MeanSZC = 0;
    state->count_sil = 0;
    state->count_update = 0;
    state->count_ext = 0;
    state->less_count = 0;
    state->flag = 1;
    state->Min = G729A_MAX_16;
    state->Prev_Min = 0;
    state->Next_Min = 0;
    state->prev_energy = 0;
    state->v_flag = 0;
}


/*-----------------------------------------------------------------*
 * Functions g729_Vad                                              *
 *           ~~~~~~~~                                              *
 * Input:                                                          *
 *   rc            : reflection coefficient                        *
 *   lsf[]         : unquantized lsf vector                        *
 *   r_h[]         : upper 16-bits of the autocorrelation vector   *
 *   r_l[]         : lower 16-bits of the autocorrelation vector   *
 *   exp_R0        : exponent of the autocorrelation vector        *
 *   sigpp[]       : preprocessed input signal                     *
 *   frm_count     : frame counter                                 *
 *   prev_marker   : VAD decision of the last frame                *
 *   pprev_marker  : VAD decision of the frame before last frame   *
 *                                                                 *
 * Output:                                                         *
 *                                                                 *
 *   marker        : VAD decision of the current frame             *
 *                                                                 *
 *-----------------------------------------------------------------*/
void g729_Vad(
         g729a_vad_state * state,
         G729_Word16 rc,
         G729_Word16 *lsf,
         G729_Word16 *r_h,
         G729_Word16 *r_l,
         G729_Word16 exp_R0,
         G729_Word16 *sigpp,
         G729_Word16 frm_count,
         G729_Word16 prev_marker,
         G729_Word16 pprev_marker,
         G729_Word16 *marker)
{
 /* scalar */
  G729_Word32 acc0;
  G729_Word16 i, j, exp, frac;
  G729_Word16 ENERGY, ENERGY_low, SD, ZC, dSE, dSLE, dSZC;
  G729_Word16 COEF, C_COEF, COEFZC, C_COEFZC, COEFSD, C_COEFSD;

  /* compute the frame energy */
  acc0 = g729_L_Comp(r_h[0], r_l[0]);
  g729_Log2(acc0, &exp, &frac);
  acc0 = g729_Mpy_32_16(exp, frac, 9864);
  i = g729_sub(exp_R0, 1);
  i = g729_sub(i, 1);
  acc0 = g729_L_mac(acc0, 9864, i);
  acc0 = g729_L_shl(acc0, 11);
  ENERGY = g729_extract_h(acc0);
  ENERGY = g729_sub(ENERGY, 4875);

  /* compute the low band energy */
  acc0 = 0;
  for (i=1; i<=NP; i++)
    acc0 = g729_L_mac(acc0, r_h[i], g729_lbf_corr[i]);
  acc0 = g729_L_shl(acc0, 1);
  acc0 = g729_L_mac(acc0, r_h[0], g729_lbf_corr[0]);
  g729_Log2(acc0, &exp, &frac);
  acc0 = g729_Mpy_32_16(exp, frac, 9864);
  i = g729_sub(exp_R0, 1);
  i = g729_sub(i, 1);
  acc0 = g729_L_mac(acc0, 9864, i);
  acc0 = g729_L_shl(acc0, 11);
  ENERGY_low = g729_extract_h(acc0);
  ENERGY_low = g729_sub(ENERGY_low, 4875);

  /* compute SD */
  acc0 = 0;
  for (i=0; i<M; i++){
    j = g729_sub(lsf[i], state->MeanLSF[i]);
    acc0 = g729_L_mac(acc0, j, j);
  }
  SD = g729_extract_h(acc0);      /* Q15 */

  /* compute # zero crossing */
  ZC = 0;
  for (i=ZC_START+1; i<=ZC_END; i++)
    if (g729_mult(sigpp[i-1], sigpp[i]) < 0)
      ZC = g729_add(ZC, 410);     /* Q15 */

  /* Initialize and update Mins */
  if(g729_sub(frm_count, 129) < 0){
    if (g729_sub(ENERGY, state->Min) < 0){
      state->Min = ENERGY;
      state->Prev_Min = ENERGY;
    }

    if((frm_count & 0x0007) == 0){
      i = g729_sub(g729_shr(frm_count,3),1);
      state->Min_buffer[i] = state->Min;
      state->Min = G729A_MAX_16;
    }
  }

  if((frm_count & 0x0007) == 0){
    state->Prev_Min = state->Min_buffer[0];
    for (i=1; i<16; i++){
      if (g729_sub(state->Min_buffer[i], state->Prev_Min) < 0)
        state->Prev_Min = state->Min_buffer[i];
    }
  }

  if(g729_sub(frm_count, 129) >= 0){
    if(((frm_count & 0x0007) ^ (0x0001)) == 0){
      state->Min = state->Prev_Min;
      state->Next_Min = G729A_MAX_16;
    }
    if (g729_sub(ENERGY, state->Min) < 0)
      state->Min = ENERGY;
    if (g729_sub(ENERGY, state->Next_Min) < 0)
      state->Next_Min = ENERGY;

    if((frm_count & 0x0007) == 0){
      for (i=0; i<15; i++)
        state->Min_buffer[i] = state->Min_buffer[i+1];
      state->Min_buffer[15] = state->Next_Min;
      state->Prev_Min = state->Min_buffer[0];
      for (i=1; i<16; i++)
        if (g729_sub(state->Min_buffer[i], state->Prev_Min) < 0)
          state->Prev_Min = state->Min_buffer[i];
    }

  }

  if (g729_sub(frm_count, INIT_FRAME) <= 0)
  {
    if(g729_sub(ENERGY, 3072) < 0){
      *marker = NOISE;
      state->less_count++;
    }
    else{
      *marker = VOICE;
      acc0 = g729_L_deposit_h(state->MeanE);
      acc0 = g729_L_mac(acc0, ENERGY, 1024);
      state->MeanE = g729_extract_h(acc0);
      acc0 = g729_L_deposit_h(state->MeanSZC);
      acc0 = g729_L_mac(acc0, ZC, 1024);
      state->MeanSZC = g729_extract_h(acc0);
      for (i=0; i<M; i++){
        acc0 = g729_L_deposit_h(state->MeanLSF[i]);
        acc0 = g729_L_mac(acc0, lsf[i], 1024);
        state->MeanLSF[i] = g729_extract_h(acc0);
      }
    }
  }

  if (g729_sub(frm_count, INIT_FRAME) >= 0){
    if (g729_sub(frm_count, INIT_FRAME) == 0){
      acc0 = g729_L_mult(state->MeanE, g729_factor_fx[state->less_count]);
      acc0 = g729_L_shl(acc0, g729_shift_fx[state->less_count]);
      state->MeanE = g729_extract_h(acc0);

      acc0 = g729_L_mult(state->MeanSZC, g729_factor_fx[state->less_count]);
      acc0 = g729_L_shl(acc0, g729_shift_fx[state->less_count]);
      state->MeanSZC = g729_extract_h(acc0);

      for (i=0; i<M; i++){
        acc0 = g729_L_mult(state->MeanLSF[i], g729_factor_fx[state->less_count]);
        acc0 = g729_L_shl(acc0, g729_shift_fx[state->less_count]);
        state->MeanLSF[i] = g729_extract_h(acc0);
      }

      state->MeanSE = g729_sub(state->MeanE, 2048);   /* Q11 */
      state->MeanSLE = g729_sub(state->MeanE, 2458);  /* Q11 */
    }

    dSE = g729_sub(state->MeanSE, ENERGY);
    dSLE = g729_sub(state->MeanSLE, ENERGY_low);
    dSZC = g729_sub(state->MeanSZC, ZC);

    if(g729_sub(ENERGY, 3072) < 0)
      *marker = NOISE;
    else
      *marker = MakeDec(dSLE, dSE, SD, dSZC);

    state->v_flag = 0;
    if((prev_marker==VOICE) && (*marker==NOISE) && (g729_add(dSE,410) < 0)
       && (g729_sub(ENERGY, 3072)>0)){
      *marker = VOICE;
      state->v_flag = 1;
    }

    if(state->flag == 1){
      if((pprev_marker == VOICE) &&
         (prev_marker == VOICE) &&
         (*marker == NOISE) &&
         (g729_sub(g729_abs_s(g729_sub(state->prev_energy,ENERGY)), 614) <= 0)){
        state->count_ext++;
        *marker = VOICE;
        state->v_flag = 1;
        if(g729_sub(state->count_ext, 4) <= 0)
          state->flag=1;
        else{
          state->count_ext=0;
          state->flag=0;
        }
      }
    }
    else
      state->flag=1;

    if(*marker == NOISE)
      state->count_sil++;

    if((*marker == VOICE) && (g729_sub(state->count_sil, 10) > 0) &&
       (g729_sub(g729_sub(ENERGY,state->prev_energy), 614) <= 0)){
      *marker = NOISE;
      state->count_sil=0;
    }

    if(*marker == VOICE)
      state->count_sil=0;

    if ((g729_sub(g729_sub(ENERGY, 614), state->MeanSE) < 0) && (g729_sub(frm_count, 128) > 0)
        && (!state->v_flag) && (g729_sub(rc, 19661) < 0))
      *marker = NOISE;

    if ((g729_sub(g729_sub(ENERGY,614),state->MeanSE) < 0) && (g729_sub(rc, 24576) < 0)
        && (g729_sub(SD, 83) < 0)){
      state->count_update++;
      if (g729_sub(state->count_update, INIT_COUNT) < 0){
        COEF = 24576;
        C_COEF = 8192;
        COEFZC = 26214;
        C_COEFZC = 6554;
        COEFSD = 19661;
        C_COEFSD = 13017;
      }
      else
        if (g729_sub(state->count_update, INIT_COUNT+10) < 0){
          COEF = 31130;
          C_COEF = 1638;
          COEFZC = 30147;
          C_COEFZC = 2621;
          COEFSD = 21299;
          C_COEFSD = 11469;
        }
        else
          if (g729_sub(state->count_update, INIT_COUNT+20) < 0){
            COEF = 31785;
            C_COEF = 983;
            COEFZC = 30802;
            C_COEFZC = 1966;
            COEFSD = 22938;
            C_COEFSD = 9830;
          }
          else
            if (g729_sub(state->count_update, INIT_COUNT+30) < 0){
              COEF = 32440;
              C_COEF = 328;
              COEFZC = 31457;
              C_COEFZC = 1311;
              COEFSD = 24576;
              C_COEFSD = 8192;
            }
            else
              if (g729_sub(state->count_update, INIT_COUNT+40) < 0){
                COEF = 32604;
                C_COEF = 164;
                COEFZC = 32440;
                C_COEFZC = 328;
                COEFSD = 24576;
                C_COEFSD = 8192;
              }
              else{
                COEF = 32604;
                C_COEF = 164;
                COEFZC = 32702;
                C_COEFZC = 66;
                COEFSD = 24576;
                C_COEFSD = 8192;
              }


      /* compute MeanSE */
      acc0 = g729_L_mult(COEF, state->MeanSE);
      acc0 = g729_L_mac(acc0, C_COEF, ENERGY);
      state->MeanSE = g729_extract_h(acc0);

      /* compute MeanSLE */
      acc0 = g729_L_mult(COEF, state->MeanSLE);
      acc0 = g729_L_mac(acc0, C_COEF, ENERGY_low);
      state->MeanSLE = g729_extract_h(acc0);

      /* compute MeanSZC */
      acc0 = g729_L_mult(COEFZC, state->MeanSZC);
      acc0 = g729_L_mac(acc0, C_COEFZC, ZC);
      state->MeanSZC = g729_extract_h(acc0);

      /* compute MeanLSF */
      for (i=0; i<M; i++){
        acc0 = g729_L_mult(COEFSD, state->MeanLSF[i]);
        acc0 = g729_L_mac(acc0, C_COEFSD, lsf[i]);
        state->MeanLSF[i] = g729_extract_h(acc0);
      }
    }

    if((g729_sub(frm_count, 128) > 0) && (((g729_sub(state->MeanSE,state->Min) < 0) &&
                        (g729_sub(SD, 83) < 0)) || (g729_sub(state->MeanSE,state->Min) > 2048))){
      state->MeanSE = state->Min;
      state->count_update = 0;
    }
  }

  state->prev_energy = ENERGY;

}

/* local function */
static G729_Word16 MakeDec(
               G729_Word16 dSLE,    /* (i)  : differential low band energy */
               G729_Word16 dSE,     /* (i)  : differential full band energy */
               G729_Word16 SD,      /* (i)  : differential spectral distortion */
               G729_Word16 dSZC     /* (i)  : differential zero crossing rate */
               )
{
  G729_Word32 acc0;

  /* SD vs dSZC */
  acc0 = g729_L_mult(dSZC, -14680);          /* Q15*Q23*2 = Q39 */
  acc0 = g729_L_mac(acc0, 8192, -28521);     /* Q15*Q23*2 = Q39 */
  acc0 = g729_L_shr(acc0, 8);                /* Q39 -> Q31 */
  acc0 = g729_L_add(acc0, g729_L_deposit_h(SD));
  if (acc0 > 0) return(VOICE);

  acc0 = g729_L_mult(dSZC, 19065);           /* Q15*Q22*2 = Q38 */
  acc0 = g729_L_mac(acc0, 8192, -19446);     /* Q15*Q22*2 = Q38 */
  acc0 = g729_L_shr(acc0, 7);                /* Q38 -> Q31 */
  acc0 = g729_L_add(acc0, g729_L_deposit_h(SD));
  if (acc0 > 0) return(VOICE);

  /* dSE vs dSZC */
  acc0 = g729_L_mult(dSZC, 20480);           /* Q15*Q13*2 = Q29 */
  acc0 = g729_L_mac(acc0, 8192, 16384);      /* Q13*Q15*2 = Q29 */
  acc0 = g729_L_shr(acc0, 2);                /* Q29 -> Q27 */
  acc0 = g729_L_add(acc0, g729_L_deposit_h(dSE));
  if (acc0 < 0) return(VOICE);

  acc0 = g729_L_mult(dSZC, -16384);          /* Q15*Q13*2 = Q29 */
  acc0 = g729_L_mac(acc0, 8192, 19660);      /* Q13*Q15*2 = Q29 */
  acc0 = g729_L_shr(acc0, 2);                /* Q29 -> Q27 */
  acc0 = g729_L_add(acc0, g729_L_deposit_h(dSE));
  if (acc0 < 0) return(VOICE);

  acc0 = g729_L_mult(dSE, 32767);            /* Q11*Q15*2 = Q27 */
  acc0 = g729_L_mac(acc0, 1024, 30802);      /* Q10*Q16*2 = Q27 */
  if (acc0 < 0) return(VOICE);

  /* dSE vs SD */
  acc0 = g729_L_mult(SD, -28160);            /* Q15*Q5*2 = Q22 */
  acc0 = g729_L_mac(acc0, 64, 19988);        /* Q6*Q14*2 = Q22 */
  acc0 = g729_L_mac(acc0, dSE, 512);         /* Q11*Q9*2 = Q22 */
  if (acc0 < 0) return(VOICE);

  acc0 = g729_L_mult(SD, 32767);             /* Q15*Q15*2 = Q31 */
  acc0 = g729_L_mac(acc0, 32, -30199);       /* Q5*Q25*2 = Q31 */
  if (acc0 > 0) return(VOICE);

  /* dSLE vs dSZC */
  acc0 = g729_L_mult(dSZC, -20480);          /* Q15*Q13*2 = Q29 */
  acc0 = g729_L_mac(acc0, 8192, 22938);      /* Q13*Q15*2 = Q29 */
  acc0 = g729_L_shr(acc0, 2);                /* Q29 -> Q27 */
  acc0 = g729_L_add(acc0, g729_L_deposit_h(dSE));
  if (acc0 < 0) return(VOICE);

  acc0 = g729_L_mult(dSZC, 23831);           /* Q15*Q13*2 = Q29 */
  acc0 = g729_L_mac(acc0, 4096, 31576);      /* Q12*Q16*2 = Q29 */
  acc0 = g729_L_shr(acc0, 2);                /* Q29 -> Q27 */
  acc0 = g729_L_add(acc0, g729_L_deposit_h(dSE));
  if (acc0 < 0) return(VOICE);

  acc0 = g729_L_mult(dSE, 32767);            /* Q11*Q15*2 = Q27 */
  acc0 = g729_L_mac(acc0, 2048, 17367);      /* Q11*Q15*2 = Q27 */
  if (acc0 < 0) return(VOICE);

  /* dSLE vs SD */
  acc0 = g729_L_mult(SD, -22400);            /* Q15*Q4*2 = Q20 */
  acc0 = g729_L_mac(acc0, 32, 25395);        /* Q5*Q14*2 = Q20 */
  acc0 = g729_L_mac(acc0, dSLE, 256);        /* Q11*Q8*2 = Q20 */
  if (acc0 < 0) return(VOICE);

  /* dSLE vs dSE */
  acc0 = g729_L_mult(dSE, -30427);           /* Q11*Q15*2 = Q27 */
  acc0 = g729_L_mac(acc0, 256, -29959);      /* Q8*Q18*2 = Q27 */
  acc0 = g729_L_add(acc0, g729_L_deposit_h(dSLE));
  if (acc0 > 0) return(VOICE);

  acc0 = g729_L_mult(dSE, -23406);           /* Q11*Q15*2 = Q27 */
  acc0 = g729_L_mac(acc0, 512, 28087);       /* Q19*Q17*2 = Q27 */
  acc0 = g729_L_add(acc0, g729_L_deposit_h(dSLE));
  if (acc0 < 0) return(VOICE);

  acc0 = g729_L_mult(dSE, 24576);            /* Q11*Q14*2 = Q26 */
  acc0 = g729_L_mac(acc0, 1024, 29491);      /* Q10*Q15*2 = Q26 */
  acc0 = g729_L_mac(acc0, dSLE, 16384);      /* Q11*Q14*2 = Q26 */
  if (acc0 < 0) return(VOICE);

  return (NOISE);
}
