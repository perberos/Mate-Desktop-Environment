#!/bin/sh

# This is a generic script for firing up a server, waiting for it to write
# its stringified IOR to a file, then firing up a server

[ -z "$USER" ] && USER=`id -un`

## Disable core dumps because matecomponent-activation-empty-server is aborting with the error:
##   "This process has not registered the required OAFIID your source
##   code should register '%s'. If your code is performing delayed
##   registration and this message is trapped in error, see
##   matecomponent_activation_i"...
## And the core file is making distcheck fail.
## TODO: Check why is matecomponent-activation-empty-server aborting.

ulimit -c 0

eval $(dbus-launch --sh-syntax)

if test "z$MATECORBA_TMPDIR" = "z"; then
	MATECORBA_TMPDIR="/tmp/matecorba-$USER/tst"
	rm -Rf $MATECORBA_TMPDIR
	mkdir -p $MATECORBA_TMPDIR
fi
TMPDIR=$MATECORBA_TMPDIR;
export TMPDIR;

MATECOMPONENT_ACTIVATION_SERVER="../../activation-server/matecomponent-activation-server";
MATECOMPONENT_ACTIVATION_DEBUG_OUTPUT="1";
PATH=".:./.libs:$PATH";
LD_LIBRARY_PATH="./.libs:$LD_LIBRARY_PATH";

export MATECOMPONENT_ACTIVATION_SERVER MATECOMPONENT_ACTIVATION_DEBUG_OUTPUT MATECOMPONENT_ACTIVATION_PATH PATH LD_LIBRARY_PATH

if ./matecomponent-activation-test; then
    kill -15 $DBUS_SESSION_BUS_PID
    exit 0;
else
    kill -15 $DBUS_SESSION_BUS_PID
    exit 1;
fi
