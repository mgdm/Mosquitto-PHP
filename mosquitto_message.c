#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_variables.h"
#include "zend_exceptions.h"
#include "zend_API.h"
#include "ext/standard/info.h"
#include "php_mosquitto.h"

zend_class_entry *mosquitto_ce_message;
static zend_object_handlers mosquitto_message_object_handlers;
static HashTable *php_mosquitto_message_properties;

PHP_METHOD(Mosquitto_Message, __construct)
{
	mosquitto_message_object *object;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_message_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
}

PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_READER_FUNCTION(mid);
PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_READER_FUNCTION(qos);

static int php_mosquitto_message_read_retain(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC)
{
	MAKE_STD_ZVAL(*retval);
	ZVAL_BOOL(*retval, mosquitto_object->message->retain);
	return SUCCESS;
}

static int php_mosquitto_message_read_topic(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC)
{
	MAKE_STD_ZVAL(*retval);
	ZVAL_STRINGL(*retval, mosquitto_object->message->topic, strlen(mosquitto_object->message->topic), 1);
	return SUCCESS;
}

static int php_mosquitto_message_read_payload(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC)
{
	MAKE_STD_ZVAL(*retval);
	ZVAL_STRINGL(*retval, mosquitto_object->message->payload, mosquitto_object->message->payloadlen, 1);
	return SUCCESS;
}

PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_WRITER_FUNCTION(mid);
PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_WRITER_FUNCTION(qos);

static int php_mosquitto_message_write_retain(mosquitto_message_object *mosquitto_object, zval *newval TSRMLS_DC)
{
	zval ztmp;
	if (Z_TYPE_P(newval) != IS_BOOL) {
		ztmp = *newval;
		zval_copy_ctor(&ztmp);
		convert_to_boolean(&ztmp);
		newval = &ztmp;
	}

	mosquitto_object->message->retain = Z_LVAL_P(newval);

	if (newval == &ztmp) {
		zval_dtor(newval);
	}

	return SUCCESS;
}

static int php_mosquitto_message_write_topic(mosquitto_message_object *mosquitto_object, zval *newval TSRMLS_DC)
{
	zval ztmp;
	if (Z_TYPE_P(newval) != IS_STRING) {
		ztmp = *newval;
		zval_copy_ctor(&ztmp);
		convert_to_string(&ztmp);
		newval = &ztmp;
	}

	mosquitto_object->message->topic = estrdup(Z_STRVAL_P(newval));
	mosquitto_object->owned_topic = 1;

	if (newval == &ztmp) {
		zval_dtor(newval);
	}

	return SUCCESS;
}

static int php_mosquitto_message_write_payload(mosquitto_message_object *mosquitto_object, zval *newval TSRMLS_DC)
{
	zval ztmp;
	if (Z_TYPE_P(newval) != IS_STRING) {
		ztmp = *newval;
		zval_copy_ctor(&ztmp);
		convert_to_string(&ztmp);
		newval = &ztmp;
	}

	mosquitto_object->message->payload = estrdup(Z_STRVAL_P(newval));
	mosquitto_object->message->payloadlen = Z_STRLEN_P(newval);
	mosquitto_object->owned_payload = 1;

	if (newval == &ztmp) {
		zval_dtor(newval);
	}

	return SUCCESS;
}

const php_mosquitto_prop_handler php_mosquitto_message_property_entries[] = {
	PHP_MOSQUITTO_MESSAGE_PROPERTY_ENTRY_RECORD(mid),
	PHP_MOSQUITTO_MESSAGE_PROPERTY_ENTRY_RECORD(topic),
	PHP_MOSQUITTO_MESSAGE_PROPERTY_ENTRY_RECORD(payload),
	PHP_MOSQUITTO_MESSAGE_PROPERTY_ENTRY_RECORD(qos),
	PHP_MOSQUITTO_MESSAGE_PROPERTY_ENTRY_RECORD(retain),
	{NULL, 0, NULL, NULL}
};

zval *php_mosquitto_read_property(zval *object, zval *member, int type, const zend_literal *key TSRMLS_DC)
{
	zval tmp_member;
	zval *retval;
	mosquitto_message_object *message_object;
	php_mosquitto_prop_handler *hnd;
	int ret;

	message_object = (mosquitto_message_object *) zend_object_store_get_object(object TSRMLS_CC);

	if (Z_TYPE_P(member) != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	ret = zend_hash_find(php_mosquitto_message_properties, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void **) &hnd);

	if (ret == SUCCESS && hnd->read_func) {
		ret = hnd->read_func(message_object, &retval TSRMLS_CC);
		if (ret == SUCCESS) {
			/* ensure we're creating a temporary variable */
			Z_SET_REFCOUNT_P(retval, 0);
		} else {
			retval = EG(uninitialized_zval_ptr);
		}
	} else {
		zend_object_handlers * std_hnd = zend_get_std_object_handlers();
		retval = std_hnd->read_property(object, member, type, key TSRMLS_CC);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}

	return(retval);
}

void php_mosquitto_add_property(HashTable *h, const char *name, size_t name_length, php_mosquitto_read_t read_func, php_mosquitto_write_t write_func TSRMLS_DC)
{
	php_mosquitto_prop_handler p;

	p.name = (char*) name;
	p.name_length = name_length;
	p.read_func = (read_func) ? read_func : NULL;
	p.write_func = (write_func) ? write_func : NULL;
	zend_hash_add(h, (char *)name, name_length + 1, &p, sizeof(php_mosquitto_prop_handler), NULL);
}

static void mosquitto_message_object_destroy(void *object TSRMLS_DC)
{
	mosquitto_message_object *message = (mosquitto_message_object *) object;
	zend_hash_destroy(message->std.properties);
	FREE_HASHTABLE(message->std.properties);

	if (message->owned_topic == 1) {
		efree(message->message->topic);
	}

	if (message->owned_payload == 1) {
		efree(message->message->payload);
	}

	efree(object);
}

static zend_object_value mosquitto_message_object_new() {

	zend_object_value retval;
	mosquitto_message_object *message_obj;
	zval *temp;

	message_obj = ecalloc(1, sizeof(mosquitto_message_object));
	message_obj->std.ce = mosquitto_ce_message;
	message_obj->message = NULL;

	ALLOC_HASHTABLE(message_obj->std.properties);
	zend_hash_init(message_obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(message_obj->std.properties, &mosquitto_ce_message->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &temp, sizeof(zval *));
#else
	object_properties_init(&message_obj->std, mosquitto_ce_message);
#endif
	retval.handle = zend_objects_store_put(message_obj, NULL, (zend_objects_free_object_storage_t) mosquitto_message_object_destroy, NULL TSRMLS_CC);
	retval.handlers = &mosquitto_std_object_handlers;
	return retval;
}

const zend_function_entry mosquitto_message_methods[] = {
	PHP_FE_END
};

PHP_MINIT_FUNCTION(mosquitto_message)
{
	zend_class_entry message_ce;

	INIT_NS_CLASS_ENTRY(message_ce, "Mosquitto", "Message", mosquitto_message_methods);
	mosquitto_ce_message = zend_register_internal_class(&message_ce TSRMLS_CC);

	zend_hash_init(php_mosquitto_message_properties, 0, NULL, NULL, 1);
	PHP_MOSQUITTO_ADD_PROPERTIES(php_mosquitto_message_properties, php_mosquitto_message_property_entries);

	return SUCCESS;
}	
