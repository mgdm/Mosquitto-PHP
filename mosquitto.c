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

static inline mosquitto_client_object *mosquitto_client_object_get(zval *zobj TSRMLS_DC);
static int php_mosquitto_pw_callback(char *buf, int size, int rwflag, void *userdata);

/* {{{ Arginfo */

ZEND_BEGIN_ARG_INFO(Mosquitto_Client___construct_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, id)
	ZEND_ARG_INFO(0, cleanSession)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_callback_args, ZEND_SEND_BY_VAL)
#if PHP_VERSION_ID > 50400
	ZEND_ARG_TYPE_INFO(0, onConnect, IS_CALLABLE, 0)
#else
	ZEND_ARG_INFO(0, onConnect)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_setCredentials_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, username)
	ZEND_ARG_INFO(0, password)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_setWill_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, topic)
	ZEND_ARG_INFO(0, payload)
	ZEND_ARG_INFO(0, qos)
	ZEND_ARG_INFO(0, retain)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_setReconnectDelay_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, reconnectDelay)
	ZEND_ARG_INFO(0, maxReconnectDelay)
	ZEND_ARG_INFO(0, exponentialBackoff)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_setMessageRetry_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, messageRetry)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_connect_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, host)
	ZEND_ARG_INFO(0, port)
	ZEND_ARG_INFO(0, keepalive)
	ZEND_ARG_INFO(0, interface)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_setMaxInFlightMessages_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, max)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_publish_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, topic)
	ZEND_ARG_INFO(0, payload)
	ZEND_ARG_INFO(0, qos)
	ZEND_ARG_INFO(0, retain)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_subscribe_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, topic)
	ZEND_ARG_INFO(0, qos)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_unsubscribe_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, topic)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_loop_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, maxPackets)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(Mosquitto_Client_loopForever_args, ZEND_SEND_BY_VAL)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, maxPackets)
ZEND_END_ARG_INFO()

/* }}} */

/* {{{ Mosquitto\Client::__construct() */
PHP_METHOD(Mosquitto_Client, __construct)
{
	mosquitto_client_object *object;
	char *id = NULL;
	int id_len = 0;
	zend_bool clean_session = 1;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!b", &id, &id_len, &clean_session) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	object->client = mosquitto_new(id, clean_session, object);

	if (!object->client) {
		char *message = php_mosquitto_strerror_wrapper(errno);
		zend_throw_exception(mosquitto_ce_exception, message, 1 TSRMLS_CC);
		efree(message);
	}
}
/* }}} */

/* {{{ Mosquitto\Client::setTlsCertificates() */
PHP_METHOD(Mosquitto_Client, setTlsCertificates)
{
	mosquitto_client_object *object;
	char *ca_path = NULL, *cert_path = NULL, *key_path = NULL, *key_pw = NULL;
	int ca_path_len = 0, cert_path_len = 0, key_path_len = 0, key_pw_len, retval = 0;
	zval stat;
	zend_bool is_dir = 0;
	int (*pw_callback)(char *, int, int, void *) = NULL;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!|s!s!s!",
				&ca_path, &ca_path_len,
				&cert_path, &cert_path_len,
				&key_path, &key_path_len,
				&key_pw, &key_pw_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}

	if ((php_check_open_basedir(ca_path TSRMLS_CC) < 0) ||
		(php_check_open_basedir(cert_path TSRMLS_CC) < 0) ||
		(php_check_open_basedir(key_path TSRMLS_CC) < 0))
	{
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}

	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	php_stat(ca_path, ca_path_len, FS_IS_DIR, &stat TSRMLS_CC);
	is_dir = Z_BVAL(stat);

	if (key_pw != NULL) {
		pw_callback = php_mosquitto_pw_callback;
		MQTTG(client_key) = estrdup(key_pw);
		MQTTG(client_key_len) = key_pw_len;
	}

	if (is_dir) {
		retval = mosquitto_tls_set(object->client, NULL, ca_path, cert_path, key_path, pw_callback);
	} else {
		retval = mosquitto_tls_set(object->client, ca_path, NULL, cert_path, key_path, pw_callback);
	}

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
	RETURN_LONG(retval);
}
/* }}} */

/* {{{ Mosquitto\Client::setTlsInsecure() */
PHP_METHOD(Mosquitto_Client, setTlsInsecure)
{
	mosquitto_client_object *object;
	zend_bool value = 0;
	int retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &value) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	retval = mosquitto_tls_insecure_set(object->client, value);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::setTlsOptions() */
PHP_METHOD(Mosquitto_Client, setTlsOptions)
{
	mosquitto_client_object *object;
	char *tls_version = NULL, *ciphers = NULL;
	int tls_version_len = 0, ciphers_len = 0, retval = 0;
	int verify_peer = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|s!s!",
				&verify_peer,
				&tls_version, &tls_version_len,
				&ciphers, &ciphers_len
				) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	retval = mosquitto_tls_opts_set(object->client, verify_peer, tls_version, ciphers);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
	RETURN_LONG(retval);
}
/* }}} */

/* {{{ Mosquitto\Client::setTlsPSK() */
PHP_METHOD(Mosquitto_Client, setTlsPSK)
{
	mosquitto_client_object *object;
	char *psk = NULL, *identity = NULL, *ciphers = NULL;
	int psk_len = 0, identity_len = 0, ciphers_len = 0, retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!s!|s!",
				&psk, &psk_len, &identity, &identity_len, &ciphers, &ciphers_len
				) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	retval = mosquitto_tls_psk_set(object->client, psk, identity, ciphers);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
	RETURN_LONG(retval);
}
/* }}} */

/* {{{ Mosquitto\Client::setCredentials() */
PHP_METHOD(Mosquitto_Client, setCredentials)
{
	mosquitto_client_object *object;
	char *username = NULL, *password = NULL;
	int username_len, password_len, retval;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &username, &username_len, &password, &password_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	retval = mosquitto_username_pw_set(object->client, username, password);
	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::setWill() */
PHP_METHOD(Mosquitto_Client, setWill)
{
	mosquitto_client_object *object;
	int topic_len, payload_len, retval;
	long qos;
	zend_bool retain;
	char *topic, *payload;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sslb",
				&topic, &topic_len, &payload, &payload_len, &qos, &retain) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	retval = mosquitto_will_set(object->client, topic, payload_len, (void *) payload, qos, retain);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::clearWill() */
PHP_METHOD(Mosquitto_Client, clearWill)
{
	mosquitto_client_object *object;
	int retval;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	retval = mosquitto_will_clear(object->client);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::setReconnectDelay() */
PHP_METHOD(Mosquitto_Client, setReconnectDelay)
{
	mosquitto_client_object *object;
	int retval;
	long reconnect_delay = 0, reconnect_delay_max = 0;
	zend_bool exponential_backoff = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|lb",
				&reconnect_delay, &reconnect_delay_max, &exponential_backoff)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	retval = mosquitto_reconnect_delay_set(object->client, reconnect_delay, reconnect_delay_max, exponential_backoff);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::connect() */
PHP_METHOD(Mosquitto_Client, connect)
{
	mosquitto_client_object *object;
	char *host = NULL, *interface = NULL;
	int host_len, interface_len, retval;
	long port = 1883;
	long keepalive = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lls!",
				&host, &host_len, &port, &keepalive,
				&interface, &interface_len)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);

	if (interface == NULL) {
		retval = mosquitto_connect(object->client, host, port, keepalive);
	} else {
		retval = mosquitto_connect_bind(object->client, host, port, keepalive, interface);
	}

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
	RETURN_LONG(retval);
}
/* }}} */

/* {{{ Mosquitto\Client::disconnect() */
PHP_METHOD(Mosquitto_Client, disconnect)
{
	mosquitto_client_object *object;
	int retval;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);

	retval = mosquitto_disconnect(object->client);
	php_mosquitto_exit_loop(object);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::onConnect() */
PHP_METHOD(Mosquitto_Client, onConnect)
{
	mosquitto_client_object *object;
	zend_fcall_info connect_callback = empty_fcall_info;
	zend_fcall_info_cache connect_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&connect_callback, &connect_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(connect_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->connect_callback = connect_callback;
	object->connect_callback_cache = connect_callback_cache;
	Z_ADDREF_P(connect_callback.function_name);

	if (connect_callback.object_ptr != NULL) {
		Z_ADDREF_P(connect_callback.object_ptr);
	}

	mosquitto_connect_callback_set(object->client, php_mosquitto_connect_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onDisconnect() */
PHP_METHOD(Mosquitto_Client, onDisconnect)
{
	mosquitto_client_object *object;
	zend_fcall_info disconnect_callback = empty_fcall_info;
	zend_fcall_info_cache disconnect_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&disconnect_callback, &disconnect_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(disconnect_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->disconnect_callback = disconnect_callback;
	object->disconnect_callback_cache = disconnect_callback_cache;
	Z_ADDREF_P(disconnect_callback.function_name);

	if (disconnect_callback.object_ptr != NULL) {
		Z_ADDREF_P(disconnect_callback.object_ptr);
	}

	mosquitto_disconnect_callback_set(object->client, php_mosquitto_disconnect_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onLog() */
PHP_METHOD(Mosquitto_Client, onLog)
{
	mosquitto_client_object *object;
	zend_fcall_info log_callback = empty_fcall_info;
	zend_fcall_info_cache log_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&log_callback, &log_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(log_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->log_callback = log_callback;
	object->log_callback_cache = log_callback_cache;
	Z_ADDREF_P(log_callback.function_name);

	if (log_callback.object_ptr != NULL) {
		Z_ADDREF_P(log_callback.object_ptr);
	}

	mosquitto_log_callback_set(object->client, php_mosquitto_log_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onSubscribe() */
PHP_METHOD(Mosquitto_Client, onSubscribe)
{
	mosquitto_client_object *object;
	zend_fcall_info subscribe_callback = empty_fcall_info;
	zend_fcall_info_cache subscribe_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&subscribe_callback, &subscribe_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(subscribe_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->subscribe_callback = subscribe_callback;
	object->subscribe_callback_cache = subscribe_callback_cache;
	Z_ADDREF_P(subscribe_callback.function_name);

	if (subscribe_callback.object_ptr != NULL) {
		Z_ADDREF_P(subscribe_callback.object_ptr);
	}

	mosquitto_subscribe_callback_set(object->client, php_mosquitto_subscribe_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onUnsubscribe() */
PHP_METHOD(Mosquitto_Client, onUnsubscribe)
{
	mosquitto_client_object *object;
	zend_fcall_info unsubscribe_callback = empty_fcall_info;
	zend_fcall_info_cache unsubscribe_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&unsubscribe_callback, &unsubscribe_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(unsubscribe_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->unsubscribe_callback = unsubscribe_callback;
	object->unsubscribe_callback_cache = unsubscribe_callback_cache;
	Z_ADDREF_P(unsubscribe_callback.function_name);

	if (unsubscribe_callback.object_ptr != NULL) {
		Z_ADDREF_P(unsubscribe_callback.object_ptr);
	}

	mosquitto_unsubscribe_callback_set(object->client, php_mosquitto_unsubscribe_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onMessage() */
PHP_METHOD(Mosquitto_Client, onMessage)
{
	mosquitto_client_object *object;
	zend_fcall_info message_callback = empty_fcall_info;
	zend_fcall_info_cache message_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&message_callback, &message_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(message_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->message_callback = message_callback;
	object->message_callback_cache = message_callback_cache;
	Z_ADDREF_P(message_callback.function_name);

	if (message_callback.object_ptr != NULL) {
		Z_ADDREF_P(message_callback.object_ptr);
	}

	mosquitto_message_callback_set(object->client, php_mosquitto_message_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::onPublish() */
PHP_METHOD(Mosquitto_Client, onPublish)
{
    mosquitto_client_object *object;
    zend_fcall_info publish_callback = empty_fcall_info;
    zend_fcall_info_cache publish_callback_cache = empty_fcall_info_cache;

    PHP_MOSQUITTO_ERROR_HANDLING();
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
    &publish_callback, &publish_callback_cache)  == FAILURE) {

    PHP_MOSQUITTO_RESTORE_ERRORS();
    return;
    }
    PHP_MOSQUITTO_RESTORE_ERRORS();

    object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);

    if (!ZEND_FCI_INITIALIZED(publish_callback)) {
        zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
    }

    object->publish_callback = publish_callback;
    object->publish_callback_cache = publish_callback_cache;
    Z_ADDREF_P(publish_callback.function_name);

    if (publish_callback.object_ptr != NULL) {
        Z_ADDREF_P(publish_callback.object_ptr);
    }

    mosquitto_publish_callback_set(object->client, php_mosquitto_publish_callback);
}
/* }}} */

/* {{{ Mosquitto\Client::getSocket() */
PHP_METHOD(Mosquitto_Client, getSocket)
{
	mosquitto_client_object *object;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none()  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	RETURN_LONG(mosquitto_socket(object->client));
}
/* }}} */

/* {{{ Mosquitto\Client::setMaxInFlightMessages() */
PHP_METHOD(Mosquitto_Client, setMaxInFlightMessages)
{
	mosquitto_client_object *object;
	int retval;
	long max = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &max)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	retval = mosquitto_max_inflight_messages_set(object->client, max);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::setMessageRetry() */
PHP_METHOD(Mosquitto_Client, setMessageRetry)
{
	mosquitto_client_object *object;
	long retry = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &retry)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	mosquitto_message_retry_set(object->client, retry);
}
/* }}} */

/* {{{ Mosquitto\Client::publish() */
PHP_METHOD(Mosquitto_Client, publish)
{
	mosquitto_client_object *object;
	int mid, topic_len, payload_len, retval;
	long qos = 0;
	zend_bool retain = 0;
	char *topic, *payload;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|lb",
				&topic, &topic_len, &payload, &payload_len, &qos, &retain) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	retval = mosquitto_publish(object->client, &mid, topic, payload_len, (void *) payload, qos, retain);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

	RETURN_LONG(mid);
}
/* }}} */

/* {{{ Mosquitto\Client::subscribe() */
PHP_METHOD(Mosquitto_Client, subscribe)
{
	mosquitto_client_object *object;
	char *sub;
	int sub_len, retval, mid;
	long qos;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
				&sub, &sub_len, &qos) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	retval = mosquitto_subscribe(object->client, &mid, sub, qos);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

	RETURN_LONG(mid);
}
/* }}} */

/* {{{ Mosquitto\Client::unsubscribe() */
PHP_METHOD(Mosquitto_Client, unsubscribe)
{
	mosquitto_client_object *object;
	char *sub;
	int sub_len, retval, mid;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
				&sub, &sub_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	retval = mosquitto_unsubscribe(object->client, &mid, sub);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

	RETURN_LONG(mid);
}
/* }}} */

/* {{{ Mosquitto\Client::loop() */
PHP_METHOD(Mosquitto_Client, loop)
{
	mosquitto_client_object *object;
	long timeout = 1000, max_packets = 1, retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll",
				&timeout, &max_packets) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	retval = mosquitto_loop(object->client, timeout, max_packets);
	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::loopForever() */
PHP_METHOD(Mosquitto_Client, loopForever)
{
	mosquitto_client_object *object;
	long timeout = 1000, max_packets = 1, retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll",
				&timeout, &max_packets) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
	object->looping = 1;

	while (object->looping) {
		retval = mosquitto_loop(object->client, timeout, max_packets);
		php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

		if (EG(exception)) {
			break;
		}
	}
}
/* }}} */

/* {{{ Mosquitto\Client::exitLoop() */
PHP_METHOD(Mosquitto_Client, exitLoop)
{
	mosquitto_client_object *object;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) mosquitto_client_object_get(getThis() TSRMLS_CC);
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
	char *buf = ecalloc(256, sizeof(char));
#ifdef STRERROR_R_CHAR_P
	return strerror_r(err, buf, 256);
#else
	if (!strerror_r(err, buf, 256)) {
		return buf;
	}
	efree(buf);
	return NULL;
#endif
}

PHP_MOSQUITTO_API void php_mosquitto_exit_loop(mosquitto_client_object *object)
{
	object->looping = 0;
}

static inline mosquitto_client_object *mosquitto_client_object_get(zval *zobj TSRMLS_DC)
{
    mosquitto_client_object *pobj = zend_object_store_get_object(zobj TSRMLS_CC);

    if (pobj->client == NULL) {
        php_error(E_ERROR, "Internal surface object missing in %s wrapper, you must call parent::__construct in extended classes", Z_OBJCE_P(zobj)->name);
    }
    return pobj;
}

static void mosquitto_client_object_destroy(void *object TSRMLS_DC)
{
	mosquitto_client_object *client = (mosquitto_client_object *) object;

	/* Disconnect cleanly, but disregard an error if it wasn't connected */
	/* We must loop here so that the disconnect packet is sent and acknowledged */
	mosquitto_disconnect(client->client);
	mosquitto_loop(client->client, 100, 1);
	mosquitto_destroy(client->client);

	if (MQTTG(client_key_len) > 0) {
		efree(MQTTG(client_key));
	}

	PHP_MOSQUITTO_FREE_CALLBACK(connect);
	PHP_MOSQUITTO_FREE_CALLBACK(subscribe);
	PHP_MOSQUITTO_FREE_CALLBACK(unsubscribe);
	PHP_MOSQUITTO_FREE_CALLBACK(publish);
	PHP_MOSQUITTO_FREE_CALLBACK(message);
	PHP_MOSQUITTO_FREE_CALLBACK(disconnect);
	PHP_MOSQUITTO_FREE_CALLBACK(log);

	if (client->std.properties) {
		zend_hash_destroy(client->std.properties);
		FREE_HASHTABLE(client->std.properties);
	}

	efree(object);
}

static zend_object_value mosquitto_client_object_new(zend_class_entry *ce TSRMLS_DC) {

	zend_object_value retval;
	mosquitto_client_object *client;
#if PHP_VERSION_ID < 50399
	zval *temp;
#endif

	client = ecalloc(1, sizeof(mosquitto_client_object));
	client->std.ce = ce;
	client->client = NULL;

#ifdef ZTS
	client->TSRMLS_C = TSRMLS_C;
#endif

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

void php_mosquitto_handle_errno(int retval, int err TSRMLS_DC) {
	const char *message;

	switch (retval) {
		case MOSQ_ERR_SUCCESS:
			return;

		case MOSQ_ERR_ERRNO:
			message = php_mosquitto_strerror_wrapper(errno);
			break;

		default:
			message = mosquitto_strerror(retval);
			break;
	}

	zend_throw_exception(mosquitto_ce_exception, (char *) message, 0 TSRMLS_CC);
}

PHP_MOSQUITTO_API void php_mosquitto_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	mosquitto_client_object *object = (mosquitto_client_object *) obj;
	zval *retval_ptr = NULL, *rc_zval = NULL, *message_zval = NULL;
	zval **params[2];
	const char *message;
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->connect_callback)) {
		return;
	}

	MAKE_STD_ZVAL(rc_zval);
	ZVAL_LONG(rc_zval, rc);
	params[0] = &rc_zval;

	message = mosquitto_connack_string(rc);
	MAKE_STD_ZVAL(message_zval);
	ZVAL_STRINGL(message_zval, message, strlen(message), 1);
	params[1] = &message_zval;

	object->connect_callback.params = params;
	object->connect_callback.param_count = 2;
	object->connect_callback.retval_ptr_ptr = &retval_ptr;

	if (zend_call_function(&object->connect_callback, &object->connect_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke connect callback %s()", Z_STRVAL_P(object->connect_callback.function_name));
		}
	}

	zval_ptr_dtor(&rc_zval);
	zval_ptr_dtor(&message_zval);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
}

PHP_MOSQUITTO_API void php_mosquitto_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	mosquitto_client_object *object = (mosquitto_client_object *) obj;
	zval *retval_ptr = NULL, *rc_zval = NULL;
	zval **params[1];
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->disconnect_callback)) {
		return;
	}

	MAKE_STD_ZVAL(rc_zval);
	ZVAL_LONG(rc_zval, rc);
	params[0] = &rc_zval;

	object->disconnect_callback.params = params;
	object->disconnect_callback.param_count = 1;
	object->disconnect_callback.retval_ptr_ptr = &retval_ptr;

	if (zend_call_function(&object->disconnect_callback, &object->disconnect_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke disconnect callback %s()", Z_STRVAL_P(object->disconnect_callback.function_name));
		}
	}

	zval_ptr_dtor(&rc_zval);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
}

PHP_MOSQUITTO_API void php_mosquitto_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	mosquitto_client_object *object = (mosquitto_client_object *) obj;
	zval *retval_ptr = NULL;
	zval *level_zval = NULL, *str_zval = NULL;
	zval **params[2];
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->log_callback)) {
		return;
	}

	MAKE_STD_ZVAL(level_zval);
	ZVAL_LONG(level_zval, level);
	MAKE_STD_ZVAL(str_zval);
	ZVAL_STRINGL(str_zval, str, strlen(str), 1);

	params[0] = &level_zval;
	params[1] = &str_zval;

	object->log_callback.params = params;
	object->log_callback.param_count = 2;
	object->log_callback.retval_ptr_ptr = &retval_ptr;

	if (zend_call_function(&object->log_callback, &object->log_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke log callback %s()", Z_STRVAL_P(object->log_callback.function_name));
		}
	}

	zval_ptr_dtor(params[0]);
	zval_ptr_dtor(params[1]);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
}

PHP_MOSQUITTO_API void php_mosquitto_message_callback(struct mosquitto *mosq, void *client_obj, const struct mosquitto_message *message)
{
    mosquitto_client_object *object = (mosquitto_client_object *) client_obj;
    mosquitto_message_object *message_object;
    zval *retval_ptr = NULL, *message_zval = NULL;
    zval **params[1];
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

    if (!ZEND_FCI_INITIALIZED(object->message_callback)) {
        return;
    }

    MAKE_STD_ZVAL(message_zval);
    object_init_ex(message_zval, mosquitto_ce_message);
    message_object = (mosquitto_message_object *) zend_object_store_get_object(message_zval TSRMLS_CC);
    mosquitto_message_copy(&message_object->message, message);
    params[0] = &message_zval;

    object->message_callback.params = params;
    object->message_callback.param_count = 1;
    object->message_callback.retval_ptr_ptr = &retval_ptr;

    if (zend_call_function(&object->message_callback, &object->message_callback_cache TSRMLS_CC) == FAILURE) {
        if (!EG(exception)) {
            zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke message callback %s()", Z_STRVAL_P(object->message_callback.function_name));
        }
    }

    zval_ptr_dtor(&message_zval);

    if (retval_ptr != NULL) {
        zval_ptr_dtor(&retval_ptr);
    }
}


PHP_MOSQUITTO_API void php_mosquitto_publish_callback(struct mosquitto *mosq, void *client_obj, int mid)
{
    mosquitto_client_object *object = (mosquitto_client_object *) client_obj;
    zval *retval_ptr = NULL;
    zval *mid_zval;
    zval **params[1];
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

    if (!ZEND_FCI_INITIALIZED(object->publish_callback)) {
        return;
    }

    MAKE_STD_ZVAL(mid_zval);
    ZVAL_LONG(mid_zval, mid);
    params[0] = &mid_zval;

    object->publish_callback.params = params;
    object->publish_callback.param_count = 1;
    object->publish_callback.retval_ptr_ptr = &retval_ptr;

    if (zend_call_function(&object->publish_callback, &object->publish_callback_cache TSRMLS_CC) == FAILURE) {
        if (!EG(exception)) {
            zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke publish callback %s()", Z_STRVAL_P(object->publish_callback.function_name));
        }
    }

    zval_ptr_dtor(params[0]);

    if (retval_ptr != NULL) {
        zval_ptr_dtor(&retval_ptr);
    }
}

PHP_MOSQUITTO_API void php_mosquitto_subscribe_callback(struct mosquitto *mosq, void *client_obj, int mid, int qos_count, const int *granted_qos)
{
	mosquitto_client_object *object = (mosquitto_client_object *) client_obj;
	zval *retval_ptr = NULL;
	zval *mid_zval, *qos_count_zval, *granted_qos_zval;
	zval **params[3];
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->subscribe_callback)) {
		return;
	}

	/* Since we can only subscribe to one topic per message, it seems reasonable to
	 * take just the first entry from granted_qos as the granted QoS value */
	MAKE_STD_ZVAL(mid_zval);
	MAKE_STD_ZVAL(qos_count_zval);
	MAKE_STD_ZVAL(granted_qos_zval);
	ZVAL_LONG(mid_zval, mid);
	ZVAL_LONG(qos_count_zval, qos_count);
	ZVAL_LONG(granted_qos_zval, *granted_qos);
	params[0] = &mid_zval;
	params[1] = &qos_count_zval;
	params[2] = &granted_qos_zval;

	object->subscribe_callback.params = params;
	object->subscribe_callback.param_count = 3;
	object->subscribe_callback.retval_ptr_ptr = &retval_ptr;

	if (zend_call_function(&object->subscribe_callback, &object->subscribe_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke subscribe callback %s()", Z_STRVAL_P(object->subscribe_callback.function_name));
		}
	}

	zval_ptr_dtor(params[0]);
	zval_ptr_dtor(params[1]);
	zval_ptr_dtor(params[2]);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
}

PHP_MOSQUITTO_API void php_mosquitto_unsubscribe_callback(struct mosquitto *mosq, void *client_obj, int mid)
{
	mosquitto_client_object *object = (mosquitto_client_object *) client_obj;
	zval *retval_ptr = NULL;
	zval *mid_zval;
	zval **params[1];
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->unsubscribe_callback)) {
		return;
	}

	MAKE_STD_ZVAL(mid_zval);
	ZVAL_LONG(mid_zval, mid);
	params[0] = &mid_zval;

	object->unsubscribe_callback.params = params;
	object->unsubscribe_callback.param_count = 1;
	object->unsubscribe_callback.retval_ptr_ptr = &retval_ptr;

	if (zend_call_function(&object->unsubscribe_callback, &object->unsubscribe_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke unsubscribe callback %s()", Z_STRVAL_P(object->unsubscribe_callback.function_name));
		}
	}

	zval_ptr_dtor(params[0]);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
}

static int php_mosquitto_pw_callback(char *buf, int size, int rwflag, void *userdata) {
	TSRMLS_FETCH();
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
	PHP_ME(Mosquitto_Client, onConnect, Mosquitto_Client_callback_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onDisconnect, Mosquitto_Client_callback_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onLog, Mosquitto_Client_callback_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onSubscribe, Mosquitto_Client_callback_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onUnsubscribe, Mosquitto_Client_callback_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onMessage, Mosquitto_Client_callback_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onPublish, Mosquitto_Client_callback_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, getSocket, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setTlsCertificates, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setTlsInsecure, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setTlsOptions, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setTlsPSK, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setCredentials, Mosquitto_Client_setCredentials_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setWill, Mosquitto_Client_setWill_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, clearWill, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setReconnectDelay, Mosquitto_Client_setReconnectDelay_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setMessageRetry, Mosquitto_Client_setMessageRetry_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, connect, Mosquitto_Client_connect_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, disconnect, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setMaxInFlightMessages, Mosquitto_Client_setMaxInFlightMessages_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, publish, Mosquitto_Client_publish_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, subscribe, Mosquitto_Client_subscribe_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, unsubscribe, Mosquitto_Client_unsubscribe_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, loop, Mosquitto_Client_loop_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, loopForever, Mosquitto_Client_loopForever_args, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, exitLoop, NULL, ZEND_ACC_PUBLIC)
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
	PHP_MOSQUITTO_VERSION,
#endif
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

	INIT_NS_CLASS_ENTRY(client_ce, "Mosquitto", "Client", mosquitto_client_methods);
	mosquitto_ce_client = zend_register_internal_class_ex(&client_ce, NULL, NULL TSRMLS_CC);
	mosquitto_ce_client->create_object = mosquitto_client_object_new;

	INIT_NS_CLASS_ENTRY(exception_ce, "Mosquitto", "Exception", NULL);
	mosquitto_ce_exception = zend_register_internal_class_ex(&exception_ce,
			zend_exception_get_default(TSRMLS_C), "Exception" TSRMLS_CC);

	#define REGISTER_MOSQUITTO_LONG_CONST(const_name, value) \
	zend_declare_class_constant_long(mosquitto_ce_client, const_name, sizeof(const_name)-1, (long)value TSRMLS_CC); \
	REGISTER_LONG_CONSTANT(#value,  value,  CONST_CS | CONST_PERSISTENT);

	REGISTER_MOSQUITTO_LONG_CONST("LOG_INFO", MOSQ_LOG_INFO);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_NOTICE", MOSQ_LOG_NOTICE);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_WARNING", MOSQ_LOG_WARNING);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_ERR", MOSQ_LOG_ERR);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_DEBUG", MOSQ_LOG_DEBUG);
	
	REGISTER_MOSQUITTO_LONG_CONST("SSL_VERIFY_NONE", 0);
	REGISTER_MOSQUITTO_LONG_CONST("SSL_VERIFY_PEER", 1);

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
