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

/* DTX and Comfort Noise Generator - Encoder part */

#include "g729a_typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "ld8a.h"
#include "tab_ld8a.h"

#include "vad.h"
#include "dtx.h"
#include "tab_dtx.h"
#include "sid.h"

#include "g729a_encoder.h"

/* Local functions */
static void Calc_pastfilt(g729a_dtx_state * state, G729_Word16 *Coeff);
static void Calc_RCoeff(G729_Word16 *Coeff, G729_Word16 *RCoeff, G729_Word16 *sh_RCoeff);
static G729_Word16 Cmp_filt(G729_Word16 *RCoeff, G729_Word16 sh_RCoeff, G729_Word16 *acf,
                                        G729_Word16 alpha, G729_Word16 Fracthresh);
static void Calc_sum_acf(G729_Word16 *acf, G729_Word16 *sh_acf,
                    G729_Word16 *sum, G729_Word16 *sh_sum, G729_Word16 nb);
static void Update_sumAcf(g729a_dtx_state * state);

/* Levinson-Durbin with residual energy output (see note in the body below) */
static void Levinson(g729a_dtx_state * state,
  G729_Word16 Rh[],      /* (i)     : Rh[M+1] Vector of autocorrelations (msb) */
  G729_Word16 Rl[],      /* (i)     : Rl[M+1] Vector of autocorrelations (lsb) */
  G729_Word16 A[],       /* (o) Q12 : A[M]    LPC coefficients  (m = 10)       */
  G729_Word16 rc[],      /* (o) Q15 : rc[M]   Reflection coefficients.         */
  G729_Word16 *Err       /* (o)     : Residual energy                          */
);

/*-----------------------------------------------------------*
 * procedure g729_Init_Cod_cng:                              *
 *           ~~~~~~~~~~~~~~~~~                               *
 *   Initialize variables used for dtx at the encoder        *
 *-----------------------------------------------------------*/
void g729_Init_Cod_cng(g729a_dtx_state * state)
{
  G729_Word16 i;

  for(i=0; i<SIZ_SUMACF; i++) state->sumAcf[i] = 0;
  for(i=0; i<NB_SUMACF; i++) state->sh_sumAcf[i] = 40;

  for(i=0; i<SIZ_ACF; i++) state->Acf[i] = 0;
  for(i=0; i<NB_CURACF; i++) state->sh_Acf[i] = 40;

  for(i=0; i<NB_GAIN; i++) state->sh_ener[i] = 40;
  for(i=0; i<NB_GAIN; i++) state->ener[i] = 0;

  state->cur_gain = 0;
  state->fr_cur = 0;
  state->flag_chang = 0;

  g729_Set_zero(state->lspSid_q, M);
  g729_Set_zero(state->pastCoeff, MP1);
  g729_Set_zero(state->RCoeff, MP1);
  state->sh_RCoeff = 0;
  state->nb_ener = 0;
  state->sid_gain = 0;
  state->prev_energy = 0;
  state->count_fr0 = 0;

  /* Last A(z) for case of unstable filter (local Levinson fallback) */
  state->old_A[0] = 4096;
  for(i=1; i<MP1; i++) state->old_A[i] = 0;
  state->old_rc[0] = 0;
  state->old_rc[1] = 0;

  /* Initialize the noise LSF predictor coefficients */
  g729_Init_lsfq_noise(state->noise_fg);

  return;
}


/*-----------------------------------------------------------*
 * procedure g729_Cod_cng:                                   *
 *           ~~~~~~~~~~~~                                    *
 *   computes DTX decision                                   *
 *   encodes SID frames                                      *
 *   computes CNG excitation for encoder update              *
 *-----------------------------------------------------------*/
void g729_Cod_cng(
  g729a_dtx_state * state,
  G729_Word16 *exc,          /* (i/o) : excitation array                     */
  G729_Word16 pastVad,       /* (i)   : previous VAD decision                */
  G729_Word16 *lsp_old_q,    /* (i/o) : previous quantized lsp               */
  G729_Word16 *Aq,           /* (o)   : set of interpolated LPC coefficients */
  G729_Word16 *ana,          /* (o)   : coded SID parameters                 */
  G729_Word16 freq_prev[MA_NP][M],
                        /* (i/o) : previous LPS for quantization        */
  G729_Word16 *seed,         /* (i/o) : random generator seed                */
  g729a_taming_state * taming_state
                        /* (i/o) : taming state for excitation error update */
)
{

  G729_Word16 i;

  G729_Word16 curAcf[MP1];
  G729_Word16 bid[M], zero[MP1];
  G729_Word16 curCoeff[MP1];
  G729_Word16 lsp_new[M];
  G729_Word16 *lpcCoeff;
  G729_Word16 cur_igain;
  G729_Word16 energyq, temp;

  /* Update Ener and sh_ener */
  for(i = NB_GAIN-1; i>=1; i--) {
    state->ener[i] = state->ener[i-1];
    state->sh_ener[i] = state->sh_ener[i-1];
  }

  /* Compute current Acfs */
  Calc_sum_acf(state->Acf, state->sh_Acf, curAcf, &state->sh_ener[0], NB_CURACF);

  /* Compute LPC coefficients and residual energy */
  if(curAcf[0] == 0) {
    state->ener[0] = 0;             /* should not happen */
  }
  else {
    g729_Set_zero(zero, MP1);
    Levinson(state, curAcf, zero, curCoeff, bid, &state->ener[0]);
  }

  /* if first frame of silence => SID frame */
  if(pastVad != 0) {
    ana[0] = 2;
    state->count_fr0 = 0;
    state->nb_ener = 1;
    g729_Qua_Sidgain(state->ener, state->sh_ener, state->nb_ener, &energyq, &cur_igain);

  }
  else {
    state->nb_ener = g729_add(state->nb_ener, 1);
    if(g729_sub(state->nb_ener, NB_GAIN) > 0) state->nb_ener = NB_GAIN;
    g729_Qua_Sidgain(state->ener, state->sh_ener, state->nb_ener, &energyq, &cur_igain);

    /* Compute stationarity of current filter   */
    /* versus reference filter                  */
    if(Cmp_filt(state->RCoeff, state->sh_RCoeff, curAcf, state->ener[0], FRAC_THRESH1) != 0) {
      state->flag_chang = 1;
    }

    /* compare energy difference between current frame and last frame */
    temp = g729_abs_s(g729_sub(state->prev_energy, energyq));
    temp = g729_sub(temp, 2);
    if (temp > 0) state->flag_chang = 1;

    state->count_fr0 = g729_add(state->count_fr0, 1);
    if(g729_sub(state->count_fr0, FR_SID_MIN) < 0) {
      ana[0] = 0;               /* no transmission */
    }
    else {
      if(state->flag_chang != 0) {
        ana[0] = 2;             /* transmit SID frame */
      }
      else{
        ana[0] = 0;
      }

      state->count_fr0 = FR_SID_MIN;   /* to avoid overflow */
    }
  }


  if(g729_sub(ana[0], 2) == 0) {

    /* Reset frame count and change flag */
    state->count_fr0 = 0;
    state->flag_chang = 0;

    /* Compute past average filter */
    Calc_pastfilt(state, state->pastCoeff);
    Calc_RCoeff(state->pastCoeff, state->RCoeff, &state->sh_RCoeff);

    /* Compute stationarity of current filter   */
    /* versus past average filter               */


    /* if stationary */
    /* transmit average filter => new ref. filter */
    if(Cmp_filt(state->RCoeff, state->sh_RCoeff, curAcf, state->ener[0], FRAC_THRESH2) == 0) {
      lpcCoeff = state->pastCoeff;
    }

    /* else */
    /* transmit current filter => new ref. filter */
    else {
      lpcCoeff = curCoeff;
      Calc_RCoeff(curCoeff, state->RCoeff, &state->sh_RCoeff);
    }

    /* Compute SID frame codes */

    g729_Az_lsp(lpcCoeff, lsp_new, lsp_old_q); /* From A(z) to lsp */

    /* LSP quantization */
    g729_lsfq_noise(lsp_new, state->lspSid_q, state->noise_fg, freq_prev, &ana[1]);

    state->prev_energy = energyq;
    ana[4] = cur_igain;
    state->sid_gain = g729_tab_Sidgain[cur_igain];


  } /* end of SID frame case */

  /* Compute new excitation */
  if(pastVad != 0) {
    state->cur_gain = state->sid_gain;
  }
  else {
    state->cur_gain = g729_mult_r(state->cur_gain, A_GAIN0);
    state->cur_gain = g729_add(state->cur_gain, g729_mult_r(state->sid_gain, A_GAIN1));
  }

  g729_Calc_exc_rand(state->cur_gain, exc, seed, FLAG_COD, taming_state);

  g729_Int_qlpc(lsp_old_q, state->lspSid_q, Aq);
  for(i=0; i<M; i++) {
    lsp_old_q[i]   = state->lspSid_q[i];
  }

  /* Update sumAcf if fr_cur = 0 */
  if(state->fr_cur == 0) {
    Update_sumAcf(state);
  }

  return;
}

/*-----------------------------------------------------------*
 * procedure g729_Update_cng:                                *
 *           ~~~~~~~~~~~~~~~                                 *
 *   Updates autocorrelation arrays                          *
 *   used for DTX/CNG                                        *
 *   If Vad=1 : updating of array sumAcf                     *
 *-----------------------------------------------------------*/
void g729_Update_cng(
  g729a_dtx_state * state,
  G729_Word16 *r_h,      /* (i) :   MSB of frame autocorrelation        */
  G729_Word16 exp_r,     /* (i) :   scaling factor associated           */
  G729_Word16 Vad        /* (i) :   current Vad decision                */
)
{
  G729_Word16 i;
  G729_Word16 *ptr1, *ptr2;

  /* Update Acf and shAcf */
  ptr1 = state->Acf + SIZ_ACF - 1;
  ptr2 = ptr1 - MP1;
  for(i=0; i<(SIZ_ACF-MP1); i++) {
    *ptr1-- = *ptr2--;
  }
  for(i=NB_CURACF-1; i>=1; i--) {
    state->sh_Acf[i] = state->sh_Acf[i-1];
  }

  /* Save current Acf */
  state->sh_Acf[0] = g729_negate(g729_add(16, exp_r));
  for(i=0; i<MP1; i++) {
    state->Acf[i] = r_h[i];
  }

  state->fr_cur = g729_add(state->fr_cur, 1);
  if(g729_sub(state->fr_cur, NB_CURACF) == 0) {
    state->fr_cur = 0;
    if(Vad != 0) {
      Update_sumAcf(state);
    }
  }

  return;
}


/*-----------------------------------------------------------*
 *         Local procedures                                  *
 *         ~~~~~~~~~~~~~~~~                                  *
 *-----------------------------------------------------------*/

/* Compute scaled autocorr of LPC coefficients used for Itakura distance */
/*************************************************************************/
static void Calc_RCoeff(G729_Word16 *Coeff, G729_Word16 *RCoeff, G729_Word16 *sh_RCoeff)
{
  G729_Word16 i, j;
  G729_Word16 sh1;
  G729_Word32 L_acc;

  /* RCoeff[0] = SUM(j=0->M) Coeff[j] ** 2 */
  L_acc = 0L;
  for(j=0; j <= M; j++) {
    L_acc = g729_L_mac(L_acc, Coeff[j], Coeff[j]);
  }

  /* Compute exponent RCoeff */
  sh1 = g729_norm_l(L_acc);
  L_acc = g729_L_shl(L_acc, sh1);
  RCoeff[0] = g729_round(L_acc);

  /* RCoeff[i] = SUM(j=0->M-i) Coeff[j] * Coeff[j+i] */
  for(i=1; i<=M; i++) {
    L_acc = 0L;
    for(j=0; j<=M-i; j++) {
      L_acc = g729_L_mac(L_acc, Coeff[j], Coeff[j+i]);
    }
    L_acc = g729_L_shl(L_acc, sh1);
    RCoeff[i] = g729_round(L_acc);
  }
  *sh_RCoeff = sh1;
  return;
}

/* g729_L_mac() with local overflow detection.                          */
/* The ITU reference uses a global Overflow flag (removed in this port) */
/* to trigger the rescaling loop of Cmp_filt() below.                   */
static G729_Word32 L_mac_ovf(G729_Word32 L_acc, G729_Word16 var1, G729_Word16 var2,
                             G729_Flag *overflow)
{
  G729_Word32 L_prod, L_res;

  L_prod = (G729_Word32)var1 * (G729_Word32)var2;
  if(L_prod == (G729_Word32)0x40000000L) {      /* (-32768) * (-32768) */
    *overflow = 1;
    L_prod = G729A_MAX_32;                      /* g729_L_mult saturation */
  }
  else {
    L_prod = L_prod << 1;                       /* safe: |L_prod| <= 2**31-2 */
  }

  L_res = L_acc + L_prod;
  if(((L_acc ^ L_prod) & G729A_MIN_32) == 0) {
    if((L_res ^ L_acc) & G729A_MIN_32) {
      *overflow = 1;
      L_res = (L_acc < 0) ? G729A_MIN_32 : G729A_MAX_32;
    }
  }
  return(L_res);
}

/* Compute Itakura distance and compare to threshold */
/*****************************************************/
static G729_Word16 Cmp_filt(G729_Word16 *RCoeff, G729_Word16 sh_RCoeff, G729_Word16 *acf,
                                        G729_Word16 alpha, G729_Word16 FracThresh)
{
  G729_Word32 L_temp0, L_temp1;
  G729_Word16 temp1, temp2, sh[2], ind;
  G729_Word16 i;
  G729_Word16 diff, flag;
  G729_Flag overflow;

  sh[0] = 0;
  sh[1] = 0;
  ind = 1;
  flag = 0;
  do {
    overflow = 0;
    temp1 = g729_shr(RCoeff[0], sh[0]);
    temp2 = g729_shr(acf[0], sh[1]);
    L_temp0 = g729_L_shr(L_mac_ovf(0, temp1, temp2, &overflow),1);
    for(i=1; i <= M; i++) {
      temp1 = g729_shr(RCoeff[i], sh[0]);
      temp2 = g729_shr(acf[i], sh[1]);
      L_temp0 = L_mac_ovf(L_temp0, temp1, temp2, &overflow);
    }
    if(overflow != 0) {
      sh[ind] = g729_add(sh[ind], 1);
      ind = g729_sub(1, ind);
    }
    else flag = 1;
  } while (flag == 0);


  temp1 = g729_mult_r(alpha, FracThresh);
  L_temp1 = g729_L_add(g729_L_deposit_l(temp1), g729_L_deposit_l(alpha));
  temp1 = g729_add(sh_RCoeff, 9);  /* 9 = Lpc_justif. * 2 - 16 + 1 */
  temp2 = g729_add(sh[0], sh[1]);
  temp1 = g729_sub(temp1, temp2);
  L_temp1 = g729_L_shl(L_temp1, temp1);

  L_temp0 = g729_L_sub(L_temp0, L_temp1);
  if(L_temp0 > 0L) diff = 1;
  else diff = 0;

  return(diff);
}

/* Compute past average filter */
/*******************************/
static void Calc_pastfilt(g729a_dtx_state * state, G729_Word16 *Coeff)
{
  G729_Word16 i;
  G729_Word16 s_sumAcf[MP1];
  G729_Word16 bid[M], zero[MP1];
  G729_Word16 temp;

  Calc_sum_acf(state->sumAcf, state->sh_sumAcf, s_sumAcf, &temp, NB_SUMACF);

  if(s_sumAcf[0] == 0L) {
    Coeff[0] = 4096;
    for(i=1; i<=M; i++) Coeff[i] = 0;
    return;
  }

  g729_Set_zero(zero, MP1);
  Levinson(state, s_sumAcf, zero, Coeff, bid, &temp);
  return;
}

/* Update sumAcf */
/*****************/
static void Update_sumAcf(g729a_dtx_state * state)
{
  G729_Word16 *ptr1, *ptr2;
  G729_Word16 i;

  /*** Move sumAcf ***/
  ptr1 = state->sumAcf + SIZ_SUMACF - 1;
  ptr2 = ptr1 - MP1;
  for(i=0; i<(SIZ_SUMACF-MP1); i++) {
    *ptr1-- = *ptr2--;
  }
  for(i=NB_SUMACF-1; i>=1; i--) {
    state->sh_sumAcf[i] = state->sh_sumAcf[i-1];
  }

  /* Compute new sumAcf */
  Calc_sum_acf(state->Acf, state->sh_Acf, state->sumAcf, state->sh_sumAcf, NB_CURACF);
  return;
}

/* Compute sum of acfs (curAcf, sumAcf or s_sumAcf) */
/****************************************************/
static void Calc_sum_acf(G729_Word16 *acf, G729_Word16 *sh_acf,
                         G729_Word16 *sum, G729_Word16 *sh_sum, G729_Word16 nb)
{

  G729_Word16 *ptr1;
  G729_Word32 L_temp, L_tab[MP1];
  G729_Word16 sh0, temp;
  G729_Word16 i, j;

  /* Compute sum = sum of nb acfs */
  /* Find sh_acf minimum */
  sh0 = sh_acf[0];
  for(i=1; i<nb; i++) {
    if(g729_sub(sh_acf[i], sh0) < 0) sh0 = sh_acf[i];
  }
  sh0 = g729_add(sh0, 14);           /* 2 bits of margin */

  for(j=0; j<MP1; j++) {
    L_tab[j] = 0L;
  }
  ptr1 = acf;
  for(i=0; i<nb; i++) {
    temp = g729_sub(sh0, sh_acf[i]);
    for(j=0; j<MP1; j++) {
      L_temp = g729_L_deposit_l(*ptr1++);
      L_temp = g729_L_shl(L_temp, temp); /* shift right if temp<0 */
      L_tab[j] = g729_L_add(L_tab[j], L_temp);
    }
  }
  temp = g729_norm_l(L_tab[0]);
  for(i=0; i<=M; i++) {
    sum[i] = g729_extract_h(g729_L_shl(L_tab[i], temp));
  }
  temp = g729_sub(temp, 16);
  *sh_sum = g729_add(sh0, temp);
  return;
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *  Levinson-Durbin with residual energy output.                             *
 *                                                                           *
 *  The ITU Annex B reference shares one Levinson() (with Err output and an  *
 *  old_A/old_rc instability fallback) between the speech and the DTX paths. *
 *  This library's g729_Levinson() (lpc.c) has neither, so the DTX path      *
 *  keeps its own local copy here; the fallback memory lives in the DTX      *
 *  state instead of file-scope statics.                                     *
 *                                                                           *
 *---------------------------------------------------------------------------*/

static void Levinson(g729a_dtx_state * state,
  G729_Word16 Rh[],      /* (i)     : Rh[M+1] Vector of autocorrelations (msb) */
  G729_Word16 Rl[],      /* (i)     : Rl[M+1] Vector of autocorrelations (lsb) */
  G729_Word16 A[],       /* (o) Q12 : A[M]    LPC coefficients  (m = 10)       */
  G729_Word16 rc[],      /* (o) Q15 : rc[M]   Reflection coefficients.         */
  G729_Word16 *Err       /* (o)     : Residual energy                          */
)
{
 G729_Word16 i, j;
 G729_Word16 hi, lo;
 G729_Word16 Kh, Kl;                /* reflection coefficient; hi and lo           */
 G729_Word16 alp_h, alp_l, alp_exp; /* Prediction gain; hi lo and exponent         */
 G729_Word16 Ah[M+1], Al[M+1];      /* LPC coef. in double prec.                   */
 G729_Word16 Anh[M+1], Anl[M+1];    /* LPC coef.for next iteration in double prec. */
 G729_Word32 t0, t1, t2;            /* temporary variable                          */


/* K = A[1] = -R[1] / R[0] */

  t1  = g729_L_Comp(Rh[1], Rl[1]);           /* R[1] in Q31      */
  t2  = g729_L_abs(t1);                      /* abs R[1]         */
  t0  = g729_Div_32(t2, Rh[0], Rl[0]);       /* R[1]/R[0] in Q31 */
  if(t1 > 0) t0= g729_L_negate(t0);          /* -R[1]/R[0]       */
  g729_L_Extract(t0, &Kh, &Kl);              /* K in DPF         */
  rc[0] = Kh;
  t0 = g729_L_shr(t0,4);                     /* A[1] in Q27      */
  g729_L_Extract(t0, &Ah[1], &Al[1]);        /* A[1] in DPF      */

/*  Alpha = R[0] * (1-K**2) */

  t0 = g729_Mpy_32(Kh ,Kl, Kh, Kl);          /* K*K      in Q31 */
  t0 = g729_L_abs(t0);                       /* Some case <0 !! */
  t0 = g729_L_sub( (G729_Word32)0x7fffffffL, t0 ); /* 1 - K*K  in Q31 */
  g729_L_Extract(t0, &hi, &lo);              /* DPF format      */
  t0 = g729_Mpy_32(Rh[0] ,Rl[0], hi, lo);    /* Alpha in Q31    */

/* Normalize Alpha */

  alp_exp = g729_norm_l(t0);
  t0 = g729_L_shl(t0, alp_exp);
  g729_L_Extract(t0, &alp_h, &alp_l);         /* DPF format    */

/*--------------------------------------*
 * ITERATIONS  I=2 to M                 *
 *--------------------------------------*/

  for(i= 2; i<=M; i++)
  {

    /* t0 = SUM ( R[j]*A[i-j] ,j=1,i-1 ) +  R[i] */

    t0 = 0;
    for(j=1; j<i; j++)
      t0 = g729_L_add(t0, g729_Mpy_32(Rh[j], Rl[j], Ah[i-j], Al[i-j]));

    t0 = g729_L_shl(t0,4);                  /* result in Q27 -> convert to Q31 */
                                            /* No overflow possible            */
    t1 = g729_L_Comp(Rh[i],Rl[i]);
    t0 = g729_L_add(t0, t1);                /* add R[i] in Q31                 */

    /* K = -t0 / Alpha */

    t1 = g729_L_abs(t0);
    t2 = g729_Div_32(t1, alp_h, alp_l);     /* abs(t0)/Alpha                   */
    if(t0 > 0) t2= g729_L_negate(t2);       /* K =-t0/Alpha                    */
    t2 = g729_L_shl(t2, alp_exp);           /* denormalize; compare to Alpha   */
    g729_L_Extract(t2, &Kh, &Kl);           /* K in DPF                        */
    rc[i-1] = Kh;

    /* Test for unstable filter. If unstable keep old A(z) */

    if (g729_sub(g729_abs_s(Kh), 32750) > 0)
    {
      for(j=0; j<=M; j++)
      {
        A[j] = state->old_A[j];
      }
      rc[0] = state->old_rc[0];   /* only two rc coefficients are needed */
      rc[1] = state->old_rc[1];
      return;
    }

    /*------------------------------------------*
     *  Compute new LPC coeff. -> An[i]         *
     *  An[j]= A[j] + K*A[i-j]     , j=1 to i-1 *
     *  An[i]= K                                *
     *------------------------------------------*/


    for(j=1; j<i; j++)
    {
      t0 = g729_Mpy_32(Kh, Kl, Ah[i-j], Al[i-j]);
      t0 = g729_L_add(t0, g729_L_Comp(Ah[j], Al[j]));
      g729_L_Extract(t0, &Anh[j], &Anl[j]);
    }
    t2 = g729_L_shr(t2, 4);                  /* t2 = K in Q31 ->convert to Q27  */
    g729_L_Extract(t2, &Anh[i], &Anl[i]);    /* An[i] in Q27                    */

    /*  Alpha = Alpha * (1-K**2) */

    t0 = g729_Mpy_32(Kh ,Kl, Kh, Kl);          /* K*K      in Q31 */
    t0 = g729_L_abs(t0);                       /* Some case <0 !! */
    t0 = g729_L_sub( (G729_Word32)0x7fffffffL, t0 ); /* 1 - K*K  in Q31 */
    g729_L_Extract(t0, &hi, &lo);              /* DPF format      */
    t0 = g729_Mpy_32(alp_h , alp_l, hi, lo);   /* Alpha in Q31    */

    /* Normalize Alpha */

    j = g729_norm_l(t0);
    t0 = g729_L_shl(t0, j);
    g729_L_Extract(t0, &alp_h, &alp_l);         /* DPF format    */
    alp_exp = g729_add(alp_exp, j);             /* Add normalization to alp_exp */

    /* A[j] = An[j] */

    for(j=1; j<=i; j++)
    {
      Ah[j] =Anh[j];
      Al[j] =Anl[j];
    }
  }

  *Err = g729_shr(alp_h, alp_exp);

  /* Truncate A[i] in Q27 to Q12 with rounding */

  A[0] = 4096;
  for(i=1; i<=M; i++)
  {
    t0   = g729_L_Comp(Ah[i], Al[i]);
    state->old_A[i] = A[i] = g729_round(g729_L_shl(t0, 1));
  }
  state->old_rc[0] = rc[0];
  state->old_rc[1] = rc[1];

  return;
}
