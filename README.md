# g729a

ITU-T G.729A (Annex A) 8 kbit/s speech codec, fixed point ANSI-C reference
implementation restructured to support multi-instance use in multi-threaded
and RTOS based environments.

- Bit-exact with the ITU-T G.729A test vectors, in both the 10-byte compressed
  payload format and the 164-byte ITU test format.
- All codec state lives in caller-provided state structs; there is no global
  or static mutable state left on the codec paths.
- Portable C99, no dependencies beyond the C standard library.

## Building

Requires CMake 3.16+ and a C99 compiler.

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build

This builds the static library `g729a` and two command line tools, `coder`
(PCM -> bitstream) and `decoder` (bitstream -> PCM):
`coder speech_file bitstream_file`, `decoder bitstream_file synth_file`.

By default the tools use the 10-byte compressed payload. The ITU test-vector
bitstream format is selected with the `G729A_ITU_TEST_FORMAT` option (it
defines `CONTROL_OPT_ITU=1` for the library and the tools):

    cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DG729A_ITU_TEST_FORMAT=ON
    cmake --build build

### Running the test vectors

    bash tests/run_vectors.sh build

The script encodes every `test_vectors/IN/*.IN` speech file and compares the
result with `test_vectors/BIT/*.BIT`, then decodes every bitstream and compares
it with `test_vectors/PST/*.PST`. It must be run against a build configured
with `-DG729A_ITU_TEST_FORMAT=ON`; a clean run ends with `RESULT: PASS`.
See `test_vectors/READMETV.txt` for the origin and coverage of the vectors.

## API

The public API is `src/interface/g729a_interface.h`. Both directions follow the
same three steps: query the state size, initialize a caller-allocated buffer of
that size, then process frames. Any number of instances can coexist.

    #include "g729a_interface.h"

    /* Encoder */
    G729A_Enc_state enc = malloc(G729A_Encoder_Get_Size());
    G729A_Encoder_Init(enc);                    /* reset / start a stream */

    G729_Word16 pcmIn[80];                      /* 10 ms of 16-bit PCM */
    G729_UWord8 bits[10];                       /* one encoded frame   */

    G729A_Encoder_Process(enc, pcmIn, bits);

    /* Decoder */
    G729A_Dec_state dec = malloc(G729A_Decoder_Get_Size());
    G729A_Decoder_Init(dec);

    G729_Word16 pcmOut[80];

    G729A_Decoder_Process(dec, bits, pcmOut);

Frame format:

- Speech is 16-bit signed linear PCM (`G729_Word16`), 8 kHz sampling rate,
  processed in frames of 80 samples (10 ms).
- One encoded frame is 10 bytes (80 bits). `G729A_Encoder_Process()` writes
  10 bytes, `G729A_Decoder_Process()` consumes 10 bytes.
- `G729A_Encoder_Process_Testing()` and `G729A_Decoder_Process_Testing()` use
  the ITU bitstream layout instead: 82 16-bit words per frame, namely a 2-byte
  synchronization word `0x6b21`, a 2-byte size word `80`, followed by the 80
  speech bits stored one bit per word as `BIT_0`/`BIT_1`. The decoder treats a
  frame whose speech bits are all zero as an erased frame (`bfi`). This is the
  format of the files under `test_vectors/`.

Return values and error codes (`src/interface/g729a_errors.h`): every API
function returns `G729A_NO_ERROR` (0) on success and a negative code on
failure.

- `G729A_ERROR_NULL_STATE` (-1): a state pointer argument is `NULL`.
- `G729A_ERROR_NULL_BUFFER` (-2): an input or output buffer pointer is `NULL`.
- `G729A_ERROR_NOT_INITIALIZED` (-3): the state was never initialized with
  `G729A_Encoder_Init()` / `G729A_Decoder_Init()`, detected through a magic
  value stored in the state.

`G729A_Encoder_Get_Error()` / `G729A_Decoder_Get_Error()` return the last error
code stored in a state. The error checks only reject misuse; they do not change
the processing path itself.

## Multi-instance and reentrancy notes

- Every piece of codec state (filter memories, LSP predictors, encoder ACELP
  scratch, error code) is held in the state structs. Two instances are fully
  independent; a single instance must not be used from two threads at the same
  time.
- `G729A_Decoder_Init()` resets `random_seed` to its initial value. The ITU
  reference code kept this seed in a process-wide static, so re-initializing
  one decoder used to reset the concealment noise of every other decoder; with
  per-state seeds the reset only affects the instance being initialized.
- The ITU fallback in `Levinson()` that restores the previous filter when
  |reflection coefficient| > 32750 is intentionally not implemented. G.729A
  does not use the reflection coefficients, and the threshold was verified to
  be unreachable over the full test-vector suite; see the note in `src/lpc.c`.
- The old `USE_GLOBAL_OVERFLOW_FLAG` switch has been removed. Defining it now
  fails the build with a `#error`; overflow is always tracked through local
  variables.

## License

Released under the [BSD 2-Clause License](LICENSE).

The FFmpeg-derived LGPL bitstream helpers used by earlier revisions have been
replaced by a self-contained MSB-first bit reader/writer (`src/bitstream.h`).

## Notice

**Most of source codes are under the following ITU notice:**

> ITU-T G.729 Software Package Release 2 (November 2006)
> 
> ITU-T G.729A Speech Coder    ANSI-C Source Code
> Version 1.1    Last modified: September 1996
> 
> Copyright (c) 1996,
> AT&T, France Telecom, NTT, Universite de Sherbrooke
> All rights reserved.
