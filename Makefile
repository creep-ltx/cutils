# CUtils - Unix-style commands for AmigaOS 3.2, in C. Cross-built with bebbo's
# gcc (~/opt/amiga), -fbbb=- as the whole terminal family
# (Knowledge/amiga/toolchain-and-testing.md). -resident32: pure, so each can
# be made Resident (the p bit is set by make).
# make test: tests/run.sh in the private FS-UAE (CTerm/tests/files/uae.sh).

AMIGA   = $(HOME)/opt/amiga
CC      = $(AMIGA)/bin/m68k-amigaos-gcc
CFLAGS  = -Os -m68020 -noixemul -resident32 -fomit-frame-pointer -fno-common -fbbb=- -Wall -Wno-pointer-sign -Ilib
LDFLAGS = -m68020 -noixemul -resident32 -s
CMDS    = cat head tail wc grep tee cp mv rm mkdir touch ls find du df sort uniq cut tr xargs diff which date basename dirname seq file stat ln chmod man ps kill free uptime tar gzip gunzip bzip2 bunzip2 unxz unzstd
LIB     = build/libcu.a
LIBOBJ  = build/cu.o build/cu_each.o build/cu_in.o build/cu_tree.o build/cu_walk.o build/cu_colours.o build/regex.o build/cu_lines.o build/cu_date.o build/cu_prot.o build/tui.o build/cu_tasks.o build/cu_long.o \
          $(CZOBJ) $(TPOBJ)

# the squeezers (lib/cz*.c, cz_int.h says how a command gets only its own
# kinds) and the libraries under them (third_party/README.md)
CZOBJ   = build/cz.o build/cz_w.o build/cz_crc.o build/cz_gz.o build/cz_gzw.o build/cz_bz.o build/cz_bzw.o \
          build/cz_bzerr.o build/cz_xz.o build/cz_zst.o build/cz_all.o build/cz_allw.o build/czc.o build/czc_pack.o
MZOBJ   = build/mz_miniz_tinfl.o build/mz_miniz_tdef.o
BZOBJ   = build/bz_blocksort.o build/bz_huffman.o build/bz_crctable.o build/bz_randtable.o build/bz_compress.o \
          build/bz_decompress.o build/bz_bzlib_enc.o build/bz_bzlib_dec.o
XZOBJ   = build/xz_xz_dec_stream.o build/xz_xz_dec_lzma2.o build/xz_xz_dec_bcj.o build/xz_xz_crc32.o \
          build/xz_xz_crc64.o build/xz_xz_sha256.o
ZSOBJ   = build/zs_zstd_decompress.o build/zs_zstd_decompress_block.o build/zs_huf_decompress.o build/zs_zstd_ddict.o \
          build/zs_entropy_common.o build/zs_fse_decompress.o build/zs_xxhash.o build/zs_zstd_common.o build/zs_error_private.o
TPOBJ   = $(MZOBJ) $(BZOBJ) $(XZOBJ) $(ZSOBJ)
CZDEFS  = -DBZ_NO_STDIO -DZSTD_DISABLE_ASM -DZSTD_NO_INTRINSICS -DZSTD_LEGACY_SUPPORT=0 -DHUF_FORCE_DECOMPRESS_X1 \
          -DZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT -DZSTD_NO_TRACE -DZSTD_STRIP_ERROR_STRINGS -DNDEBUG
CZINC   = -Ithird_party/miniz -Ithird_party/bzip2 -Ithird_party/xz -Ithird_party/zstd -Ithird_party/zstd/common
# the libraries' own warnings silenced, only for their files
TPFLAGS = $(filter-out -Wall,$(CFLAGS)) -w $(CZDEFS)

all: $(addprefix bin/,$(CMDS))

build/%.o: lib/%.c lib/cu.h lib/cz.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/cz%.o: lib/cz%.c lib/cu.h lib/cz.h lib/cz_int.h lib/czc.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(CZDEFS) $(CZINC) -c $< -o $@

build/mz_%.o: third_party/miniz/%.c
	@mkdir -p build
	$(CC) $(TPFLAGS) -Ithird_party/miniz -c $< -o $@

build/bz_%.o: third_party/bzip2/%.c
	@mkdir -p build
	$(CC) $(TPFLAGS) -Ithird_party/bzip2 -c $< -o $@

build/xz_%.o: third_party/xz/%.c
	@mkdir -p build
	$(CC) $(TPFLAGS) -Ithird_party/xz -c $< -o $@

build/zs_%.o: third_party/zstd/common/%.c
	@mkdir -p build
	$(CC) $(TPFLAGS) -c $< -o $@

build/zs_%.o: third_party/zstd/decompress/%.c
	@mkdir -p build
	$(CC) $(TPFLAGS) -c $< -o $@

build/cmd_%.o: src/%.c lib/cu.h lib/cz.h lib/czc.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

# an archive, so each command links only the parts it uses
# (link with -m68020: without it gcc takes the 68000 libnix, not the a4 one
# -resident32 needs, and the program crashes at start)
$(LIB): $(LIBOBJ)
	rm -f $@
	$(AMIGA)/bin/m68k-amigaos-ar rcs $@ $^

bin/%: build/cmd_%.o $(LIB)
	@mkdir -p bin
	$(CC) $(LDFLAGS) $< $(LIB) -o $@
	chmod +x $@

test: all
	tests/run.sh

clean:
	rm -rf build bin

.PHONY: all test clean

.SECONDARY:
