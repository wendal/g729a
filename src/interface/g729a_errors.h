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

#ifndef __G729_ERRORS_H__
#define __G729_ERRORS_H__

/*--------------------------------------------------------------------------*
 * Error codes                                                              *
 *                                                                          *
 * The public API functions return G729A_NO_ERROR (0) on success and a      *
 * negative code on failure.  The last error of an initialized state is     *
 * kept in the state and can be read back with G729A_Encoder_Get_Error()    *
 * and G729A_Decoder_Get_Error().                                           *
 *--------------------------------------------------------------------------*/

#define G729A_NO_ERROR              0

/*---------------------------------------------*
 * Generic errors (encoder and decoder)        *
 *---------------------------------------------*/

#define G729A_ERROR_NULL_STATE      (-1)  /* state pointer is NULL                       */
#define G729A_ERROR_NULL_BUFFER     (-2)  /* input or output buffer pointer is NULL      */
#define G729A_ERROR_NOT_INITIALIZED (-3)  /* state was not set up by G729A_*_Init()      */

/* Value the Init functions store in the magic field of a state struct.
   Any other value means the state was never initialized. */
#define G729A_STATE_MAGIC           0x47373239u  /* 'G','7','2','9' */

#endif  /* __G729_ERRORS_H__ */
/* end of file */
