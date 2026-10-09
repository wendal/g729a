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
 *  ITU-T G.729A Speech Coder with Annex B    ANSI-C Source Code
 *  Version 1.5    Last modified: October 2006
 *
 *  Copyright (c) 1996,
 *  AT&T, France Telecom, NTT, Universite de Sherbrooke, Lucent Technologies,
 *  Rockwell International
 *  All rights reserved.
 */

/*-------------------------------------------------------------------*
 * Main program of the ITU-T G.729A  8 kbit/s encoder (with Annex B).*
 *                                                                   *
 *    Usage : coder speech_file  bitstream_file  [VAD_flag]          *
 *-------------------------------------------------------------------*/

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS   /* fopen() in this ITU-style test main */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "g729a_typedef.h"
#include "g729a_interface.h"

#define FRAMESIZE      80

#if defined(CONTROL_OPT_ITU) && (CONTROL_OPT_ITU == 1)
#define SERIALSIZE     (80+2)   /* SYNC + SIZE + up to 80 bit words    */
#else
#define SERIALSIZE     10
#endif

int main(int argc, char *argv[])
{
    FILE *f_speech;               /* File of speech data                   */
    FILE *f_serial;               /* File of serial bits for transmission  */
    
    G729_Word16 speechIn[FRAMESIZE];
    
#if defined(CONTROL_OPT_ITU) && (CONTROL_OPT_ITU == 1)
    G729_Word16
#else
    G729_UWord8
#endif
    serial[SERIALSIZE];     /* Output bitstream buffer               */
    
    G729_Word16 frame;                  /* frame counter */
    G729_UWord32 nb_out;                /* words/bytes written this frame */
    G729_Word32 vad_enable;             /* VAD flag (G.729 Annex B) */
    
    G729A_Enc_state state;
    
    printf("\n");
    printf("***********    ITU G.729A 8 KBIT/S SPEECH CODER    ***********\n");
    printf("                        (WITH ANNEX B)                        \n");
    printf("\n");
    printf("------------------- Fixed point C simulation -----------------\n");
    printf("\n");
    printf("------------ Version 1.5 (Release 2, November 2006) --------\n");
    printf("\n");
    
    
    /*--------------------------------------------------------------------------*
     * Open speech file and result file (output serial bit stream)              *
     *--------------------------------------------------------------------------*/
    
    if ( argc != 3 && argc != 4 )
    {
        printf("Usage : coder speech_file  bitstream_file  [VAD_flag]\n");
        printf("\n");
        printf("Format for speech_file:\n");
        printf("  Speech is read from a binary file of 16 bits PCM data.\n");
        printf("\n");
        printf("Format for bitstream_file (ITU test format build):\n");
        printf("  One (2-byte) synchronization word \n");
        printf("  One (2-byte) size word,\n");
        printf("  80 words (2-byte) containing 80 bits (16 for a SID frame,\n");
        printf("  none for an untransmitted frame).\n");
        printf("\n");
        printf("VAD flag:\n");
        printf("  0 to disable the VAD (default)\n");
        printf("  1 to enable the VAD\n");
        exit(1);
    }
    
    if ( (f_speech = fopen(argv[1], "rb")) == NULL) {
        printf("%s - Error opening file  %s !!\n", argv[0], argv[1]);
        exit(0);
    }
    printf(" Input speech file    :  %s\n", argv[1]);
    
    if ( (f_serial = fopen(argv[2], "wb")) == NULL) {
        printf("%s - Error opening file  %s !!\n", argv[0], argv[2]);
        exit(0);
    }
    printf(" Output bitstream file:  %s\n", argv[2]);
    
    vad_enable = 0;
    if ( argc == 4 ) vad_enable = atoi(argv[3]);
    if ( vad_enable == 1 )
        printf(" VAD enabled\n");
    else
        printf(" VAD disabled\n");
    
    /*--------------------------------------------------------------------------*
     * Initialization of the coder.                                             *
     *--------------------------------------------------------------------------*/
    
    state = malloc(G729A_Encoder_Get_Size());
    if ( NULL == state ) return -1;
    
    G729A_Encoder_Init(state);
    G729A_Encoder_Set_Vad(state, vad_enable);
    
    /* Loop for each "L_FRAME" speech data. */
    
    frame =0;
    while( fread(speechIn, sizeof(G729_Word16), FRAMESIZE, f_speech) == FRAMESIZE)
    {
        printf("Frame =%d\r", frame++);
        
#if defined(CONTROL_OPT_ITU) && (CONTROL_OPT_ITU == 1)
        G729A_Encoder_Process_Testing(state, speechIn, serial, &nb_out);
        fwrite(serial, sizeof(G729_Word16), nb_out, f_serial);
#else
        G729A_Encoder_Process(state, speechIn, serial, &nb_out);
        if ( vad_enable == 1 )
        {
            /* Demonstration container for the compressed format with VAD
               enabled: one length byte, then the payload (0, 2 or 10 bytes).
               With the VAD disabled the output stays the legacy raw
               10-bytes-per-frame stream. */
            G729_UWord8 len = (G729_UWord8)nb_out;
            fwrite(&len, sizeof(G729_UWord8), 1, f_serial);
        }
        fwrite(serial, sizeof(G729_UWord8), nb_out, f_serial);
#endif
    }
    
    printf("Frame =%d\n", frame);
    return (0);
}
