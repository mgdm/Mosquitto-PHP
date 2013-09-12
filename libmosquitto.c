#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_libmosquitto.h"

zend_class_entry *libmosquitto_ce_client;
zend_object_handlers libmosquitto_std_object_handlers;

PHP_FUNCTION(mosquitto_version)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_LONG(mosquitto_lib_version(NULL, NULL, NULL));
}

/* {{{ libmosquitto_functions[] */
const zend_function_entry libmosquitto_functions[] = {
	PHP_FE(mosquitto_version, NULL)
	PHP_FE_END	/* Must be the last line in libmosquitto_functions[] */
};
/* }}} */

/* {{{ libmosquitto_module_entry */
zend_module_entry libmosquitto_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"libmosquitto",
	libmosquitto_functions,
	PHP_MINIT(libmosquitto),
	PHP_MSHUTDOWN(libmosquitto),
	NULL,
	NULL,
	PHP_MINFO(libmosquitto),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_LIBMOSQUITTO
ZEND_GET_MODULE(libmosquitto)
#endif

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(libmosquitto)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(libmosquitto)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(libmosquitto)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "libmosquitto support", "enabled");
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
