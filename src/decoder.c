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

/*-----------------------------------------------------------------*
 * Main program of the G.729a 8.0 kbit/s decoder (with Annex B).   *
 *                                                                 *
 *    Usage : decoder  bitstream_file  synth_file  [VAD_flag]      *
 *-----------------------------------------------------------------*/

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS   /* fopen() in this ITU-style test main */
#endif

#include <stdlib.h>
#include <stdio.h>

#include "g729a_typedef.h"
#include "g729a_interface.h"

#define FRAMESIZE      80

#if defined(CONTROL_OPT_ITU) && (CONTROL_OPT_ITU == 1)
#define SERIALSIZE     (80+2)   /* SYNC + SIZE + up to 80 bit words    */
#else
#define SERIALSIZE     10
#endif

/*-----------------------------------------------------------------*
 *            Main decoder routine                                 *
 *-----------------------------------------------------------------*/

int main(int argc, char *argv[] )
{
#if defined(CONTROL_OPT_ITU) && (CONTROL_OPT_ITU == 1)
    G729_Word16
#else
    G729_UWord8
#endif
    serial[SERIALSIZE];
    G729_Word16  speechOut[FRAMESIZE];
    
    G729_Word16 frame;
    G729_UWord32 nb_in;                 /* words/bytes read for this frame */
    G729_Word32 vad_flag;               /* Annex B container format flag   */
    FILE   *f_syn, *f_serial;
    
    G729A_Dec_state state;
    
    printf("\n");
    printf("************   G.729a 8.0 KBIT/S SPEECH DECODER  ************\n");
    printf("                       (WITH ANNEX B)                           \n");
    printf("\n");
    printf("------------------- Fixed point C simulation ----------------\n");
    printf("\n");
    printf("------------ Version 1.5 (Release 2, November 2006) --------\n");
    printf("\n");
    
    /* Passed arguments */
    
    if ( argc != 3 && argc != 4)
    {
        printf("Usage :%s bitstream_file  outputspeech_file  [VAD_flag]\n",argv[0]);
        printf("\n");
        printf("Format for bitstream_file (ITU test format build):\n");
        printf("  One (2-byte) synchronization word \n");
        printf("  One (2-byte) size word,\n");
        printf("  SIZE words (2-byte) containing the frame bits (80 for a speech\n");
        printf("  frame, 16 for a SID frame, 0 for an untransmitted frame).\n");
        printf("\n");
        printf("VAD_flag (compressed format build only):\n");
        printf("  1 if the input uses the demonstration length-prefixed\n");
        printf("  container written by 'coder' with VAD enabled\n");
        printf("\n");
        printf("Format for outputspeech_file:\n");
        printf("  Synthesis is written to a binary file of 16 bits data.\n");
        exit( 1 );
    }
    
    /* Open file for synthesis and packed serial stream */
    
    if( (f_serial = fopen(argv[1],"rb") ) == NULL )
    {
        printf("%s - Error opening file  %s !!\n", argv[0], argv[1]);
        exit(0);
    }
    
    if( (f_syn = fopen(argv[2], "wb") ) == NULL )
    {
        printf("%s - Error opening file  %s !!\n", argv[0], argv[2]);
        exit(0);
    }
    
    printf("Input bitstream file  :   %s\n",argv[1]);
    printf("Synthesis speech file :   %s\n",argv[2]);
    
    vad_flag = 0;
    if ( argc == 4 ) vad_flag = atoi(argv[3]);
    
    /*-----------------------------------------------------------------*
     *           Initialization of decoder                             *
     *-----------------------------------------------------------------*/
    
    state = malloc(G729A_Decoder_Get_Size());
    if ( NULL == state ) return -1;
    
    G729A_Decoder_Init(state);
    
    /*-----------------------------------------------------------------*
     *            Loop for each "L_FRAME" speech data                  *
     *-----------------------------------------------------------------*/
    
    frame = 0;
    
    for (;;)
    {
#if defined(CONTROL_OPT_ITU) && (CONTROL_OPT_ITU == 1)
        /* Variable length frames: SYNC word + SIZE word + SIZE bit words */
        if ( fread(serial, sizeof(G729_Word16), 2, f_serial) != 2 ) break;
        nb_in = (G729_UWord32)serial[1] + 2;
        if ( serial[1] > 0 )
        {
            if ( fread(&serial[2], sizeof(G729_Word16), serial[1], f_serial)
                 != (size_t)serial[1] ) break;
        }
        G729A_Decoder_Process_Testing(state, serial, nb_in, speechOut);
#else
        if ( vad_flag == 1 )
        {
            /* Demonstration length-prefixed container (see coder.c) */
            G729_UWord8 len;
            if ( fread(&len, sizeof(G729_UWord8), 1, f_serial) != 1 ) break;
            nb_in = len;
            if ( len > 0 )
            {
                if ( fread(serial, sizeof(G729_UWord8), len, f_serial)
                     != (size_t)len ) break;
            }
        }
        else
        {
            /* Legacy raw stream: fixed 10 bytes per frame */
            nb_in = 10;
            if ( fread(serial, sizeof(G729_UWord8), 10, f_serial) != 10 ) break;
        }
        G729A_Decoder_Process(state, serial, nb_in, speechOut);
#endif
        
        printf("Frame =%d\r", frame++);
        fwrite(speechOut, sizeof(G729_Word16), FRAMESIZE, f_syn);
    }
    
    printf("Frame =%d\n", frame);
    return(0);
}
