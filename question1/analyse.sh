#!/bin/sh
# Runs the static analysis commands on both binaries and saves each output
# to outputs/<binary>/. Only inspection tools are used: the target binaries
# are never executed, and ldd is not used.
set -e

cd bin

run()
{
	out="$1"
	shift
	{
		echo "\$ $*"
		echo
		"$@" 2>&1 || true
	} > "$out"
}

for bin in data_sync data_sync_v2
do
	o="../outputs/$bin"
	rm -rf "$o"
	mkdir -p "$o"

	run "$o/01_file.txt"           file "$bin"
	run "$o/02_readelf_h.txt"      readelf -h "$bin"
	run "$o/03_readelf_S.txt"      readelf -S -W "$bin"
	run "$o/04_readelf_l.txt"      readelf -l -W "$bin"
	run "$o/05_readelf_s.txt"      readelf -s -W "$bin"
	run "$o/06_readelf_d.txt"      readelf -d "$bin"
	run "$o/07_nm_n.txt"           nm -n "$bin"
	run "$o/08_nm_D.txt"           nm -D "$bin"
	run "$o/09_objdump_h.txt"      objdump -h "$bin"
	run "$o/10_objdump_t.txt"      objdump -t "$bin"
	run "$o/11_objdump_T.txt"      objdump -T "$bin"
	run "$o/12_objdump_d.txt"      objdump -d "$bin"
	run "$o/13_objdump_p.txt"      objdump -p "$bin"
	run "$o/14_readelf_rodata.txt" readelf -p .rodata "$bin"
	run "$o/15_readelf_r.txt"      readelf -r -W "$bin"
	run "$o/16_readelf_n.txt"      readelf -n "$bin"
	run "$o/17_readelf_V.txt"      readelf -V "$bin"
	run "$o/18_readelf_comment.txt" readelf -p .comment "$bin"
done

# Only data_sync has DWARF debug info; its first entry records the
# compiler command-line options
{
	echo "\$ readelf --debug-dump=info data_sync | grep -m 3 -A 1 DW_AT_producer"
	echo
	readelf --debug-dump=info data_sync | grep -m 3 -A 1 DW_AT_producer
} > ../outputs/data_sync/19_debug_producer.txt

# Side by side list of imported (undefined dynamic) function names
o="../outputs/compare"
rm -rf "$o"
mkdir -p "$o"
nm -D --undefined-only data_sync | awk '{print $NF}' | sort > "$o/imports_data_sync.txt"
nm -D --undefined-only data_sync_v2 | awk '{print $NF}' | sort > "$o/imports_data_sync_v2.txt"
{
	echo "\$ comm -3 imports_data_sync.txt imports_data_sync_v2.txt"
	echo "(left column: only in data_sync, right column: only in data_sync_v2)"
	echo
	comm -3 "$o/imports_data_sync.txt" "$o/imports_data_sync_v2.txt"
	echo
	echo "\$ comm -12 imports_data_sync.txt imports_data_sync_v2.txt"
	echo "(imported by both)"
	echo
	comm -12 "$o/imports_data_sync.txt" "$o/imports_data_sync_v2.txt"
} > "$o/imports_diff.txt"
{
	echo "\$ ls -l data_sync data_sync_v2"
	echo
	ls -l data_sync data_sync_v2
	echo
	echo "\$ sha256sum data_sync data_sync_v2"
	echo
	sha256sum data_sync data_sync_v2
} > "$o/sizes_and_hashes.txt"

# Validation, only possible because we own the source: rebuild v2 without
# strip, check that stripping it gives the same file, and read the
# function addresses that strip removed. The copy is built and read, never run.
tmp=$(mktemp -d)
gcc -Wall -Wextra -Werror -pedantic -std=gnu89 -O2 ../src/data_sync_v2/*.c \
	-o "$tmp/v2_unstripped"
strip --strip-all -o "$tmp/v2_restripped" "$tmp/v2_unstripped"
{
	echo "\$ cmp v2_restripped data_sync_v2"
	echo
	if cmp "$tmp/v2_restripped" data_sync_v2
	then
		echo "identical: data_sync_v2 is exactly the unstripped build after strip"
	fi
	echo
	echo "\$ nm -n v2_unstripped | grep -i ' t '"
	echo
	nm -n "$tmp/v2_unstripped" | grep -i ' t '
} > "$o/v2_unstripped_check.txt"
rm -rf "$tmp"

echo "Outputs saved in outputs/"
