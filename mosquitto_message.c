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

#ifdef ZEND_ENGINE_3
typedef size_t mosquitto_strlen_type;
#else
# ifndef Z_OBJ_P
#  define Z_OBJ_P(pzv) ((zend_object*)zend_object_store_get_object(pzv TSRMLS_CC))
# endif
typedef int mosquitto_strlen_type;
#endif

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
	mosquitto_strlen_type topic_len, subscription_len;
	zend_bool result;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &topic, &topic_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_sub_topic_tokenise(topic, &topics, (int*)&count);

	if (retval == MOSQ_ERR_NOMEM) {
		zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to tokenise topic");
		return;
	}

	array_init(return_value);
	for (i = 0; i < count; i++) {
		if (topics[i] == NULL) {
			add_next_index_null(return_value);
		} else {
#ifdef ZEND_ENGINE_3
			add_next_index_string(return_value, topics[i]);
#else
			add_next_index_string(return_value, topics[i], 1);
#endif
		}
	}

	mosquitto_sub_topic_tokens_free(&topics, count);
}
/* }}} */

PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_READER_FUNCTION(mid);
PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_READER_FUNCTION(qos);

#ifdef ZEND_ENGINE_3
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
#else
static int php_mosquitto_message_read_retain(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC)
{
	MAKE_STD_ZVAL(*retval);
	ZVAL_BOOL(*retval, mosquitto_object->message.retain);
	return SUCCESS;
}

static int php_mosquitto_message_read_topic(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC)
{
	MAKE_STD_ZVAL(*retval);

	if (mosquitto_object->message.topic != NULL) {
		ZVAL_STRING(*retval, mosquitto_object->message.topic, 1);
	} else {
		ZVAL_NULL(*retval);
	}

	return SUCCESS;
}

static int php_mosquitto_message_read_payload(mosquitto_message_object *mosquitto_object, zval **retval TSRMLS_DC)
{
	MAKE_STD_ZVAL(*retval);
	ZVAL_STRINGL(*retval, mosquitto_object->message.payload, mosquitto_object->message.payloadlen, 1);
	return SUCCESS;
}
#endif

PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_WRITER_FUNCTION(mid);
PHP_MOSQUITTO_MESSAGE_LONG_PROPERTY_WRITER_FUNCTION(qos);

static int php_mosquitto_message_write_retain(mosquitto_message_object *mosquitto_object, zval *newval TSRMLS_DC)
{
	mosquitto_object->message.retain = zend_is_true(newval);

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

static int php_mosquitto_message_write_payload(mosquitto_message_object *mosquitto_object, zval *newval TSRMLS_DC)
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

#ifdef ZEND_ENGINE_3
# define READ_PROPERTY_DC , void **cache_slot, zval *retval
# define READ_PROPERTY_CC , cache_slot, retval
# define WRITE_PROPERTY_DC , void **cache_slot
# define WRITE_PROPERTY_CC , cache_slot
# define HAS_PROPERTY_DC , void **cache_slot
# define HAS_PROPERTY_CC , cache_slot

static php_mosquitto_prop_handler *mosquitto_get_prop_handler(zval *prop) {
	zval *ret = zend_hash_find(&php_mosquitto_message_properties, Z_STR_P(prop));
	if (!ret || Z_TYPE_P(ret) != IS_PTR) {
		return NULL;
	}
	return (php_mosquitto_prop_handler*)Z_PTR_P(ret);
}
#else
# define READ_PROPERTY_DC ZEND_LITERAL_KEY_DC TSRMLS_DC
# define READ_PROPERTY_CC ZEND_LITERAL_KEY_CC TSRMLS_CC
# define WRITE_PROPERTY_DC ZEND_LITERAL_KEY_DC TSRMLS_DC
# define WRITE_PROPERTY_CC ZEND_LITERAL_KEY_CC TSRMLS_CC
# define HAS_PROPERTY_DC ZEND_LITERAL_KEY_DC TSRMLS_DC
# define HAS_PROPERTY_CC ZEND_LITERAL_KEY_CC TSRMLS_CC

static php_mosquitto_prop_handler *mosquitto_get_prop_handler(zval *prop) {
	php_mosquitto_prop_handler *hnd;
	if (FAILURE == zend_hash_find(&php_mosquitto_message_properties, Z_STRVAL_P(prop), Z_STRLEN_P(prop)+1, (void**) &hnd)) {
		return NULL;
	}
	return hnd;
}
#endif

zval *php_mosquitto_message_read_property(zval *object, zval *member, int type READ_PROPERTY_DC) {
	zval tmp_member;
#ifndef ZEND_ENGINE_3
	zval *retval;
#endif
	mosquitto_message_object *message_object = mosquitto_message_object_from_zend_object(Z_OBJ_P(object));
	php_mosquitto_prop_handler *hnd;

	if (Z_TYPE_P(member) != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}
	hnd = mosquitto_get_prop_handler(member);

	if (hnd && hnd->read_func) {
#ifdef ZEND_ENGINE_3
		if (FAILURE == hnd->read_func(message_object, retval)) {
			ZVAL_NULL(retval);
		}
#else
		if (SUCCESS == hnd->read_func(message_object, &retval TSRMLS_CC)) {
			/* ensure we're creating a temporary variable */
			Z_SET_REFCOUNT_P(retval, 0);
		} else {
			retval = EG(uninitialized_zval_ptr);
		}
#endif
	} else {
		zend_object_handlers * std_hnd = zend_get_std_object_handlers();
		retval = std_hnd->read_property(object, member, type READ_PROPERTY_CC);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}

	return(retval);
}

void php_mosquitto_message_write_property(zval *object, zval *member, zval *value WRITE_PROPERTY_DC)
{
	zval tmp_member;
	mosquitto_message_object *obj = mosquitto_message_object_from_zend_object(Z_OBJ_P(object));
	php_mosquitto_prop_handler *hnd;

	if (Z_TYPE_P(member) != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	hnd = mosquitto_get_prop_handler(member);

	if (hnd && hnd->write_func) {
		hnd->write_func(obj, value TSRMLS_CC);
#ifdef ZEND_ENGINE_3
		if (Z_REFCOUNTED_P(value)) {
			Z_ADDREF_P(value);
			zval_ptr_dtor(value);
		}
#else
		if (! PZVAL_IS_REF(value) && Z_REFCOUNT_P(value) == 0) {
			Z_ADDREF_P(value);
			zval_ptr_dtor(&value);
		}
#endif
	} else {
		zend_object_handlers * std_hnd = zend_get_std_object_handlers();
		std_hnd->write_property(object, member, value WRITE_PROPERTY_CC);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
}

static int php_mosquitto_message_has_property(zval *object, zval *member, int has_set_exists HAS_PROPERTY_DC)
{
	php_mosquitto_prop_handler *hnd = mosquitto_get_prop_handler(member);
	int ret = 0;
#ifdef ZEND_ENGINE_3
	zval rv;
	zval *retval = &rv;
#endif

	if (hnd) {
		switch (has_set_exists) {
			case 2:
				ret = 1;
				break;
			case 0: {
				zval *value = php_mosquitto_message_read_property(object, member, BP_VAR_IS READ_PROPERTY_CC);
#ifdef ZEND_ENGINE_3
				if (Z_REFCOUNTED_P(value)) {
					Z_ADDREF_P(value);
					zval_ptr_dtor(value);
				}
#else
				if (value != EG(uninitialized_zval_ptr)) {
					ret = Z_TYPE_P(value) != IS_NULL? 1:0;
					/* refcount is 0 */
					Z_ADDREF_P(value);
					zval_ptr_dtor(&value);
				}
#endif
				break;
			}
			default: {
				zval *value = php_mosquitto_message_read_property(object, member, BP_VAR_IS READ_PROPERTY_CC);
#ifdef ZEND_ENGINE_3
				if (Z_REFCOUNTED_P(value)) {
					Z_ADDREF_P(value);
					zval_ptr_dtor(value);
				}
#else
				if (value != EG(uninitialized_zval_ptr)) {
					convert_to_boolean(value);
					ret = Z_BVAL_P(value)? 1:0;
					/* refcount is 0 */
					Z_ADDREF_P(value);
					zval_ptr_dtor(&value);
				}
#endif
				break;
			}
		}
	} else {
		zend_object_handlers * std_hnd = zend_get_std_object_handlers();
		ret = std_hnd->has_property(object, member, has_set_exists HAS_PROPERTY_CC);
	}
	return ret;
}

#ifndef ZEND_ENGINE_3
# ifndef ZEND_HASH_FOREACH_PTR
#  define ZEND_HASH_FOREACH_KEY_PTR(ht, idx, key, ptr) \
   { \
     HashPosition pos; \
     for (zend_hash_internal_pointer_reset_ex(ht, &pos); \
          zend_hash_get_current_data_ex(ht, (void**)&ptr, &pos) == SUCCESS; \
          zend_hash_move_forward_ex(ht, &pos)) { \
       key = NULL; \
       zend_hash_get_current_key_ex(ht, &key, &key##_len, &idx, 0, &pos); \
       {
# endif
# ifndef ZEND_HASH_FOREACH_END
#  define ZEND_HASH_FOREACH_END() \
       } \
     } \
   }
# endif
#endif

static HashTable *php_mosquitto_message_get_properties(zval *object TSRMLS_DC)
{
	mosquitto_message_object *obj = mosquitto_message_object_from_zend_object(Z_OBJ_P(object));
	php_mosquitto_prop_handler *hnd;
	HashTable *props;
#ifdef ZEND_ENGINE_3
	zend_string *key;
	zend_long num_key;
#else
	char *key;
	uint key_len;
	ulong num_key;
#endif

	props = zend_std_get_properties(object TSRMLS_CC);

	ZEND_HASH_FOREACH_KEY_PTR(&php_mosquitto_message_properties, num_key, key, hnd) {
#ifdef ZEND_ENGINE_3
		zval val;
		if (!hnd->read_func || (hnd->read_func(obj, &val) != SUCCESS)) {
			ZVAL_NULL(&val);
		}
		if (key) {
			zend_hash_update(props, key, &val);
		} else {
			zend_hash_index_update(props, num_key, &val);
		}
#else
		zval *val;
		if (!hnd->read_func || hnd->read_func(obj, &val TSRMLS_CC) != SUCCESS) {
			val = EG(uninitialized_zval_ptr);
			Z_ADDREF_P(val);
		}
		if (key) {
			zend_hash_update(props, key, key_len, (void *)&val, sizeof(zval*), NULL);
        } else {
			zend_hash_index_update(props, num_key, (void *)&val, sizeof(zval*), NULL);
        }
#endif
	} ZEND_HASH_FOREACH_END();

	return obj->std.properties;
}


void php_mosquitto_message_add_property(HashTable *h, const char *name, size_t name_length, php_mosquitto_read_t read_func, php_mosquitto_write_t write_func TSRMLS_DC)
{
#ifdef ZEND_ENGINE_3
	php_mosquitto_prop_handler *p = (php_mosquitto_prop_handler*)pemalloc(sizeof(php_mosquitto_prop_handler), 1);
#else
	php_mosquitto_prop_handler val, *p = &val;
#endif

	p->name = (char*) name;
	p->name_length = name_length;
	p->read_func = read_func;
	p->write_func = write_func;
#ifdef ZEND_ENGINE_3
	{
		zend_string *key = zend_string_init(name, name_length, 1);
		zval hnd;
		ZVAL_PTR(&hnd, p);
		zend_hash_add(h, key, &hnd);
	}
#else
	zend_hash_add(h, (char *)name, name_length + 1, p, sizeof(php_mosquitto_prop_handler), NULL);
#endif
}

static void mosquitto_message_object_destroy(zend_object *object TSRMLS_DC)
{
	mosquitto_message_object *message = mosquitto_message_object_from_zend_object(object);
#ifdef ZEND_ENGINE_3
        zend_object_std_dtor(object);
#else
	zend_hash_destroy(message->std.properties);
	FREE_HASHTABLE(message->std.properties);
#endif
	if (message->owned_topic == 1) {
		efree(message->message.topic);
	}

	if (message->owned_payload == 1) {
		efree(message->message.payload);
	}

#ifndef ZEND_ENGINE_3
	efree(object);
#endif
}

#ifdef ZEND_ENGINE_3
static zend_object *mosquitto_message_object_new(zend_class_entry *ce) {
	mosquitto_message_object *msg = ecalloc(1, sizeof(mosquitto_message_object) + zend_object_properties_size(ce));
	zend_object *ret = mosquitto_message_object_to_zend_object(msg);

#ifdef MOSQUITTO_NEED_TSRMLS
	message_obj->TSRMLS_C = TSRMLS_C;
#endif

	zend_object_std_init(ret, ce);
	ret->handlers = &mosquitto_message_object_handlers;

	return ret;
}
#else
static zend_object_value mosquitto_message_object_new(zend_class_entry *ce TSRMLS_DC) {

	zend_object_value retval;
	mosquitto_message_object *message_obj;
#if PHP_VERSION_ID < 50399
	zval *temp;
#endif

	message_obj = ecalloc(1, sizeof(mosquitto_message_object));
	message_obj->std.ce = ce;

#ifdef MOSQUITTO_NEED_TSRMLS
	message_obj->TSRMLS_C = TSRMLS_C;
#endif

	ALLOC_HASHTABLE(message_obj->std.properties);
	zend_hash_init(message_obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(message_obj->std.properties, &mosquitto_ce_message->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &temp, sizeof(zval *));
#else
	object_properties_init(&message_obj->std, mosquitto_ce_message);
#endif
	retval.handle = zend_objects_store_put(message_obj, NULL, (zend_objects_free_object_storage_t) mosquitto_message_object_destroy, NULL TSRMLS_CC);
	retval.handlers = &mosquitto_message_object_handlers;
	return retval;
}
#endif

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
#ifdef ZEND_ENGINE_3
    mosquitto_message_object_handlers.offset    = XtOffsetOf(mosquitto_message_object, std);
    mosquitto_message_object_handlers.free_obj  = mosquitto_message_object_destroy;
#endif

	INIT_NS_CLASS_ENTRY(message_ce, "Mosquitto", "Message", mosquitto_message_methods);
	mosquitto_ce_message = zend_register_internal_class(&message_ce TSRMLS_CC);
	mosquitto_ce_message->create_object = mosquitto_message_object_new;

	zend_hash_init(&php_mosquitto_message_properties, 0, NULL, NULL, 1);
	PHP_MOSQUITTO_ADD_PROPERTIES(&php_mosquitto_message_properties, php_mosquitto_message_property_entries);

	return SUCCESS;
}	
