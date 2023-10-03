#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_variables.h"
#include "zend_exceptions.h"
#include "zend_API.h"
#include "ext/standard/php_filestat.h"
#include "ext/standard/info.h"
#include "php_mosquitto.h"

zend_class_entry *mosquitto_ce_client;
zend_class_entry *mosquitto_ce_exception;
zend_object_handlers mosquitto_std_object_handlers;

ZEND_DECLARE_MODULE_GLOBALS(mosquitto)

#ifndef Z_BVAL
# define Z_BVAL(zv) (Z_TYPE(zv) == IS_TRUE)
# define Z_BVAL_P(pzv) Z_BVAL(*pzv)
#endif
#define ZO_HANDLE_DC
typedef size_t mosquitto_strlen_type;

static inline mosquitto_client_object *mosquitto_client_object_get(zval *zobj) {
	// TODO: ZEND_ASSERT()s
	mosquitto_client_object *obj = mosquitto_client_object_from_zend_object(Z_OBJ_P(zobj));
	if (!obj->client) {
		php_error(E_ERROR, "Internal surface object missing in %s wrapper, "
		                   "you must call parent::__construct in extended classes", (char *) Z_OBJCE_P(zobj)->name);
	}
	return obj;
}

static inline void mosquitto_callback_addref(zend_fcall_info *finfo) {
	zval tmp;
	Z_TRY_ADDREF(finfo->function_name);
	if (finfo->object) {
		ZVAL_OBJ(&tmp, finfo->object);
		Z_TRY_ADDREF(tmp);
	}
}

static inline const char *mosquitto_finfo_name(zend_fcall_info *info) {
	return Z_STRVAL(info->function_name);
}

static int php_mosquitto_pw_callback(char *buf, int size, int rwflag, void *userdata);

/* {{{ Arginfo */

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client___construct_args, 0, 0, 0)
	ZEND_ARG_TYPE_INFO(0, id, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO(0, cleanSession, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_onConnect_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, onConnect, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_onDisconnect_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, onDisconnect, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_onLog_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, onLog, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_onSubscribe_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, onSubscribe, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_onUnsubscribe_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, onUnsubscribe, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_onMessage_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, onMessage, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_onPublish_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, onPublish, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Mosquitto_Client_getSocket_args, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Mosquitto_Client_setTlsCertificates_args, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, ca_path, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO(0, cert_path, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO(0, key_path, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO(0, key_pw, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_setTlsInsecure_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Mosquitto_Client_setTlsOptions_args, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, verify_peer, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, tls_version, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO(0, ciphers, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Mosquitto_Client_setTlsPSK_args, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, psk, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO(0, identity, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO(0, ciphers, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_setCredentials_args, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, username, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_setWill_args, 0, 0, 4)
	ZEND_ARG_TYPE_INFO(0, topic, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, payload, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, qos, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, retain, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_setReconnectDelay_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, reconnectDelay, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxReconnectDelay, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, exponentialBackoff, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Mosquitto_Client_connect_args, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, port, IS_LONG, 0, "1883")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, keepalive, IS_LONG, 0, "60")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, interface, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_setMaxInFlightMessages_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, max, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_setMessageRetry_args, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, messageRetry, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Mosquitto_Client_publish_args, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, topic, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, payload, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, qos, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, retain, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Mosquitto_Client_subscribe_args, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, topic, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, qos, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Mosquitto_Client_unsubscribe_args, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, topic, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_loop_args, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, timeout, IS_LONG, 0, "1000")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxPackets, IS_LONG, 0, "1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_loopForever_args, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, timeout, IS_LONG, 0, "1000")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxPackets, IS_LONG, 0, "1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(Mosquitto_Client_void_args, 0, 0, 0)
ZEND_END_ARG_INFO()

/* }}} */

/* {{{ Mosquitto\Client::__construct() */
PHP_METHOD(Mosquitto_Client, __construct)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	char *id = NULL;
	mosquitto_strlen_type id_len = 0;
	zend_bool clean_session = 1;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s!b", &id, &id_len, &clean_session) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object->client = mosquitto_new(id, clean_session, object);
	if (!object->client) {
		char *message = php_mosquitto_strerror_wrapper(errno);
		zend_throw_exception(mosquitto_ce_exception, message, 1);
		if (message) {
			efree(message);
		}
	}
}
/* }}} */

/* {{{ Mosquitto\Client::setTlsCertificates() */
PHP_METHOD(Mosquitto_Client, setTlsCertificates)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
#if ZEND_MODULE_API_NO >= 20210902
	zend_string *ca_path = NULL;
#else
	char *ca_path = NULL;
	mosquitto_strlen_type ca_path_len = 0;
#endif
	char *cert_path = NULL, *key_path = NULL, *key_pw = NULL;
	mosquitto_strlen_type cert_path_len = 0, key_path_len = 0, key_pw_len;
	int retval = 0;
	zval stat;
	zend_bool is_dir = 0;
	int (*pw_callback)(char *, int, int, void *) = NULL;

	PHP_MOSQUITTO_ERROR_HANDLING();
#if ZEND_MODULE_API_NO >= 20210902
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S!|s!s!s!",
				&ca_path,
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s!|s!s!s!",
				&ca_path, &ca_path_len,
#endif
				&cert_path, &cert_path_len,
				&key_path, &key_path_len,
				&key_pw, &key_pw_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}

#if ZEND_MODULE_API_NO >= 20210902
	if ((php_check_open_basedir(ZSTR_VAL(ca_path)) < 0) ||
#else
	if ((php_check_open_basedir(ca_path) < 0) ||
#endif
		(php_check_open_basedir(cert_path) < 0) ||
		(php_check_open_basedir(key_path) < 0))
	{
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}

	PHP_MOSQUITTO_RESTORE_ERRORS();

#if ZEND_MODULE_API_NO >= 20210902
	php_stat(ca_path, FS_IS_DIR, &stat);
#else
	php_stat(ca_path, ca_path_len, FS_IS_DIR, &stat);
#endif
	is_dir = Z_BVAL(stat);

	if (key_pw != NULL) {
		pw_callback = php_mosquitto_pw_callback;
		MQTTG(client_key) = estrdup(key_pw);
		MQTTG(client_key_len) = key_pw_len;
	}

	if (is_dir) {
#if ZEND_MODULE_API_NO >= 20210902
		retval = mosquitto_tls_set(object->client, NULL, ZSTR_VAL(ca_path), cert_path, key_path, pw_callback);
	} else {
		retval = mosquitto_tls_set(object->client, ZSTR_VAL(ca_path), NULL, cert_path, key_path, pw_callback);
#else
		retval = mosquitto_tls_set(object->client, NULL, ca_path, cert_path, key_path, pw_callback);
	} else {
		retval = mosquitto_tls_set(object->client, ca_path, NULL, cert_path, key_path, pw_callback);
#endif
	}

	php_mosquitto_handle_errno(retval, errno);
	RETURN_LONG(retval);
}
/* }}} */

/* {{{ Mosquitto\Client::setTlsInsecure() */
PHP_METHOD(Mosquitto_Client, setTlsInsecure)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_bool value = 0;
	int retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "b", &value) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_tls_insecure_set(object->client, value);

	php_mosquitto_handle_errno(retval, errno);
}
/* }}} */

/* {{{ Mosquitto\Client::setTlsOptions() */
PHP_METHOD(Mosquitto_Client, setTlsOptions)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	char *tls_version = NULL, *ciphers = NULL;
	mosquitto_strlen_type tls_version_len = 0, ciphers_len = 0;
	int retval = 0, verify_peer = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|s!s!",
				&verify_peer,
				&tls_version, &tls_version_len,
				&ciphers, &ciphers_len
				) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_tls_opts_set(object->client, verify_peer, tls_version, ciphers);

	php_mosquitto_handle_errno(retval, errno);
	RETURN_LONG(retval);
}
/* }}} */

/* {{{ Mosquitto\Client::setTlsPSK() */
PHP_METHOD(Mosquitto_Client, setTlsPSK)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	char *psk = NULL, *identity = NULL, *ciphers = NULL;
	mosquitto_strlen_type psk_len = 0, identity_len = 0, ciphers_len = 0;
	int retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s!s!|s!",
				&psk, &psk_len, &identity, &identity_len, &ciphers, &ciphers_len
				) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_tls_psk_set(object->client, psk, identity, ciphers);

	php_mosquitto_handle_errno(retval, errno);
	RETURN_LONG(retval);
}
/* }}} */

/* {{{ Mosquitto\Client::setCredentials() */
PHP_METHOD(Mosquitto_Client, setCredentials)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	char *username = NULL, *password = NULL;
	mosquitto_strlen_type username_len, password_len;
	int retval;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &username, &username_len, &password, &password_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_username_pw_set(object->client, username, password);
	php_mosquitto_handle_errno(retval, errno);
}
/* }}} */

/* {{{ Mosquitto\Client::setWill() */
PHP_METHOD(Mosquitto_Client, setWill)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	mosquitto_strlen_type topic_len, payload_len;
	int retval;
	zend_long qos;
	zend_bool retain;
	char *topic, *payload;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sslb",
				&topic, &topic_len, &payload, &payload_len, &qos, &retain) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_will_set(object->client, topic, payload_len, (void *) payload, qos, retain);

	php_mosquitto_handle_errno(retval, errno);
}
/* }}} */

/* {{{ Mosquitto\Client::clearWill() */
PHP_METHOD(Mosquitto_Client, clearWill)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	int retval;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_will_clear(object->client);

	php_mosquitto_handle_errno(retval, errno);
}
/* }}} */

/* {{{ Mosquitto\Client::setReconnectDelay() */
PHP_METHOD(Mosquitto_Client, setReconnectDelay)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	int retval;
	zend_long reconnect_delay = 0, reconnect_delay_max = 0;
	zend_bool exponential_backoff = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|lb",
				&reconnect_delay, &reconnect_delay_max, &exponential_backoff)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_reconnect_delay_set(object->client, reconnect_delay, reconnect_delay_max, exponential_backoff);

	php_mosquitto_handle_errno(retval, errno);
}
/* }}} */

/* {{{ Mosquitto\Client::connect() */
PHP_METHOD(Mosquitto_Client, connect)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	char *host = NULL, *interface = NULL;
	mosquitto_strlen_type host_len, interface_len;
	int retval;
	zend_long port = 1883;
	zend_long keepalive = 60;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|lls!",
				&host, &host_len, &port, &keepalive,
				&interface, &interface_len)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (interface == NULL) {
		retval = mosquitto_connect(object->client, host, port, keepalive);
	} else {
		retval = mosquitto_connect_bind(object->client, host, port, keepalive, interface);
	}

	php_mosquitto_handle_errno(retval, errno);
	RETURN_LONG(retval);
}
/* }}} */

/* {{{ Mosquitto\Client::disconnect() */
PHP_METHOD(Mosquitto_Client, disconnect)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	int retval;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_disconnect(object->client);
	php_mosquitto_exit_loop(object);

	php_mosquitto_handle_errno(retval, errno);
}
/* }}} */

/* {{{ Mosquitto\Client::onConnect() */
PHP_METHOD(Mosquitto_Client, onConnect)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_fcall_info connect_callback = empty_fcall_info;
	zend_fcall_info_cache connect_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "f!",
				&connect_callback, &connect_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(connect_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0);
	}

	PHP_MOSQUITTO_FREE_CALLBACK(object, connect);
	object->connect_callback = connect_callback;
	object->connect_callback_cache = connect_callback_cache;
	mosquitto_callback_addref(&(object->connect_callback));

	mosquitto_connect_callback_set(object->client, php_mosquitto_connect_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onDisconnect() */
PHP_METHOD(Mosquitto_Client, onDisconnect)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_fcall_info disconnect_callback = empty_fcall_info;
	zend_fcall_info_cache disconnect_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "f!",
				&disconnect_callback, &disconnect_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(disconnect_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0);
	}

	PHP_MOSQUITTO_FREE_CALLBACK(object, disconnect);
	object->disconnect_callback = disconnect_callback;
	object->disconnect_callback_cache = disconnect_callback_cache;
	mosquitto_callback_addref(&(object->disconnect_callback));

	mosquitto_disconnect_callback_set(object->client, php_mosquitto_disconnect_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onLog() */
PHP_METHOD(Mosquitto_Client, onLog)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_fcall_info log_callback = empty_fcall_info;
	zend_fcall_info_cache log_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "f!",
				&log_callback, &log_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(log_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0);
	}

	PHP_MOSQUITTO_FREE_CALLBACK(object, log);
	object->log_callback = log_callback;
	object->log_callback_cache = log_callback_cache;
	mosquitto_callback_addref(&(object->log_callback));

	mosquitto_log_callback_set(object->client, php_mosquitto_log_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onSubscribe() */
PHP_METHOD(Mosquitto_Client, onSubscribe)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_fcall_info subscribe_callback = empty_fcall_info;
	zend_fcall_info_cache subscribe_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "f!",
				&subscribe_callback, &subscribe_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(subscribe_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0);
	}

	PHP_MOSQUITTO_FREE_CALLBACK(object, subscribe);
	object->subscribe_callback = subscribe_callback;
	object->subscribe_callback_cache = subscribe_callback_cache;
	mosquitto_callback_addref(&(object->subscribe_callback));

	mosquitto_subscribe_callback_set(object->client, php_mosquitto_subscribe_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onUnsubscribe() */
PHP_METHOD(Mosquitto_Client, onUnsubscribe)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_fcall_info unsubscribe_callback = empty_fcall_info;
	zend_fcall_info_cache unsubscribe_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "f!",
				&unsubscribe_callback, &unsubscribe_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(unsubscribe_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0);
	}

	PHP_MOSQUITTO_FREE_CALLBACK(object, unsubscribe);
	object->unsubscribe_callback = unsubscribe_callback;
	object->unsubscribe_callback_cache = unsubscribe_callback_cache;
	mosquitto_callback_addref(&(object->unsubscribe_callback));

	mosquitto_unsubscribe_callback_set(object->client, php_mosquitto_unsubscribe_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onMessage() */
PHP_METHOD(Mosquitto_Client, onMessage)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_fcall_info message_callback = empty_fcall_info;
	zend_fcall_info_cache message_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "f!",
				&message_callback, &message_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(message_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0);
	}

	PHP_MOSQUITTO_FREE_CALLBACK(object, message);
	object->message_callback = message_callback;
	object->message_callback_cache = message_callback_cache;
	mosquitto_callback_addref(&(object->message_callback));

	mosquitto_message_callback_set(object->client, php_mosquitto_message_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onPublish() */
PHP_METHOD(Mosquitto_Client, onPublish)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_fcall_info publish_callback = empty_fcall_info;
	zend_fcall_info_cache publish_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "f!",
				&publish_callback, &publish_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(publish_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0);
	}

	PHP_MOSQUITTO_FREE_CALLBACK(object, publish);
	object->publish_callback = publish_callback;
	object->publish_callback_cache = publish_callback_cache;
	mosquitto_callback_addref(&(object->publish_callback));

	mosquitto_publish_callback_set(object->client, php_mosquitto_publish_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::getSocket() */
PHP_METHOD(Mosquitto_Client, getSocket)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none()  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	RETURN_LONG(mosquitto_socket(object->client));
}
/* }}} */

/* {{{ Mosquitto\Client::setMaxInFlightMessages() */
PHP_METHOD(Mosquitto_Client, setMaxInFlightMessages)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	int retval;
	zend_long max = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &max)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_max_inflight_messages_set(object->client, max);

	php_mosquitto_handle_errno(retval, errno);
}
/* }}} */

/* {{{ Mosquitto\Client::setMessageRetry() */
PHP_METHOD(Mosquitto_Client, setMessageRetry)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_long retry = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &retry)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	mosquitto_message_retry_set(object->client, retry);
}
/* }}} */

/* {{{ Mosquitto\Client::publish() */
PHP_METHOD(Mosquitto_Client, publish)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	int mid, retval;
	mosquitto_strlen_type topic_len, payload_len;
	zend_long qos = 0;
	zend_bool retain = 0;
	char *topic, *payload;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss!|lb",
				&topic, &topic_len, &payload, &payload_len, &qos, &retain) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_publish(object->client, &mid, topic, payload_len, (void *) payload, qos, retain);

	php_mosquitto_handle_errno(retval, errno);

	RETURN_LONG(mid);
}
/* }}} */

/* {{{ Mosquitto\Client::subscribe() */
PHP_METHOD(Mosquitto_Client, subscribe)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	char *sub;
	mosquitto_strlen_type sub_len;
	int retval, mid;
	zend_long qos;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sl",
				&sub, &sub_len, &qos) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_subscribe(object->client, &mid, sub, qos);

	php_mosquitto_handle_errno(retval, errno);

	RETURN_LONG(mid);
}
/* }}} */

/* {{{ Mosquitto\Client::unsubscribe() */
PHP_METHOD(Mosquitto_Client, unsubscribe)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	char *sub;
	mosquitto_strlen_type sub_len;
	int retval, mid;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s",
				&sub, &sub_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_unsubscribe(object->client, &mid, sub);

	php_mosquitto_handle_errno(retval, errno);

	RETURN_LONG(mid);
}
/* }}} */

/* {{{ Mosquitto\Client::loop() */
PHP_METHOD(Mosquitto_Client, loop)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_long timeout = 1000, max_packets = 1;
	long retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|ll",
				&timeout, &max_packets) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_loop(object->client, timeout, max_packets);
	php_mosquitto_handle_errno(retval, errno);
}
/* }}} */

/* {{{ Mosquitto\Client::loopForever() */
PHP_METHOD(Mosquitto_Client, loopForever)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_long timeout = 1000, max_packets = 1;
	long retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|ll",
				&timeout, &max_packets) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object->looping = 1;

	while (object->looping) {
		retval = mosquitto_loop(object->client, timeout, max_packets);
		php_mosquitto_handle_errno(retval, errno);

		if (EG(exception)) {
			break;
		}
	}
}
/* }}} */

/* {{{ Mosquitto\Client::exitLoop() */
PHP_METHOD(Mosquitto_Client, exitLoop)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	php_mosquitto_exit_loop(object);
}
/* }}} */

/* Internal functions */

#if defined(PHP_WIN32)
static int strerror_r(int errnum, char *buf, size_t buf_len)
{
	return strerror_s(buf, buf_len, errnum);
}
#endif

PHP_MOSQUITTO_API char *php_mosquitto_strerror_wrapper(int err)
{
	char *ret, *buf = ecalloc(256, sizeof(char));
	ret = strerror_r(err, buf, 256);
	if (!buf[0]) {
		efree(buf);
		return NULL;
	}
	return buf;
}

PHP_MOSQUITTO_API void php_mosquitto_exit_loop(mosquitto_client_object *object)
{
	object->looping = 0;
}

static void mosquitto_client_object_destroy(zend_object *object ZO_HANDLE_DC)
{
	mosquitto_client_object *client = mosquitto_client_object_from_zend_object(object);

	/* Disconnect cleanly, but disregard an error if it wasn't connected */
	/* We must loop here so that the disconnect packet is sent and acknowledged */
	mosquitto_disconnect(client->client);
	mosquitto_loop(client->client, 100, 1);
	mosquitto_destroy(client->client);
	client->client = NULL;

	PHP_MOSQUITTO_FREE_CALLBACK(client, connect);
	PHP_MOSQUITTO_FREE_CALLBACK(client, subscribe);
	PHP_MOSQUITTO_FREE_CALLBACK(client, unsubscribe);
	PHP_MOSQUITTO_FREE_CALLBACK(client, publish);
	PHP_MOSQUITTO_FREE_CALLBACK(client, message);
	PHP_MOSQUITTO_FREE_CALLBACK(client, disconnect);
	PHP_MOSQUITTO_FREE_CALLBACK(client, log);
}

static void mosquitto_client_object_free(zend_object *object) {
	mosquitto_client_object *client = mosquitto_client_object_from_zend_object(object);
	zend_object_std_dtor(object);
}

static zend_object *mosquitto_client_object_new(zend_class_entry *ce) {
	mosquitto_client_object *client = ecalloc(1, sizeof(mosquitto_client_object) + zend_object_properties_size(ce));
	zend_object *ret = mosquitto_client_object_to_zend_object(client);

	zend_object_std_init(ret, ce);
	ret->handlers = &mosquitto_std_object_handlers;

	return ret;
}

void php_mosquitto_handle_errno(int retval, int err) {
	if (retval == MOSQ_ERR_ERRNO) {
		char *message = php_mosquitto_strerror_wrapper(errno);
		if (message) {
			zend_throw_exception(mosquitto_ce_exception, message, 0);
			efree(message);
		}
	} else if (retval != MOSQ_ERR_SUCCESS) {
		const char *message = mosquitto_strerror(retval);
		if (message && *message) {
			zend_throw_exception(mosquitto_ce_exception, message, 0);
		}
	}
}

PHP_MOSQUITTO_API void php_mosquitto_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	mosquitto_client_object *object = (mosquitto_client_object*)obj;
	zval params[2], retval;
	const char *message;

	if (!ZEND_FCI_INITIALIZED(object->connect_callback)) {
		return;
	}

	message = mosquitto_connack_string(rc);
	ZVAL_LONG(&params[0], rc);
	ZVAL_STRING(&params[1], message);

	ZVAL_UNDEF(&retval);
	object->connect_callback.retval = &retval;
	object->connect_callback.params = params;
	object->connect_callback.param_count = 2;

	if (zend_call_function(&object->connect_callback, &object->connect_callback_cache) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0, "Failed to invoke connect callback %s()", mosquitto_finfo_name(&object->connect_callback));
		}
	}

	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&params[1]);
	zval_ptr_dtor(&retval);
}

PHP_MOSQUITTO_API void php_mosquitto_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	mosquitto_client_object *object = (mosquitto_client_object*)obj;
	zval params[1], retval;

	if (!ZEND_FCI_INITIALIZED(object->disconnect_callback)) {
		return;
	}

	ZVAL_LONG(&params[0], rc);
	ZVAL_UNDEF(&retval);

	object->disconnect_callback.retval = &retval;
	object->disconnect_callback.params = params;
	object->disconnect_callback.param_count = 1;

	if (zend_call_function(&object->disconnect_callback, &object->disconnect_callback_cache) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0, "Failed to invoke disconnect callback %s()", mosquitto_finfo_name(&object->disconnect_callback));
		}
	}

	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&retval);
}

PHP_MOSQUITTO_API void php_mosquitto_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	mosquitto_client_object *object = (mosquitto_client_object*)obj;
	zval params[2], retval;

	if (!ZEND_FCI_INITIALIZED(object->log_callback)) {
		return;
	}

	ZVAL_LONG(&params[0], level);
	ZVAL_STRING(&params[1], str);
	ZVAL_UNDEF(&retval);

	object->log_callback.retval = &retval;
	object->log_callback.params = params;
	object->log_callback.param_count = 2;

	if (zend_call_function(&object->log_callback, &object->log_callback_cache) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0, "Failed to invoke log callback %s()", mosquitto_finfo_name(&object->log_callback));
		}
	}

	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&params[1]);
	zval_ptr_dtor(&retval);
}

PHP_MOSQUITTO_API void php_mosquitto_message_callback(struct mosquitto *mosq, void *client_obj, const struct mosquitto_message *message)
{
	mosquitto_client_object *object = (mosquitto_client_object*)client_obj;
	mosquitto_message_object *message_object;
	zval params[1], retval, *message_zval;

	if (!ZEND_FCI_INITIALIZED(object->message_callback)) {
		return;
	}

	message_zval = &params[0];
	ZVAL_UNDEF(&retval);
	object->message_callback.retval = &retval;

	object_init_ex(message_zval, mosquitto_ce_message);
	message_object = mosquitto_message_object_from_zend_object(Z_OBJ_P(message_zval));
	message_object->message.mid = message->mid;
	message_object->message.qos = message->qos;
	message_object->message.retain = message->retain;
	message_object->message.topic = estrdup(message->topic);
	message_object->owned_topic = 1;
	message_object->message.payloadlen = message->payloadlen;

	message_object->message.payload = ecalloc(message->payloadlen, sizeof(char));
	memcpy(message_object->message.payload, message->payload, message->payloadlen);
	message_object->owned_payload = 1;

	object->message_callback.params = params;
	object->message_callback.param_count = 1;

	if (zend_call_function(&object->message_callback, &object->message_callback_cache) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0, "Failed to invoke message callback %s()", mosquitto_finfo_name(&object->message_callback));
		}
	}

	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&retval);
}


PHP_MOSQUITTO_API void php_mosquitto_publish_callback(struct mosquitto *mosq, void *client_obj, int mid)
{
	mosquitto_client_object *object = (mosquitto_client_object*)client_obj;
	zval params[1], retval;

	if (!ZEND_FCI_INITIALIZED(object->publish_callback)) {
		return;
	}

	ZVAL_LONG(&params[0], mid);
	ZVAL_UNDEF(&retval);
	object->publish_callback.retval = &retval;
	object->publish_callback.params = params;
	object->publish_callback.param_count = 1;

	if (zend_call_function(&object->publish_callback, &object->publish_callback_cache) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0, "Failed to invoke publish callback %s()", mosquitto_finfo_name(&object->publish_callback));
		}
	}

	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&retval);
}

PHP_MOSQUITTO_API void php_mosquitto_subscribe_callback(struct mosquitto *mosq, void *client_obj, int mid, int qos_count, const int *granted_qos)
{
	mosquitto_client_object *object = (mosquitto_client_object*)client_obj;
	zval params[3], retval;

	if (!ZEND_FCI_INITIALIZED(object->subscribe_callback)) {
		return;
	}

	/* Since we can only subscribe to one topic per message, it seems reasonable to
	 * take just the first entry from granted_qos as the granted QoS value */
	ZVAL_LONG(&params[0], mid);
	ZVAL_LONG(&params[1], qos_count);
	ZVAL_LONG(&params[2], *granted_qos);
	ZVAL_UNDEF(&retval);

	object->subscribe_callback.retval = &retval;
	object->subscribe_callback.params = params;
	object->subscribe_callback.param_count = 3;

	if (zend_call_function(&object->subscribe_callback, &object->subscribe_callback_cache) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0, "Failed to invoke subscribe callback %s()", mosquitto_finfo_name(&object->subscribe_callback));
		}
	}

	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&params[1]);
	zval_ptr_dtor(&params[2]);
	zval_ptr_dtor(&retval);
}

PHP_MOSQUITTO_API void php_mosquitto_unsubscribe_callback(struct mosquitto *mosq, void *client_obj, int mid)
{
	mosquitto_client_object *object = (mosquitto_client_object*)client_obj;
	zval params[1], retval;

	if (!ZEND_FCI_INITIALIZED(object->unsubscribe_callback)) {
		return;
	}

	ZVAL_LONG(&params[0], mid);
	ZVAL_UNDEF(&retval);

	object->unsubscribe_callback.retval = &retval;
	object->unsubscribe_callback.params = params;
	object->unsubscribe_callback.param_count = 1;

	if (zend_call_function(&object->unsubscribe_callback, &object->unsubscribe_callback_cache) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0, "Failed to invoke unsubscribe callback %s()", mosquitto_finfo_name(&object->unsubscribe_callback));
		}
	}

	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&retval);
}

static int php_mosquitto_pw_callback(char *buf, int size, int rwflag, void *userdata) {
	int key_len;

	strncpy(buf, MQTTG(client_key), size);
	key_len = MQTTG(client_key_len);
	efree(MQTTG(client_key));
	MQTTG(client_key_len) = 0;

	return key_len;
}

/* {{{ mosquitto_client_methods */
const zend_function_entry mosquitto_client_methods[] = {
	PHP_ME(Mosquitto_Client, __construct, Mosquitto_Client___construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Mosquitto_Client, onConnect, Mosquitto_Client_onConnect_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onDisconnect, Mosquitto_Client_onDisconnect_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onLog, Mosquitto_Client_onLog_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onSubscribe, Mosquitto_Client_onSubscribe_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onUnsubscribe, Mosquitto_Client_onUnsubscribe_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onMessage, Mosquitto_Client_onMessage_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onPublish, Mosquitto_Client_onPublish_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, getSocket, Mosquitto_Client_getSocket_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setTlsCertificates, Mosquitto_Client_setTlsCertificates_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setTlsInsecure, Mosquitto_Client_setTlsInsecure_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setTlsOptions, Mosquitto_Client_setTlsOptions_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setTlsPSK, Mosquitto_Client_setTlsPSK_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setCredentials, Mosquitto_Client_setCredentials_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setWill, Mosquitto_Client_setWill_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, clearWill, Mosquitto_Client_void_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setReconnectDelay, Mosquitto_Client_setReconnectDelay_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setMessageRetry, Mosquitto_Client_setMessageRetry_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, connect, Mosquitto_Client_connect_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, disconnect, Mosquitto_Client_void_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setMaxInFlightMessages, Mosquitto_Client_setMaxInFlightMessages_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, publish, Mosquitto_Client_publish_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, subscribe, Mosquitto_Client_subscribe_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, unsubscribe, Mosquitto_Client_unsubscribe_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, loop, Mosquitto_Client_loop_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, loopForever, Mosquitto_Client_loopForever_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, exitLoop, Mosquitto_Client_void_args, ZEND_ACC_PUBLIC)
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
	STANDARD_MODULE_HEADER,
	"mosquitto",
	NULL,
	PHP_MINIT(mosquitto),
	PHP_MSHUTDOWN(mosquitto),
	NULL,
	NULL,
	PHP_MINFO(mosquitto),
	PHP_MOSQUITTO_VERSION,
	PHP_MODULE_GLOBALS(mosquitto),
	NULL,
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_MOSQUITTO
ZEND_GET_MODULE(mosquitto)
#endif

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(mosquitto)
{
	zend_class_entry client_ce, exception_ce;

	memcpy(&mosquitto_std_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	mosquitto_std_object_handlers.clone_obj = NULL;
	mosquitto_std_object_handlers.offset    = XtOffsetOf(mosquitto_client_object, std);
	mosquitto_std_object_handlers.free_obj  = mosquitto_client_object_free;
	mosquitto_std_object_handlers.dtor_obj  = mosquitto_client_object_destroy;

	INIT_NS_CLASS_ENTRY(client_ce, "Mosquitto", "Client", mosquitto_client_methods);
	mosquitto_ce_client = zend_register_internal_class(&client_ce);
	mosquitto_ce_client->create_object = mosquitto_client_object_new;

	INIT_NS_CLASS_ENTRY(exception_ce, "Mosquitto", "Exception", NULL);
	mosquitto_ce_exception = zend_register_internal_class_ex(&exception_ce, zend_exception_get_default());

	#define REGISTER_MOSQUITTO_LONG_CONST(const_name, value) \
	zend_declare_class_constant_long(mosquitto_ce_client, const_name, sizeof(const_name)-1, (long)value); \
	REGISTER_LONG_CONSTANT(#value,  value,  CONST_CS | CONST_PERSISTENT);

	REGISTER_MOSQUITTO_LONG_CONST("LOG_INFO", MOSQ_LOG_INFO);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_NOTICE", MOSQ_LOG_NOTICE);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_WARNING", MOSQ_LOG_WARNING);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_ERR", MOSQ_LOG_ERR);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_DEBUG", MOSQ_LOG_DEBUG);

	zend_declare_class_constant_long(mosquitto_ce_client, "SSL_VERIFY_NONE", sizeof("SSL_VERIFY_NONE")-1, 0);
	zend_declare_class_constant_long(mosquitto_ce_client, "SSL_VERIFY_PEER", sizeof("SSL_VERIFY_PEER")-1, 1);

	mosquitto_lib_init();

	PHP_MINIT(mosquitto_message)(INIT_FUNC_ARGS_PASSTHRU);

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
	char tmp[16];
	snprintf(tmp, sizeof(tmp), "%d.%d.%d", LIBMOSQUITTO_MAJOR, LIBMOSQUITTO_MINOR, LIBMOSQUITTO_REVISION);

	php_info_print_table_start();
	php_info_print_table_header(2, "Mosquitto support", "enabled");
	php_info_print_table_colspan_header(2,
#ifdef COMPILE_DL_MOSQUITTO
			"Compiled as dynamic module"
#else
			"Compiled as static module"
#endif
		);
	php_info_print_table_row(2, "libmosquitto version", tmp);
	php_info_print_table_row(2, "Extension version", PHP_MOSQUITTO_VERSION);
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
