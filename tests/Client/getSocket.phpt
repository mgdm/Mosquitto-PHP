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

try {
    var_dump($client->getSocket());
} catch (Mosquitto\Exception $e) {
    var_dump($e->getMessage());
}

$client->connect(TEST_MQTT_HOST);
var_dump($client->getSocket());
?>
--EXPECTF--
string(32) "Unable to create socket resource"
resource(%d) of type (Socket)
