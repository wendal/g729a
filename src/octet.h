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
 * OCTET.H                                                                  *
 * ~~~~~~~                                                                  *
 * Definition for Octet Transmission mode.                                  *
 *--------------------------------------------------------------------------*/

#ifndef __G729_OCTET_H__
#define __G729_OCTET_H__

/* Definition for Octet Transmission mode */
/* When Annex B is used for transmission systems that operate on octet boundary,
   an extra bit (with value zero) will be packed at the end of a SID bit stream.
   This will change the number of bits in a SID bit stream from 15 bits to
   16 bits (i.e., 2 bytes).
*/

#define OCTET_TX_MODE
#define RATE_SID_OCTET    16     /* number of bits in Octet Transmission mode */

#endif  /* __G729_OCTET_H__ */
/* end of file */
