--TEST--
Mosquitto\Client::onConnect()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

try {
    $client = new Mosquitto\Client;
    $client->onConnect('foo');
} catch (Error $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}
unset($client);

$client = new Mosquitto\Client;
$client->onConnect(function() {
    var_dump(func_get_args());
});

$client->connect(TEST_MQTT_HOST);

for ($i = 0; $i < 2; $i++) {
    $client->loop(50);
}
?>
--EXPECTF--
Caught TypeError with code 0 and message: Argument 1 passed to Mosquitto\Client::onConnect() must be callable, string given
array(2) {
  [0]=>
  int(0)
  [1]=>
  string(20) "Connection Accepted."
}
