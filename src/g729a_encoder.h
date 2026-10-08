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

#ifndef __G729_ENCODER_H__
#define __G729_ENCODER_H__

#include "g729a_typedef.h"
#include "g729a_defines.h"

#include "dtx.h"    /* Annex B DTX/CNG constants (NB_GAIN, SIZ_ACF, ...) */

typedef struct _g729a_pre_process_state
{
    /*--------------------------------------------------------------------------*
     * pre_proc.c                                                               *
     *--------------------------------------------------------------------------*/
    
    G729_Word16 y2_hi;
    G729_Word16 y2_lo;
    G729_Word16 y1_hi;
    G729_Word16 y1_lo;
    G729_Word16 x0;
    G729_Word16 x1;
} g729a_pre_process_state;

typedef struct _g729a_lspenc_state
{
    /*--------------------------------------------------------------------------*
     * qua_lsp.c                                                                *
     *--------------------------------------------------------------------------*/
    
    G729_Word16 freq_prev[MA_NP][M];  /* Q13:previous LSP vector */
} g729a_lspenc_state;

typedef struct _g729a_taming_state
{
    /*--------------------------------------------------------------------------*
     * taming.c                                                                 *
     *--------------------------------------------------------------------------*/
    
    G729_Word32 L_exc_err[4];
} g729a_taming_state;

/*--------------------------------------------------------------------------*
 * Annex B (VAD/DTX/CNG)                                                    *
 *--------------------------------------------------------------------------*/

typedef struct _g729a_vad_state
{
    /*--------------------------------------------------------------------------*
     * vad.c                                                                    *
     *--------------------------------------------------------------------------*/
    
    G729_Word16 MeanLSF[M];
    G729_Word16 Min_buffer[16];
    G729_Word16 Prev_Min, Next_Min, Min;
    G729_Word16 MeanE, MeanSE, MeanSLE, MeanSZC;
    G729_Word16 prev_energy;
    G729_Word16 count_sil, count_update, count_ext;
    G729_Word16 flag, v_flag, less_count;
} g729a_vad_state;

typedef struct _g729a_dtx_state
{
    /*--------------------------------------------------------------------------*
     * dtx.c                                                                    *
     *--------------------------------------------------------------------------*/
    
    G729_Word16 lspSid_q[M];
    G729_Word16 pastCoeff[MP1];
    G729_Word16 RCoeff[MP1];
    G729_Word16 sh_RCoeff;
    G729_Word16 Acf[SIZ_ACF];
    G729_Word16 sh_Acf[NB_CURACF];
    G729_Word16 sumAcf[SIZ_SUMACF];
    G729_Word16 sh_sumAcf[NB_SUMACF];
    G729_Word16 ener[NB_GAIN];
    G729_Word16 sh_ener[NB_GAIN];
    G729_Word16 fr_cur;
    G729_Word16 cur_gain;
    G729_Word16 nb_ener;
    G729_Word16 sid_gain;
    G729_Word16 flag_chang;
    G729_Word16 prev_energy;
    G729_Word16 count_fr0;
    
    /* Last A(z) for case of unstable filter (local Levinson fallback) */
    G729_Word16 old_A[MP1];
    G729_Word16 old_rc[2];
    
    /* Noise LSF predictor coefficients; run-time-initialized noise_fg
       table of the ITU reference (tab_dtx.c), kept here per instance. */
    G729_Word16 noise_fg[MODE][MA_NP][M];
} g729a_dtx_state;

typedef struct _g729a_encoder_state
{
    G729_Word32 error;  /* Last error code, see g729a_errors.h                  */
    G729_UWord32 magic; /* G729A_ENC_STATE_MAGIC once G729A_Encoder_Init() ran  */
    
    /*--------------------------------------------------------------------------*
     * cod_ld8a.c                                                               *
     *--------------------------------------------------------------------------*/
    
    /* Speech vector */
    G729_Word16 old_speech[L_TOTAL];
    G729_Word16 *speech;
    G729_Word16 *p_window;
    G729_Word16 *new_speech;                    /* Global variable */
    
    /* Weighted speech vector */
    G729_Word16 old_wsp[L_FRAME+PIT_MAX];
    G729_Word16 *wsp;
    
    /* Excitation vector */
    G729_Word16 old_exc[L_FRAME+PIT_MAX+L_INTERPOL];
    G729_Word16 *exc;
    
    /* Lsp (Line spectral pairs) */
    G729_Word16 lsp_old[M];
    G729_Word16 lsp_old_q[M];
    
    /* Filter's memory */
    G729_Word16 mem_w0[M];
    G729_Word16 mem_w[M];
    G729_Word16 mem_zero[M];
    G729_Word16 sharp;
    
    /*--------------------------------------------------------------------------*
     * qua_gain.c                                                               *
     *--------------------------------------------------------------------------*/
    
    /* Gain predictor, Past quantized energies = -14.0 in Q10 */
    G729_Word16 past_qua_en[4];
    
    /*--------------------------------------------------------------------------*
     * acelp_ca.c                                                               *
     *--------------------------------------------------------------------------*/
    
    /* Correlations of the impulse response h[]; scratch space of
       g729_ACELP_Code_A(), kept here to avoid a large stack frame. */
    G729_Word16 acelp_rr[DIM_RR];
    
    /*--------------------------------------------------------------------------*
     *--------------------------------------------------------------------------*/
    
    g729a_pre_process_state  pre_process_state;
    g729a_lspenc_state       lspenc_state;
    g729a_taming_state       taming_state;
    
    /*--------------------------------------------------------------------------*
     * Annex B (VAD/DTX/CNG)                                                    *
     *--------------------------------------------------------------------------*/
    
    g729a_vad_state          vad_state;
    g729a_dtx_state          dtx_state;
} g729a_encoder_state;

#ifdef __cplusplus
extern "C" {
#endif
    
/*--------------------------------------------------------------------------*
 * cod_ld8a.c                                                               *
 * Main coder functions                                                     *
 *--------------------------------------------------------------------------*/
    
void g729_Init_Coder_ld8a(g729a_encoder_state * state);

void g729_Coder_ld8a(
    g729a_encoder_state * state,
    G729_Word16 ana[]                    /* output  : Analysis parameters */
);
    
/*-------------------------------*
 * Pre-process.                  *
 *-------------------------------*/
    
void g729_Init_Pre_Process(g729a_pre_process_state * state);

void g729_Pre_Process(
    g729a_pre_process_state * state,
    const G729_Word16 singal_in[],  /* Input signal (read only) */
    G729_Word16 signal_out[],   /* Output signal */
    G729_Word16 lg              /* Length of signal    */
);
    
/*-------------------------------*
 * lspenc                        *
 *-------------------------------*/
    
void g729_Lsp_encw_reset(g729a_lspenc_state * state);

void g729_Qua_lsp(
    g729a_lspenc_state * state,
    G729_Word16 lsp[],       /* (i) Q15 : Unquantized LSP            */
    G729_Word16 lsp_q[],     /* (o) Q15 : Quantized LSP              */
    G729_Word16 ana[]        /* (o)     : indexes                    */
);
    
/*-------------------------------*
 * taming                        *
 *-------------------------------*/
void   g729_Init_exc_err(g729a_taming_state * state);
void   g729_update_exc_err(g729a_taming_state * state, G729_Word16 gain_pit, G729_Word16 t0);
G729_Word16 g729_test_err(g729a_taming_state * state, G729_Word16 t0, G729_Word16 t0_frac);
    
/*--------------------------------------------------------------------------*
 * gain VQ functions.                                                       *
 *--------------------------------------------------------------------------*/
G729_Word16 g729_Qua_gain(
    g729a_encoder_state * state,
    G729_Word16 code[],        /* (i) Q13 : Innovative vector.                         */
    G729_Word16 g_coeff[],     /* (i)     : Correlations <xn y1> -2<y1 y1>             */
                               /*            <y2,y2>, -2<xn,y2>, 2<y1,y2>              */
    G729_Word16 exp_coeff[],   /* (i)    : Q-Format g_coeff[]                         */
    G729_Word16 L_subfr,       /* (i)     : Subframe length.                           */
    G729_Word16 *gain_pit,     /* (o) Q14 : Pitch gain.                                */
    G729_Word16 *gain_cod,     /* (o) Q1  : Code gain.                                 */
    G729_Word16 tameflag       /* (i)     : flag set to 1 if taming is needed          */
);
    
/*--------------------------------------------------------------------------*
 * acelp_ca.c                                                               *
 *--------------------------------------------------------------------------*/
    
G729_Word16 g729_ACELP_Code_A(     /* (o)     :index of pulses positions    */
    g729a_encoder_state * state,   /* (i/o)   :Encoder state (scratch)      */
    G729_Word16 x[],               /* (i)     :Target vector                */
    G729_Word16 h[],               /* (i) Q12 :Inpulse response of filters  */
    G729_Word16 T0,                /* (i)     :Pitch lag                    */
    G729_Word16 pitch_sharp,       /* (i) Q14 :Last quantized pitch gain    */
    G729_Word16 code[],            /* (o) Q13 :Innovative codebook          */
    G729_Word16 y[],               /* (o) Q12 :Filtered innovative codebook */
    G729_Word16 *sign              /* (o)     :Signs of 4 pulses            */
);
    
/*--------------------------------------------------------------------------*
 * Annex B (VAD/DTX/CNG)                                                    *
 *--------------------------------------------------------------------------*/

/*-------------------------------*
 * vad.c                         *
 *-------------------------------*/

void g729_Init_Vad(g729a_vad_state * state);

void g729_Vad(
    g729a_vad_state * state,
    G729_Word16 rc,            /* (i) : reflection coefficient                      */
    G729_Word16 *lsf,          /* (i) : unquantized lsf vector                      */
    G729_Word16 *r_h,          /* (i) : MSB of the autocorrelation vector           */
    G729_Word16 *r_l,          /* (i) : LSB of the autocorrelation vector           */
    G729_Word16 exp_R0,        /* (i) : exponent of the autocorrelation vector      */
    G729_Word16 *sigpp,        /* (i) : preprocessed input signal                   */
    G729_Word16 frm_count,     /* (i) : frame counter                               */
    G729_Word16 prev_marker,   /* (i) : VAD decision of the last frame              */
    G729_Word16 pprev_marker,  /* (i) : VAD decision of the frame before last frame */
    G729_Word16 *marker        /* (o) : VAD decision of the current frame           */
);

/*-------------------------------*
 * dtx.c                         *
 *-------------------------------*/

void g729_Init_Cod_cng(g729a_dtx_state * state);

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
                               /* (i/o) : taming state for excitation update   */
);

void g729_Update_cng(
    g729a_dtx_state * state,
    G729_Word16 *r_h,          /* (i) :   MSB of frame autocorrelation        */
    G729_Word16 exp_r,         /* (i) :   scaling factor associated           */
    G729_Word16 Vad            /* (i) :   current Vad decision                */
);

/*-------------------------------*
 * qsidlsf.c                     *
 *-------------------------------*/

void g729_lsfq_noise(
    G729_Word16 *lsp,          /* (i)   : unquantized lsp vector              */
    G729_Word16 *lspq,         /* (o)   : quantized lsp vector                */
    const G729_Word16 noise_fg[MODE][MA_NP][M],
                               /* (i)   : noise LSF predictor coefficients    */
    G729_Word16 freq_prev[MA_NP][M],
                               /* (i/o) : memory of the lsf predictor         */
    G729_Word16 *ana           /* (o)   : indices                             */
);

/*-------------------------------*
 * qsidgain.c                    *
 *-------------------------------*/

void g729_Qua_Sidgain(
    G729_Word16 *ener,         /* (i)   array of energies                   */
    G729_Word16 *sh_ener,      /* (i)   corresponding scaling factors       */
    G729_Word16 nb_ener,       /* (i)   number of energies or               */
    G729_Word16 *enerq,        /* (o)   decoded energies in dB              */
    G729_Word16 *idx           /* (o)   SID gain quantization index         */
);

/*-------------------------------*
 * tab_dtx.c                     *
 *-------------------------------*/

void g729_Init_lsfq_noise(
    G729_Word16 noise_fg[MODE][MA_NP][M] /* (o) : noise LSF predictor coef.  */
);

/*-------------------------------*
 * calcexc.c                     *
 *-------------------------------*/

void g729_Calc_exc_rand(
    G729_Word16 cur_gain,      /* (i)   :   target sample gain                 */
    G729_Word16 *exc,          /* (i/o) :   excitation array                   */
    G729_Word16 *seed,         /* (i/o) :   random generator seed              */
    G729_Flag flag_cod,        /* (i)   :   encoder/decoder flag               */
    g729a_taming_state * taming_state
                               /* (i/o) :   taming state (encoder only,        */
                               /*           pass 0 at the decoder)             */
);
    
#ifdef __cplusplus
}
#endif

#endif  /* __G729_ENCODER_H__ */
/* end of file */
