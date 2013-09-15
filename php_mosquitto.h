#ifndef PHP_MOSQUITTO_H
#define PHP_MOSQUITTO_H

extern zend_module_entry mosquitto_module_entry;
#define phpext_mosquitto_ptr &mosquitto_module_entry

#ifdef PHP_WIN32
#	define PHP_MOSQUITTO_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_MOSQUITTO_API __attribute__ ((visibility("default")))
#else
#	define PHP_MOSQUITTO_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include <mosquitto.h>

typedef struct _mosquitto_context_object {
	zend_object std;
	struct mosquitto *client;
} mosquitto_context_object;

PHP_MINIT_FUNCTION(mosquitto);
PHP_MSHUTDOWN_FUNCTION(mosquitto);
PHP_MINFO_FUNCTION(mosquitto);

PHP_FUNCTION(mosquitto_version);

#endif	/* PHP_MOSQUITTO_H */

/* __footer_here__ */
