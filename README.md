# g729a

[![CI](https://github.com/wendal/g729a/actions/workflows/ci.yml/badge.svg)](https://github.com/wendal/g729a/actions/workflows/ci.yml)

ITU-T G.729A (Annex A) 8 kbit/s speech codec with Annex B silence
compression (VAD/DTX/CNG), fixed point ANSI-C reference implementation
restructured to support multi-instance use in multi-threaded and RTOS
based environments.

- Bit-exact with the ITU-T G.729A test vectors in the 164-byte ITU test
  format and with the G.729A+AnnexB vectors of the ITU Release 2 package
  (`bash tests/run_vectors.sh build`: 17/17 Annex A + 10/10 Annex B),
  verified by CI on Ubuntu (gcc), Windows (MSVC) and macOS (clang).
- The compressed octet format has no ITU reference files: there the codec
  is verified by round-trip, encoding the ITU speech files and decoding
  the result reproduces the reference synthesis byte for byte.
- All codec state lives in caller-provided state structs; there is no global
  or static mutable state left on the codec paths.
- Portable C99, no dependencies beyond the C standard library. The library
  itself performs no heap allocation and no stdio calls; all lookup tables
  are `const` and land in read-only memory.

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
    G729A_Encoder_Set_Vad(enc, 1);              /* optional: enable Annex B
                                                   VAD/DTX (default off) */

    G729_Word16 pcmIn[80];                      /* 10 ms of 16-bit PCM */
    G729_UWord8 bits[10];                       /* one encoded frame   */
    G729_UWord32 bitsLen;                       /* 10, 2 or 0 bytes    */

    G729A_Encoder_Process(enc, pcmIn, bits, &bitsLen);

    /* Decoder */
    G729A_Dec_state dec = malloc(G729A_Decoder_Get_Size());
    G729A_Decoder_Init(dec);

    G729_Word16 pcmOut[80];

    G729A_Decoder_Process(dec, bits, bitsLen, pcmOut);

Frame format:

- Speech is 16-bit signed linear PCM (`G729_Word16`), 8 kHz sampling rate,
  processed in frames of 80 samples (10 ms).
- Encoded frames are variable length octet strings; the length is the frame
  type (`G729A_FRAME_LEN_*`):
  - 10 bytes (`G729A_FRAME_LEN_VOICE`): an 80-bit speech frame.
  - 2 bytes (`G729A_FRAME_LEN_SID`): a 16-bit SID (silence insertion
    descriptor) frame, produced only when Annex B VAD/DTX is enabled.
  - 0 bytes (`G729A_FRAME_LEN_NODATA`): nothing transmitted; the decoder
    keeps generating comfort noise from the last SID. `inData` may be
    `NULL` in this case.
- With VAD off (the default), the encoder always produces 10-byte frames
  and the bitstream is identical to earlier revisions. The decoder accepts
  all three lengths regardless of any setting.
- This format cannot signal an erased frame: the frame erasure flag is
  always 0, so packet loss concealment requires the ITU format below.
- `G729A_Encoder_Process_Testing()` and `G729A_Decoder_Process_Testing()` use
  the ITU bitstream layout instead: a 2-byte synchronization word `0x6b21`,
  a 2-byte size word (80 for speech, 16 for a SID frame in octet mode, 0 for
  an untransmitted frame), followed by the frame bits stored one bit per
  16-bit word as `BIT_0`/`BIT_1`; the length is passed through the
  `outLen`/`inLen` argument. A frame is treated as erased (`bfi`, i.e.
  nothing received) if any of the bit words is zero; that is how a lost
  frame is signalled. This is the format of the files under
  `test_vectors/` and `test_vectors/annexb/`.

Return values and error codes (`src/interface/g729a_errors.h`): the Init,
Process and Process_Testing entry points return `G729A_NO_ERROR` (0) on success
and a negative code on failure; `G729A_*_Get_Size()` returns the state size and
`G729A_Get_Version()` a version string.

- `G729A_ERROR_NULL_STATE` (-1): a state pointer argument is `NULL`.
- `G729A_ERROR_NULL_BUFFER` (-2): an input or output buffer pointer is `NULL`.
- `G729A_ERROR_NOT_INITIALIZED` (-3): the state was never initialized with
  `G729A_Encoder_Init()` / `G729A_Decoder_Init()`, detected through a magic
  value stored in the state.
- `G729A_ERROR_BAD_LENGTH` (-4): a frame length is not 10/2/0, or the ITU
  size word is inconsistent with the passed frame length.

`G729A_Encoder_Get_Error()` / `G729A_Decoder_Get_Error()` return the last error
code stored in a state. The error checks only reject misuse; they do not change
the processing path itself.

## Embedding the library

For integration into another build system (RTOS SDK, LuatOS component, ...),
no CMake is required:

- Compile every `src/*.c` **except** `coder.c` and `decoder.c` (those carry
  the command line `main()` functions).
- Add `src/interface/` to the public include path; the sources additionally
  expect `src/` itself on the include path.
- No preprocessor configuration is needed for normal operation.

Resource footprint (32-bit host, approximate):

| Item | Encoder | Decoder |
|---|---|---|
| State struct (`G729A_*_Get_Size()`) | ~3.3 KB | ~1.8 KB |
| Peak stack usage | ~1 KB | ~0.5 KB |

About 7 KB of lookup tables are `const` and stay in ROM/flash. With
`-ffunction-sections` and `--gc-sections`, linking only the decoder API
discards the encoder code paths and vice versa.

## Migrating from earlier revisions

From the 1.x (Annex A only) API to 2.0:

- `G729A_Encoder_Process()` gained an `outLen` argument and
  `G729A_Decoder_Process()` an `inLen` argument (frame length 10/2/0, see
  the frame format above); the `_Testing` variants changed the same way.
- Annex B VAD/DTX is off by default; enable it with
  `G729A_Encoder_Set_Vad(enc, 1)`. With VAD off, output is bit-identical
  to 1.x.
- State sizes grew (VAD/DTX/CNG sub-states); as always, allocate with
  `G729A_*_Get_Size()`.

Compared with the pre-modernization code (and the original ITU release):

- State sizes changed (see the table above): always allocate with
  `G729A_*_Get_Size()`, never hardcode a size, and always call the matching
  `Init` before `Process` — uninitialized states are now rejected with
  `G729A_ERROR_NOT_INITIALIZED` instead of being silently tolerated.
- `Process`/`Process_Testing` can return `-1`/`-2`/`-3` (see the error code
  list above); callers that only checked `== -1` need to handle the new codes.
- The `speechIn`/`inData` arguments of the `Process` functions are now
  `const`, and the `Get_Size`/`Get_Version` prototypes use `(void)`.
- `G729A_Decoder_Init()` / `G729A_Decoder_Get_Error()` no longer take an
  encoder state by mistake (the parameter type was wrong in both directions).
- The `USE_GLOBAL_OVERFLOW_FLAG` build switch is gone; defining it fails the
  build with `#error`.
- The legacy `src/makefile` is superseded by CMake and kept only for
  reference.

## Multi-instance and reentrancy notes

- Every piece of codec state (filter memories, LSP predictors, encoder ACELP
  scratch, error code) is held in the state structs. Two instances are fully
  independent; a single instance must not be used from two threads at the same
  time.
- `G729A_Encoder_Init()` / `G729A_Decoder_Init()` mark a state as initialized
  with a magic value that differs per direction, so handing an encoder state to
  the decoder API (or vice versa) is rejected with
  `G729A_ERROR_NOT_INITIALIZED` instead of touching a buffer with a different
  layout. That check is a heuristic, not a type system: do not reuse memory
  that held an initialized state for anything else without calling the matching
  Init function again.
- `G729A_Decoder_Init()` resets `random_seed` to its initial value. The ITU
  reference code kept this seed in a process-wide static, so re-initializing
  one decoder used to reset the concealment noise of every other decoder; with
  per-state seeds the reset only affects the instance being initialized.
- The ITU fallback in `Levinson()` that restores the previous filter when
  |reflection coefficient| > 32750 is implemented (Annex B needs it); the
  fallback memory lives in the encoder state instead of the ITU version's
  global. Over the full test-vector suite the fallback was verified to be
  unreachable, so its presence does not change any reference output.
- The fixed-point code relies on two's-complement wrap-around semantics for
  signed overflow (the same assumption the ITU reference makes, equivalent
  to `-fwrapv`). This is the behavior of every supported toolchain
  (gcc/clang/MSVC/arm-none-eabi-gcc); do not build with
  `-fstrict-overflow` / `-fno-wrapv`.
- The old `USE_GLOBAL_OVERFLOW_FLAG` switch has been removed. Defining it now
  fails the build with a `#error`; overflow is always tracked through local
  variables.

## License

Released under the [BSD 2-Clause License](LICENSE).

The FFmpeg-derived LGPL bitstream helpers used by earlier revisions have been
replaced by a self-contained MSB-first bit reader/writer (`src/bitstream.h`).

## Notice

**Most of source codes are under the following ITU notices:**

> ITU-T G.729 Software Package Release 2 (November 2006)
> 
> ITU-T G.729A Speech Coder    ANSI-C Source Code
> Version 1.1    Last modified: September 1996
> 
> Copyright (c) 1996,
> AT&T, France Telecom, NTT, Universite de Sherbrooke
> All rights reserved.

> ITU-T G.729 Software Package Release 2 (November 2006)
> 
> ITU-T G.729A Speech Coder with Annex B    ANSI-C Source Code
> Version 1.5    Last modified: October 2006
> 
> Copyright (c) 1996,
> AT&T, France Telecom, NTT, Universite de Sherbrooke, Lucent Technologies,
> Rockwell International
> All rights reserved.
