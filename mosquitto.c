#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_mosquitto.h"

zend_class_entry *mosquitto_ce_client;
zend_object_handlers mosquitto_std_object_handlers;

PHP_FUNCTION(mosquitto_version)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_LONG(mosquitto_lib_version(NULL, NULL, NULL));
}

/* Internal functions */

static void mosquitto_client_object_destroy(void *object TSRMLS_DC)
{
	mosquitto_client_object *client = (mosquitto_client_object *) object;
	zend_hash_destroy(context->std.properties);
	FREE_HASHTABLE(context->std.properties);
	mosquitto_destroy(client->client);
	efree(object);
}

static zend_object_value mosquitto_client_object_new() {

	zend_object_value retval;
	mosquitto_client_object *client;
	zval *temp;

	client = ecalloc(1, sizeof(mosquitto_client_object));
	client->std.ce = mosquitto_ce_client;
	client->client = NULL;

	ALLOC_HASHTABLE(client->std.properties);
	zend_hash_init(client->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(client->std.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &temp, sizeof(zval *));
#else
	object_properties_init(&client->std, ce);
#endif
	retval.handle = zend_objects_store_put(client, NULL, (zend_objects_free_object_storage_t) mosquitto_client_object_destroy, NULL TSRMLS_CC);
	retval.handlers = &mosquitto_std_object_handlers;
	return retval;
}

/* {{{ mosquitto_functions[] */
const zend_function_entry mosquitto_functions[] = {
	PHP_FE(mosquitto_version, NULL)
	PHP_FE_END	/* Must be the last line in mosquitto_functions[] */
};
/* }}} */

/* {{{ mosquitto_module_entry */
zend_module_entry mosquitto_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"mosquitto",
	mosquitto_functions,
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

#ifdef COMPILE_DL_mosquitto
ZEND_GET_MODULE(mosquitto)
#endif

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(mosquitto)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(mosquitto)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
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
