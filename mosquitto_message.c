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

/* {{{ Arginfo */

ZEND_BEGIN_ARG_INFO(Mosquitto_Message_topicMatchesSub_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, topic)
	ZEND_ARG_INFO(0, subscription)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Message_tokeniseTopic_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, topic)
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
	int topic_len, subscription_len;
	zend_bool result;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s!s!",
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
	int topic_len = 0, retval = 0, count = 0, i = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &topic, &topic_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_sub_topic_tokenise(topic, &topics, &count);

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
		ZVAL_STRINGL(retval, mosquitto_object->message.topic, strlen(mosquitto_object->message.topic));
	} else {
		ZVAL_NULL(retval);
	}

	return SUCCESS;
}

static int php_mosquitto_message_read_payload(mosquitto_message_object *mosquitto_object, zval *retval)
{
	if (mosquitto_object->message.payload != NULL) {
		ZVAL_STRINGL(retval, mosquitto_object->message.payload, mosquitto_object->message.payloadlen);
	} else {
		ZVAL_NULL(retval);
	}
	return SUCCESS;
}

PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_WRITER_FUNCTION(mid);
PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_WRITER_FUNCTION(qos);

static int php_mosquitto_message_write_retain(mosquitto_message_object *mosquitto_object, zval *newval)
{
	zval ztmp;
	if (Z_TYPE_P(newval) != IS_TRUE && Z_TYPE_P(newval) != IS_FALSE) {
		ztmp = *newval;
		zval_copy_ctor(&ztmp);
		convert_to_boolean(&ztmp);
		newval = &ztmp;
	}

	mosquitto_object->message.retain = Z_LVAL_P(newval);

	if (newval == &ztmp) {
		zval_dtor(newval);
	}

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

zval *php_mosquitto_message_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv)
{
	zval tmp_member;
	zval retval;
	mosquitto_message_object *message_object;
	php_mosquitto_prop_handler *hnd;
	int ret;

	message_object = (mosquitto_message_object *) php_mosquitto_message_fetch_object(Z_OBJ_P(object));

	if (Z_TYPE_P(member) != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

    hnd = zend_hash_find_ptr(&php_mosquitto_message_properties, Z_STR_P(member));

	if (hnd != NULL && hnd->read_func) {
		ret = hnd->read_func(message_object, rv);
		if (ret == SUCCESS) {
			/* ensure we're creating a temporary variable */
			Z_TRY_ADDREF_P(rv);
		} else {
			ZVAL_UNDEF_P(rv);
		}
	} else {
		zend_object_handlers * std_hnd = zend_get_std_object_handlers();
		rv = std_hnd->read_property(object, member, type, NULL, NULL);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}

	return(rv);
}

void php_mosquitto_message_write_property(zval *object, zval *member, zval *value, void **cache_slot)
{
	zval tmp_member;
	mosquitto_message_object *obj;
	php_mosquitto_prop_handler *hnd;

	if (Z_TYPE_P(member) != IS_STRING) {
        ZVAL_STR(&tmp_member, zval_get_string(member));
        member = &tmp_member;
	}

    obj = php_mosquitto_message_fetch_object(Z_OBJ_P(object));
    hnd = zend_hash_find_ptr(&php_mosquitto_message_properties, Z_STR_P(member));

	if (hnd != NULL && hnd->write_func) {
		hnd->write_func(obj, value);
		if (!Z_ISREF_P(value) && Z_REFCOUNT_P(value) == 0) {
			Z_TRY_ADDREF_P(value);
		}
	} else {
		zend_object_handlers * std_hnd = zend_get_std_object_handlers();
		std_hnd->write_property(object, member, value, NULL);
	}
}

static int php_mosquitto_message_has_property(zval *object, zval *member, int has_set_exists, void **cache_slot)
{
	php_mosquitto_prop_handler *hnd;
    int ret;

    hnd = zend_hash_find_ptr(&php_mosquitto_message_properties, Z_STR_P(member));

	if (hnd != NULL) {
		switch (has_set_exists) {
			case 2:
				ret = 1;
				break;
			case 0: {
				zval *value = php_mosquitto_message_read_property(object, member, 0, NULL, NULL);
				if (!Z_ISUNDEF_P(value)) {
					ret = Z_TYPE_P(value) != IS_NULL ? 1 : 0;
					/* refcount is 0 */
//					Z_TRY_ADDREF_P(value);
//					zval_ptr_dtor(&value);
				}
				break;
			}
			default: {
				zval *value = php_mosquitto_message_read_property(object, member, 0, NULL, NULL);
				if (!Z_ISUNDEF_P(value)) {
					convert_to_boolean(value);
					ret = value ? 1 : 0;
					/* refcount is 0 */
//					Z_ADDREF_P(value);
//					zval_ptr_dtor(&value);
				}
				break;
			}
		}
	} else {
		zend_object_handlers * std_hnd = zend_get_std_object_handlers();
		ret = std_hnd->has_property(object, member, has_set_exists, NULL);
	}
	return ret;
}

static HashTable *php_mosquitto_message_get_properties(zval *object)
{
	mosquitto_message_object *obj;
	php_mosquitto_prop_handler *hnd;
	HashTable *props;
	zval val;
	zend_string *key;
	uint key_len;
	HashPosition pos;
	ulong num_key;

	obj = php_mosquitto_message_fetch_object(Z_OBJ_P(object));
	props = zend_std_get_properties(object);

	ZEND_HASH_FOREACH_STR_KEY_PTR(&php_mosquitto_message_properties, key, hnd) {
		if (!hnd->read_func || hnd->read_func(obj, &val) != SUCCESS) {
			ZVAL_UNDEF(&val);
		}
		zend_hash_update(props, key, &val);
		zval_ptr_dtor(&val);
	} ZEND_HASH_FOREACH_END();

	return obj->std.properties;
}


void php_mosquitto_message_add_property(HashTable *h, const char *name, size_t name_length, php_mosquitto_read_t read_func, php_mosquitto_write_t write_func)
{
	php_mosquitto_prop_handler p;

	p.name = (char*) name;
	p.name_length = name_length;
	p.read_func = (read_func) ? read_func : NULL;
	p.write_func = (write_func) ? write_func : NULL;
    zend_hash_str_add_ptr(h, name, name_length + 1, &p);
}

static void mosquitto_message_object_destroy(zend_object *object)
{
	mosquitto_message_object *message = (mosquitto_message_object *) object;

	if (message->owned_topic == 1) {
		efree(message->message.topic);
	}

	if (message->owned_payload == 1) {
		efree(message->message.payload);
	}
	
	zend_object_std_dtor(&message->std);
	//Z_TRY_DELREF(message->std);
	efree(message);
}

static zend_object *mosquitto_message_object_new(zend_class_entry *ce) {
	mosquitto_message_object *object = ecalloc(1, sizeof(mosquitto_message_object) + zend_object_properties_size(ce));
	zend_object_std_init(&object->std, ce);
	object->std.handlers = &mosquitto_message_object_handlers;
	
    return &object->std;
}

/* {{{ */
mosquitto_message_object *php_mosquitto_message_fetch_object(zend_object *obj) {
	return (mosquitto_message_object *)((char*)(obj) - XtOffsetOf(mosquitto_message_object, std));
}
/* }}} */

const zend_function_entry mosquitto_message_methods[] = {
	PHP_ME(Mosquitto_Message, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
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
	//mosquitto_message_object_handlers.free_obj = mosquitto_message_object_destroy;
	mosquitto_message_object_handlers.offset = XtOffsetOf(mosquitto_message_object, std);

	INIT_NS_CLASS_ENTRY(message_ce, "Mosquitto", "Message", mosquitto_message_methods);
	mosquitto_ce_message = zend_register_internal_class(&message_ce);
	mosquitto_ce_message->create_object = mosquitto_message_object_new;

	zend_hash_init(&php_mosquitto_message_properties, 0, NULL, NULL, 1);
	PHP_MOSQUITTO_ADD_PROPERTIES(&php_mosquitto_message_properties, php_mosquitto_message_property_entries);

	return SUCCESS;
}	
