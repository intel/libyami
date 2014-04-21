#!/bin/sh
#
# Check that the code follows a consistant code style
#

# Check for existence of indent, and error out if not present.
# On some *bsd systems the binary seems to be called gnunindent,
# so check for that first.

version=`gnuindent --version 2>/dev/null`
if test "x$version" = "x"; then
  version=`indent --version 2>/dev/null`
  if test "x$version" = "x"; then
    echo "GStreamer git pre-commit hook:"
    echo "Did not find GNU indent, please install it before continuing."
    exit 1
  fi
  INDENT=indent
else
  INDENT=gnuindent
fi

case `$INDENT --version` in
  GNU*)
      ;;
  default)
      echo "GStreamer git pre-commit hook:"
      echo "Did not find GNU indent, please install it before continuing."
      echo "(Found $INDENT, but it doesn't seem to be GNU indent)"
      exit 1
      ;;
esac

INDENT_PARAMETERS="--braces-on-if-line \
	--case-brace-indentation0 \
	--case-indentation2 \
	--braces-after-struct-decl-line \
	--line-length80 \
	--no-tabs \
	--cuddle-else \
	--dont-line-up-parentheses \
	--continuation-indentation4 \
	--honour-newlines \
	--tab-size8 \
	--indent-level2 \
	--leave-preprocessor-space"

# FIXME: Call indent twice as it tends to do line-breaks
# different for every second call.
$INDENT ${INDENT_PARAMETERS} $1
$INDENT ${INDENT_PARAMETERS} $1
