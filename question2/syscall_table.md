# System Call Summary

From `traces/trace_success.txt` (A: parent 1533, child 1534) and
`traces/trace_failed.txt` (B, read-only backup directory: parent 1543,
child 1544). Ubuntu 24.04, AArch64, glibc 2.39, strace 6.8.

| System call | Purpose | Relevant arguments | Return value or error | Role in the program |
|-------------|---------|--------------------|-----------------------|---------------------|
| `openat()` | Open or create a file, return a descriptor | `AT_FDCWD`, `"backup_tool.log"`, `O_WRONLY\|O_CREAT\|O_APPEND`, `0644` · `"source.txt"`, `O_RDONLY` · `"backup/source.txt"`, `O_WRONLY\|O_CREAT\|O_TRUNC`, `0644` | A: `3`, `4`, `5` · B: source `4`, destination **`-1 EACCES (Permission denied)`** | `open()` of the log (parent), source and destination (child) |
| `read()` | Read bytes from a descriptor | fd 4 (`source.txt`), buffer, `4096` | A: `4096`, `4096`, `3208`, then `0` (end of file) | Copy loop in the child |
| `write()` | Write bytes to a descriptor | fd 5 (copy) + length · fd 3 (log) + one line · fd 1 (stdout) | A: `4096`, `4096`, `3208` to fd 5; every log line written in full (e.g. `57`) | Writes the copy, the log lines and the buffered screen output |
| `close()` | Release a descriptor | fd 3, 4, 5 | `0` | Child closes source, copy and its log fd; parent closes its log fd. In B the child closes the source after the failed `openat` |
| `mkdirat()` | Create a directory | `AT_FDCWD`, `"backup"`, `0755` | A: `0` · B: `-1 EEXIST (File exists)` | `mkdir()` of the backup directory (parent) |
| `newfstatat()` | Get file information by path | `AT_FDCWD`, path, `struct stat *`, `0` | A: `0`, both `st_size=11400` · B: `0`, `st_mode=S_IFDIR\|0555` | `stat()`: B checks the existing path is a directory; A checks the copy's size |
| `clone()` | Create a new process | `child_stack=NULL`, `flags=CLONE_CHILD_CLEARTID\|CLONE_CHILD_SETTID\|SIGCHLD` | Child PID: A `1534`, B `1544` | `fork()`: starts the child that copies the file |
| `wait4()` | Wait for a child to finish | pid, `&status`, options `0`, rusage `NULL` | A: `1534`, `WEXITSTATUS(s) == 0` · B: `1544`, `WEXITSTATUS(s) == 3` | `waitpid()`: the parent blocks until the copy ends and learns the result |
| `exit_group()` | End the process | exit code | Never returns (`= ?`). Child A `0`, B `3`; parent A `0`, B `1` | `exit()`: reports the result to the parent and the shell |
| `execve()` | Load and run a program | `"./backup_tool"`, argv `["./backup_tool", "source.txt", "backup"]` | `0` | Starts the program (done by strace); the child does not call it |
| `faccessat()` | Check access permission | `"/etc/ld.so.preload"`, `R_OK` | `-1 ENOENT` | Dynamic loader at start-up only, not our code |
| `getpid()` | Get the caller's process ID | none | Parent `1533`, child `1534` | Puts the PID in each log line |
