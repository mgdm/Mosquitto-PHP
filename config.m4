dnl $Id$
dnl config.m4 for extension mosquitto

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

PHP_ARG_WITH(mosquitto, for mosquitto support,
[  --with-mosquitto             Include mosquitto support])


if test "$PHP_MOSQUITTO" != "no"; then
  dnl Write more examples of tests here...

  # --with-mosquitto -> check with-path
  SEARCH_PATH="/usr/local /usr"     # you might want to change this
  SEARCH_FOR="/include/mosquitto.h"  # you most likely want to change this
  if test -r $PHP_MOSQUITTO/$SEARCH_FOR; then # path given as parameter
    MOSQUITTO_DIR=$PHP_MOSQUITTO
  else # search default path list
    AC_MSG_CHECKING([for mosquitto files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        MOSQUITTO_DIR=$i
		AC_MSG_CHECKING($MOSQUITTO_DIR)
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi
  
  if test -z "$MOSQUITTO_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the mosquitto distribution])
  fi

  # --with-mosquitto -> add include path
  PHP_ADD_INCLUDE($MOSQUITTO_DIR/include)

  # --with-mosquitto -> check for lib and symbol presence
  LIBNAME=mosquitto # you may want to change this

  PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $MOSQUITTO_DIR/$PHP_LIBDIR, MOSQUITTO_SHARED_LIBADD)
  
  PHP_SUBST(MOSQUITTO_SHARED_LIBADD)

  PHP_NEW_EXTENSION(mosquitto, mosquitto.c mosquitto_message.c, $ext_shared)

  AC_FUNC_STRERROR_R
fi
