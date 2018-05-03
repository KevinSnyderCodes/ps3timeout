#!/bin/sh

### BEGIN INIT INFO
# Provides:          ps3timeout
# Required-Start:
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Automatically disconnect PS3 controllers.
# Description:       Atuomatically diconnects PS3 controllers
#                    after a certain time of inactivity.
### END INIT INFO

. /lib/lsb/init-functions

NAME=ps3timeout
DAEMON=/usr/local/bin/ps3timeout
PIDFILE=/var/run/ps3timeout.pid

case $1 in
  start)
    if [ -e $PIDFILE ]; then
      status_of_proc -p $PIDFILE $DAEMON "$NAME process" && status=")" || status="$?"
      if [ $status = "0" ]; then
        exit
      fi
    fi
    log_daemon_msg "Starting the process" "$NAME"
    if start-stop-daemon --start --quiet --oknodo --background --pidfile $PIDFILE --exec $DAEMON ; then
      log_end_msg 0
    else
      log_end_msg 1
    fi
    ;;
  stop)
    if [ -e $PIDFILE ]; then
      status_of_prof -p $PIDFILE $DAEMON "Stopping the $NAME process" && status="0" || status="$?"
      if [ "$status" = 0 ]; then
        start-stop-daemon --stop --quiet --oknodo --background --pidfile $PIDFILE
        rm -rf $PIDFILE
      fi
    else
      log_daemon_msg "$NAME process is not running"
      log_end_msg 0
    fi
    ;;
  restart)
    $0 stop && sleep 2 && $0 start
    ;;
  status)
    if [ -e $PIDFILE ]; then
      status_of_proc -p $PIDFILE $DAEMON "$NAME process" && exit 0 || exit $?
    else
      log_daemon_msg "$NAME Process is not running"
      log_end_msg 0
    fi
    ;;
  *)
    echo "Usage: $0 {start|stop|restart|status}"
    exit 2
    ;;
esac
  

./ps3timeout > /dev/null 2>&1 &
