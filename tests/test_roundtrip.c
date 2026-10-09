/**
 *  Copyright (c) 2026, wendal
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
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

/*-----------------------------------------------------------------*
 * Round-trip self test of the compressed (octet) public API,      *
 * exercising the Annex B variable frame lengths (10/2/0 bytes).   *
 * Exits non-zero on any failure.                                  *
 *-----------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "g729a_interface.h"

#define N_SILENCE   300
#define N_ACTIVE    100

static int failures = 0;

#define CHECK(cond, msg) do { \
    if ( !(cond) ) { printf("FAIL: %s\n", msg); failures++; } \
} while (0)

int main(void)
{
    G729A_Enc_state enc, enc2;
    G729A_Dec_state dec;
    G729_Word16 pcm[80];
    G729_Word16 out[80];
    G729_UWord8 bits[10];
    G729_UWord8 bits_ref[10];
    G729_UWord32 len, len_ref;
    int i, f;
    int voice, sid, nodata;

    enc = malloc(G729A_Encoder_Get_Size());
    enc2 = malloc(G729A_Encoder_Get_Size());
    dec = malloc(G729A_Decoder_Get_Size());
    if ( !enc || !enc2 || !dec )
    {
        printf("FAIL: out of memory\n");
        return 1;
    }

    CHECK(G729A_Encoder_Init(enc) == G729A_NO_ERROR, "encoder init");
    CHECK(G729A_Decoder_Init(dec) == G729A_NO_ERROR, "decoder init");

    /*--- 1. VAD off (default): every frame is a 10-byte voice frame ---*/
    memset(pcm, 0, sizeof(pcm));
    for ( f = 0; f < 20; f++ )
    {
        CHECK(G729A_Encoder_Process(enc, pcm, bits, &len) == G729A_NO_ERROR,
              "encode (VAD off)");
        CHECK(len == G729A_FRAME_LEN_VOICE, "VAD off must produce voice frames");
    }

    /*--- 2. VAD on + silence: SID appears, then NO_DATA frames -------*/
    CHECK(G729A_Encoder_Set_Vad(enc, 1) == G729A_NO_ERROR, "Set_Vad on");
    voice = 0; sid = 0; nodata = 0;
    for ( f = 0; f < N_SILENCE; f++ )
    {
        CHECK(G729A_Encoder_Process(enc, pcm, bits, &len) == G729A_NO_ERROR,
              "encode silence (VAD on)");
        if ( len == G729A_FRAME_LEN_VOICE ) voice++;
        else if ( len == G729A_FRAME_LEN_SID ) sid++;
        else if ( len == G729A_FRAME_LEN_NODATA ) nodata++;
        else { CHECK(0, "unexpected frame length"); break; }

        /* every produced frame length must decode, NULL allowed for 0 */
        CHECK(G729A_Decoder_Process(dec, len ? bits : NULL, len, out)
              == G729A_NO_ERROR, "decode silence frame");
    }
    CHECK(sid >= 1, "silence must produce at least one SID frame");
    CHECK(nodata > 0, "silence must produce NO_DATA frames");
    printf("silence: voice=%d sid=%d nodata=%d\n", voice, sid, nodata);

    /*--- 3. VAD on + active signal: voice frames appear ---------------*/
    voice = 0;
    for ( f = 0; f < N_ACTIVE; f++ )
    {
        for ( i = 0; i < 80; i++ )
        {
            /* strong alternating signal, period varies per frame */
            pcm[i] = (G729_Word16)(((i + f) % 8 < 4) ? 12000 : -12000);
        }
        CHECK(G729A_Encoder_Process(enc, pcm, bits, &len) == G729A_NO_ERROR,
              "encode active (VAD on)");
        if ( len == G729A_FRAME_LEN_VOICE ) voice++;
        CHECK(G729A_Decoder_Process(dec, bits, len, out) == G729A_NO_ERROR,
              "decode active frame");
    }
    CHECK(voice > N_ACTIVE / 2, "active signal must produce voice frames");
    printf("active: voice=%d/%d\n", voice, N_ACTIVE);

    /*--- 4. Determinism: two instances in lockstep produce identical
             frame lengths and payload bytes ---------------------------*/
    CHECK(G729A_Encoder_Init(enc) == G729A_NO_ERROR, "encoder re-init");
    CHECK(G729A_Encoder_Set_Vad(enc, 1) == G729A_NO_ERROR, "Set_Vad on");
    CHECK(G729A_Encoder_Init(enc2) == G729A_NO_ERROR, "encoder2 init");
    CHECK(G729A_Encoder_Set_Vad(enc2, 1) == G729A_NO_ERROR, "Set_Vad on (2)");
    memset(pcm, 0, sizeof(pcm));
    for ( f = 0; f < N_SILENCE; f++ )
    {
        G729A_Encoder_Process(enc, pcm, bits_ref, &len_ref);
        G729A_Encoder_Process(enc2, pcm, bits, &len);
        CHECK(len == len_ref, "frame length mismatch between instances");
        if ( len > 0 )
        {
            CHECK(memcmp(bits, bits_ref, len) == 0,
                  "frame payload mismatch between instances");
        }
    }

    /*--- 5. Error paths ------------------------------------------------*/
    CHECK(G729A_Decoder_Process(dec, bits, 3, out) == G729A_ERROR_BAD_LENGTH,
          "inLen=3 must be rejected");
    CHECK(G729A_Decoder_Process(dec, NULL, G729A_FRAME_LEN_VOICE, out)
          == G729A_ERROR_NULL_BUFFER, "NULL data with voice length rejected");
    CHECK(G729A_Encoder_Set_Vad(NULL, 1) == G729A_ERROR_NULL_STATE,
          "Set_Vad(NULL) rejected");
    CHECK(G729A_Encoder_Set_Vad(enc, 0) == G729A_NO_ERROR, "Set_Vad off");

    free(enc);
    free(enc2);
    free(dec);

    if ( failures )
    {
        printf("ROUNDTRIP: FAIL (%d)\n", failures);
        return 1;
    }
    printf("ROUNDTRIP: PASS\n");
    return 0;
}
