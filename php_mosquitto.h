#ifndef PHP_MOSQUITTO_H
#define PHP_MOSQUITTO_H

#define PHP_MOSQUITTO_VERSION "0.4.0"

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

#if defined(ZEND_ENGINE_2) && defined(ZTS)
# define MOSQUITTO_NEED_TSRMLS
#endif

typedef struct _mosquitto_client_object {
#ifndef ZEND_ENGINE_3
	zend_object std;
#endif
	struct mosquitto *client;

	zend_fcall_info connect_callback;
	zend_fcall_info_cache connect_callback_cache;
	zend_fcall_info subscribe_callback;
	zend_fcall_info_cache subscribe_callback_cache;
	zend_fcall_info unsubscribe_callback;
	zend_fcall_info_cache unsubscribe_callback_cache;
	zend_fcall_info message_callback;
	zend_fcall_info_cache message_callback_cache;
	zend_fcall_info publish_callback;
	zend_fcall_info_cache publish_callback_cache;
	zend_fcall_info disconnect_callback;
	zend_fcall_info_cache disconnect_callback_cache;
	zend_fcall_info log_callback;
	zend_fcall_info_cache log_callback_cache;

	int looping;

#ifdef MOSQUITTO_NEED_TSRMLS
	TSRMLS_D;
#endif
#ifdef ZEND_ENGINE_3
	zend_object std; /* Must be last */
#endif
} mosquitto_client_object;

typedef struct _mosquitto_message_object {
#ifndef ZEND_ENGINE_3
	zend_object std;
#endif
	struct mosquitto_message message;
	zend_bool owned_topic;
	zend_bool owned_payload;
#ifdef MOSQUITTO_NEED_TSRMLS
	TSRMLS_D;
#endif
#ifdef ZEND_ENGINE_3
	zend_object std; /* Must be last */
#endif
} mosquitto_message_object;

static inline
mosquitto_client_object *mosquitto_client_object_from_zend_object(zend_object* obj) {
	return (mosquitto_client_object*)(
		((char*)obj) - XtOffsetOf(mosquitto_client_object, std)
	);
}

static inline
zend_object *mosquitto_client_object_to_zend_object(mosquitto_client_object* client) {
	return &(client->std);
}

static inline
mosquitto_message_object *mosquitto_message_object_from_zend_object(zend_object* obj) {
	return (mosquitto_message_object*)(
		((char*)obj) - XtOffsetOf(mosquitto_message_object, std)
	);
}

static inline
zend_object *mosquitto_message_object_to_zend_object(mosquitto_message_object* msg) {
	return &(msg->std);
}

#ifdef ZEND_ENGINE_3
typedef int (*php_mosquitto_read_t)(mosquitto_message_object *mosquitto_object, zval *retval);
#else
typedef int (*php_mosquitto_read_t)(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC);
#endif
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

#ifdef ZEND_ENGINE_3
# define PHP_MOSQUITTO_FREE_CALLBACK(client, CALLBACK) \
    if (ZEND_FCI_INITIALIZED(client->CALLBACK ## _callback)) { \
        zval_ptr_dtor(&client->CALLBACK ## _callback.function_name); \
    } \
 \
    if (client->CALLBACK ## _callback.object != NULL) { \
		zval tmp_; \
		ZVAL_OBJ(&tmp_, client->CALLBACK ## _callback.object); \
		zval_ptr_dtor(&tmp_); \
    } \
	client->CALLBACK ## _callback = empty_fcall_info; \
	client->CALLBACK ## _callback_cache = empty_fcall_info_cache;
#else
# define PHP_MOSQUITTO_FREE_CALLBACK(client, CALLBACK) \
    if (ZEND_FCI_INITIALIZED(client->CALLBACK ## _callback)) { \
        zval_ptr_dtor(&client->CALLBACK ## _callback.function_name); \
    } \
 \
	if (client->CALLBACK ## _callback.object_ptr != NULL) { \
		zval_ptr_dtor(&client->CALLBACK ## _callback.object_ptr); \
	} \
	client->CALLBACK ## _callback = empty_fcall_info; \
	client->CALLBACK ## _callback_cache = empty_fcall_info_cache;
#endif

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

#ifdef ZEND_ENGINE_3
# define PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_READER_FUNCTION(name) \
	static int php_mosquitto_message_read_##name(mosquitto_message_object *mosquitto_object, zval *retval) { \
		ZVAL_LONG(retval, mosquitto_object->message.name); \
		return SUCCESS; \
	}
#else
# define PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_READER_FUNCTION(name) \
	static int php_mosquitto_message_read_##name(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC) { \
		MAKE_STD_ZVAL(*retval); \
		ZVAL_LONG(*retval, mosquitto_object->message.name); \
		return SUCCESS; \
	}
#endif

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
	int client_key_len;
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
PHP_MOSQUITTO_API void php_mosquitto_publish_callback(struct mosquitto *mosq, void *client_obj, int mid);
PHP_MOSQUITTO_API void php_mosquitto_disconnect_callback(struct mosquitto *mosq, void *obj, int rc);

PHP_MOSQUITTO_API char *php_mosquitto_strerror_wrapper(int err);
void php_mosquitto_handle_errno(int retval, int err TSRMLS_DC);
void php_mosquitto_exit_loop(mosquitto_client_object *object);

PHP_MINIT_FUNCTION(mosquitto);
PHP_MINIT_FUNCTION(mosquitto_message);
PHP_MSHUTDOWN_FUNCTION(mosquitto);
PHP_MINFO_FUNCTION(mosquitto);

#endif	/* PHP_MOSQUITTO_H */

/* __footer_here__ */
