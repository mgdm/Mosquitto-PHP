<?php
$defaults = array(
    'TEST_MQTT_HOST' => 'test.mosquitto.org',
    'TEST_MQTT_PORT' => 1883,
    'TEST_MQTT_TLS_PORT' => 8883,
    'TEST_MQTT_TLS_CERT_PORT' => 8884,
);

foreach ($defaults as $index => $default) {
    if (isset($_ENV[$index])) {
        define($index, $_ENV[$index]);
    } else {
        define($index, $default);
    }
}

