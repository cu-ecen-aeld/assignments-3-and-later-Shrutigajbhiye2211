#!/bin/bash

DAEMON = "aesdsocket"

case "${1}" in
  start)
          echo "starting %s\n" "${DAEMON}"
          start-stop-daemon -S -n ${DAEMON}  -a /usr/bin/aesdsocket -- -d
          ;;
  stop)
  	     echo "stopping %s\n" "${DAEMON}"
  	     start-stop-daemon -K -n /usr/bin/aesdsocket
  	     ;;
 *)
 	echo "Usage : $0 {start|stop}"
 	exit 1
 	esac
          	
