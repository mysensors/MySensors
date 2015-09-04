#!/bin/sh
# pidfile must be first argument
PIDFILE=$1
mkdir -p /home/pi/logs
cd /home/pi/Arduino/NodeJsController
/opt/node/bin/node NodeJsController.js >>/home/pi/logs/NodeJsController.log 2>&1 </dev/null &
CHILD="$!"
echo $CHILD > $PIDFILE
