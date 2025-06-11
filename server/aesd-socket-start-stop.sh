#!/bin/sh

case "$1" in
    start)
        echo "Starting aesdsocket daemon"
        # Starts a process called aesdsocket by executing /usr/bin/aesdsocket
        # with the d flag, -- -d indicates that -d is a arg for aesdsocket, no start-stop
        # daemon
        start-stop-daemon -S -n aesdsocket --exec /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket daemon"
        # Sends SIGTERM instead of default SIGKILL, --oknodo suppresses errors
        start-stop-daemon -K -n aesdsocket --signal TERM --oknodo
        ;;
    *)
    echo "Usage $0 (start_stop)"
    exit 1
esac

exit 0