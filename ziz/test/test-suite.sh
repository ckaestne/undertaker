#!/bin/sh

#set -x

default_path=".."
default_cmd="zizler --short \$file"
tests_list=`find in/ -name '*.c' | sed -e 's#^\./\(.*\)#\1#' | sort`
prog_name=`basename $0`

# counts:
#	- tests that have not been converted to test-suite format
#	- tests that passed
#	- tests that failed
#	- tests that failed but are known to fail
unhandled_tests=0
ok_tests=0
ko_tests=0
known_ko_tests=0

# defaults to not verbose
[ -z "$V" ] && V=0

##
# get_value(key, file) - gets the value of a (key, value) pair in file.
#
# returns 0 on success, 1 if the file does not have the key
get_value()
{
	last_result=`grep $1: $2 | sed -e "s/^.*$1:\(.*\)$/\1/"`
	[ -z "$last_result" ] && return 1
	return 0
}

##
# get_tag(key, file) - does file has the tag key in it ?
#
# returns 0 if present, 1 otherwise
get_tag()
{
	last_result=`grep $1 $2`
	return $?
}

##
# verbose(string) - prints string if we are in verbose mode
verbose()
{
	[ "$V" -eq "1" ] && echo "        $1"
	return 0
}

##
# error(string[, die]) - prints an error and exits with value die if given
error()
{
	echo "error: $1"
	[ -n "$2" ] && exit $2
	return 0
}

do_usage()
{
echo "$prog_name - a tiny automatic testing script"
echo "Usage: $prog_name [command] [command arguments]"
echo
echo "commands:"
echo "    none                       runs the whole test suite"
echo "    single file                runs the test in 'file'"
echo "    format file [name [cmd]]   helps writing a new test case using cmd"
echo
echo "    help                       prints usage"
}

##
# do_test(file) - tries to validate a test case
#
# it "parses" file, looking for check-* tags and tries to validate
# the test against an expected result
# returns:
#	- 0 if the test passed,
#	- 1 if it failed,
#	- 2 if it is not a "test-suite" test.
do_test()
{
	test_failed=0
	file="$1"
	basename=`basename $file`

	# can this test be handled by test-suite?
	# (there must exist *.testspec *.output *.error files for it)
	if [ ! -r "$file.testspec" ]; then
		echo "warning: test '$basename' unhandled (spec file missing)"
		unhandled_tests=`expr $unhandled_tests + 1`
		return 2
	fi
	get_value "check-name" $file.testspec
	if [ "$?" -eq 1 ]; then
		echo "warning: test '$basename' unhandled"
		unhandled_tests=`expr $unhandled_tests + 1`
		return 2
	fi
	test_name=$last_result

	# which tests to actually perform
	diff=""
	if [ -r "$file.output" ]; then diff="output"       ; fi
	if [ -r "$file.error"  ]; then diff="$diff error"  ; fi

	echo "     TEST    $test_name ($basename)"

	# does the test provide a specific command ?
	cmd=`eval echo $default_path/$default_cmd`
	get_value "check-command" $file.testspec
	if [ "$?" -eq "0" ]; then
		last_result=`echo $last_result | sed -e 's/^ *//'`
		cmd=`eval echo $default_path/$last_result`
	fi
	verbose "Using command       : $cmd"

	# grab the expected exit value
	get_value "check-exit-value" $file.testspec
	if [ "$?" -eq "0" ]; then
		expected_exit_value=`echo $last_result | tr -d ' '`
	else
		expected_exit_value=0
	fi
	verbose "Expecting exit value: $expected_exit_value"

	# grab the actual output & exit value
	$cmd 1> "out/$basename.output" 2> "out/$basename.error"
	actual_exit_value=$?

	for stream in $diff; do
		diff -u "in/$basename.$stream" "out/$basename.$stream" \
		    > "out/$basename.$stream.diff"
		if [ "$?" -ne "0" ]; then
			error "actual $stream text does not match expected $stream text."
			error  "see out/$basename.$stream[.diff] for further investigation."
			test_failed=1
		fi
	done

	if [ "$actual_exit_value" -ne "$expected_exit_value" ]; then
		error "Actual exit value does not match the expected one."
		error "expected $expected_exit_value, got $actual_exit_value."
		test_failed=1
	fi

	if [ "$test_failed" -eq "1" ]; then
		ko_tests=`expr $ko_tests + 1`
		get_tag "check-known-to-fail" $file
		if [ "$?" -eq "0" ]; then
			echo "info: test '$basename' is known to fail"
			known_ko_tests=`expr $known_ko_tests + 1`
		fi
		return 1
	else
		ok_tests=`expr $ok_tests + 1`
		return 0
	fi
}

do_test_suite()
{
	for i in $tests_list; do
		do_test "$i"
	done

	# prints some numbers
	tests_nr=`expr $ok_tests + $ko_tests`
	echo -n "Out of $tests_nr tests, $ok_tests passed, $ko_tests failed"
	echo " ($known_ko_tests of them are known to fail)"
	if [ "$unhandled_tests" -ne "0" ]; then
		echo "$unhandled_tests tests could not be handled by $prog_name"
	fi
}

##
# do_format(file[, name[, cmd]]) - helps a test writer to format test-suite tags
do_format()
{
	if [ -z "$2" ]; then
		fname="$1"
		fcmd=$default_cmd
	elif [ -z "$3" ]; then
		fname="$2"
		fcmd=$default_cmd
	else
		fname="$2"
		fcmd="$3"
	fi
	file="$1"
	cmd=`eval echo $default_path/$fcmd`
	$cmd 1> $file.output.got 2> $file.error.got
	fexit_value=$?
	cat <<_EOF
/*
 * check-name: $fname
_EOF
	if [ "$fcmd" != "$default_cmd" ]; then
		echo " * check-command: $fcmd"
	fi
	if [ "$fexit_value" -ne "0" ]; then
		echo " * check-exit-value: $fexit_value"
	fi
	for stream in output error; do
		if [ -s "$file.$stream.got" ]; then
			echo " *"
			echo " * check-$stream-start"
			cat "$file.$stream.got"
			echo " * check-$stream-end"
		fi
	done
	echo " */"
	return 0
}

##
# arg_file(filename) - checks if filename exists
arg_file()
{
	[ -z "$1" ] && {
		do_usage
		exit 1
	}
	[ -e "$1" ] || {
		error "Can't open file $1"
		exit 1
	}
	return 0
}

case "$1" in
	'')
		do_test_suite
		if test `expr $ko_tests - $known_ko_tests` -gt 0; then
		    for f in out/*.error.diff out/*.output.diff; do
		        if test -s "$f"; then
		            echo "Contents of $f:"
		            cat "$f"
		            echo "==="
		        fi
		    done
		    exit 1
		fi
		;;
	single)
		arg_file "$2"
		do_test "$2"
		case "$?" in
			0) echo "$2 passed !";;
			1) echo "$2 failed !";;
			2) echo "$2 can't be handled by $prog_name";;
		esac
		;;
	format)
		arg_file "$2"
		do_format "$2" "$3" "$4"
		;;
	help | *)
		do_usage
		exit 1
		;;
esac

exit 0

