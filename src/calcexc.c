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

/* Computation of Comfort Noise excitation             */

#include "g729a_typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "ld8a.h"

#include "dtx.h"

#include "g729a_encoder.h"

/* Local functions */
static G729_Word16 Gauss(G729_Word16 *seed);
static G729_Word16 Sqrt( G729_Word32 Num);

/*-----------------------------------------------------------*
 * procedure g729_Calc_exc_rand                              *
 *           ~~~~~~~~~~~~~~~~~~                              *
 *   Computes comfort noise excitation                       *
 *   for SID and not-transmitted frames                      *
 *                                                           *
 *   taming_state is used only at the encoder                *
 *   (flag_cod != FLAG_DEC); pass 0 at the decoder.          *
 *-----------------------------------------------------------*/
void g729_Calc_exc_rand(
  G729_Word16 cur_gain,      /* (i)   :   target sample gain                 */
  G729_Word16 *exc,          /* (i/o) :   excitation array                   */
  G729_Word16 *seed,         /* (i/o) :   random generator seed              */
  G729_Flag flag_cod,        /* (i)   :   encoder/decoder flag               */
  g729a_taming_state * taming_state /* (i/o) : taming state (encoder only)   */
)
{
  G729_Word16 i, j, i_subfr;
  G729_Word16 temp1, temp2;
  G729_Word16 pos[4];
  G729_Word16 sign[4];
  G729_Word16 t0, frac;
  G729_Word16 *cur_exc;
  G729_Word16 g, Gp, Gp2;
  G729_Word16 excg[L_SUBFR], excs[L_SUBFR];
  G729_Word32 L_acc, L_ener, L_k;
  G729_Word16 max, hi, lo, inter_exc;
  G729_Word16 sh;
  G729_Word16 x1, x2;

  if(cur_gain == 0) {

    for(i=0; i<L_FRAME; i++) {
      exc[i] = 0;
    }
    Gp = 0;
    t0 = g729_add(L_SUBFR,1);
    for (i_subfr = 0;  i_subfr < L_FRAME; i_subfr += L_SUBFR) {
      if(flag_cod != FLAG_DEC) g729_update_exc_err(taming_state, Gp, t0);
    }

    return;
  }



  /* Loop on subframes */

  cur_exc = exc;

  for (i_subfr = 0;  i_subfr < L_FRAME; i_subfr += L_SUBFR) {

    /* generate random adaptive codebook & fixed codebook parameters */
    /*****************************************************************/
    temp1 = g729_Random(seed);
    frac = g729_sub((temp1 & (G729_Word16)0x0003), 1);
    if(g729_sub(frac, 2) == 0) frac = 0;
    temp1 = g729_shr(temp1, 2);
    t0 = g729_add((temp1 & (G729_Word16)0x003F), 40);
    temp1 = g729_shr(temp1, 6);
    temp2 = temp1 & (G729_Word16)0x0007;
    pos[0] = g729_add(g729_shl(temp2, 2), temp2); /* 5 * temp2 */
    temp1 = g729_shr(temp1, 3);
    sign[0] = temp1 & (G729_Word16)0x0001;
    temp1 = g729_shr(temp1, 1);
    temp2 = temp1 & (G729_Word16)0x0007;
    temp2 = g729_add(g729_shl(temp2, 2), temp2);
    pos[1] = g729_add(temp2, 1);     /* 5 * x + 1 */
    temp1 = g729_shr(temp1, 3);
    sign[1] = temp1 & (G729_Word16)0x0001;
    temp1 = g729_Random(seed);
    temp2 = temp1 & (G729_Word16)0x0007;
    temp2 = g729_add(g729_shl(temp2, 2), temp2);
    pos[2] = g729_add(temp2, 2);     /* 5 * x + 2 */
    temp1 = g729_shr(temp1, 3);
    sign[2] = temp1 & (G729_Word16)0x0001;
    temp1 = g729_shr(temp1, 1);
    temp2 = temp1 & (G729_Word16)0x000F;
    pos[3] = g729_add((temp2 & (G729_Word16)1), 3); /* j+3*/
    temp2 = (g729_shr(temp2, 1)) & (G729_Word16)7;
    temp2 = g729_add(g729_shl(temp2, 2), temp2); /* 5i */
    pos[3] = g729_add(pos[3], temp2);
    temp1 = g729_shr(temp1, 4);
    sign[3] = temp1 & (G729_Word16)0x0001;
    Gp = g729_Random(seed) & (G729_Word16)0x1FFF; /* < 0.5 Q14 */
    Gp2 = g729_shl(Gp, 1);           /* Q15 */


    /* Generate gaussian excitation */
    /********************************/
    L_acc = 0L;
    for(i=0; i<L_SUBFR; i++) {
      temp1 = Gauss(seed);
      L_acc = g729_L_mac(L_acc, temp1, temp1);
      excg[i] = temp1;
    }

/*
    Compute fact = alpha x cur_gain * sqrt(L_SUBFR / Eg)
    with Eg = SUM(i=0->39) excg[i]^2
    and alpha = 0.5
    alpha x sqrt(L_SUBFR)/2 = 1 + FRAC1
*/
    L_acc = g729_Inv_sqrt(g729_L_shr(L_acc,1));  /* Q30 */
    g729_L_Extract(L_acc, &hi, &lo);
    /* cur_gain = cur_gainR << 3 */
    temp1 = g729_mult_r(cur_gain, FRAC1);
    temp1 = g729_add(cur_gain, temp1);
    /* <=> alpha x cur_gainR x 2^2 x sqrt(L_SUBFR) */

    L_acc = g729_Mpy_32_16(hi, lo, temp1);   /* fact << 17 */
    sh = g729_norm_l(L_acc);
    temp1 = g729_extract_h(g729_L_shl(L_acc, sh));  /* fact << (sh+1) */

    sh = g729_sub(sh, 14);
    for(i=0; i<L_SUBFR; i++) {
      temp2 = g729_mult_r(excg[i], temp1);
      temp2 = g729_shr_r(temp2, sh);   /* shl if sh < 0 */
      excg[i] = temp2;
    }

    /* generate random  adaptive excitation */
    /****************************************/
    g729_Pred_lt_3(cur_exc, t0, frac, L_SUBFR);


    /* compute adaptive + gaussian exc -> cur_exc */
    /**********************************************/
    max = 0;
    for(i=0; i<L_SUBFR; i++) {
      temp1 = g729_mult_r(cur_exc[i], Gp2);
      temp1 = g729_add(temp1, excg[i]); /* may overflow */
      cur_exc[i] = temp1;
      temp1 = g729_abs_s(temp1);
      if(g729_sub(temp1,max) > 0) max = temp1;
    }

    /* rescale cur_exc -> excs */
    if(max == 0) sh = 0;
    else {
      sh = g729_sub(3, g729_norm_s(max));
      if(sh <= 0) sh = 0;
    }
    for(i=0; i<L_SUBFR; i++) {
      excs[i] = g729_shr(cur_exc[i], sh);
    }

    /* Compute fixed code gain */
    /***************************/

    /**********************************************************/
    /*** Solve EQ(X) = 4 X**2 + 2 b X + c                     ***/
    /**********************************************************/

    L_ener = 0L;
    for(i=0; i<L_SUBFR; i++) {
      L_ener = g729_L_mac(L_ener, excs[i], excs[i]);
    } /* ener x 2^(-2sh + 1) */

    /* inter_exc = b >> sh */
    inter_exc = 0;
    for(i=0; i<4; i++) {
      j = pos[i];
      if(sign[i] == 0) {
        inter_exc = g729_sub(inter_exc, excs[j]);
      }
      else {
        inter_exc = g729_add(inter_exc, excs[j]);
      }
    }

    /* Compute k = cur_gainR x cur_gainR x L_SUBFR */
    L_acc = g729_L_mult(cur_gain, L_SUBFR);
    L_acc = g729_L_shr(L_acc, 6);
    temp1 = g729_extract_l(L_acc);   /* cur_gainR x L_SUBFR x 2^(-2) */
    L_k   = g729_L_mult(cur_gain, temp1); /* k << 2 */
    temp1 = g729_add(1, g729_shl(sh,1));
    L_acc = g729_L_shr(L_k, temp1);  /* k x 2^(-2sh+1) */

    /* Compute delta = b^2 - 4 c */
    L_acc = g729_L_sub(L_acc, L_ener); /* - 4 c x 2^(-2sh-1) */
    inter_exc = g729_shr(inter_exc, 1);
    L_acc = g729_L_mac(L_acc, inter_exc, inter_exc); /* 2^(-2sh-1) */
    sh = g729_add(sh, 1);
    /* inter_exc = b x 2^(-sh) */
    /* L_acc = delta x 2^(-2sh+1) */

    if(L_acc < 0) {

      /* adaptive excitation = 0 */
      g729_Copy(excg, cur_exc, L_SUBFR);
      temp1 = g729_abs_s(excg[(int)pos[0]]) | g729_abs_s(excg[(int)pos[1]]);
      temp2 = g729_abs_s(excg[(int)pos[2]]) | g729_abs_s(excg[(int)pos[3]]);
      temp1 = temp1 | temp2;
      sh = ((temp1 & (G729_Word16)0x4000) == 0) ? (G729_Word16)1 : (G729_Word16)2;
      inter_exc = 0;
      for(i=0; i<4; i++) {
        temp1 = g729_shr(excg[(int)pos[i]], sh);
        if(sign[i] == 0) {
          inter_exc = g729_sub(inter_exc, temp1);
        }
        else {
          inter_exc = g729_add(inter_exc, temp1);
        }
      } /* inter_exc = b >> sh */
      g729_L_Extract(L_k, &hi, &lo);
      L_acc = g729_Mpy_32_16(hi, lo, K0); /* k x (1- alpha^2) << 2 */
      temp1 = g729_sub(g729_shl(sh, 1), 1); /* temp1 > 0 */
      L_acc = g729_L_shr(L_acc, temp1); /* 4k x (1 - alpha^2) << (-2sh+1) */
      L_acc = g729_L_mac(L_acc, inter_exc, inter_exc); /* delta << (-2sh+1) */
      Gp = 0;
    }

    temp2 = Sqrt(L_acc);        /* >> sh */
    x1 = g729_sub(temp2, inter_exc);
    x2 = g729_negate(g729_add(inter_exc, temp2)); /* x 2^(-sh+2) */
    if(g729_sub(g729_abs_s(x2),g729_abs_s(x1)) < 0) x1 = x2;
    temp1 = g729_sub(2, sh);
    g = g729_shr_r(x1, temp1);       /* shl if temp1 < 0 */
    if(g >= 0) {
      if(g729_sub(g, G_MAX) > 0) g = G_MAX;
    }
    else {
      if(g729_add(g, G_MAX) < 0) g = g729_negate(G_MAX);
    }

    /* Update cur_exc with ACELP excitation */
    for(i=0; i<4; i++) {
      j = pos[i];
      if(sign[i] != 0) {
        cur_exc[j] = g729_add(cur_exc[j], g);
      }
      else {
        cur_exc[j] = g729_sub(cur_exc[j], g);
      }
    }

    if(flag_cod != FLAG_DEC) g729_update_exc_err(taming_state, Gp, t0);

    cur_exc += L_SUBFR;


  } /* end of loop on subframes */

  return;
}

/*-----------------------------------------------------------*
 *         Local procedures                                  *
 *         ~~~~~~~~~~~~~~~~                                  *
 *-----------------------------------------------------------*/

/* Gaussian generation */
/***********************/
static G729_Word16 Gauss(G729_Word16 *seed)
{

/****  Xi = uniform v.a. in [-32768, 32767]       ****/
/****  Z = SUM(i=1->12) Xi / 2 x 32768 is N(0,1)  ****/
/****  output : Z x 512 < 2^12                    ****/

  G729_Word16 i;
  G729_Word16 temp;
  G729_Word32 L_acc;

  L_acc = 0L;
  for(i=0; i<12; i++) {
    L_acc = g729_L_add(L_acc, g729_L_deposit_l(g729_Random(seed)));
  }
  L_acc = g729_L_shr(L_acc, 7);
  temp = g729_extract_l(L_acc);
  return(temp);
}

/* Square root function : returns sqrt(Num/2) */
/**********************************************/
static G729_Word16   Sqrt( G729_Word32 Num )
{
  G729_Word16   i  ;

  G729_Word16   Rez = (G729_Word16) 0 ;
  G729_Word16   Exp = (G729_Word16) 0x4000 ;

  G729_Word32   Acc, L_temp;

  for ( i = 0 ; i < 14 ; i ++ ) {
    Acc = g729_L_mult(g729_add(Rez, Exp), g729_add(Rez, Exp) );
    L_temp = g729_L_sub(Num, Acc);
    if(L_temp >= 0L) Rez = g729_add( Rez, Exp);
    Exp = g729_shr( Exp, (G729_Word16) 1 ) ;
  }
  return Rez ;
}
