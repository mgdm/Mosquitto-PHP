#ifndef PHP_LIBMOSQUITTO_H
#define PHP_LIBMOSQUITTO_H

extern zend_module_entry libmosquitto_module_entry;
#define phpext_libmosquitto_ptr &libmosquitto_module_entry

#ifdef PHP_WIN32
#	define PHP_LIBMOSQUITTO_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_LIBMOSQUITTO_API __attribute__ ((visibility("default")))
#else
#	define PHP_LIBMOSQUITTO_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include <mosquitto.h>

typedef struct _libmosquitto_context_object {
	zend_object std;
	struct mosquitto *client;
} libmosquitto_context_object;

PHP_MINIT_FUNCTION(libmosquitto);
PHP_MSHUTDOWN_FUNCTION(libmosquitto);
PHP_MINFO_FUNCTION(libmosquitto);

PHP_FUNCTION(mosquitto_version);

#endif	/* PHP_LIBMOSQUITTO_H */

/* __footer_here__ */
