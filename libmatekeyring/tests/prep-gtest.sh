#!/bin/sh -eu

set -eu

# --------------------------------------------------------------------
# FUNCTIONS

usage()
{
	echo "usage: prep-gtest.sh -b base-name files.c ..." >&2
	exit 2
}

# --------------------------------------------------------------------
# SOURCE FILE

file_to_name()
{
	echo -n $1 | sed -e 's/unit-test-//' -e 's/\.c//'
}

build_header()
{
	local _file

	echo '/* This is auto-generated code. Edit at your own peril. */'
	echo '#include "tests/gtest-helpers.h"'
	echo

	for _file in $@; do
		sed -ne 's/.*DEFINE_SETUP[ 	]*(\([^)]\+\))/DECLARE_SETUP(\1);/p' $_file
		sed -ne 's/.*DEFINE_TEARDOWN[ 	]*(\([^)]\+\))/DECLARE_TEARDOWN(\1);/p' $_file
		sed -ne 's/.*DEFINE_TEST[ 	]*(\([^)]\+\))/DECLARE_TEST(\1);/p' $_file
		sed -ne 's/.*DEFINE_START[ 	]*(\([^)]\+\))/DECLARE_START(\1);/p' $_file
		sed -ne 's/.*DEFINE_STOP[ 	]*(\([^)]\+\))/DECLARE_STOP(\1);/p' $_file
	done
	echo
}

build_source()
{
	local _tcases _file _name _setup _teardown

	echo '/* This is auto-generated code. Edit at your own peril. */'
	echo "#include \"tests/gtest-helpers.h\""
	echo "#include \"$BASE.h\""
	echo

	# Startup function
	echo "static void start_tests (void) {"
		for _file in $@; do
			sed -ne "s/.*DEFINE_START(\([^)]\+\)).*/	start_\1 ();/p" $_file
		done
	echo "}"
	echo

	# Shutdown function
	echo "static void stop_tests (void) {"
		for _file in $@; do
			sed -ne "s/.*DEFINE_STOP(\([^)]\+\)).*/	stop_\1 ();/p" $_file
		done
	echo "}"
	echo

	echo "static void initialize_tests (void) {"
	# Include each file, and build a test case for it
	_tcases=""
	for _file in $@; do
		_name=`file_to_name $_file`

		# Calculate what our setup and teardowns are.
		_setup=`sed -ne 's/.*DEFINE_SETUP(\([^)]\+\)).*/setup_\1/p' $_file || echo "NULL"`
		if [ -z "$_setup" ]; then
			_setup="NULL"
		fi

		_teardown=`sed -ne 's/.*DEFINE_TEARDOWN(\([^)]\+\)).*/teardown_\1/p' $_file`
		if [ -z "$_teardown" ]; then
			_teardown="NULL"
		fi

		# Add all tests to the test case
		sed -ne "s/.*DEFINE_TEST(\([^)]\+\)).*/	g_test_add(\"\/$_name\/\1\", int, NULL, $_setup, test_\1, $_teardown);/p" $_file

	done

	echo "}"
	echo

	echo "#include \"tests/gtest-helpers.c\""
}

# --------------------------------------------------------------------
# ARGUMENT PARSING

BASE=unit

while [ $# -gt 0 ]; do
	case "$1" in
	-b)
		BASE="$2"
		shift
		;;
	--)
		shift
		break
		;;
	-*)
		usage
		;;
	*)
		break
		;;
	esac
	shift
done

build_header $* > $BASE.h
build_source $* > $BASE.c
