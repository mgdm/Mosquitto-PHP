--TEST--
Mosquitto\Client::setTlsPSK()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client;

try {
    $client->setTlsPSK();
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsPsk('1234567890abcdef');
} catch (Exception $e) {
    writeException($e);
}

try {
    $client->setTlsPsk("This is invalid hex", "Username");
} catch (Exception $e) {
    writeException($e);
}

/* This actually doesn't fail */
try {
    $client->setTlsPsk('1234567890abcdef', 'username', "This is not a cipher string");
} catch (Exception $e) {
    writeException($e);
}

$client->setTlsPsk('1234567890abcdef', 'testuser');
var_dump($client);

$client->setTlsPsk('1234567890abcdef', 'testuser', 'DEFAULT');
var_dump($client);

$client->onConnect(function() use ($client) {
    echo "Connected successfully\n";
    $client->disconnect();
});

$client->connect(TEST_MQTT_HOST, TEST_MQTT_TLS_PSK_PORT);
$client->loopForever();

?>
--EXPECTF--
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setTlsPSK() expects at least 2 parameters, 0 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setTlsPSK() expects at least 2 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Invalid function arguments provided.
object(Mosquitto\Client)#%d (0) {
}
object(Mosquitto\Client)#%d (0) {
}
Connected successfully

