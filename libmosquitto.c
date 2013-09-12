#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_libmosquitto.h"

/* {{{ libmosquitto_functions[] */
const zend_function_entry libmosquitto_functions[] = {
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
	PHP_RINIT(libmosquitto),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(libmosquitto),	/* Replace with NULL if there's nothing to do at request end */
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

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("libmosquitto.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_libmosquitto_globals, libmosquitto_globals)
    STD_PHP_INI_ENTRY("libmosquitto.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_libmosquitto_globals, libmosquitto_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_libmosquitto_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_libmosquitto_init_globals(zend_libmosquitto_globals *libmosquitto_globals)
{
	libmosquitto_globals->global_value = 0;
	libmosquitto_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(libmosquitto)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(libmosquitto)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(libmosquitto)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(libmosquitto)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(libmosquitto)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "libmosquitto support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
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
