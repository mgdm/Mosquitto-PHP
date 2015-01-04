--TEST--
Mosquitto\Client::getSocket()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
use Mosquitto\Client;
include(dirname(__DIR__) . '/setup.php');

/* Not connected */
$client = new Client();
var_dump($client->getSocket());

$client->connect(TEST_MQTT_HOST);
var_dump($client->getSocket() > 0);
?>
--EXPECTF--
int(-1)
bool(true)
