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

#include "g729a_interface.h"

#include <stddef.h>

#include "g729a_encoder.h"
#include "g729a_decoder.h"

#include "ld8a.h"

/*---------------------------------------------*
 * Encoder functions                           *
 *---------------------------------------------*/

G729_UWord32 G729A_Encoder_Get_Size(void)
{
    return sizeof(g729a_encoder_state);
}

G729_Word32 G729A_Encoder_Init(G729A_Enc_state encState)
{
    g729a_encoder_state * state;
    if ( NULL == encState ) return G729A_ERROR_NULL_STATE;
    
    state = (g729a_encoder_state *)encState;
    
    g729_Init_Pre_Process(&(state->pre_process_state));
    g729_Init_Coder_ld8a(state);
    
    state->magic = G729A_ENC_STATE_MAGIC;
    state->error = G729A_NO_ERROR;
    
    return G729A_NO_ERROR;
}

G729_Word32 G729A_Encoder_Set_Vad(G729A_Enc_state encState, G729_Word32 on)
{
    g729a_encoder_state * state;
    if ( NULL == encState ) return G729A_ERROR_NULL_STATE;
    
    state = (g729a_encoder_state *)encState;
    
    if ( G729A_ENC_STATE_MAGIC != state->magic ) return G729A_ERROR_NOT_INITIALIZED;
    
    state->vad_enable = ( on != 0 ) ? 1 : 0;
    
    return G729A_NO_ERROR;
}

G729_Word32 G729A_Encoder_Process(G729A_Enc_state encState, const G729_Word16 * speechIn, G729_UWord8 * outData, G729_UWord32 * outLen)
{
    g729a_encoder_state * state;
    G729_Word16 prm[PRM_SIZE+1];  /* Frame type + analysis parameters. */
    
    if ( NULL == encState ) return G729A_ERROR_NULL_STATE;
    
    state = (g729a_encoder_state *)encState;
    
    if ( G729A_ENC_STATE_MAGIC != state->magic ) return G729A_ERROR_NOT_INITIALIZED;
    if ( (NULL == speechIn) || (NULL == outData) || (NULL == outLen) )
    {
        state->error = G729A_ERROR_NULL_BUFFER;
        return state->error;
    }
    
    state->error = G729A_NO_ERROR;
    
    /* frame counter for the VAD, same wraparound as the ITU reference coder */
    if ( state->frame == 32767 ) state->frame = 256;
    else state->frame++;
    
    g729_Pre_Process(&(state->pre_process_state), speechIn, state->new_speech, L_FRAME);
    g729_Coder_ld8a(state, prm, state->frame, state->vad_enable);
    *outLen = (G729_UWord32)g729_prm2bits_ld8k_compressed(prm, outData);
    
    return G729A_NO_ERROR;
}

G729_Word32 G729A_Encoder_Get_Error(G729A_Enc_state encState)
{
    g729a_encoder_state * state;
    if ( NULL == encState ) return G729A_ERROR_NULL_STATE;
    
    state = (g729a_encoder_state *)encState;
    
    if ( G729A_ENC_STATE_MAGIC != state->magic ) return G729A_ERROR_NOT_INITIALIZED;
    
    return state->error;
}

/*---------------------------------------------*
 * Decoder functions                           *
 *---------------------------------------------*/

G729_UWord32 G729A_Decoder_Get_Size(void)
{
    return sizeof(g729a_decoder_state);
}

G729_Word32 G729A_Decoder_Init(G729A_Dec_state decState)
{
    g729a_decoder_state * state;
    if ( NULL == decState ) return G729A_ERROR_NULL_STATE;
    
    state = (g729a_decoder_state *)decState;
    
    g729_Set_zero(state->synth_buf, M);
    state->synth = state->synth_buf + M;
    
    state->random_seed = 21845;
    state->bad_lsf = 0;
    
    g729_Init_Decod_ld8a(state);
    g729_Init_Post_Filter(&(state->post_filter_state));
    g729_Init_Post_Process(&(state->post_process_state));
    
    state->magic = G729A_DEC_STATE_MAGIC;
    state->error = G729A_NO_ERROR;
    
    return G729A_NO_ERROR;
}

G729_Word32 G729A_Decoder_Process(G729A_Dec_state decState, const G729_UWord8 * inData, G729_UWord32 inLen, G729_Word16 * speechOut)
{
    G729_Word16  parm[PRM_SIZE+2];           /* Synthesis parameters        */
    G729_Word16  Az_dec[MP1*2];              /* Decoded Az for post-filter  */
    G729_Word16  T2[2];                      /* Pitch lag for 2 subframes   */
    G729_Word16  Vad;                        /* Frame type                  */

    g729a_decoder_state *state;
    if ( NULL == decState ) return G729A_ERROR_NULL_STATE;
    
    state = (g729a_decoder_state *)decState;

    if ( G729A_DEC_STATE_MAGIC != state->magic ) return G729A_ERROR_NOT_INITIALIZED;
    if ( NULL == speechOut )
    {
        state->error = G729A_ERROR_NULL_BUFFER;
        return state->error;
    }
    if ( (inLen != G729A_FRAME_LEN_VOICE) && (inLen != G729A_FRAME_LEN_SID) && (inLen != G729A_FRAME_LEN_NODATA) )
    {
        state->error = G729A_ERROR_BAD_LENGTH;
        return state->error;
    }
    if ( (NULL == inData) && (inLen != G729A_FRAME_LEN_NODATA) )
    {
        state->error = G729A_ERROR_NULL_BUFFER;
        return state->error;
    }
    
    state->error = G729A_NO_ERROR;
    
    g729_bits2prm_ld8k_compressed(inData, parm, (G729_Word16)inLen);
    
    parm[0] = 0;           /* No frame erasure */
    
    if ( parm[1] == 1 )
    {
        /* check pitch parity and put 1 in parm[5] if parity error */
        parm[5] = g729_Check_Parity_Pitch(parm[4], parm[5]);
    }
    
    g729_Decod_ld8a(state, parm, state->synth, Az_dec, T2, state->bad_lsf, &Vad);
    g729_Post_Filter(&(state->post_filter_state), state->synth, Az_dec, T2, Vad);
    g729_Post_Process(&(state->post_process_state), state->synth, speechOut, L_FRAME);
    
    return G729A_NO_ERROR;
}

G729_Word32 G729A_Decoder_Get_Error(G729A_Dec_state decState)
{
    g729a_decoder_state * state;
    if ( NULL == decState ) return G729A_ERROR_NULL_STATE;
    
    state = (g729a_decoder_state *)decState;
    
    if ( G729A_DEC_STATE_MAGIC != state->magic ) return G729A_ERROR_NOT_INITIALIZED;
    
    return state->error;
}

/*---------------------------------------------*
 * Generic functions                           *
 *---------------------------------------------*/

const char * G729A_Get_Version(void)
{
    static const char * version = "2.0";
    return version;
}

/*---------------------------------------------*
 * Testing functions                           *
 *---------------------------------------------*/

G729_Word32 G729A_Encoder_Process_Testing(G729A_Enc_state encState, const G729_Word16 * speechIn, G729_Word16 * outData, G729_UWord32 * outLen)
{
    g729a_encoder_state * state;
    G729_Word16 prm[PRM_SIZE+1];  /* Frame type + analysis parameters. */
    
    if ( NULL == encState ) return G729A_ERROR_NULL_STATE;
    
    state = (g729a_encoder_state *)encState;
    
    if ( G729A_ENC_STATE_MAGIC != state->magic ) return G729A_ERROR_NOT_INITIALIZED;
    if ( (NULL == speechIn) || (NULL == outData) || (NULL == outLen) )
    {
        state->error = G729A_ERROR_NULL_BUFFER;
        return state->error;
    }
    
    state->error = G729A_NO_ERROR;
    
    /* frame counter for the VAD, same wraparound as the ITU reference coder */
    if ( state->frame == 32767 ) state->frame = 256;
    else state->frame++;
    
    g729_Pre_Process(&(state->pre_process_state), speechIn, state->new_speech, L_FRAME);
    g729_Coder_ld8a(state, prm, state->frame, state->vad_enable);
    g729_prm2bits_ld8k(prm, outData);
    
    *outLen = (G729_UWord32)outData[1] + 2;   /* SYNC word + SIZE word + SIZE words */
    
    return G729A_NO_ERROR;
}

G729_Word32 G729A_Decoder_Process_Testing(G729A_Dec_state decState, const G729_Word16 * inData, G729_UWord32 inLen, G729_Word16 * speechOut)
{
    G729_Word16 i;
    G729_Word16 parm[PRM_SIZE+2];           /* Synthesis parameters        */
    G729_Word16 Az_dec[MP1*2];              /* Decoded Az for post-filter  */
    G729_Word16 T2[2];                      /* Pitch lag for 2 subframes   */
    G729_Word16 Vad;                        /* Frame type                  */
    
    g729a_decoder_state *state;
    if ( NULL == decState ) return G729A_ERROR_NULL_STATE;
    
    state = (g729a_decoder_state *)decState;
    
    if ( G729A_DEC_STATE_MAGIC != state->magic ) return G729A_ERROR_NOT_INITIALIZED;
    if ( (NULL == inData) || (NULL == speechOut) )
    {
        state->error = G729A_ERROR_NULL_BUFFER;
        return state->error;
    }
    if ( (inLen < 2)
         || (inData[1] < 0) || (inData[1] > 80)
         || ((G729_UWord32)inData[1] + 2 != inLen) )
    {
        /* SIZE word inconsistent with the frame length, or out of range */
        state->error = G729A_ERROR_BAD_LENGTH;
        return state->error;
    }
    
    state->error = G729A_NO_ERROR;
    
    g729_bits2prm_ld8k(&inData[1], parm);
    
    /* Frame erasure detection, from the ITU reference read_frame()
       (bits.c, Annex B): for speech and SID frames, the hardware detects
       frame erasures by checking if any bit is set to zero; for
       untransmitted frames, by testing the sync word. */
    parm[0] = 0;           /* No frame erasure */
    if ( inData[1] != 0 )
    {
        for ( i = 0; i < inData[1]; ++i )
        {
            if ( inData[i+2] == 0 ) parm[0] = 1;    /* frame erased */
        }
    }
    else if ( inData[0] != SYNC_WORD ) parm[0] = 1;
    
    if ( parm[1] == 1 )
    {
        /* check pitch parity and put 1 in parm[5] if parity error */
        parm[5] = g729_Check_Parity_Pitch(parm[4], parm[5]);
    }
    
    g729_Decod_ld8a(state, parm, state->synth, Az_dec, T2, state->bad_lsf, &Vad);
    g729_Post_Filter(&(state->post_filter_state), state->synth, Az_dec, T2, Vad);
    g729_Post_Process(&(state->post_process_state), state->synth, speechOut, L_FRAME);
    
    return G729A_NO_ERROR;
}
/* end of file */
