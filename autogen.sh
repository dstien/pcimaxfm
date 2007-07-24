#!/bin/sh

usage()
{
	echo "Usage: autogen.sh [--clean]"
	exit 1
}

clean()
{
	make maintainer-clean
	exit 0
}

check()
{
	STATUS=0

	(aclocal --version) < /dev/null > /dev/null 2>&1 || {
		echo "Error: Couldn't find aclocal."
		STATUS=1
	}

	(autoheader --version) < /dev/null > /dev/null 2>&1 || {
		echo "Error: Couldn't find autoheader."
		STATUS=1
	}

	(automake --version) < /dev/null > /dev/null 2>&1 || {
		echo "Error: Couldn't find automake."
		STATUS=1
	}

	(autoconf --version) < /dev/null > /dev/null 2>&1 || {
		echo "Error: Couldn't find autoconf."
		STATUS=1
	}

	if [ $STATUS -ne 0 ]; then
		exit 1
	fi
}

run()
{
	echo "aclocal..."
	aclocal

	echo "autoheader..."
	autoheader

	echo "automake..."
	automake --gnu --add-missing --force-missing --copy

	echo "autoconf..."
	autoconf

	exit 0
}

if [ $# -gt 1 ]; then
	usage
elif [ $# -eq 1 ]; then
	if [ $1 = "--clean" ]; then
		clean
	else
		usage
	fi
else
	check
	run
fi
