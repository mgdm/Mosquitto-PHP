dnl $Id$
dnl config.m4 for extension libmosquitto

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

PHP_ARG_WITH(libmosquitto, for libmosquitto support,
[  --with-libmosquitto             Include libmosquitto support])


if test "$PHP_LIBMOSQUITTO" != "no"; then
  dnl Write more examples of tests here...

  # --with-libmosquitto -> check with-path
  SEARCH_PATH="/usr/local /usr"     # you might want to change this
  SEARCH_FOR="/include/mosquitto.h"  # you most likely want to change this
  if test -r $PHP_LIBMOSQUITTO/$SEARCH_FOR; then # path given as parameter
    LIBMOSQUITTO_DIR=$PHP_LIBMOSQUITTO
  else # search default path list
    AC_MSG_CHECKING([for libmosquitto files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        LIBMOSQUITTO_DIR=$i
		AC_MSG_CHECKING($LIBMOSQUITTO_DIR)
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi
  
  if test -z "$LIBMOSQUITTO_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the libmosquitto distribution])
  fi

  # --with-libmosquitto -> add include path
  PHP_ADD_INCLUDE($LIBMOSQUITTO_DIR/include)

  # --with-libmosquitto -> check for lib and symbol presence
  LIBNAME=mosquitto # you may want to change this

  PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $LIBMOSQUITTO_DIR/lib, LIBMOSQUITTO_SHARED_LIBADD)
  
  PHP_SUBST(LIBMOSQUITTO_SHARED_LIBADD)

  PHP_NEW_EXTENSION(libmosquitto, libmosquitto.c, $ext_shared)
fi
