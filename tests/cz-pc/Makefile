# tests/cz-pc - the squeezers (lib/cz*.c + third_party) built two ways from
# the same sources: for Linux (pc, and pc-asan with the sanitizers) and for
# the 68020 (am: czt.c linked with the very objects of build/libcu.a, run
# under vamos). run.sh compares them. Not part of CUtils' commands.
TOP     = ../..
CZSRC   = $(addprefix $(TOP)/lib/,cz.c cz_w.c cz_crc.c cz_gz.c cz_gzw.c cz_bz.c cz_bzw.c cz_bzerr.c cz_xz.c cz_zst.c cz_all.c cz_allw.c)
TP      = $(TOP)/third_party
TPSRC   = $(TP)/miniz/miniz_tinfl.c $(TP)/miniz/miniz_tdef.c \
          $(addprefix $(TP)/bzip2/,blocksort.c huffman.c crctable.c randtable.c compress.c decompress.c bzlib_enc.c bzlib_dec.c) \
          $(addprefix $(TP)/xz/,xz_dec_stream.c xz_dec_lzma2.c xz_dec_bcj.c xz_crc32.c xz_crc64.c xz_sha256.c) \
          $(addprefix $(TP)/zstd/decompress/,zstd_decompress.c zstd_decompress_block.c huf_decompress.c zstd_ddict.c) \
          $(addprefix $(TP)/zstd/common/,entropy_common.c fse_decompress.c xxhash.c zstd_common.c error_private.c)
# the same switches as the main Makefile's CZDEFS
DEFS    = -DBZ_NO_STDIO -DZSTD_DISABLE_ASM -DZSTD_NO_INTRINSICS -DZSTD_LEGACY_SUPPORT=0 -DHUF_FORCE_DECOMPRESS_X1 \
          -DZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT -DZSTD_NO_TRACE -DZSTD_STRIP_ERROR_STRINGS -DNDEBUG
INC     = -Ifake -I$(TOP)/lib -I$(TP)/miniz -I$(TP)/bzip2 -I$(TP)/xz -I$(TP)/zstd -I$(TP)/zstd/common
AMIGA   = $(HOME)/opt/amiga

all: pc pc-asan am

pc: czt.c shim.c $(CZSRC) $(TPSRC)
	gcc -O2 -w $(DEFS) $(INC) czt.c shim.c $(CZSRC) $(TPSRC) -o $@

pc-asan: czt.c shim.c $(CZSRC) $(TPSRC)
	gcc -O1 -g -w -fsanitize=address,undefined -fno-sanitize-recover=undefined $(DEFS) $(INC) czt.c shim.c $(CZSRC) $(TPSRC) -o $@

# build/libcu.a first: make in the top drawer
am: czt.c $(TOP)/build/libcu.a
	$(AMIGA)/bin/m68k-amigaos-gcc -Os -m68020 -noixemul -resident32 -fomit-frame-pointer -fno-common -fbbb=- -w -I$(TOP)/lib -c czt.c -o czt-am.o
	$(AMIGA)/bin/m68k-amigaos-gcc -m68020 -noixemul -resident32 czt-am.o $(TOP)/build/libcu.a -o $@

clean:
	rm -f pc pc-asan am czt-am.o

.PHONY: all clean
