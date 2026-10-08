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

/* Quantize SID gain                                      */

#include "g729a_typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "ld8a.h"

#include "vad.h"
#include "dtx.h"
#include "sid.h"
#include "tab_dtx.h"

#include "g729a_encoder.h"

/* Local function */
static G729_Word16 Quant_Energy(
  G729_Word32 L_x,    /* (i)  : Energy                 */
  G729_Word16 sh,     /* (i)  : Exponent of the energy */
  G729_Word16 *enerq  /* (o)  : quantized energy in dB */
);

/*-------------------------------------------------------------------*
 * Function  g729_Qua_Sidgain                                        *
 *           ~~~~~~~~~~~~~~~~                                        *
 *-------------------------------------------------------------------*/
void g729_Qua_Sidgain(
  G729_Word16 *ener,     /* (i)   array of energies                   */
  G729_Word16 *sh_ener,  /* (i)   corresponding scaling factors       */
  G729_Word16 nb_ener,   /* (i)   number of energies or               */
  G729_Word16 *enerq,    /* (o)   decoded energies in dB              */
  G729_Word16 *idx       /* (o)   SID gain quantization index         */
)
{
  G729_Word16 i;
  G729_Word32 L_x;
  G729_Word16 sh1, temp;
  G729_Word16 hi, lo;
  G729_Word32 L_acc;

  if(nb_ener == 0) {
    /* Quantize energy saved for frame erasure case                */
    /* L_x = average_ener                                          */
    L_acc = g729_L_deposit_l(*ener);
    L_acc = g729_L_shl(L_acc, *sh_ener); /* >> if *sh_ener < 0 */
    g729_L_Extract(L_acc, &hi, &lo);
    L_x = g729_Mpy_32_16(hi, lo, g729_fact[0]);
    sh1 = 0;
  }
  else {

    /*
     * Compute weighted average of energies
     * ener[i] = enerR[i] x 2**sh_ener[i]
     * L_x = k[nb_ener] x SUM(i=0->nb_ener-1) enerR[i]
     * with k[nb_ener] =  fact_ener / nb_ener x L_FRAME x nbAcf
     */
    sh1 = sh_ener[0];
    for(i=1; i<nb_ener; i++) {
      if(sh_ener[i] < sh1) sh1 = sh_ener[i];
    }
    sh1 = g729_add(sh1, (16-g729_marg[nb_ener]));
    L_x = 0L;
    for(i=0; i<nb_ener; i++) {
      temp = g729_sub(sh1, sh_ener[i]);
      L_acc = g729_L_deposit_l(ener[i]);
      L_acc = g729_L_shl(L_acc, temp);
      L_x = g729_L_add(L_x, L_acc);
    }
    g729_L_Extract(L_x, &hi, &lo);
    L_x = g729_Mpy_32_16(hi, lo, g729_fact[i]);
  }

  *idx = Quant_Energy(L_x, sh1, enerq);

  return;
}


/* Local function */

static G729_Word16 Quant_Energy(
  G729_Word32 L_x,    /* (i)  : Energy                 */
  G729_Word16 sh,     /* (i)  : Exponent of the energy */
  G729_Word16 *enerq  /* (o)  : quantized energy in dB */
)
{

  G729_Word16 exp, frac;
  G729_Word16 e_tmp, temp, index;

  g729_Log2(L_x, &exp, &frac);
  temp = g729_sub(exp, sh);
  e_tmp = g729_shl(temp, 10);
  e_tmp = g729_add(e_tmp, g729_mult_r(frac, 1024)); /* 2^10 x log2(L_x . 2^-sh) */
  /* log2(ener) = 10log10(ener) / K */
  /* K = 10 Log2 / Log10 */

  temp = g729_sub(e_tmp, -2721);      /* -2721 -> -8dB */
  if(temp <= 0) {
    *enerq = -12;
    return(0);
  }

  temp = g729_sub(e_tmp, 22111);      /* 22111 -> 65 dB */
  if(temp > 0) {
    *enerq = 66;
    return(31);
  }

  temp = g729_sub(e_tmp, 4762);       /* 4762 -> 14 dB */
  if(temp <= 0){
    e_tmp = g729_add(e_tmp, 3401);
    index = g729_mult(e_tmp, 24);
    if (index < 1) index = 1;
    *enerq = g729_sub(g729_shl(index, 2), 8);
    return(index);
  }

  e_tmp = g729_sub(e_tmp, 340);
  index = g729_sub(g729_shr(g729_mult(e_tmp, 193), 2), 1);
  if (index < 6) index = 6;
  *enerq = g729_add(g729_shl(index, 1), 4);
  return(index);
}
