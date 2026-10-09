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
 * TAB_DTX.H                                                                *
 * ~~~~~~~~~                                                                *
 * Tables used for VAD/DTX/CNG (G.729 Annex B).                             *
 *                                                                          *
 * Note: the ITU reference keeps noise_fg[MODE][MA_NP][M] here as a global  *
 * table initialized at run time by Init_lsfq_noise(). In this port it is   *
 * per-instance state (g729a_dtx_state / g729a_cng_state), filled by        *
 * g729_Init_lsfq_noise().                                                  *
 *--------------------------------------------------------------------------*/

#ifndef __G729_TAB_DTX_H__
#define __G729_TAB_DTX_H__

#ifdef __cplusplus
extern "C" {
#endif

/* VAD constants */
extern const G729_Word16 g729_lbf_corr[NP+1];
extern const G729_Word16 g729_shift_fx[33];
extern const G729_Word16 g729_factor_fx[33];

/* SID LSF quantization */
extern const G729_Word16 g729_noise_fg_sum[MODE][M];
extern const G729_Word16 g729_noise_fg_sum_inv[MODE][M];
extern const G729_Word16 g729_PtrTab_1[32];
extern const G729_Word16 g729_PtrTab_2[2][16];
extern const G729_Word16 g729_Mp[MODE];

/* SID gain quantization */
extern const G729_Word16 g729_fact[NB_GAIN+1];
extern const G729_Word16 g729_marg[NB_GAIN+1];
extern const G729_Word16 g729_tab_Sidgain[32];

#ifdef __cplusplus
}
#endif

#endif  /* __G729_TAB_DTX_H__ */
/* end of file */
