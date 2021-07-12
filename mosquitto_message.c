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
static HashTable php_mosquitto_message_properties;

typedef size_t mosquitto_strlen_type;

/* {{{ Arginfo */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Mosquitto_Message_topicMatchesSub_args, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, topic, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, subscription, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Message_tokeniseTopic_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, topic, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Message_void_args, 0, 0, 0)
ZEND_END_ARG_INFO()

/* }}} */

PHP_METHOD(Mosquitto_Message, __construct)
{
	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();
}

/* {{{ Mosquitto\Message::topicMatchesSub() */
PHP_METHOD(Mosquitto_Message, topicMatchesSub)
{
	char *topic = NULL, *subscription = NULL;
	mosquitto_strlen_type topic_len, subscription_len;
	zend_bool result;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss",
				&topic, &topic_len, &subscription, &subscription_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	mosquitto_topic_matches_sub(subscription, topic, (bool *) &result);
	RETURN_BOOL(result);
}
/* }}} */

/* {{{ Mosquitto\Message::tokeniseTopic() */
PHP_METHOD(Mosquitto_Message, tokeniseTopic)
{
	char *topic = NULL, **topics = NULL;
	mosquitto_strlen_type topic_len = 0, retval = 0, count = 0, i = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &topic, &topic_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_sub_topic_tokenise(topic, &topics, (int*)&count);

	if (retval == MOSQ_ERR_NOMEM) {
		zend_throw_exception_ex(mosquitto_ce_exception, 0, "Failed to tokenise topic");
		return;
	}

	array_init(return_value);
	for (i = 0; i < count; i++) {
		if (topics[i] == NULL) {
			add_next_index_null(return_value);
		} else {
			add_next_index_string(return_value, topics[i]);
		}
	}

	mosquitto_sub_topic_tokens_free(&topics, count);
}
/* }}} */

PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_READER_FUNCTION(mid);
PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_READER_FUNCTION(qos);

static int php_mosquitto_message_read_retain(mosquitto_message_object *mosquitto_object, zval *retval)
{
	ZVAL_BOOL(retval, mosquitto_object->message.retain);
	return SUCCESS;
}

static int php_mosquitto_message_read_topic(mosquitto_message_object *mosquitto_object, zval *retval)
{
	if (mosquitto_object->message.topic != NULL) {
		ZVAL_STRING(retval, mosquitto_object->message.topic);
	} else {
		ZVAL_NULL(retval);
	}

	return SUCCESS;
}

static int php_mosquitto_message_read_payload(mosquitto_message_object *mosquitto_object, zval *retval)
{
	ZVAL_STRINGL(retval, mosquitto_object->message.payload, mosquitto_object->message.payloadlen);
	return SUCCESS;
}

PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_WRITER_FUNCTION(mid);
PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_WRITER_FUNCTION(qos);

static int php_mosquitto_message_write_retain(mosquitto_message_object *mosquitto_object, zval *newval)
{
	mosquitto_object->message.retain = zend_is_true(newval);

	return SUCCESS;
}

static int php_mosquitto_message_write_topic(mosquitto_message_object *mosquitto_object, zval *newval)
{
	zval ztmp;
	if (Z_TYPE_P(newval) != IS_STRING) {
		ztmp = *newval;
		zval_copy_ctor(&ztmp);
		convert_to_string(&ztmp);
		newval = &ztmp;
	}

	if (mosquitto_object->message.topic && mosquitto_object->owned_topic) {
		efree(mosquitto_object->message.topic);
	}

	mosquitto_object->message.topic = estrdup(Z_STRVAL_P(newval));
	mosquitto_object->owned_topic = 1;

	if (newval == &ztmp) {
		zval_dtor(newval);
	}

	return SUCCESS;
}

static int php_mosquitto_message_write_payload(mosquitto_message_object *mosquitto_object, zval *newval)
{
	zval ztmp;
	if (Z_TYPE_P(newval) != IS_STRING) {
		ztmp = *newval;
		zval_copy_ctor(&ztmp);
		convert_to_string(&ztmp);
		newval = &ztmp;
	}

	if (mosquitto_object->message.payload && mosquitto_object->owned_payload) {
		efree(mosquitto_object->message.payload);
		mosquitto_object->message.payloadlen = 0;
	}

	mosquitto_object->message.payload = estrdup(Z_STRVAL_P(newval));
	mosquitto_object->message.payloadlen = Z_STRLEN_P(newval);
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

#define READ_PROPERTY_DC , void **cache_slot, zval *retval
#define READ_PROPERTY_CC , cache_slot, retval
#define WRITE_PROPERTY_DC , void **cache_slot
#define WRITE_PROPERTY_CC , cache_slot
#define HAS_PROPERTY_DC , void **cache_slot
#define HAS_PROPERTY_CC , cache_slot

static php_mosquitto_prop_handler *mosquitto_get_prop_handler(zend_string *prop) {
	zval *ret = zend_hash_find(&php_mosquitto_message_properties, prop); //Z_STR_P(prop)
	if (!ret || Z_TYPE_P(ret) != IS_PTR) {
		return NULL;
	}
	return (php_mosquitto_prop_handler*)Z_PTR_P(ret);
}

zval *php_mosquitto_message_read_property(zend_object *object, zend_string *member, int type READ_PROPERTY_DC) {
	mosquitto_message_object *message_object = mosquitto_message_object_from_zend_object(object); //Z_OBJ_P(object)
	php_mosquitto_prop_handler *hnd;

	hnd = mosquitto_get_prop_handler(member);

	if (hnd && hnd->read_func) {
		if (FAILURE == hnd->read_func(message_object, retval)) {
			ZVAL_NULL(retval);
		}
	} else {
		const zend_object_handlers *std_hnd = zend_get_std_object_handlers();
		retval = std_hnd->read_property(object, member, type READ_PROPERTY_CC);
	}

	return(retval);
}

zval *php_mosquitto_message_write_property(zend_object *object, zend_string *member, zval *value WRITE_PROPERTY_DC)
{
	mosquitto_message_object *obj = mosquitto_message_object_from_zend_object(object); // Z_OBJ_P(object)
	php_mosquitto_prop_handler *hnd;

	hnd = mosquitto_get_prop_handler(member);

	if (hnd && hnd->write_func) {
		hnd->write_func(obj, value);
		if (Z_REFCOUNTED_P(value)) {
			Z_ADDREF_P(value);
			zval_ptr_dtor(value);
		}
	} else {
		const zend_object_handlers *std_hnd = zend_get_std_object_handlers();
		std_hnd->write_property(object, member, value WRITE_PROPERTY_CC);
	}

	return (value);
}

static int php_mosquitto_message_has_property(zend_object *object, zend_string *member, int has_set_exists HAS_PROPERTY_DC)
{
	php_mosquitto_prop_handler *hnd = mosquitto_get_prop_handler(member);
	int ret = 0;
	zval rv;
	zval *retval = &rv;

	if (hnd) {
		switch (has_set_exists) {
			case 2:
				ret = 1;
				break;
			case 0: {
				zval *value = php_mosquitto_message_read_property(object, member, BP_VAR_IS READ_PROPERTY_CC);
				if (Z_REFCOUNTED_P(value)) {
					Z_ADDREF_P(value);
					zval_ptr_dtor(value);
				}
				break;
			}
			default: {
				zval *value = php_mosquitto_message_read_property(object, member, BP_VAR_IS READ_PROPERTY_CC);
				if (Z_REFCOUNTED_P(value)) {
					Z_ADDREF_P(value);
					zval_ptr_dtor(value);
				}
				break;
			}
		}
	} else {
		const zend_object_handlers *std_hnd = zend_get_std_object_handlers();
		ret = std_hnd->has_property(object, member, has_set_exists HAS_PROPERTY_CC);
	}
	return ret;
}

static HashTable *php_mosquitto_message_get_properties(zend_object *object)
{
	mosquitto_message_object *obj = mosquitto_message_object_from_zend_object(object);
	php_mosquitto_prop_handler *hnd;
	HashTable *props;
	zend_string *key;
	zend_long num_key;

	props = zend_std_get_properties(object);

	ZEND_HASH_FOREACH_KEY_PTR(&php_mosquitto_message_properties, num_key, key, hnd) {
		zval val;
		if (!hnd->read_func || (hnd->read_func(obj, &val) != SUCCESS)) {
			ZVAL_NULL(&val);
		}
		if (key) {
			zend_hash_update(props, key, &val);
		} else {
			zend_hash_index_update(props, num_key, &val);
		}
	} ZEND_HASH_FOREACH_END();

	return obj->std.properties;
}


void php_mosquitto_message_add_property(HashTable *h, const char *name, size_t name_length, php_mosquitto_read_t read_func, php_mosquitto_write_t write_func)
{
	php_mosquitto_prop_handler *p = (php_mosquitto_prop_handler*)pemalloc(sizeof(php_mosquitto_prop_handler), 1);

	p->name = (char*) name;
	p->name_length = name_length;
	p->read_func = read_func;
	p->write_func = write_func;
	{
		zend_string *key = zend_string_init(name, name_length, 1);
		zval hnd;
		ZVAL_PTR(&hnd, p);
		zend_hash_add(h, key, &hnd);
	}
}

static void mosquitto_message_object_destroy(zend_object *object)
{
	mosquitto_message_object *message = mosquitto_message_object_from_zend_object(object);
	zend_object_std_dtor(object);
	if (message->owned_topic == 1) {
		efree(message->message.topic);
	}

	if (message->owned_payload == 1) {
		efree(message->message.payload);
	}

}

static zend_object *mosquitto_message_object_new(zend_class_entry *ce) {
	mosquitto_message_object *msg = ecalloc(1, sizeof(mosquitto_message_object) + zend_object_properties_size(ce));
	zend_object *ret = mosquitto_message_object_to_zend_object(msg);


	zend_object_std_init(ret, ce);
	ret->handlers = &mosquitto_message_object_handlers;

	return ret;
}

const zend_function_entry mosquitto_message_methods[] = {
	PHP_ME(Mosquitto_Message, __construct, Mosquitto_Message_void_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Mosquitto_Message, topicMatchesSub, Mosquitto_Message_topicMatchesSub_args, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Mosquitto_Message, tokeniseTopic, Mosquitto_Message_tokeniseTopic_args, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(mosquitto_message)
{
	zend_class_entry message_ce;
	memcpy(&mosquitto_message_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	mosquitto_message_object_handlers.read_property = php_mosquitto_message_read_property;
	mosquitto_message_object_handlers.write_property = php_mosquitto_message_write_property;
	mosquitto_message_object_handlers.has_property = php_mosquitto_message_has_property;
	mosquitto_message_object_handlers.get_properties = php_mosquitto_message_get_properties;
	mosquitto_message_object_handlers.offset    = XtOffsetOf(mosquitto_message_object, std);
	mosquitto_message_object_handlers.free_obj  = mosquitto_message_object_destroy;

	INIT_NS_CLASS_ENTRY(message_ce, "Mosquitto", "Message", mosquitto_message_methods);
	mosquitto_ce_message = zend_register_internal_class(&message_ce);
	mosquitto_ce_message->create_object = mosquitto_message_object_new;

	zend_hash_init(&php_mosquitto_message_properties, 0, NULL, NULL, 1);
	PHP_MOSQUITTO_ADD_PROPERTIES(&php_mosquitto_message_properties, php_mosquitto_message_property_entries);

	return SUCCESS;
}	
