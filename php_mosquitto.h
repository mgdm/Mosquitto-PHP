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

#ifdef __GLIBC__
#define POSSIBLY_UNUSED __attribute__((unused))
#else
#define POSSIBLY_UNUSED
#endif

#if defined(PHP_VERSION_ID) && (PHP_VERSION_ID >= 50399)
#	define ZEND_LITERAL_KEY_DC , const zend_literal *_zend_literal_key
#	define ZEND_LITERAL_KEY_CC , _zend_literal_key
#	define ZEND_LITERAL_NIL_CC , NULL
#else
#	define ZEND_LITERAL_KEY_DC
#	define ZEND_LITERAL_KEY_CC
#	define ZEND_LITERAL_NIL_CC
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include <mosquitto.h>

typedef struct _mosquitto_client_object {
	zend_object std;
	struct mosquitto *client;

	zend_fcall_info connect_callback;
	zend_fcall_info_cache connect_callback_cache;
	zend_fcall_info subscribe_callback;
	zend_fcall_info_cache subscribe_callback_cache;
	zend_fcall_info unsubscribe_callback;
	zend_fcall_info_cache unsubscribe_callback_cache;
	zend_fcall_info message_callback;
	zend_fcall_info_cache message_callback_cache;
	zend_fcall_info disconnect_callback;
	zend_fcall_info_cache disconnect_callback_cache;
	zend_fcall_info log_callback;
	zend_fcall_info_cache log_callback_cache;

#ifdef ZTS
	TSRMLS_D;
#endif
} mosquitto_client_object;

typedef struct _mosquitto_message_object {
	zend_object std;
	struct mosquitto_message message;
	zend_bool owned_topic;
	zend_bool owned_payload;
#ifdef ZTS
	TSRMLS_D;
#endif
} mosquitto_message_object;

typedef int (*php_mosquitto_read_t)(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC);
typedef int (*php_mosquitto_write_t)(mosquitto_message_object *mosquitto_object, zval *newval TSRMLS_DC);

typedef struct _php_mosquitto_prop_handler {
	const char *name;
	size_t name_length;
	php_mosquitto_read_t read_func;
	php_mosquitto_write_t write_func;
} php_mosquitto_prop_handler;


#define PHP_MOSQUITTO_ERROR_HANDLING() \
	zend_replace_error_handling(EH_THROW, mosquitto_ce_exception, &MQTTG(mosquitto_original_error_handling) TSRMLS_CC)

#define PHP_MOSQUITTO_RESTORE_ERRORS() \
	zend_restore_error_handling(&MQTTG(mosquitto_original_error_handling) TSRMLS_CC)




#define PHP_MOSQUITTO_MESSAGE_PROPERTY_ENTRY_RECORD(name) \
	{ "" #name "",		sizeof("" #name "") - 1,	php_mosquitto_message_read_##name,	php_mosquitto_message_write_##name }

#define PHP_MOSQUITTO_ADD_PROPERTIES(a, b) \
{ \
	int i = 0; \
	while (b[i].name != NULL) { \
		php_mosquitto_message_add_property((a), (b)[i].name, (b)[i].name_length, \
							(php_mosquitto_read_t)(b)[i].read_func, (php_mosquitto_write_t)(b)[i].write_func TSRMLS_CC); \
		i++; \
	} \
}

#define PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_READER_FUNCTION(name) \
	static int php_mosquitto_message_read_##name(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC) \
	{ \
		MAKE_STD_ZVAL(*retval); \
		ZVAL_LONG(*retval, mosquitto_object->message.name); \
		return SUCCESS; \
	}

#define PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_WRITER_FUNCTION(name) \
static int php_mosquitto_message_write_##name(mosquitto_message_object *mosquitto_object, zval *newval TSRMLS_DC) \
{ \
	zval ztmp; \
	if (Z_TYPE_P(newval) != IS_LONG) { \
		ztmp = *newval; \
		zval_copy_ctor(&ztmp); \
		convert_to_long(&ztmp); \
		newval = &ztmp; \
	} \
\
	mosquitto_object->message.name = Z_LVAL_P(newval); \
\
	if (newval == &ztmp) { \
		zval_dtor(newval); \
	} \
	return SUCCESS; \
}

ZEND_BEGIN_MODULE_GLOBALS(mosquitto)
	char *client_key;
	zend_object_handlers mosquitto_std_object_handlers;
	zend_error_handling mosquitto_original_error_handling;
ZEND_END_MODULE_GLOBALS(mosquitto)

#ifdef ZTS
# define MQTTG(v) TSRMG(mosquitto_globals_id, zend_mosquitto_globals *, v)
#else
# define MQTTG(v) (mosquitto_globals.v)
#endif

ZEND_EXTERN_MODULE_GLOBALS(mosquitto)

extern zend_class_entry *mosquitto_ce_exception;
extern zend_class_entry *mosquitto_ce_client;
extern zend_class_entry *mosquitto_ce_message;

PHP_MOSQUITTO_API void php_mosquitto_connect_callback(struct mosquitto *mosq, void *obj, int rc);
PHP_MOSQUITTO_API void php_mosquitto_disconnect_callback(struct mosquitto *mosq, void *obj, int rc);
PHP_MOSQUITTO_API void php_mosquitto_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str);
PHP_MOSQUITTO_API void php_mosquitto_message_callback(struct mosquitto *mosq, void *client_obj, const struct mosquitto_message *message);
PHP_MOSQUITTO_API void php_mosquitto_subscribe_callback(struct mosquitto *mosq, void *client_obj, int mid, int qos_count, const int *granted_qos);
PHP_MOSQUITTO_API void php_mosquitto_unsubscribe_callback(struct mosquitto *mosq, void *client_obj, int mid);
PHP_MOSQUITTO_API void php_mosquitto_disconnect_callback(struct mosquitto *mosq, void *obj, int rc);

char *php_mosquitto_strerror_wrapper(int err);
void php_mosquitto_handle_errno(int retval, int err TSRMLS_DC);

PHP_MINIT_FUNCTION(mosquitto);
PHP_MINIT_FUNCTION(mosquitto_message);
PHP_MSHUTDOWN_FUNCTION(mosquitto);
PHP_MINFO_FUNCTION(mosquitto);

#endif	/* PHP_MOSQUITTO_H */

/* __footer_here__ */
