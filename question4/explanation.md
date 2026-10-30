# Signal-Driven Service Controller: Technical Explanation

**Build:** `gcc -Wall -Wextra -Werror -pedantic -std=gnu89 -o server_controller server_controller.c`
(Ubuntu 24.04, GCC 13.3). **Evidence:** `demo_output.txt`, made by
`./demo.sh`, which compiles the program and sends every signal with
`kill`.

**Registering and handling signals.** `install_handlers()` calls
`sigaction()` for SIGUSR1, SIGUSR2, SIGINT and SIGTERM, all with one
handler, `on_signal()`. `sa_mask` holds all four signals, so one handler
cannot interrupt another. `sa_flags` is 0 (no `SA_RESTART`), so a signal
wakes the loop. The four signals are first **blocked** with
`sigprocmask()`. The main loop waits in `pselect()`, which unblocks them
and sleeps in one atomic step, until a signal arrives or the next 5-second
check is due. Signals therefore arrive only while the loop is idle, and
none can slip in between "check the flags" and "sleep", the race a plain
`sleep(5)` loop has.

**Why flags make it safe.** The handler only does `flag = 1` on a
`volatile sig_atomic_t`. It never calls `printf()`, `malloc()` or `fopen()`
(a signal can arrive inside them, and calling them again can corrupt
their buffers or deadlock), and it never changes the state. After
`pselect()` returns, `handle_requests()` reads and clears the flags in
normal code, with the signals blocked again, and does all the printing
and state changes there.

**Operational state.** A `service_t` struct holds `mode` (`MODE_NORMAL` /
`MODE_MAINTENANCE`), the PID, the start time and counters. Only the main
loop changes it.
- **Every 5 s** (timed on `CLOCK_MONOTONIC`, so signals do not shift it):
  `monitor_tick()` prints *running normally…* or *MAINTENANCE mode:
  monitoring paused…*.
- **SIGUSR2:** `toggle_mode()` flips `mode`.
- **SIGUSR1:** `status_report()` prints the PID, state, uptime and
  counters.

The demo shows the state changing: `NORMAL` → SIGUSR2 → status
`MAINTENANCE` and the 5-second message changes → SIGUSR2 → `NORMAL`
(`Mode changes : 2`). Standard signals do not queue: the same signal sent
twice before the loop runs is delivered once.

**SIGINT (graceful) vs SIGTERM (emergency).**
- **SIGINT** → `graceful_shutdown()`: final summary, remove the PID file,
  write and `fclose()` (flush) the log, then `exit(0)`. The demo shows
  `exit status: 0` and that the PID file is gone.
- **SIGTERM** → `emergency_terminate()`: one emergency line, then
  `_exit(143)` (128 + 15, "ended by SIGTERM"). There is no summary, no
  stdio/`atexit` clean-up and no file removal. The streams are
  line-buffered, so the message is already written. The demo shows
  `exit status: 143` and a **stale PID file**.
- **On start**, `write_pid_file()` checks the PID file with
  `kill(pid, 0)`. A live PID means a second instance is refused (Run 1).
  A dead PID means a stale file, which is replaced (Run 3).

SIGKILL (`kill -9`) cannot be caught and would end the service with no
message. systemd sends SIGTERM first, so a real service would usually
treat SIGTERM as a polite request. Here it is an emergency stop because
the task asks for it.
