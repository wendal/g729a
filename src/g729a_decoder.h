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

#ifndef __G729_DECODER_H__
#define __G729_DECODER_H__

#include "g729a_typedef.h"
#include "g729a_defines.h"

typedef struct _g729a_lspdec_state
{
    /*--------------------------------------------------------------------------*
     * lspdec.c
     *--------------------------------------------------------------------------*/
    
    G729_Word16 freq_prev[MA_NP][M];      /* Q13 */
    G729_Word16 prev_ma;                  /* previous MA prediction coef.*/
    G729_Word16 prev_lsp[M];              /* previous LSP vector         */
} g729a_lspdec_state;

typedef struct _g729a_post_filter_state
{
    /*--------------------------------------------------------------------------*
     * postfilt.c
     *--------------------------------------------------------------------------*/
    
    /* inverse filtered synthesis (with A(z/GAMMA2_PST))   */
    G729_Word16 res2_buf[PIT_MAX + L_SUBFR];
    G729_Word16 *res2;
    G729_Word16 scal_res2_buf[PIT_MAX + L_SUBFR];
    G729_Word16 *scal_res2;
    
    /* memory of filter 1/A(z/GAMMA1_PST) */
    G729_Word16 mem_syn_pst[M];
    
    G729_Word16 mem_pre;
    G729_Word16 past_gain;
} g729a_post_filter_state;

typedef struct _g729a_post_process_state
{
    /*--------------------------------------------------------------------------*
     * post_pro.c
     *--------------------------------------------------------------------------*/
    
    G729_Word16 y2_hi;
    G729_Word16 y2_lo;
    G729_Word16 y1_hi;
    G729_Word16 y1_lo;
    G729_Word16 x0;
    G729_Word16 x1;
} g729a_post_process_state;

/*--------------------------------------------------------------------------*
 * Annex B (VAD/DTX/CNG)                                                    *
 *--------------------------------------------------------------------------*/

typedef struct _g729a_cng_state
{
    /*--------------------------------------------------------------------------*
     * dec_sid.c
     *--------------------------------------------------------------------------*/
    
    G729_Word16 cur_gain;
    G729_Word16 lspSid[M];
    G729_Word16 sid_gain;
    
    /* Noise LSF predictor coefficients; run-time-initialized noise_fg
       table of the ITU reference (tab_dtx.c), kept here per instance. */
    G729_Word16 noise_fg[MODE][MA_NP][M];
} g729a_cng_state;

typedef struct _g729a_decoder_state
{
    G729_Word32 error;  /* Last error code, see g729a_errors.h                 */
    G729_UWord32 magic; /* G729A_DEC_STATE_MAGIC once G729A_Decoder_Init() ran */
    
    /*--------------------------------------------------------------------------*
     * dec_ld8a.c
     *--------------------------------------------------------------------------*/
    
    /* Excitation vector */
    G729_Word16 old_exc[L_FRAME+PIT_MAX+L_INTERPOL];
    G729_Word16 *exc;
    
    /* Lsp (Line spectral pairs) */
    G729_Word16 lsp_old[M];
    
    /* Filter's memory */
    G729_Word16 mem_syn[M];
    
    G729_Word16 sharp;           /* pitch sharpening of previous frame */
    G729_Word16 old_T0;          /* integer delay of previous frame    */
    G729_Word16 gain_code;       /* Code gain                          */
    G729_Word16 gain_pitch;      /* Pitch gain                         */
    G729_Word16 random_seed;     /* Seed for the random generator      */
    G729_Word16 bad_lsf;         /* Bad LSF indicator                  */
    
    G729_Word16 synth_buf[L_FRAME + M];
    G729_Word16 *synth;
    
    /*--------------------------------------------------------------------------*
     * dec_gain.c
     *--------------------------------------------------------------------------*/
    
    /* Gain predictor, Past quantized energies = -14.0 in Q10 */
    G729_Word16 past_qua_en[4];
    
    /*--------------------------------------------------------------------------*
     *--------------------------------------------------------------------------*/
    
    g729a_lspdec_state        lspdec_state;
    g729a_post_filter_state   post_filter_state;
    g729a_post_process_state  post_process_state;
    
    /*--------------------------------------------------------------------------*
     * Annex B (VAD/DTX/CNG)                                                    *
     *--------------------------------------------------------------------------*/
    
    g729a_cng_state           cng_state;
} g729a_decoder_state;

#ifdef __cplusplus
extern "C" {
#endif
    
/*--------------------------------------------------------------------------*
 * cod_ld8a.c                                                               *
 * Main decoder functions                                                   *
 *--------------------------------------------------------------------------*/

void g729_Init_Decod_ld8a(g729a_decoder_state * state);

void g729_Decod_ld8a(
    g729a_decoder_state * state,
    G729_Word16  parm[],      /* (i)   : vector of synthesis parameters
                                         parm[0] = bad frame indicator (bfi)  */
    G729_Word16  synth[],     /* (o)   : synthesis speech                     */
    G729_Word16  A_t[],       /* (o)   : decoded LP filter in 2 subframes     */
    G729_Word16  *T2,         /* (o)   : decoded pitch lag in 2 subframes     */
    G729_Word16 bad_lsf       /* (i)   : bad LSF indicator   */
);
    
/*-------------------------------*
 * Post filter                   *
 *-------------------------------*/
    
void g729_Init_Post_Filter(g729a_post_filter_state * state);

void g729_Post_Filter(
    g729a_post_filter_state * state,
    G729_Word16 *syn,        /* in/out: synthesis speech (postfiltered is output)    */
    G729_Word16 *Az_4,       /* input : interpolated LPC parameters in all subframes */
    G729_Word16 *T           /* input : decoded pitch lags in all subframes          */
);
    
/*-------------------------------*
 * Post-process                  *
 *-------------------------------*/

void g729_Init_Post_Process(g729a_post_process_state * state);

void g729_Post_Process(
    g729a_post_process_state * state,
    G729_Word16 signal_in[],    /* Input signal        */
    G729_Word16 signal_out[],   /* Output signal       */
    G729_Word16 lg              /* Length of signal    */
);
    
/*-------------------------------*
 * lspdec                        *
 *-------------------------------*/
    
void g729_Lsp_decw_reset(g729a_lspdec_state * state);
    
void g729_D_lsp(
    g729a_lspdec_state * state,
    G729_Word16 prm[],          /* (i)     : indexes of the selected LSP */
    G729_Word16 lsp_q[],        /* (o) Q15 : Quantized LSP parameters    */
    G729_Word16 erase           /* (i)     : frame erase information     */
);
    
/*--------------------------------------------------------------------------*
 * gain VQ functions.                                                       *
 *--------------------------------------------------------------------------*/

void g729_Dec_gain(
    g729a_decoder_state * state,
    G729_Word16 index,     /* (i)     : Index of quantization.                     */
    G729_Word16 code[],    /* (i) Q13 : Innovative vector.                         */
    G729_Word16 L_subfr,   /* (i)     : Subframe length.                           */
    G729_Word16 bfi,       /* (i)     : Bad frame indicator                        */
    G729_Word16 *gain_pit, /* (o) Q14 : Pitch gain.                                */
    G729_Word16 *gain_cod  /* (o) Q1  : Code gain.                                 */
);
    
/*--------------------------------------------------------------------------*
 * Annex B (VAD/DTX/CNG)                                                    *
 *--------------------------------------------------------------------------*/

/*-------------------------------*
 * dec_sid.c                     *
 *-------------------------------*/

void g729_Init_Dec_cng(g729a_cng_state * state);

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
);

void g729_sid_lsfq_decode(
    g729a_cng_state * state,   /* (i)   : CNG state (noise_fg)                */
    G729_Word16 *index,        /* (i)   : quantized indices                   */
    G729_Word16 *lspq,         /* (o)   : quantized lsp vector                */
    G729_Word16 freq_prev[MA_NP][M]
                               /* (i)   : memory of predictor                 */
);

/*-------------------------------*
 * qsidgain.c (shared with encoder)                                         *
 *-------------------------------*/

void g729_Qua_Sidgain(
    G729_Word16 *ener,         /* (i)   array of energies                   */
    G729_Word16 *sh_ener,      /* (i)   corresponding scaling factors       */
    G729_Word16 nb_ener,       /* (i)   number of energies or               */
    G729_Word16 *enerq,        /* (o)   decoded energies in dB              */
    G729_Word16 *idx           /* (o)   SID gain quantization index         */
);

/*-------------------------------*
 * tab_dtx.c (shared with encoder)                                          *
 *-------------------------------*/

void g729_Init_lsfq_noise(
    G729_Word16 noise_fg[MODE][MA_NP][M] /* (o) : noise LSF predictor coef.  */
);

/*-------------------------------*
 * calcexc.c (shared with encoder)                                          *
 *-------------------------------*/

struct _g729a_taming_state;  /* defined in g729a_encoder.h */

void g729_Calc_exc_rand(
    G729_Word16 cur_gain,      /* (i)   :   target sample gain                 */
    G729_Word16 *exc,          /* (i/o) :   excitation array                   */
    G729_Word16 *seed,         /* (i/o) :   random generator seed              */
    G729_Flag flag_cod,        /* (i)   :   encoder/decoder flag               */
    struct _g729a_taming_state * taming_state
                               /* (i/o) :   taming state (encoder only,        */
                               /*           pass 0 at the decoder)             */
);
    
#ifdef __cplusplus
}
#endif

#endif  /* __G729_DECODER_H__ */
/* end of file */
