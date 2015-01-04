--TEST--
Mosquitto\Client::onConnect()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

try {
    $client = new Mosquitto\Client;
    $client->onMessage('foo');
} catch (Exception $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}
unset($client);

$client = new Mosquitto\Client;
$client->onConnect(function() {
    var_dump(func_get_args());
});

$client->connect(TEST_MQTT_HOST);

for ($i = 0; $i < 2; $i++) {
    $client->loop();
}
?>
--EXPECTF--
Caught error 4096 (Argument 1 passed to Mosquitto\Client::onMessage() must be callable, string given) in %s on line 6
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::onMessage() expects parameter 1 to be a valid callback, function 'foo' not found or invalid function name
array(2) {
  [0]=>
  int(0)
  [1]=>
  string(20) "Connection Accepted."
}
