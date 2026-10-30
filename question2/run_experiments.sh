#!/bin/sh
# Builds backup_tool and runs the two experiments under strace:
#
#   traces/trace_success.txt  A: valid source, backup directory not there yet
#   traces/trace_failed.txt   B: backup directory exists but is read-only
#
# Must run as a normal (non-root) user: root ignores file permissions, so
# experiment B would not fail.
set -e

if [ "$(id -u)" -eq 0 ]
then
	echo "Run this as a normal user, not root." >&2
	exit 1
fi

gcc -Wall -Wextra -Werror -pedantic -std=gnu89 -g src/*.c -o backup_tool

here=$(pwd)
lab=/tmp/backup_lab
mkdir -p traces

# run <trace file>: runs backup_tool under strace in the current directory
run()
{
	strace -f -tt -y -s 100 -o "$1" \
		./backup_tool source.txt backup || true
}

# A: success
chmod -R u+w "$lab" 2>/dev/null || true
rm -rf "$lab"
mkdir -p "$lab/A_success"
cd "$lab/A_success"
cp "$here/backup_tool" .
seq -f "line %05g of the important data file" 1 300 > source.txt
echo "--- Experiment A: success ---"
run "$here/traces/trace_success.txt"
cd "$here"

# B: read-only backup directory
mkdir -p "$lab/B_permission_denied"
cd "$lab/B_permission_denied"
cp "$here/backup_tool" .
seq -f "line %05g of the important data file" 1 300 > source.txt
mkdir backup
chmod 555 backup
echo "--- Experiment B: read-only backup directory ---"
run "$here/traces/trace_failed.txt"
cd "$here"

chmod -R u+w "$lab"
rm -rf "$lab"
echo "Traces saved in traces/"
