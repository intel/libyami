#!/bin/sh
#
# Convert your file to a K&R indent code style through indent tool
# All *.c and *.h file or files in specified folder will be converted
# to K&R code indent style.

# The indent propram changs the appearance of source code by
# inserting or deletting whitespace to unify the indentation
# format for declaration, functions, sentence and etc, you are
# allowed to configure the detail formats or simplily apply one
# pre-defined code styles, such as K&R, GNU, Kerkeley, Linux.
# we use K&R style here

# Kernighan & Ritchie code style
# The Kernighan & Ritchie style is used throughout their well-known
# book The C programming Language. It is enabled with the '-kr' option.
# The Kernighan & Ritchie style corresponding to following set of
# options, both long term options and abbreviation works:

# -nbad : --no-blank-lines-after-declarations
# -bap  : --blank-lines-after-procedures
# -bbo  : --break-before-boolean-operator
# -nbc  : --no-blank-lines-after-commas
# -br   : --braces-on-if-line
# -brs  : --braces-on-struct-decl-line
# -c33  : --comment-indentation33
# -cd33 : --declaration-comment-column33
# -ncdb : --no-comments-delimiters-on-blank-lines
# -ce   : --cuddle-else
# -ci4  : --continuation-indentation4
# cli0  : --case-indentation0
# -cp33 : --else-endif-column33
# -cs   : --space-after-cast
# -d0   : --line-comments-indentation
# -di1  : --declaration-indentation
# -nfc1 : --dont-format-first-column-comments
# -nfca : --dont-format-comments
# -hnl  : --honour-newlines
# -i4   : --indent-level4
# -ip0  : --parameter-indentation0
# -l75  : --line-length75
# -lp   : --continue-at-parentheses
# -npcs : --no-space-after-function-call-names
# -nprs : --no-space-after-parentheses
# -saf  : --space-after-for
# -sai  : --space-after-if
# -saw  : --space-after-while
# -nsc  : --no-space-after-casts
# -nsob : --leave-optional-blank-lines
# -nss  : --dont-space-special-semicolon

# Check for existence of indent, and error out if not present.
# On some *bsd systems the binary seems to be called gnunindent,
# so check for that first.
version=`gnuindent --version 2>/dev/null`
if test "x$version" = "x"; then
  version=`indent --version 2>/dev/null`
  if test "x$version" = "x"; then
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
      echo "Did not find GNU indent, please install it before continuing."
      echo "(Found $INDENT, but it doesn't seem to be GNU indent)"
      exit 1
      ;;
esac

# Convert code indent style
INDENT_PARAMETERS="-kr"
echo "Convert code to Kernighan & Ritchie style: "
for file in `find $1 -name "*.[hc]*"`; do
   echo "processing file : $file"
   $INDENT ${INDENT_PARAMETERS} $file -o $file 2>> /dev/NULL
done

echo "conversion done!"
