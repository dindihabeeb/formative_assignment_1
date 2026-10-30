#!/bin/bash
# Terminal demonstration: starts server_controller in the background and
# sends it every signal with kill. The service and this script write to
# the same terminal, so the output below is in real time order.
#
#   ./demo.sh | tee demo_output.txt
set -u
cd "$(dirname "$0")"
rm -f server_controller.log server_controller.pid

# step <command...>: print the command with the time, then run it
step()
{
	echo
	echo "$(date +%T) \$ $*"
	"$@"
}

start()
{
	echo
	echo "$(date +%T) \$ ./server_controller &"
	./server_controller &
	pid=$!
	sleep 0.5
}

finish()
{
	local status
	wait "$pid"
	status=$?
	echo "$(date +%T) [shell] server_controller exit status: $status"
}

echo "===== Build ====="
step gcc -Wall -Wextra -Werror -pedantic -std=gnu89 -o server_controller server_controller.c
echo
echo "===== Run 1: SIGUSR1 (status), SIGUSR2 (toggle), SIGINT (graceful) ====="
start
step cat server_controller.pid
step ps -o pid,stat,cmd -p "$pid"
step ./server_controller
echo "$(date +%T) [shell] second instance refused, as expected"
sleep 5
step kill -SIGUSR1 "$pid"
sleep 1
step kill -SIGUSR2 "$pid"
sleep 1
step kill -SIGUSR1 "$pid"
sleep 4
step kill -SIGUSR2 "$pid"
sleep 1
step kill -SIGUSR1 "$pid"
sleep 2
step kill -SIGINT "$pid"
finish
step ls -l server_controller.pid
step ps -o pid,stat,cmd -p "$pid"

echo
echo "===== Run 2: SIGTERM (emergency termination) ====="
start
sleep 6
step kill -SIGTERM "$pid"
finish
step cat server_controller.pid

echo
echo "===== Run 3: restart after SIGTERM finds the stale PID file ====="
start
sleep 1
step kill -SIGINT "$pid"
finish
rm -f server_controller.log
