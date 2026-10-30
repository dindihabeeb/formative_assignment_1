#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#define TAG "[Server Controller]"
#define INTERVAL_S 5
#define PID_FILE "server_controller.pid"
#define LOG_FILE "server_controller.log"

/**
 * enum svc_mode - Operational state of the service
 *
 * @MODE_NORMAL: Monitoring the server
 * @MODE_MAINTENANCE: Monitoring paused for maintenance
 */
typedef enum svc_mode
{
	MODE_NORMAL,
	MODE_MAINTENANCE
} svc_mode_t;

/**
 * struct service_s - Everything the service keeps track of
 *
 * @mode: Current operational state
 * @pid: Process ID of the service
 * @started: Wall-clock start time
 * @checks: Number of monitoring checks done
 * @mode_changes: Number of NORMAL <-> MAINTENANCE switches
 * @log: Log file, line buffered
 * @next_check: CLOCK_MONOTONIC time of the next monitoring check
 */
typedef struct service_s
{
	svc_mode_t	mode;
	pid_t		pid;
	time_t		started;
	unsigned long	checks;
	unsigned long	mode_changes;
	FILE		*log;
	struct timespec	next_check;
} service_t;

/*
 * Set by the signal handler, read and cleared by the main loop. The
 * handler only ever assigns 1: that is the one thing C guarantees is safe
 * to do with a variable from inside a signal handler.
 */
static volatile sig_atomic_t status_requested;
static volatile sig_atomic_t toggle_requested;
static volatile sig_atomic_t shutdown_requested;
static volatile sig_atomic_t terminate_requested;

/**
 * on_signal - the only signal handler: records the request and returns
 * @sig: the signal number
 *
 * Description: no printf(), no malloc(), no state changes here. All real
 * work is done by handle_requests() in the main loop.
 *
 * Return: nothing
 */
static void on_signal(int sig)
{
	if (sig == SIGUSR1)
		status_requested = 1;
	else if (sig == SIGUSR2)
		toggle_requested = 1;
	else if (sig == SIGINT)
		shutdown_requested = 1;
	else if (sig == SIGTERM)
		terminate_requested = 1;
}

/**
 * install_handlers - registers on_signal() for the four signals and
 * blocks them, so they can only arrive while the loop waits in pselect()
 * @handled: filled with the four signals
 * @wait_mask: filled with the mask to use while waiting (they unblocked)
 *
 * Return: 0 on success, -1 on error (errno set)
 */
static int install_handlers(sigset_t *handled, sigset_t *wait_mask)
{
	const int sigs[] = {SIGUSR1, SIGUSR2, SIGINT, SIGTERM};
	struct sigaction sa;
	size_t i;

	sigemptyset(handled);
	for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
		sigaddset(handled, sigs[i]);
	if (sigprocmask(SIG_BLOCK, handled, wait_mask) == -1)
		return (-1);
	for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
		sigdelset(wait_mask, sigs[i]);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_signal;
	sa.sa_mask = *handled;	/* no handler interrupts another */
	sa.sa_flags = 0;	/* no SA_RESTART: pselect() must wake up */
	for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
		if (sigaction(sigs[i], &sa, NULL) == -1)
			return (-1);
	return (0);
}

/**
 * say - prints a tagged line and appends it, timestamped, to the log
 * @svc: the service
 * @fmt: printf-style format
 *
 * Return: nothing
 */
static void say(service_t *svc, const char *fmt, ...)
{
	char msg[256], stamp[16];
	time_t now = time(NULL);
	struct tm tm;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	if (!localtime_r(&now, &tm) ||
	    !strftime(stamp, sizeof(stamp), "%H:%M:%S", &tm))
		strcpy(stamp, "--:--:--");
	printf("%s %s %s\n", stamp, TAG, msg);
	if (svc->log)
		fprintf(svc->log, "%s [pid %ld] %s\n", stamp, (long)svc->pid,
			msg);
}

/**
 * mode_str - name of an operational state
 * @mode: the state
 *
 * Return: "NORMAL" or "MAINTENANCE"
 */
static const char *mode_str(svc_mode_t mode)
{
	return (mode == MODE_NORMAL ? "NORMAL" : "MAINTENANCE");
}

/**
 * write_pid_file - records our PID so administrators can find us,
 * refusing to start if another live instance owns the file
 * @svc: the service
 *
 * Return: 0 on success, -1 on failure
 */
static int write_pid_file(service_t *svc)
{
	FILE *fp = fopen(PID_FILE, "r");
	long old;

	if (fp)
	{
		if (fscanf(fp, "%ld", &old) == 1 && old > 0 &&
		    kill((pid_t)old, 0) == 0)
		{
			fclose(fp);
			fprintf(stderr, "%s already running with PID %ld\n",
				TAG, old);
			return (-1);
		}
		fclose(fp);
		say(svc, "Found stale PID file %s, replacing it", PID_FILE);
	}
	fp = fopen(PID_FILE, "w");
	if (!fp || fprintf(fp, "%ld\n", (long)svc->pid) < 0)
	{
		perror(PID_FILE);
		if (fp)
			fclose(fp);
		return (-1);
	}
	if (fclose(fp) != 0)
	{
		perror(PID_FILE);
		return (-1);
	}
	return (0);
}

/**
 * monitor_tick - the periodic monitoring message
 * @svc: the service
 *
 * Return: nothing
 */
static void monitor_tick(service_t *svc)
{
	svc->checks++;
	if (svc->mode == MODE_NORMAL)
		say(svc, "Service running normally... (check #%lu)",
		    svc->checks);
	else
		say(svc, "MAINTENANCE mode: monitoring paused... (check #%lu)",
		    svc->checks);
}

/**
 * status_report - answer to SIGUSR1
 * @svc: the service
 *
 * Return: nothing
 */
static void status_report(service_t *svc)
{
	say(svc, "===== STATUS REPORT (SIGUSR1) =====");
	say(svc, "  Process ID   : %ld", (long)svc->pid);
	say(svc, "  State        : %s", mode_str(svc->mode));
	say(svc, "  Uptime       : %ld s", (long)(time(NULL) - svc->started));
	say(svc, "  Checks done  : %lu", svc->checks);
	say(svc, "  Mode changes : %lu", svc->mode_changes);
}

/**
 * toggle_mode - answer to SIGUSR2: NORMAL <-> MAINTENANCE
 * @svc: the service
 *
 * Return: nothing
 */
static void toggle_mode(service_t *svc)
{
	svc_mode_t old = svc->mode;

	svc->mode = (old == MODE_NORMAL) ? MODE_MAINTENANCE : MODE_NORMAL;
	svc->mode_changes++;
	say(svc, "SIGUSR2 received: mode changed %s -> %s", mode_str(old),
	    mode_str(svc->mode));
}

/**
 * graceful_shutdown - answer to SIGINT: report, clean up, exit 0
 * @svc: the service
 *
 * Return: does not return
 */
static void graceful_shutdown(service_t *svc)
{
	say(svc, "SIGINT received: starting graceful shutdown...");
	say(svc, "  Final state: %s, checks done: %lu, mode changes: %lu",
	    mode_str(svc->mode), svc->checks, svc->mode_changes);
	if (remove(PID_FILE) == 0)
		say(svc, "  Removed PID file %s", PID_FILE);
	else
		say(svc, "  Could not remove %s: %s", PID_FILE,
		    strerror(errno));
	say(svc, "  Closing log file %s", LOG_FILE);
	say(svc, "Shutdown complete. Goodbye.");
	if (svc->log && fclose(svc->log) != 0)
		perror(LOG_FILE);
	svc->log = NULL;
	fflush(stdout);
	exit(EXIT_SUCCESS);
}

/**
 * emergency_terminate - answer to SIGTERM: stop at once
 * @svc: the service
 *
 * Description: no summary and no cleanup. The PID file is left behind on
 * purpose and is detected as stale on the next start. Both streams are
 * line buffered, so the message is already written when _exit() runs.
 *
 * Return: does not return
 */
static void emergency_terminate(service_t *svc)
{
	say(svc, "SIGTERM received: EMERGENCY TERMINATION, exiting now!");
	_exit(128 + SIGTERM);
}

/**
 * handle_requests - acts on the flags set by the signal handler
 * @svc: the service
 *
 * Description: runs with the four signals blocked, so a flag cannot be
 * set again by a handler while it is being read and cleared here.
 * Termination is checked first so it always wins.
 *
 * Return: nothing (may exit)
 */
static void handle_requests(service_t *svc)
{
	if (terminate_requested)
		emergency_terminate(svc);
	if (shutdown_requested)
		graceful_shutdown(svc);
	if (toggle_requested)
	{
		toggle_requested = 0;
		toggle_mode(svc);
	}
	if (status_requested)
	{
		status_requested = 0;
		status_report(svc);
	}
}

/**
 * time_until - time left from now until a CLOCK_MONOTONIC deadline
 * @deadline: the deadline
 * @left: where to store the time left (zero if already passed)
 *
 * Return: 1 if time is left, 0 if the deadline has passed
 */
static int time_until(const struct timespec *deadline, struct timespec *left)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	left->tv_sec = deadline->tv_sec - now.tv_sec;
	left->tv_nsec = deadline->tv_nsec - now.tv_nsec;
	if (left->tv_nsec < 0)
	{
		left->tv_sec--;
		left->tv_nsec += 1000000000L;
	}
	return (left->tv_sec >= 0 && (left->tv_sec > 0 || left->tv_nsec > 0));
}

/**
 * main - Entry point, runs the signal-driven service controller
 *
 * Return: does not return normally; exits from the shutdown handlers
 */
int main(void)
{
	service_t svc;
	sigset_t handled, wait_mask;
	struct timespec left;

	setvbuf(stdout, NULL, _IOLBF, 0);
	memset(&svc, 0, sizeof(svc));
	svc.mode = MODE_NORMAL;
	svc.pid = getpid();
	svc.started = time(NULL);
	if (install_handlers(&handled, &wait_mask) == -1)
	{
		perror("signal setup");
		return (EXIT_FAILURE);
	}
	svc.log = fopen(LOG_FILE, "a");
	if (!svc.log)
		perror(LOG_FILE);
	else
		setvbuf(svc.log, NULL, _IOLBF, 0);
	if (write_pid_file(&svc) == -1)
		return (EXIT_FAILURE);

	say(&svc, "Started with PID %ld in %s mode (check every %d s)",
	    (long)svc.pid, mode_str(svc.mode), INTERVAL_S);
	say(&svc, "kill -USR1 %ld = status, -USR2 = toggle mode, "
	    "-INT = graceful shutdown, -TERM = emergency stop",
	    (long)svc.pid);
	clock_gettime(CLOCK_MONOTONIC, &svc.next_check);
	for (;;)
	{
		if (!time_until(&svc.next_check, &left))
		{
			monitor_tick(&svc);
			svc.next_check.tv_sec += INTERVAL_S;
			continue;
		}
		/* atomically unblock the signals and wait for one or timeout */
		if (pselect(0, NULL, NULL, NULL, &left, &wait_mask) == -1 &&
		    errno != EINTR)
		{
			perror("pselect");
			return (EXIT_FAILURE);
		}
		handle_requests(&svc);
	}
}
