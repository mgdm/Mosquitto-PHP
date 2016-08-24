--TEST--
Mosquitto\Client::setTlsOptions()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');
$client = new Mosquitto\Client;

try {
    $client->setTlsOptions();
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsOptions(new stdClass);
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsOptions(Mosquitto\Client::SSL_VERIFY_PEER, new stdClass);
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsOptions(Mosquitto\Client::SSL_VERIFY_PEER, 'tlsv1.2', new stdClass);
} catch (Exception $e) {
    writeException($e);
}

$client->setTlsOptions(Mosquitto\Client::SSL_VERIFY_PEER);
$client->setTlsOptions(Mosquitto\Client::SSL_VERIFY_PEER, 'tlsv1.2');
$client->setTlsOptions(Mosquitto\Client::SSL_VERIFY_PEER, 'tlsv1.2', 'DEFAULT');

?>
--EXPECTF--
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setTlsOptions() expects at least 1 parameter, 0 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setTlsOptions() expects parameter 1 to be %s, object given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setTlsOptions() expects parameter 2 to be string, object given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setTlsOptions() expects parameter 3 to be string, object given

