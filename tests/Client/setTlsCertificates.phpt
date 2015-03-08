--TEST--
Mosquitto\Client::setTlsCertificates()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');
$client = new Mosquitto\Client;

try {
    $client->setTlsCertificates();
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsCertificates('/does/not/exist');
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsCertificates(CERTIFICATE_DIR . 'ca.crt', '/does/not/exist');
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsCertificates(CERTIFICATE_DIR . 'ca.crt', __FILE__);
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsCertificates(CERTIFICATE_DIR . 'ca.crt', CERTIFICATE_DIR . 'client.crt');
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsCertificates(CERTIFICATE_DIR . 'ca.crt', CERTIFICATE_DIR . 'client.crt', CERTIFICATE_DIR . 'client.key');
} catch (Exception $e) {
    writeException($e);
}

$client->setTlsCertificates(CERTIFICATE_DIR . 'ca.crt');
//$client->setTlsCertificates(CERTIFICATE_DIR . 'ca.crt', CERTIFICATE_DIR . 'client.crt', CERTIFICATE_DIR . 'client-enc.key', 'Mosquitto-PHP');

$client->onConnect(function() {
    echo "Connected successfully\n";
});

$client->connect(TEST_MQTT_HOST, TEST_MQTT_TLS_PORT);

for ($i = 0; $i < 5; $i++) {
    $client->loop();
}

$client->disconnect();
unset($client);

$client2 = new Mosquitto\Client;
$client2->onConnect(function() {
    echo "Connected successfully\n";
});

$client2->setTlsCertificates(CERTIFICATE_DIR . 'ca.crt', CERTIFICATE_DIR . 'client.crt', CERTIFICATE_DIR . 'client-enc.key', 'Mosquitto-PHP');

$client2->connect(TEST_MQTT_HOST, TEST_MQTT_TLS_CERT_PORT);

for ($i = 0; $i < 5; $i++) {
    $client2->loop();
}

?>
--EXPECTF--
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setTlsCertificates() expects at least 1 parameter, 0 given
Caught Mosquitto\Exception with code 0 and message: Invalid function arguments provided.
Caught Mosquitto\Exception with code 0 and message: Invalid function arguments provided.
Caught Mosquitto\Exception with code 0 and message: Invalid function arguments provided.
Caught Mosquitto\Exception with code 0 and message: Invalid function arguments provided.
Connected successfully
Connected successfully

