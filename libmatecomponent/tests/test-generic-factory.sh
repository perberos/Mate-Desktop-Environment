#! /bin/sh

if test "z$MATECORBA_TMPDIR" = "z"; then
	MATECORBA_TMPDIR="/tmp/matecorba-$USER/tst"
	rm -Rf $MATECORBA_TMPDIR
	mkdir -p $MATECORBA_TMPDIR
fi
TMPDIR=$MATECORBA_TMPDIR;
export TMPDIR;

MATECOMPONENT_ACTIVATION_SERVER="../activation-server/matecomponent-activation-server";
PATH=".:$PATH";
LD_LIBRARY_PATH="./.libs:$LD_LIBRARY_PATH";
unset MATECOMPONENT_ACTIVATION_DEBUG_OUTPUT
PATH="./.libs:$PATH";

export MATECOMPONENT_ACTIVATION_SERVER PATH LD_LIBRARY_PATH

# job control must be active
set -m

echo "Starting factory"
./generic-factory | tr -d '\015' > generic-factory.output &
sleep 1

echo "Starting client"
./test-generic-factory | tr -d '\015' > test-generic-factory.output

echo "Waiting for factory to terminate; Please hold on a second, otherwise hit Ctrl-C."
wait %1 2> /dev/null

echo "Comparing factory output with model..."
if diff -u $MODELS_DIR/generic-factory.output generic-factory.output; then
    echo "...OK"
    rm -f generic-factory.output
else
    echo "...DIFFERENT!"
    rm -f generic-factory.output
    rm -f test-generic-factory.output
    exit 1;
fi

echo "Comparing client output with model..."
if diff -u $MODELS_DIR/test-generic-factory.output test-generic-factory.output; then
    echo "...OK"
    rm -f test-generic-factory.output
else
    echo "...DIFFERENT!"
    rm -f test-generic-factory.output
    exit 1;
fi

exit 0

