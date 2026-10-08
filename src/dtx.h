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
 * DTX.H                                                                    *
 * ~~~~~                                                                    *
 * Constants for DTX/CNG (G.729 Annex B).                                   *
 *--------------------------------------------------------------------------*/

#ifndef __G729_DTX_H__
#define __G729_DTX_H__

/*--------------------------------------------------------------------------*
 * Constants for DTX/CNG                                                    *
 *--------------------------------------------------------------------------*/

/* DTX constants */
#define FLAG_COD        (G729_Flag)1
#define FLAG_DEC        (G729_Flag)0
#define INIT_SEED       11111
#define FR_SID_MIN      3
#define NB_SUMACF       3
#define NB_CURACF       2
#define NB_GAIN         2
#define FRAC_THRESH1    4855
#define FRAC_THRESH2    3161
#define A_GAIN0         28672

#define SIZ_SUMACF      (NB_SUMACF * MP1)
#define SIZ_ACF         (NB_CURACF * MP1)
#define A_GAIN1         4096    /* 32768L - A_GAIN0 */

#define RATE_8000       80      /* Full rate  (8000 bit/s)       */
#define RATE_SID        15      /* SID                           */
#define RATE_0           0      /* 0 bit/s rate                  */

/* CNG excitation generation constant */
                                        /* alpha = 0.5 */
#define FRAC1           19043           /* (sqrt(40)xalpha/2 - 1) * 32768 */
#define K0              24576           /* (1 - alpha ** 2) in Q15        */
#define G_MAX           5000

#endif  /* __G729_DTX_H__ */
/* end of file */
