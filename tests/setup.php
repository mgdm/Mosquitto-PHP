<?php
$defaults = array(
    'TEST_MQTT_HOST' => 'localhost',
    'TEST_MQTT_PORT' => 1883,
    'TEST_MQTT_TLS_PORT' => 8883,
    'TEST_MQTT_TLS_CERT_PORT' => 8884,
);

foreach ($defaults as $index => $default) {
    if (getenv($index)) {
        define($index, getenv($index));
    } else {
        define($index, $default);
    }
}

function errorHandler($errno, $errstr, $errfile, $errline) {
    printf("Caught error %d (%s) in %s on line %d\n", $errno, $errstr, $errfile, $errline);
}
set_error_handler('errorHandler');

function writeException(Exception $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}
