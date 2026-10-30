# Forensic Assessment: `data_sync` vs `data_sync_v2`

**Purpose and methodology.** The aim was to find out what two unknown
"file synchronisation" binaries are and how they differ, without running
them. Both were built from my own C source with GCC 13.3 on Ubuntu 24.04
(AArch64). They were then examined only with static tools: `file`,
`readelf` (`-h -S -l -s -d -r -V -p`), `nm` (`-n -D`) and `objdump`
(`-h -t -T -d -p`). Neither binary was executed and `ldd` was not used.
Full outputs are in `outputs/`.

**Key findings.** Both files are 64-bit, little-endian AArch64
position-independent executables (`DYN`). They have the same 9 segments,
are dynamically linked through `/lib/ld-linux-aarch64.so.1`, use
`BIND_NOW` with `GNU_RELRO`, and have a non-executable stack. In both,
the entry point is `_start` (`0x880` in v1, `0xf00` in v2), which passes
`main` to `__libc_start_main`. `data_sync` is **not stripped** and has
DWARF debug info: 34 sections, including `.symtab` and `.debug_*`, and
`main` at `0x998`. `data_sync_v2` is **stripped**: 26 sections, no
`.symtab`, no debug info, and `nm` finds no symbols. Its `main` was still
located at `0xc80` by following `_start` → GOT slot `0x1fff0` → the
`R_AARCH64_RELATIVE` relocation. v1 depends on `libc.so.6` only. v2 also
lists `ld-linux-aarch64.so.1`, which provides `__stack_chk_guard`.

**Comparative assessment.** Both import `opendir`, `readdir`, `closedir`
and `strcmp`, which fits a tool that lists a directory and compares names.
v2 adds `stat` and `__errno_location` (file metadata and missing-file
checks), `fopen`/`fread`/`ferror`/`fclose` (reading file contents), and
`time`/`localtime`/`strftime` with `fprintf` (timestamped output). Its
`.rodata` strings fit this: `data_sync.log`, `"a"`, `"rb"`,
`%Y-%m-%d %H:%M:%S`, `CHANGED (size)` and `CHANGED (checksum)`. Its
`.text` is 2.5 times larger. This suggests v2 is an extended version of v1
that compares sizes, dates and file contents and keeps a log. Some other
differences come from the build, not from the program's behaviour.
`printf` → `__printf_chk` is fortify at `-O2`, `fwrite` is a GCC
rewrite of `fprintf`, the moved entry point is `main` being placed first
by `-O2`, and the missing symbols are from `strip`. Nothing seen suggests
networking, process creation or hidden behaviour: there are no socket,
`exec` or `fork` imports.

**Conclusion and limitations.** *Observed:* the ELF headers, sections,
segments, imports, library dependencies, strings and the stripped state
of v2. *Interpreted:* that v2 checks metadata, computes a checksum and
writes a log. An import only shows that a function *can* be called, not
that it is called, with which arguments, or on which files. A static view
cannot confirm which code paths run, what the log contains, or whether
code is loaded at run time (e.g. by `dlopen`). It also cannot show what
glibc on a target system would actually supply. Because v2 is stripped,
the names of its internal functions cannot be recovered from the file
alone. The structure above was read from call patterns and only confirmed
because the source was available. Confirming run-time behaviour would need
dynamic analysis in an isolated sandbox.
