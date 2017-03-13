--TEST--
Mosquitto\Client::onLog()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

function logger() {
    var_dump(func_get_args());
}

try {
    $client = new Mosquitto\Client;
    $client->onLog('foo');
} catch (TypeError $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
} catch (Mosquitto\Exception $e) {
    printf("Caught TypeError with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}

$client = new Mosquitto\Client;
$client->onLog('logger');
var_dump($client);

$client->connect(TEST_MQTT_HOST);
$client->loop(50);
$client->loop(50);
?>
--EXPECTF--
%ACaught TypeError with code 0 and message: %s
object(Mosquitto\Client)#%d (%d) {
}
array(2) {
  [0]=>
  int(16)
  [1]=>
  string(%d) "Client %s sending CONNECT"
}
array(2) {
  [0]=>
  int(16)
  [1]=>
  string(%d) "Client %s received CONNACK%S"
}
array(2) {
  [0]=>
  int(16)
  [1]=>
  string(%d) "Client %s sending DISCONNECT"
}
