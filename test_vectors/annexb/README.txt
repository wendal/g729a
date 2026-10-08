ITU-T G.729 Annex B test vectors (G.729A with Annex B)
=======================================================

Source: ITU-T G.729 Software Package Release 2 (November 2006),
"ITU-T G.729A Speech Coder with Annex B  ANSI-C Source Code, Version 1.5".

Files tstseq1-6 with suffix "a" (tstseqNa.bit / tstseqNa.out) are the
reference outputs for the G.729A+AnnexB implementation (see readmeabTV.txt):

  tstseq1.bin .. tstseq4.bin   encoder input PCM (16-bit, 8 kHz)
  tstseq1a.bit .. tstseq4a.bit encoder reference bitstream (ITU serial word
                               format: SYNC word, SIZE word, then SIZE 16-bit
                               words of one bit each; SIZE is 80 for a speech
                               frame and 15 for a SID frame)
  tstseq1a.out .. tstseq4a.out decoder reference output for the "a" bitstreams
  tstseq5.bit, tstseq6.bit     decoder-only bitstreams (frame erasure / SID
                               sequences)
  tstseq5a.out, tstseq6a.out   decoder reference outputs for tstseq5/6.bit

Files without the "a" suffix (tstseqN.bit / tstseqN.out) are the references
for the full-rate G.729 + Annex B implementation and are kept for
completeness only.
