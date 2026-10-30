# Comparison: data_sync vs data_sync_v2

Evidence for each row is in `outputs/`.

| Property | data_sync | data_sync_v2 |
|----------|-----------|--------------|
| ELF class and architecture | ELF64, little endian, AArch64 | ELF64, little endian, AArch64 |
| ELF file type | DYN — PIE | DYN — PIE |
| Entry-point address | `0x880` (`_start`, start of `.text`) | `0xf00` (`_start`; `main` is placed before it at `0xc80`) |
| Presence of `main()` symbol | Yes: `T main` at `0x998` (400 B) | No (stripped). Found at `0xc80` through `_start` → GOT `0x1fff0` → `R_AARCH64_RELATIVE 0xc80` |
| Important sections | 34 sections: `.text` 1,124 B, `.rodata` 243 B, `.data` 16 B, `.bss` 128 KB, `.dynsym`, `.dynamic`, `.symtab`, `.strtab`, 6× `.debug_*` | 26 sections: `.text` 2,820 B, `.rodata` 523 B, `.data` 16 B, `.bss` 128 KB, `.dynsym`, `.dynamic`. **No** `.symtab`, `.strtab` or `.debug_*` |
| Dynamic dependencies | `libc.so.6` | `libc.so.6`, `ld-linux-aarch64.so.1` |
| Relevant imported functions | `opendir`, `readdir`, `closedir`, `strcmp`, `printf`, `fprintf`, `snprintf`, `perror` (10 functions + `stderr`) | Same directory functions and `strcmp`, **plus** `stat`, `fopen`, `fread`, `ferror`, `fclose`, `time`, `localtime`, `strftime`, `__errno_location`, `fwrite`, the `_chk` print functions and `__stack_chk_*` (21 functions + 2 objects) |
| Debugging symbols | Yes: DWARF info, `file` says `with debug_info, not stripped` | No: `file` says `stripped` |
| Significant differences | Names-only comparison. No file I/O, no metadata, no time functions | Adds metadata checks, file-content reading, time-stamped writes. `.text` 2.5× bigger, but the file is smaller (67,496 vs 76,272 B) because symbols and debug info were removed |
