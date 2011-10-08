#!/bin/sh

mkdir -p ac
test -f AUTHORS   || touch AUTHORS
test -f COPYING   || touch COPYING
test -f ChangeLog || touch ChangeLog
test -f NEWS      || touch NEWS
test -f README    || cp README.md README

if [ x`uname` = x"Darwin" ]; then
	LIBTOOLIZE="glibtoolize --force --copy"
else
	LIBTOOLIZE="libtoolize --force --copy"
fi
ACLOCAL="aclocal"
AUTOHEADER="autoheader"
AUTOMAKE="automake --add-missing --copy"
AUTOCONF="autoconf"

#$LIBTOOLIZE
$ACLOCAL
$AUTOHEADER
$AUTOMAKE
$AUTOCONF

