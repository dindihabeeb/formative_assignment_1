# Technical Analysis: System Calls of `backup_tool`

**Setup.** `run_experiments.sh` compiles the program
(`gcc -Wall -Wextra -Werror -pedantic -std=gnu89 -g src/*.c -o backup_tool`)
and, as the normal user `ubuntu` (uid 1000), runs it twice under
`strace -f -tt -y -s 100 -o <file> ./backup_tool source.txt backup`.
`-f` follows the child, `-tt` adds timestamps, `-y` shows the file behind
every descriptor and `-s 100` shows the data written.

- **A** (`traces/trace_success.txt`): an 11,400-byte `source.txt`, and no
  `backup/` directory yet.
- **B** (`traces/trace_failed.txt`): the same source, but `backup/` already
  exists with mode `555` (read-only).

Lines 1–27 of each trace come before `main()`: the loader opening
`ld.so.cache` and `libc.so.6`, `faccessat("/etc/ld.so.preload")`, `mmap`,
`mprotect`, and so on. The program's own work starts at line 28. On this
AArch64 system our calls appear under their kernel names: `open` →
`openat`, `mkdir` → `mkdirat`, `stat` → `newfstatat`, `fork` → `clone`,
`waitpid` → `wait4`, `exit` → `exit_group`.

## 1. Creating, reading, writing and closing files

The program only gets a number (file descriptor) from the kernel, and
uses it for every later call:

```
1533  22:00:38.987973 openat(AT_FDCWD</tmp/backup_lab/A_success>, "backup_tool.log", O_WRONLY|O_CREAT|O_APPEND, 0644) = 3</tmp/backup_lab/A_success/backup_tool.log>
1533  22:00:38.989318 mkdirat(AT_FDCWD</tmp/backup_lab/A_success>, "backup", 0755) = 0
1534  22:00:38.990339 <... openat resumed>) = 4</tmp/backup_lab/A_success/source.txt>
1534  22:00:38.990643 openat(AT_FDCWD</tmp/backup_lab/A_success>, "backup/source.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 5</tmp/backup_lab/A_success/backup/source.txt>
1534  22:00:38.990971 read(4</tmp/backup_lab/A_success/source.txt>, "line 00001 of the important data file\n…"..., 4096) = 4096
1534  22:00:38.991071 write(5</tmp/backup_lab/A_success/backup/source.txt>, "line 00001 of the important data file\n…"..., 4096) = 4096
1534  22:00:38.991535 read(4</tmp/backup_lab/A_success/source.txt>, "", 4096) = 0
1534  22:00:38.991807 close(4</tmp/backup_lab/A_success/source.txt>) = 0
1534  22:00:38.991896 close(5</tmp/backup_lab/A_success/backup/source.txt>) = 0
```

- **Opening.** The kernel returns the lowest free descriptor (3, 4, 5).
  `O_CREAT|O_TRUNC` creates the copy or empties it. `O_APPEND` makes every
  log write go to the end of the file.
- **Copying.** The reads return 4096, 4096, 3208, and then `0`, which
  means end of file and stops the loop. Each `write(5, …)` returns the same
  count, so nothing was lost: 11,400 bytes in total. The parent's
  `newfstatat` calls later confirm `st_size=11400` for both files.
- **Closing.** Every descriptor the program opened is closed with return
  `0`.
- **Library calls.** Some calls come from library code, not from our own
  calls:
  - `fstat(1)`, `getrandom` and `brk` come from stdio and the first
    `malloc`.
  - `openat("/etc/localtime")` with its `read`/`lseek`/`close` comes from
    `localtime_r()`.

## 2. Process creation and synchronisation

```
1533  22:00:38.989750 clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0xee8004ce1fd0) = 1534
1534  22:00:38.990031 set_robust_list(0xee8004ce1fe0, 24 <unfinished ...>
1533  22:00:38.990386 wait4(1534,  <unfinished ...>
1534  22:00:38.992226 exit_group(0)     = ?
1534  22:00:38.992430 +++ exited with 0 +++
1533  22:00:38.992481 <... wait4 resumed>[{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 1534
1533  22:00:38.992546 --- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=1534, si_uid=1000, si_status=0, si_utime=0, si_stime=0} ---
```

- **`clone` is `fork()`.** There is no `fork` system call on AArch64, so
  glibc calls `clone`. Without `CLONE_VM` or `CLONE_FILES`, the child gets
  its own copy of memory and of the descriptor table. The parent gets the
  child's PID, 1534.
- **The child inherits the log.** The child writes the log through the
  inherited fd 3 (`1534 write(3</…/backup_tool.log>`). Each process later
  calls `close(3)` on its own copy.
- **Both run at once, briefly.** The `<unfinished ...>` /
  `<... resumed>` lines show the parent's log write and the child's
  `openat` overlapping in time.
- **The parent waits.** It then blocks in `wait4(1534, …)` for 2.1 ms,
  while the child does all of the copying.
- **The result comes back.** `wait4` returns only after the child's
  `exit_group(0)`, with the decoded status `WEXITSTATUS(s) == 0`, and the
  kernel then sends `SIGCHLD`. Only then does the parent check the copy.
  The child never calls `execve`: it keeps running the same program.

## 3. Successful vs failed execution

```
1543  22:00:39.000778 mkdirat(AT_FDCWD</tmp/backup_lab/B_permission_denied>, "backup", 0755) = -1 EEXIST (File exists)
1543  22:00:39.000871 newfstatat(AT_FDCWD</tmp/backup_lab/B_permission_denied>, "backup", {st_mode=S_IFDIR|0555, st_size=4096, ...}, 0) = 0
1544  22:00:39.002251 <... openat resumed>) = 4</tmp/backup_lab/B_permission_denied/source.txt>
1544  22:00:39.002472 openat(AT_FDCWD</tmp/backup_lab/B_permission_denied>, "backup/source.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = -1 EACCES (Permission denied)
1544  22:00:39.002894 close(4</tmp/backup_lab/B_permission_denied/source.txt>) = 0
1544  22:00:39.003176 exit_group(3)     = ?
1543  22:00:39.003356 <... wait4 resumed>[{WIFEXITED(s) && WEXITSTATUS(s) == 3}], 0, NULL) = 1544
1543  22:00:39.004295 exit_group(1)     = ?
```

| | A: success | B: read-only `backup/` |
|-|-----------|------------------------|
| `mkdirat("backup")` | `0` (created) | `-1 EEXIST` |
| `newfstatat("backup")` | not called | `S_IFDIR\|0555` |
| child `openat` of the destination | `5` | **`-1 EACCES`** |
| data `read`/`write` | 4 reads, 3 writes | none |
| child `exit_group` | `0` | `3` |
| parent `wait4` status | `0` | `3` |
| parent `exit_group` | `0` | `1` |

- **What the trace shows went wrong.** In B, reading the source works
  (`= 4`), but the kernel refuses to **create a file inside** `backup/`
  (`EACCES`). The reason is already visible two calls earlier:
  `newfstatat` shows the directory is `0555`, so it has no write
  permission, and the process is not root (`si_uid=1000`).
- **Why `mkdirat` gave `EEXIST`, not `EACCES`.** Creating `backup` only
  needs write permission on the *parent* folder, which the user has.
- **Was it handled well?** Yes:
  - `EEXIST` is accepted only after `newfstatat` confirms the path is a
    directory.
  - The `EACCES` is turned into "Permission denied" from `errno`.
  - The already-open source (fd 4) is closed before `exit_group(3)`.
  - The parent reads status 3 from `wait4` and exits with 1.
  - No data was read, and no partial file was created.

## 4. Unexpected or unnecessary system calls

- **`getpid()` before every log line** (9 in A, one per log line). The
  PID only changes at `fork`, so it could be saved once in each process.
- **Time-zone loading** (`openat("/etc/localtime")`, 2 × `fstat`,
  2 × `read`, `lseek`, `close`). It happens once, from the first
  `localtime_r()`. The child inherits the loaded time zone.
- **Batched stdout.** stdout is a pipe here, so stdio buffers it. Several
  messages come out in a single `write(1, …)`, and the parent's
  `INFO child … started` line is only written at its exit. The screen
  output is therefore out of order, but the trace and the log (one
  `write` per line) show the real order.
- **No `fsync`** after writing the copy. The data may still be only in the
  kernel's cache when the program reports success.
- **B forks for nothing.** The parent already knew from `newfstatat` that
  `backup/` was `0555`, but still started a child that could not succeed.

## 5. Improvement: copy to a temporary file, then rename

Right now the child writes straight into `backup/source.txt` with
`O_TRUNC`. If the copy fails half way (disk full, `EIO`, the process is
killed), the previous good backup has already been emptied. A more
reliable version would:

1. `openat("backup/.source.txt.tmp", O_WRONLY|O_CREAT|O_EXCL, 0600)`
2. copy as now
3. `fsync()`, then `close()`, checking both
4. `renameat()` it over `backup/source.txt`, which replaces the file in
   one atomic step
5. `unlinkat()` the temporary file on any error

Checking that the directory is writable (`faccessat(dir, W_OK)`) before
`fork` would also give a clear error early, as in experiment B.