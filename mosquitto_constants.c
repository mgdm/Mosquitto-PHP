#include "php.h"
#include "zend_API.h"
#include "php_mosquitto.h"

zend_class_entry *mosquitto_ce_loglevel;
zend_class_entry *mosquitto_ce_sslverify;
zend_class_entry *mosquitto_ce_protocolversion;

#define REGISTER_MOSQUITTO_CONSTANT_CLASS(class_name, classentry_name) \
    INIT_NS_CLASS_ENTRY(classentry_name ## _ce, "Mosquitto", class_name, NULL); \
    mosquitto_ce_ ## classentry_name = zend_register_internal_class(&classentry_name ## _ce TSRMLS_CC); \
    mosquitto_ce_ ## classentry_name  ->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

#define REGISTER_MOSQUITTO_CONST(classentry_name, const_name, value) \
    zend_declare_class_constant_long(mosquitto_ce_ ## classentry_name, const_name, sizeof(const_name)-1, (long)value TSRMLS_CC); \


PHP_MINIT_FUNCTION(mosquitto_constants)
{
    zend_class_entry loglevel_ce, sslverify_ce, protocolversion_ce;

    REGISTER_MOSQUITTO_CONSTANT_CLASS("LogLevel", loglevel);
	REGISTER_MOSQUITTO_CONST(loglevel, "INFO", MOSQ_LOG_INFO);
	REGISTER_MOSQUITTO_CONST(loglevel, "NOTICE", MOSQ_LOG_NOTICE);
	REGISTER_MOSQUITTO_CONST(loglevel, "WARNING", MOSQ_LOG_WARNING);
	REGISTER_MOSQUITTO_CONST(loglevel, "ERR", MOSQ_LOG_ERR);
	REGISTER_MOSQUITTO_CONST(loglevel, "DEBUG", MOSQ_LOG_DEBUG);

    REGISTER_MOSQUITTO_CONSTANT_CLASS("SslVerify", sslverify);
    REGISTER_MOSQUITTO_CONST(sslverify, "NONE", 0);
    REGISTER_MOSQUITTO_CONST(sslverify, "PEER", 1);

    REGISTER_MOSQUITTO_CONSTANT_CLASS("ProtocolVersion", protocolversion);
    REGISTER_MOSQUITTO_CONST(protocolversion, "V31", MQTT_PROTOCOL_V31);
    REGISTER_MOSQUITTO_CONST(protocolversion, "V311", MQTT_PROTOCOL_V311);
	
    return SUCCESS;
}
