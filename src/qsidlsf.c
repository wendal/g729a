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
 *  Version 1.5    Last modified: October 2006
 *
 *  Copyright (c) 1996,
 *  France Telecom, Rockwell International, Universite de Sherbrooke
 *  All rights reserved.
 */

/*--------------------------------------------------------------------------*
 * QSIDLSF.C                                                                *
 * ~~~~~~~~~                                                                *
 * Quantization of LSF vector for SID frames (G.729 Annex B).               *
 *--------------------------------------------------------------------------*/

#include "g729a_typedef.h"
#include "basic_op.h"
#include "ld8a.h"
#include "tab_ld8a.h"

#include "sid.h"
#include "vad.h"
#include "dtx.h"
#include "tab_dtx.h"

#include "g729a_encoder.h"

/* local functions */
static void Qnt_e(G729_Word16 *errlsf,    /* (i)  : error lsf vector             */
                  G729_Word16 *weight,    /* (i)  : weighting vector             */
                  G729_Word16 DIn,        /* (i)  : number of input candidates   */
                  G729_Word16 *qlsf,      /* (o)  : quantized error lsf vector   */
                  G729_Word16 *Pptr,      /* (o)  : predictor index              */
                  G729_Word16 DOut,       /* (i)  : number of quantized vectors  */
                  G729_Word16 *cluster,   /* (o)  : quantizer indices            */
                  G729_Word16 *MS         /* (i)  : size of the quantizers       */
                  );

static void New_ML_search_1(G729_Word16 *d_data,    /* (i) : error vector             */
                            G729_Word16 J,          /* (i) : number of input vectors  */
                            G729_Word16 *new_d_data,/* (o) : output vector            */
                            G729_Word16 K,          /* (i) : number of candidates     */
                            G729_Word16 *best_indx, /* (o) : best indices             */
                            G729_Word16 *ptr_back,  /* (o) : pointer for backtracking */
                            const G729_Word16 *PtrTab, /* (i) : quantizer table       */
                            G729_Word16 MQ          /* (i) : size of quantizer        */
                            );

static void New_ML_search_2(G729_Word16 *d_data,    /* (i) : error vector             */
                            G729_Word16 *weight,    /* (i) : weighting vector         */
                            G729_Word16 J,          /* (i) : number of input vectors  */
                            G729_Word16 *new_d_data,/* (o) : output vector            */
                            G729_Word16 K,          /* (i) : number of candidates     */
                            G729_Word16 *best_indx, /* (o) : best indices             */
                            G729_Word16 *ptr_prd,   /* (i) : pointer for backtracking */
                            G729_Word16 *ptr_back,  /* (o) : pointer for backtracking */
                            const G729_Word16 PtrTab[2][16],/* (i) : quantizer table  */
                            G729_Word16 MQ          /* (i) : size of quantizer        */
                            );

/* Local copy of the LSF weighting function (static in qua_lsp.c). */
static void g729_Get_wegt(G729_Word16 flsp[], G729_Word16 wegt[]);


/*-----------------------------------------------------------------*
 * Functions g729_lsfq_noise                                       *
 *           ~~~~~~~~~~~~~~~                                       *
 * Input:                                                          *
 *   lsp[]         : unquantized lsp vector                        *
 *   noise_fg[][][]: noise LSF predictor coefficients              *
 *   freq_prev[][] : memory of the lsf predictor                   *
 *                                                                 *
 * Output:                                                         *
 *                                                                 *
 *   lspq[]        : quantized lsp vector                          *
 *   ana[]         : indices                                       *
 *                                                                 *
 *-----------------------------------------------------------------*/
void g729_lsfq_noise(G729_Word16 *lsp,
                G729_Word16 *lspq,
                const G729_Word16 noise_fg[MODE][MA_NP][M],
                G729_Word16 freq_prev[MA_NP][M],
                G729_Word16 *ana
                )
{
  G729_Word16 i, lsf[M], lsfq[M], weight[M], tmpbuf[M];
  G729_Word16 MS[MODE]={32, 16}, Clust[MODE], mode, errlsf[M*MODE];

  /* convert lsp to lsf */
  g729_Lsp_lsf2(lsp, lsf, M);

  /* spacing to ~100Hz */
  if (lsf[0] < L_LIMIT)
    lsf[0] = L_LIMIT;
  for (i=0 ; i < M-1 ; i++)
    if (g729_sub(lsf[i+1], lsf[i]) < 2*GAP3)
      lsf[i+1] = g729_add(lsf[i], 2*GAP3);
  if (lsf[M-1] > M_LIMIT)
    lsf[M-1] = M_LIMIT;
  if (lsf[M-1] < lsf[M-2])
    lsf[M-2] = g729_sub(lsf[M-1], GAP3);

  /* get the lsf weighting */
  g729_Get_wegt(lsf, weight);

  /**********************/
  /* quantize the lsf's */
  /**********************/

  /* get the prediction error vector */
  for (mode=0; mode<MODE; mode++)
    g729_Lsp_prev_extract(lsf, errlsf+mode*M, noise_fg[mode], freq_prev,
                     g729_noise_fg_sum_inv[mode]);

  /* quantize the lsf and get the corresponding indices */
  Qnt_e(errlsf, weight, MODE, tmpbuf, &mode, 1, Clust, MS);
  ana[0] = mode;
  ana[1] = Clust[0];
  ana[2] = Clust[1];

  /* guarantee minimum distance of 0.0012 (~10 in Q13) between tmpbuf[j]
     and tmpbuf[j+1] */
  g729_Lsp_expand_1_2(tmpbuf, 10);

  /* compute the quantized lsf vector */
  g729_Lsp_prev_compose(tmpbuf, lsfq, noise_fg[mode], freq_prev,
                   g729_noise_fg_sum[mode]);

  /* update the prediction memory */
  g729_Lsp_prev_update(tmpbuf, freq_prev);

  /* lsf stability check */
  g729_Lsp_stability(lsfq);

  /* convert lsf to lsp */
  g729_Lsf_lsp2(lsfq, lspq, M);

}

static void Qnt_e(G729_Word16 *errlsf,    /* (i)  : error lsf vector             */
                  G729_Word16 *weight,    /* (i)  : weighting vector             */
                  G729_Word16 DIn,        /* (i)  : number of input candidates   */
                  G729_Word16 *qlsf,      /* (o)  : quantized error lsf vector   */
                  G729_Word16 *Pptr,      /* (o)  : predictor index              */
                  G729_Word16 DOut,       /* (i)  : number of quantized vectors  */
                  G729_Word16 *cluster,   /* (o)  : quantizer indices            */
                  G729_Word16 *MS         /* (i)  : size of the quantizers       */
)
{
  G729_Word16 d_data[2][R_LSFQ*M], best_indx[2][R_LSFQ];
  G729_Word16 ptr_back[2][R_LSFQ], ptr, i;

  New_ML_search_1(errlsf, DIn, d_data[0], 4, best_indx[0], ptr_back[0],
                  g729_PtrTab_1, MS[0]);
  New_ML_search_2(d_data[0], weight, 4, d_data[1], DOut, best_indx[1],
                  ptr_back[0], ptr_back[1], g729_PtrTab_2, MS[1]);

  /* backward path for the indices */
  cluster[1] = best_indx[1][0];
  ptr = ptr_back[1][0];
  cluster[0] = best_indx[0][ptr];

  /* this is the pointer to the best predictor */
  *Pptr = ptr_back[0][ptr];

  /* generating the quantized vector */
  g729_Copy(g729_lspcb1[g729_PtrTab_1[cluster[0]]], qlsf, M);
  for (i=0; i<M/2; i++)
    qlsf[i] = g729_add(qlsf[i], g729_lspcb2[g729_PtrTab_2[0][cluster[1]]][i]);
  for (i=M/2; i<M; i++)
    qlsf[i] = g729_add(qlsf[i], g729_lspcb2[g729_PtrTab_2[1][cluster[1]]][i]);

}

static void New_ML_search_1(G729_Word16 *d_data,    /* (i) : error vector             */
                            G729_Word16 J,          /* (i) : number of input vectors  */
                            G729_Word16 *new_d_data,/* (o) : output vector            */
                            G729_Word16 K,          /* (i) : number of candidates     */
                            G729_Word16 *best_indx, /* (o) : best indices             */
                            G729_Word16 *ptr_back,  /* (o) : pointer for backtracking */
                            const G729_Word16 *PtrTab, /* (i) : quantizer table       */
                            G729_Word16 MQ          /* (i) : size of quantizer        */
)
{
  G729_Word16 tmp, m, l, p, q, sum[R_LSFQ*R_LSFQ];
  G729_Word16 min[R_LSFQ], min_indx_p[R_LSFQ], min_indx_m[R_LSFQ];
  G729_Word32 acc0;

  for (q=0; q<K; q++)
    min[q] = G729A_MAX_16;

  /* compute the errors */
  for (p=0; p<J; p++)
    for (m=0; m<MQ; m++){
      acc0 = 0;
      for (l=0; l<M; l++){
        tmp = g729_sub(d_data[p*M+l], g729_lspcb1[PtrTab[m]][l]);
        acc0 = g729_L_mac(acc0, tmp, tmp);
      }
      sum[p*MQ+m] = g729_extract_h(acc0);
      sum[p*MQ+m] = g729_mult(sum[p*MQ+m], g729_Mp[p]);
    }

  /* select the candidates */
  for (q=0; q<K; q++){
    min_indx_p[q] = 0;  /* G.729 maintenance */
    min_indx_m[q] = 0;  /* G.729 maintenance */
    for (p=0; p<J; p++)
      for (m=0; m<MQ; m++)
        if (g729_sub(sum[p*MQ+m], min[q]) < 0){
          min[q] = sum[p*MQ+m];
          min_indx_p[q] = p;
          min_indx_m[q] = m;
        }

    sum[min_indx_p[q]*MQ+min_indx_m[q]] = G729A_MAX_16;
  }

  /* compute the candidates */
  for (q=0; q<K; q++){
    for (l=0; l<M; l++)
      new_d_data[q*M+l] = g729_sub(d_data[min_indx_p[q]*M+l],
                              g729_lspcb1[PtrTab[min_indx_m[q]]][l]);

    ptr_back[q] = min_indx_p[q];
    best_indx[q] = min_indx_m[q];
  }
}

static void New_ML_search_2(G729_Word16 *d_data,    /* (i) : error vector             */
                            G729_Word16 *weight,    /* (i) : weighting vector         */
                            G729_Word16 J,          /* (i) : number of input vectors  */
                            G729_Word16 *new_d_data,/* (o) : output vector            */
                            G729_Word16 K,          /* (i) : number of candidates     */
                            G729_Word16 *best_indx, /* (o) : best indices             */
                            G729_Word16 *ptr_prd,   /* (i) : pointer for backtracking */
                            G729_Word16 *ptr_back,  /* (o) : pointer for backtracking */
                            const G729_Word16 PtrTab[2][16],/* (i) : quantizer table  */
                            G729_Word16 MQ          /* (i) : size of quantizer        */
)
{
  G729_Word16 m, l, p, q, sum[R_LSFQ*R_LSFQ];
  G729_Word16 min[R_LSFQ], min_indx_p[R_LSFQ], min_indx_m[R_LSFQ];
  G729_Word16 tmp1, tmp2;
  G729_Word32 acc0;

  for (q=0; q<K; q++)
    min[q] = G729A_MAX_16;

  /* compute the errors */
  for (p=0; p<J; p++)
    for (m=0; m<MQ; m++){
      acc0 = 0;
      for (l=0; l<M/2; l++){
        tmp1 = g729_extract_h(g729_L_shl(g729_L_mult(g729_noise_fg_sum[ptr_prd[p]][l],
                                      g729_noise_fg_sum[ptr_prd[p]][l]), 2));
        tmp1 = g729_mult(tmp1, weight[l]);
        tmp2 = g729_sub(d_data[p*M+l], g729_lspcb2[PtrTab[0][m]][l]);
        tmp1 = g729_extract_h(g729_L_shl(g729_L_mult(tmp1, tmp2), 3));
        acc0 = g729_L_mac(acc0, tmp1, tmp2);
      }

      for (l=M/2; l<M; l++){
        tmp1 = g729_extract_h(g729_L_shl(g729_L_mult(g729_noise_fg_sum[ptr_prd[p]][l],
                                      g729_noise_fg_sum[ptr_prd[p]][l]), 2));
        tmp1 = g729_mult(tmp1, weight[l]);
        tmp2 = g729_sub(d_data[p*M+l], g729_lspcb2[PtrTab[1][m]][l]);
        tmp1 = g729_extract_h(g729_L_shl(g729_L_mult(tmp1, tmp2), 3));
        acc0 = g729_L_mac(acc0, tmp1, tmp2);
      }

      sum[p*MQ+m] = g729_extract_h(acc0);
    }

  /* select the candidates */
  for (q=0; q<K; q++){
    min_indx_p[q] = 0;  /* G.729 maintenance */
    min_indx_m[q] = 0;  /* G.729 maintenance */
    for (p=0; p<J; p++)
      for (m=0; m<MQ; m++)
        if (g729_sub(sum[p*MQ+m], min[q]) < 0){
          min[q] = sum[p*MQ+m];
          min_indx_p[q] = p;
          min_indx_m[q] = m;
        }

    sum[min_indx_p[q]*MQ+min_indx_m[q]] = G729A_MAX_16;
  }

  /* compute the candidates */
  for (q=0; q<K; q++){
    for (l=0; l<M/2; l++)
      new_d_data[q*M+l] = g729_sub(d_data[min_indx_p[q]*M+l],
                              g729_lspcb2[PtrTab[0][min_indx_m[q]]][l]);
    for (l=M/2; l<M; l++)
      new_d_data[q*M+l] = g729_sub(d_data[min_indx_p[q]*M+l],
                              g729_lspcb2[PtrTab[1][min_indx_m[q]]][l]);

    ptr_back[q] = min_indx_p[q];
    best_indx[q] = min_indx_m[q];
  }
}

/*--------------------------------------------------------------------------*
 * Get_wegt: compute LSF weighting coefficients.                            *
 * Local copy of the static g729_Get_wegt() in qua_lsp.c (identical code).  *
 *--------------------------------------------------------------------------*/
static void g729_Get_wegt(
    G729_Word16 flsp[],    /* (i) Q13 : M LSP parameters  */
    G729_Word16 wegt[]     /* (o) Q11->norm : M weighting coefficients */
)
{
    G729_Word16 i;
    G729_Word16 tmp;
    G729_Word32 L_acc;
    G729_Word16 sft;
    G729_Word16 buf[M]; /* in Q13 */


    buf[0] = g729_sub( flsp[1], (PI04+8192) );           /* 8192:1.0(Q13) */

    for ( i = 1 ; i < M-1 ; i++ ) {
        tmp = g729_sub( flsp[i+1], flsp[i-1] );
        buf[i] = g729_sub( tmp, 8192 );
    }

    buf[M-1] = g729_sub( (PI92-8192), flsp[M-2] );

    /* */
    for ( i = 0 ; i < M ; i++ ) {
        if ( buf[i] > 0 ){
            wegt[i] = 2048;                    /* 2048:1.0(Q11) */
        }
        else {
            L_acc = g729_L_mult( buf[i], buf[i] );           /* L_acc in Q27 */
            tmp = g729_extract_h( g729_L_shl( L_acc, 2 ) );       /* tmp in Q13 */

            L_acc = g729_L_mult( tmp, CONST10 );             /* L_acc in Q25 */
            tmp = g729_extract_h( g729_L_shl( L_acc, 2 ) );       /* tmp in Q11 */

            wegt[i] = g729_add( tmp, 2048 );                 /* wegt in Q11 */
        }
    }

    /* */
    L_acc = g729_L_mult( wegt[4], CONST12 );             /* L_acc in Q26 */
    wegt[4] = g729_extract_h( g729_L_shl( L_acc, 1 ) );       /* wegt in Q11 */

    L_acc = g729_L_mult( wegt[5], CONST12 );             /* L_acc in Q26 */
    wegt[5] = g729_extract_h( g729_L_shl( L_acc, 1 ) );       /* wegt in Q11 */

    /* wegt: Q11 -> normalized */
    tmp = 0;
    for ( i = 0; i < M; i++ ) {
        if ( g729_sub(wegt[i], tmp) > 0 ) {
            tmp = wegt[i];
        }
    }

    sft = g729_norm_s(tmp);
    for ( i = 0; i < M; i++ ) {
        wegt[i] = g729_shl(wegt[i], sft);                  /* wegt in Q(11+sft) */
    }

    return;
}
