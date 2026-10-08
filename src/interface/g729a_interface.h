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

#ifndef __G729A_INTERFACE_H__
#define __G729A_INTERFACE_H__

#include "g729a_typedef.h"
#include "g729a_errors.h"

typedef void * G729A_Enc_state;
typedef void * G729A_Dec_state;

/*---------------------------------------------*
 * Frame lengths in bytes (G.729 Annex B)      *
 *                                             *
 * The frame length also encodes the frame     *
 * type: 10 = voice, 2 = SID, 0 = untransmitted*
 *---------------------------------------------*/

#define G729A_FRAME_LEN_VOICE   10
#define G729A_FRAME_LEN_SID      2
#define G729A_FRAME_LEN_NODATA   0

#ifdef __cplusplus
extern "C" {
#endif
    
/*---------------------------------------------*
 * Encoder functions                           *
 *---------------------------------------------*/
    
/**
 *  @brief  Get size in bytes of the g729a encoder state.
 *
 *  @return  Number of bytes in g729a encoder state.
 */
G729_UWord32 G729A_Encoder_Get_Size(void);
    
/**
 *  @brief  Init or reset encoder.
 *
 *  @param encState,  Encoder state, a buffer of G729A_Encoder_Get_Size() bytes.
 *
 *  @return  G729A_NO_ERROR, succeeded
 *           G729A_ERROR_NULL_STATE, if encState is NULL
 */
G729_Word32 G729A_Encoder_Init(G729A_Enc_state encState);
    
/**
 *  @brief  Enable or disable the VAD/DTX (G.729 Annex B). Default is off.
 *
 *  @param encState,  Encoder state.
 *  @param on,        0 to disable the VAD (every frame is encoded as a
 *                    10-byte voice frame), 1 to enable it.
 *
 *  @return  G729A_NO_ERROR, succeeded
 *           G729A_ERROR_NULL_STATE, if encState is NULL
 *           G729A_ERROR_NOT_INITIALIZED, if encState was not initialized
 */
G729_Word32 G729A_Encoder_Set_Vad(G729A_Enc_state encState, G729_Word32 on);
    
/**
 *  @brief  Encode a frame of 16-bit linear PCM data with g729a (+Annex B).
 *
 *  @param encState,  Encoder state.
 *  @param speechIn,  Speech sample input vector (80 samples, read only).
 *  @param outData,   Encoded output vector (provide at least 10 bytes).
 *  @param outLen,    Receives the encoded frame length in bytes, which is
 *                    also the frame type: G729A_FRAME_LEN_VOICE (10),
 *                    G729A_FRAME_LEN_SID (2) or G729A_FRAME_LEN_NODATA (0).
 *                    Always 10 when the VAD is disabled.
 *
 *  @return  G729A_NO_ERROR, succeeded
 *           G729A_ERROR_NULL_STATE, if encState is NULL
 *           G729A_ERROR_NULL_BUFFER, if speechIn, outData or outLen is NULL
 *           G729A_ERROR_NOT_INITIALIZED, if encState was not initialized,
 *               and you can use G729A_Encoder_Get_Error to get the last error code.
 */
G729_Word32 G729A_Encoder_Process(G729A_Enc_state encState, const G729_Word16 * speechIn, G729_UWord8 * outData, G729_UWord32 * outLen);
    
/**
 *  @brief  Get last error code of encoder.
 *
 *  @param encState,  encoder state.
 *
 *  @return  G729A_ERROR_NULL_STATE, if encState is NULL
 *           G729A_ERROR_NOT_INITIALIZED, if encState was not initialized,
 *           otherwise, return the last error code of encoder
 */
G729_Word32 G729A_Encoder_Get_Error(G729A_Enc_state encState);


/*---------------------------------------------*
 * Decoder functions                           *
 *---------------------------------------------*/
    
/**
 *  @brief  Get size in bytes of the g729a decoder state.
 *
 *  @return  Number of bytes in g729a decoder state.
 */
G729_UWord32 G729A_Decoder_Get_Size(void);

/**
 *  @brief  Init or reset decoder.
 *
 *  @param decState,  Decoder state, a buffer of G729A_Decoder_Get_Size() bytes.
 *
 *  @return  G729A_NO_ERROR, succeeded
 *           G729A_ERROR_NULL_STATE, if decState is NULL
 */
G729_Word32 G729A_Decoder_Init(G729A_Dec_state decState);
    
/**
 *  @brief  Decode a frame of g729a (+Annex B) encoded bitstream data.
 *
 *  @param decState,   Decoder state.
 *  @param inData,     Encoded input vector (inLen bytes, read only).
 *                     May be NULL when inLen is G729A_FRAME_LEN_NODATA (0).
 *  @param inLen,      Frame length in bytes: G729A_FRAME_LEN_VOICE (10),
 *                     G729A_FRAME_LEN_SID (2) or G729A_FRAME_LEN_NODATA (0).
 *  @param speechOut,  Decoded output speech vector (80 samples).
 *
 *  @return  G729A_NO_ERROR, succeeded
 *           G729A_ERROR_NULL_STATE, if decState is NULL
 *           G729A_ERROR_NULL_BUFFER, if inData or speechOut is NULL
 *           G729A_ERROR_BAD_LENGTH, if inLen is not 10, 2 or 0
 *           G729A_ERROR_NOT_INITIALIZED, if decState was not initialized,
 *               and you can use G729A_Decoder_Get_Error to get the last error code.
 */
G729_Word32 G729A_Decoder_Process(G729A_Dec_state decState, const G729_UWord8 * inData, G729_UWord32 inLen, G729_Word16 * speechOut);

/**
 *  @brief  Get last error code of decoder.
 *
 *  @param decState,  Decoder state.
 *
 *  @return  G729A_ERROR_NULL_STATE, if decState is NULL
 *           G729A_ERROR_NOT_INITIALIZED, if decState was not initialized,
 *           otherwise, return the last error code of decoder
 */
G729_Word32 G729A_Decoder_Get_Error(G729A_Dec_state decState);
    
    
/*---------------------------------------------*
 * Generic functions                           *
 *---------------------------------------------*/
    
/**
 *  @brief  Get the version number.
 *
 *  @return  A pointer to string specifying the version.
 */
const char * G729A_Get_Version(void);
    
    
/*---------------------------------------------*
 * Testing functions                           *
 * Derived from ITU official testing code      *
 *---------------------------------------------*/
    
/**
 *  @param encState,  Encoder state.
 *  @param speechIn,  Speech sample input vector (80 samples, read only).
 *  @param outData,   Encoded output vector, ITU serial word format
 *                    (SYNC word, SIZE word, then SIZE 16-bit words;
 *                    provide room for 82 words).
 *  @param outLen,    Receives the number of 16-bit words written
 *                    (SIZE+2: 82 for a voice frame, 18 for a SID frame,
 *                    2 for an untransmitted frame).
 */
G729_Word32 G729A_Encoder_Process_Testing(G729A_Enc_state encState, const G729_Word16 * speechIn, G729_Word16 * outData, G729_UWord32 * outLen);
    
/**
 *  @param decState,   Decoder state.
 *  @param inData,     Encoded input vector, ITU serial word format
 *                     (SYNC word, SIZE word, then SIZE 16-bit words).
 *  @param inLen,      Number of 16-bit words in inData (SIZE+2).
 *  @param speechOut,  Decoded output speech vector (80 samples).
 */
G729_Word32 G729A_Decoder_Process_Testing(G729A_Dec_state decState, const G729_Word16 * inData, G729_UWord32 inLen, G729_Word16 * speechOut);
    
#ifdef __cplusplus
}
#endif

#endif  /* __G729A_INTERFACE_H__ */
/* end of file */
