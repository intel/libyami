
PROJECT="libyami"

test -n "$srcdir" || srcdir="`dirname \"$0\"`"
test -n "$srcdir" || srcdir=.

if ! test -f "$srcdir/configure.ac"; then
    echo "Failed to find the top-level $PROJECT directory"
    exit 1
fi

olddir="`pwd`"
cd "$srcdir"

mkdir -p m4

AUTORECONF=`which autoreconf`
if test -z $AUTORECONF; then
    echo "*** No autoreconf found ***"
    exit 1
else
    autoreconf -v --install || exit $?
fi

export CFLAGS="-D_FORTIFY_SOURCE=2 -fPIE -fPIC -fstack-protector"
export CXXFLAGS="-D_FORTIFY_SOURCE=2 -fPIE -fPIC -fstack-protector"
export LDFLAGS="-z noexecstack -z relro -z now -pie"

cd "$olddir"

if test -z "$NOCONFIGURE"; then
    $srcdir/configure "$@" && echo "Now type 'make' to compile $PROJECT."
fi

unset CFLAGS
unset CXXFLAGS
unset LDFLAGS
