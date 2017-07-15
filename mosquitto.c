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

#ifdef ZEND_ENGINE_3
# ifndef Z_BVAL
#  define Z_BVAL(zv) (Z_TYPE(zv) == IS_TRUE)
#  define Z_BVAL_P(pzv) Z_BVAL(*pzv)
# endif
# define ZO_HANDLE_DC
typedef size_t mosquitto_strlen_type;
#else /* ZEND_ENGINE_2 */
# ifndef Z_OBJ_P
#  define Z_OBJ_P(pzv) ((zend_object*)zend_object_store_get_object(pzv TSRMLS_CC))
# endif
# define ZO_HANDLE_DC , zend_object_handle handle
typedef int mosquitto_strlen_type;
typedef long zend_long;
#endif

static inline mosquitto_client_object *mosquitto_client_object_get(zval *zobj TSRMLS_DC) {
	// TODO: ZEND_ASSERT()s
	mosquitto_client_object *obj = mosquitto_client_object_from_zend_object(Z_OBJ_P(zobj));
	if (!obj->client) {
		php_error(E_ERROR, "Internal surface object missing in %s wrapper, "
		                   "you must call parent::__construct in extended classes", Z_OBJCE_P(zobj)->name);
	}
	return obj;
}

static inline void mosquitto_callback_addref(zend_fcall_info *finfo) {
#ifdef ZEND_ENGINE_3
	zval tmp;
	Z_TRY_ADDREF(finfo->function_name);
	if (finfo->object) {
		ZVAL_OBJ(&tmp, finfo->object);
		Z_TRY_ADDREF(tmp);
	}
#else
	Z_ADDREF_P(finfo->function_name);
	if (finfo->object_ptr) {
		Z_ADDREF_P(finfo->object_ptr);
	}
#endif
}

static inline const char *mosquitto_finfo_name(zend_fcall_info *info) {
#ifdef ZEND_ENGINE_3
	return Z_STRVAL(info->function_name);
#else
	return Z_STRVAL_P(info->function_name);
#endif
}

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
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	char *id = NULL;
	mosquitto_strlen_type id_len = 0;
	zend_bool clean_session = 1;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!b", &id, &id_len, &clean_session) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object->client = mosquitto_new(id, clean_session, object);
	if (!object->client) {
		char *message = php_mosquitto_strerror_wrapper(errno);
		zend_throw_exception(mosquitto_ce_exception, message, 1 TSRMLS_CC);
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
	char *ca_path = NULL, *cert_path = NULL, *key_path = NULL, *key_pw = NULL;
	mosquitto_strlen_type ca_path_len = 0, cert_path_len = 0, key_path_len = 0, key_pw_len;
	int retval = 0;
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
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_bool value = 0;
	int retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &value) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_tls_insecure_set(object->client, value);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|s!s!",
				&verify_peer,
				&tls_version, &tls_version_len,
				&ciphers, &ciphers_len
				) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_tls_opts_set(object->client, verify_peer, tls_version, ciphers);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!s!|s!",
				&psk, &psk_len, &identity, &identity_len, &ciphers, &ciphers_len
				) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_tls_psk_set(object->client, psk, identity, ciphers);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &username, &username_len, &password, &password_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_username_pw_set(object->client, username, password);
	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sslb",
				&topic, &topic_len, &payload, &payload_len, &qos, &retain) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_will_set(object->client, topic, payload_len, (void *) payload, qos, retain);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
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

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|lb",
				&reconnect_delay, &reconnect_delay_max, &exponential_backoff)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_reconnect_delay_set(object->client, reconnect_delay, reconnect_delay_max, exponential_backoff);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lls!",
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

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
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

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::onConnect() */
PHP_METHOD(Mosquitto_Client, onConnect)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_fcall_info connect_callback = empty_fcall_info;
	zend_fcall_info_cache connect_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&connect_callback, &connect_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(connect_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&disconnect_callback, &disconnect_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(disconnect_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&log_callback, &log_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(log_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&subscribe_callback, &subscribe_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(subscribe_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&unsubscribe_callback, &unsubscribe_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(unsubscribe_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&message_callback, &message_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(message_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&publish_callback, &publish_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	if (!ZEND_FCI_INITIALIZED(publish_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &max)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_max_inflight_messages_set(object->client, max);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::setMessageRetry() */
PHP_METHOD(Mosquitto_Client, setMessageRetry)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_long retry = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &retry)  == FAILURE) {
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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|lb",
				&topic, &topic_len, &payload, &payload_len, &qos, &retain) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_publish(object->client, &mid, topic, payload_len, (void *) payload, qos, retain);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
				&sub, &sub_len, &qos) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_subscribe(object->client, &mid, sub, qos);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
				&sub, &sub_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_unsubscribe(object->client, &mid, sub);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

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
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll",
				&timeout, &max_packets) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	retval = mosquitto_loop(object->client, timeout, max_packets);
	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ Mosquitto\Client::loopForever() */
PHP_METHOD(Mosquitto_Client, loopForever)
{
	mosquitto_client_object *object = mosquitto_client_object_from_zend_object(Z_OBJ_P(getThis()));
	zend_long timeout = 1000, max_packets = 1;
	long retval = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll",
				&timeout, &max_packets) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

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
	char *buf = ecalloc(256, sizeof(char));
	strerror_r(err, buf, 256);
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

static void mosquitto_client_object_destroy(zend_object *object ZO_HANDLE_DC TSRMLS_DC)
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

static void mosquitto_client_object_free(zend_object *object TSRMLS_DC) {
	mosquitto_client_object *client = mosquitto_client_object_from_zend_object(object);

#ifdef ZEND_ENGINE_3
	zend_object_std_dtor(object);
#else
	if (object->properties) {
		zend_hash_destroy(object->properties);
		FREE_HASHTABLE(object->properties);
	}
	efree(object);
#endif
}

#ifdef ZEND_ENGINE_3
static zend_object *mosquitto_client_object_new(zend_class_entry *ce) {
	mosquitto_client_object *client = ecalloc(1, sizeof(mosquitto_client_object) + zend_object_properties_size(ce));
	zend_object *ret = mosquitto_client_object_to_zend_object(client);

#ifdef MOSQUITTO_NEED_TSRMLS
	client->TSRMLS_C = TSRMLS_C;
#endif

	zend_object_std_init(ret, ce);
	ret->handlers = &mosquitto_std_object_handlers;

	return ret;
}
#else
static zend_object_value mosquitto_client_object_new(zend_class_entry *ce TSRMLS_DC) {

	zend_object_value retval;
	mosquitto_client_object *client;
#if PHP_VERSION_ID < 50399
	zval *temp;
#endif

	client = ecalloc(1, sizeof(mosquitto_client_object));
	client->std.ce = ce;
	client->client = NULL;

#ifdef MOSQUITTO_NEED_TSRMLS
	client->TSRMLS_C = TSRMLS_C;
#endif

	ALLOC_HASHTABLE(client->std.properties);
	zend_hash_init(client->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(client->std.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &temp, sizeof(zval *));
#else
	object_properties_init(&client->std, ce);
#endif
	retval.handle = zend_objects_store_put(client,
		(zend_objects_store_dtor_t)mosquitto_client_object_destroy,
		(zend_objects_free_object_storage_t)mosquitto_client_object_free, NULL TSRMLS_CC);
	retval.handlers = &mosquitto_std_object_handlers;
	return retval;
}
#endif

void php_mosquitto_handle_errno(int retval, int err TSRMLS_DC) {
	if (retval == MOSQ_ERR_ERRNO) {
		char *message = php_mosquitto_strerror_wrapper(errno);
		if (message) {
			zend_throw_exception(mosquitto_ce_exception, message, 0 TSRMLS_CC);
			efree(message);
		}
	} else if (retval != MOSQ_ERR_SUCCESS) {
		const char *message = mosquitto_strerror(retval);
		if (message && *message) {
			zend_throw_exception(mosquitto_ce_exception, message, 0 TSRMLS_CC);
		}
	}
}

PHP_MOSQUITTO_API void php_mosquitto_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	mosquitto_client_object *object = (mosquitto_client_object*)obj;
#ifdef ZEND_ENGINE_3
	zval params[2], retval;
#else
	zval *retval_ptr = NULL, *rc_zval = NULL, *message_zval = NULL;
	zval **params[2];
#endif
	const char *message;
#ifdef MOSQUITTO_NEED_TSRMLS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->connect_callback)) {
		return;
	}

	message = mosquitto_connack_string(rc);
#ifdef ZEND_ENGINE_3
	ZVAL_LONG(&params[0], rc);
	ZVAL_STRING(&params[1], message);

	ZVAL_UNDEF(&retval);
	object->connect_callback.retval = &retval;
#else
	MAKE_STD_ZVAL(rc_zval);
	ZVAL_LONG(rc_zval, rc);
	params[0] = &rc_zval;

	MAKE_STD_ZVAL(message_zval);
	ZVAL_STRING(message_zval, message, 1);
	params[1] = &message_zval;

	object->connect_callback.retval_ptr_ptr = &retval_ptr;
#endif

	object->connect_callback.params = params;
	object->connect_callback.param_count = 2;

	if (zend_call_function(&object->connect_callback, &object->connect_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke connect callback %s()", mosquitto_finfo_name(&object->connect_callback));
		}
	}

#ifdef ZEND_ENGINE_3
	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&params[1]);
	zval_ptr_dtor(&retval);
#else
	zval_ptr_dtor(&rc_zval);
	zval_ptr_dtor(&message_zval);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
#endif
}

PHP_MOSQUITTO_API void php_mosquitto_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	mosquitto_client_object *object = (mosquitto_client_object*)obj;
#ifdef ZEND_ENGINE_3
	zval params[1], retval;
#else
	zval *retval_ptr = NULL, *rc_zval = NULL;
	zval **params[1];
#endif
#ifdef MOSQUITTO_NEED_TSRMLS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->disconnect_callback)) {
		return;
	}

#ifdef ZEND_ENGINE_3
	ZVAL_LONG(&params[0], rc);
	ZVAL_UNDEF(&retval);

	object->disconnect_callback.retval = &retval;
#else
	MAKE_STD_ZVAL(rc_zval);
	ZVAL_LONG(rc_zval, rc);
	params[0] = &rc_zval;

	object->disconnect_callback.retval_ptr_ptr = &retval_ptr;
#endif

	object->disconnect_callback.params = params;
	object->disconnect_callback.param_count = 1;

	if (zend_call_function(&object->disconnect_callback, &object->disconnect_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke disconnect callback %s()", mosquitto_finfo_name(&object->disconnect_callback));
		}
	}

#ifdef ZEND_ENGINE_3
	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&retval);
#else
	zval_ptr_dtor(&rc_zval);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
#endif
}

PHP_MOSQUITTO_API void php_mosquitto_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	mosquitto_client_object *object = (mosquitto_client_object*)obj;
#ifdef ZEND_ENGINE_3
	zval params[2], retval;
#else
	zval *retval_ptr = NULL, *level_zval = NULL, *str_zval = NULL;
	zval **params[2];
#endif
#ifdef MOSQUITTO_NEED_TSRMLS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->log_callback)) {
		return;
	}

#ifdef ZEND_ENGINE_3
	ZVAL_LONG(&params[0], level);
	ZVAL_STRING(&params[1], str);
	ZVAL_UNDEF(&retval);

	object->log_callback.retval = &retval;
#else
	MAKE_STD_ZVAL(level_zval);
	ZVAL_LONG(level_zval, level);
	MAKE_STD_ZVAL(str_zval);
	ZVAL_STRING(str_zval, str, 1);

	params[0] = &level_zval;
	params[1] = &str_zval;

	object->log_callback.retval_ptr_ptr = &retval_ptr;
#endif

	object->log_callback.params = params;
	object->log_callback.param_count = 2;

	if (zend_call_function(&object->log_callback, &object->log_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke log callback %s()", mosquitto_finfo_name(&object->log_callback));
		}
	}

#ifdef ZEND_ENGINE_3
	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&params[1]);
	zval_ptr_dtor(&retval);
#else
	zval_ptr_dtor(params[0]);
	zval_ptr_dtor(params[1]);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
#endif
}

PHP_MOSQUITTO_API void php_mosquitto_message_callback(struct mosquitto *mosq, void *client_obj, const struct mosquitto_message *message)
{
	mosquitto_client_object *object = (mosquitto_client_object*)client_obj;
	mosquitto_message_object *message_object;
#ifdef ZEND_ENGINE_3
	zval params[1], retval, *message_zval;
#else
	zval *retval_ptr = NULL, *message_zval = NULL;
	zval **params[1];
#endif
#ifdef MOSQUITTO_NEED_TSRMLS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->message_callback)) {
		return;
	}

#ifdef ZEND_ENGINE_3
	message_zval = &params[0];
	ZVAL_UNDEF(&retval);
	object->message_callback.retval = &retval;
#else
	MAKE_STD_ZVAL(message_zval);
	params[0] = &message_zval;
	object->message_callback.retval_ptr_ptr = &retval_ptr;
#endif

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

	if (zend_call_function(&object->message_callback, &object->message_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke message callback %s()", mosquitto_finfo_name(&object->message_callback));
		}
	}

#ifdef ZEND_ENGINE_3
	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&retval);
#else
	zval_ptr_dtor(&message_zval);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
#endif
}


PHP_MOSQUITTO_API void php_mosquitto_publish_callback(struct mosquitto *mosq, void *client_obj, int mid)
{
	mosquitto_client_object *object = (mosquitto_client_object*)client_obj;
#ifdef ZEND_ENGINE_3
	zval params[1], retval;
#else
	zval *retval_ptr = NULL, *mid_zval;
	zval **params[1];
#endif
#ifdef MOSQUITTO_NEED_TSRMLS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->publish_callback)) {
		return;
	}

#ifdef ZEND_ENGINE_3
	ZVAL_LONG(&params[0], mid);
	ZVAL_UNDEF(&retval);
	object->publish_callback.retval = &retval;
#else
	MAKE_STD_ZVAL(mid_zval);
	ZVAL_LONG(mid_zval, mid);
	params[0] = &mid_zval;
	object->publish_callback.retval_ptr_ptr = &retval_ptr;
#endif

	object->publish_callback.params = params;
	object->publish_callback.param_count = 1;

	if (zend_call_function(&object->publish_callback, &object->publish_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke publish callback %s()", mosquitto_finfo_name(&object->publish_callback));
		}
	}

#ifdef ZEND_ENGINE_3
	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&retval);
#else
	zval_ptr_dtor(params[0]);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
#endif
}

PHP_MOSQUITTO_API void php_mosquitto_subscribe_callback(struct mosquitto *mosq, void *client_obj, int mid, int qos_count, const int *granted_qos)
{
	mosquitto_client_object *object = (mosquitto_client_object*)client_obj;
#ifdef ZEND_ENGINE_3
	zval params[3], retval;
#else
	zval *retval_ptr = NULL, *mid_zval, *qos_count_zval, *granted_qos_zval;
	zval **params[3];
#endif
#ifdef MOSQUITTO_NEED_TSRMLS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->subscribe_callback)) {
		return;
	}

	/* Since we can only subscribe to one topic per message, it seems reasonable to
	 * take just the first entry from granted_qos as the granted QoS value */
#ifdef ZEND_ENGINE_3
	ZVAL_LONG(&params[0], mid);
	ZVAL_LONG(&params[1], qos_count);
	ZVAL_LONG(&params[2], *granted_qos);
	ZVAL_UNDEF(&retval);

	object->subscribe_callback.retval = &retval;
#else
	MAKE_STD_ZVAL(mid_zval);
	MAKE_STD_ZVAL(qos_count_zval);
	MAKE_STD_ZVAL(granted_qos_zval);
	ZVAL_LONG(mid_zval, mid);
	ZVAL_LONG(qos_count_zval, qos_count);
	ZVAL_LONG(granted_qos_zval, *granted_qos);
	params[0] = &mid_zval;
	params[1] = &qos_count_zval;
	params[2] = &granted_qos_zval;

	object->subscribe_callback.retval_ptr_ptr = &retval_ptr;
#endif

	object->subscribe_callback.params = params;
	object->subscribe_callback.param_count = 3;

	if (zend_call_function(&object->subscribe_callback, &object->subscribe_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke subscribe callback %s()", mosquitto_finfo_name(&object->subscribe_callback));
		}
	}

#ifdef ZEND_ENGINE_3
	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&params[1]);
	zval_ptr_dtor(&params[2]);
	zval_ptr_dtor(&retval);
#else
	zval_ptr_dtor(params[0]);
	zval_ptr_dtor(params[1]);
	zval_ptr_dtor(params[2]);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
#endif
}

PHP_MOSQUITTO_API void php_mosquitto_unsubscribe_callback(struct mosquitto *mosq, void *client_obj, int mid)
{
	mosquitto_client_object *object = (mosquitto_client_object*)client_obj;
#ifdef ZEND_ENGINE_3
	zval params[1], retval;
#else
	zval *retval_ptr = NULL, *mid_zval;
	zval **params[1];
#endif
#ifdef MOSQUITTO_NEED_TSRMLS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->unsubscribe_callback)) {
		return;
	}

#ifdef ZEND_ENGINE_3
	ZVAL_LONG(&params[0], mid);
	ZVAL_UNDEF(&retval);

	object->unsubscribe_callback.retval = &retval;
#else
	MAKE_STD_ZVAL(mid_zval);
	ZVAL_LONG(mid_zval, mid);
	params[0] = &mid_zval;

	object->unsubscribe_callback.retval_ptr_ptr = &retval_ptr;
#endif

	object->unsubscribe_callback.params = params;
	object->unsubscribe_callback.param_count = 1;

	if (zend_call_function(&object->unsubscribe_callback, &object->unsubscribe_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke unsubscribe callback %s()", mosquitto_finfo_name(&object->unsubscribe_callback));
		}
	}

#ifdef ZEND_ENGINE_3
	zval_ptr_dtor(&params[0]);
	zval_ptr_dtor(&retval);
#else
	zval_ptr_dtor(params[0]);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
#endif
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
#ifdef ZEND_ENGINE_3
	mosquitto_std_object_handlers.offset    = XtOffsetOf(mosquitto_client_object, std);
	mosquitto_std_object_handlers.free_obj  = mosquitto_client_object_free;
	mosquitto_std_object_handlers.dtor_obj  = mosquitto_client_object_destroy;
#endif

	INIT_NS_CLASS_ENTRY(client_ce, "Mosquitto", "Client", mosquitto_client_methods);
	mosquitto_ce_client = zend_register_internal_class(&client_ce TSRMLS_CC);
	mosquitto_ce_client->create_object = mosquitto_client_object_new;

	INIT_NS_CLASS_ENTRY(exception_ce, "Mosquitto", "Exception", NULL);
	mosquitto_ce_exception = zend_register_internal_class_ex(&exception_ce, zend_exception_get_default(TSRMLS_C)
#ifndef ZEND_ENGINE_3
			, "Exception" TSRMLS_CC
#endif
	);

	#define REGISTER_MOSQUITTO_LONG_CONST(const_name, value) \
	zend_declare_class_constant_long(mosquitto_ce_client, const_name, sizeof(const_name)-1, (long)value TSRMLS_CC); \
	REGISTER_LONG_CONSTANT(#value,  value,  CONST_CS | CONST_PERSISTENT);

	REGISTER_MOSQUITTO_LONG_CONST("LOG_INFO", MOSQ_LOG_INFO);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_NOTICE", MOSQ_LOG_NOTICE);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_WARNING", MOSQ_LOG_WARNING);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_ERR", MOSQ_LOG_ERR);
	REGISTER_MOSQUITTO_LONG_CONST("LOG_DEBUG", MOSQ_LOG_DEBUG);

	zend_declare_class_constant_long(mosquitto_ce_client, "SSL_VERIFY_NONE", sizeof("SSL_VERIFY_NONE")-1, 0 TSRMLS_CC);
	zend_declare_class_constant_long(mosquitto_ce_client, "SSL_VERIFY_PEER", sizeof("SSL_VERIFY_PEER")-1, 1 TSRMLS_CC);

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
