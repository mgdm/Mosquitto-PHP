#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"
#include "php_mosquitto.h"

zend_class_entry *mosquitto_ce_client;
zend_class_entry *mosquitto_ce_exception;
zend_object_handlers mosquitto_std_object_handlers;
zend_error_handling mosquitto_original_error_handling;

/* {{{ */
PHP_METHOD(Mosquitto_Client, __construct)
{
	mosquitto_client_object *object;
	char *id = NULL;
	int id_len = 0;
	zend_bool clean_session = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!|b", &id, &id_len, &clean_session) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	object->client = mosquitto_new(id, clean_session, object);

	if (!object->client) {
		char buf[0x100];
		strerror_r(errno, buf, 0x100);
		zend_throw_exception(mosquitto_ce_exception, buf, 1 TSRMLS_CC);
	}
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, connect)
{
	mosquitto_client_object *object;
	char *host = NULL, *interface = NULL;
	int host_len, interface_len, retval;
	long port = 1883;
	long keepalive = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lls!", 
				&host, &host_len, &port, &keepalive, &interface, &interface_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_connect(object->client, host, port, keepalive);

	if (retval != MOSQ_ERR_SUCCESS) {
		char buf[0x100];
		strerror_r(errno, buf, 0x100);
		zend_throw_exception(mosquitto_ce_exception, buf, 1 TSRMLS_CC);
	}
}
/* }}} */

/* Internal functions */

static void mosquitto_client_object_destroy(void *object TSRMLS_DC)
{
	mosquitto_client_object *client = (mosquitto_client_object *) object;
	zend_hash_destroy(client->std.properties);
	FREE_HASHTABLE(client->std.properties);
	mosquitto_destroy(client->client);
	efree(object);
}

static zend_object_value mosquitto_client_object_new() {

	zend_object_value retval;
	mosquitto_client_object *client;

	client = ecalloc(1, sizeof(mosquitto_client_object));
	client->std.ce = mosquitto_ce_client;
	client->client = NULL;

	ALLOC_HASHTABLE(client->std.properties);
	zend_hash_init(client->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(client->std.properties, &mosquitto_ce_client->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &temp, sizeof(zval *));
#else
	object_properties_init(&client->std, mosquitto_ce_client);
#endif
	retval.handle = zend_objects_store_put(client, NULL, (zend_objects_free_object_storage_t) mosquitto_client_object_destroy, NULL TSRMLS_CC);
	retval.handlers = &mosquitto_std_object_handlers;
	return retval;
}

/* {{{ mosquitto_client_methods */
const zend_function_entry mosquitto_client_methods[] = {
	PHP_ME(Mosquitto_Client, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Mosquitto_Client, connect, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ mosquitto_functions[] */
const zend_function_entry mosquitto_functions[] = {
	PHP_FE_END	/* Must be the last line in mosquitto_functions[] */
};
/* }}} */

/* {{{ mosquitto_module_entry */
zend_module_entry mosquitto_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"mosquitto",
	NULL,
	PHP_MINIT(mosquitto),
	PHP_MSHUTDOWN(mosquitto),
	NULL,
	NULL,
	PHP_MINFO(mosquitto),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MOSQUITTO
ZEND_GET_MODULE(mosquitto)
#endif

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(mosquitto)
{
	memcpy(&mosquitto_std_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	mosquitto_std_object_handlers.clone_obj = NULL;

	zend_class_entry client_ce, exception_ce;
	INIT_NS_CLASS_ENTRY(client_ce, "Mosquitto", "Client", mosquitto_client_methods);
	mosquitto_ce_client = zend_register_internal_class_ex(&client_ce, NULL, NULL TSRMLS_CC);
	mosquitto_ce_client->create_object = mosquitto_client_object_new;

	INIT_NS_CLASS_ENTRY(exception_ce, "Mosquitto", "Exception", NULL);
	mosquitto_ce_exception = zend_register_internal_class_ex(&exception_ce,
			zend_exception_get_default(TSRMLS_C), "Exception" TSRMLS_CC);

	mosquitto_lib_init();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(mosquitto)
{
	mosquitto_lib_cleanup();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(mosquitto)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "mosquitto support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
