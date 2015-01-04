--TEST--
Mosquitto\Client::onSubscribe()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

try {
    $client = new Mosquitto\Client;
    $client->onSubscribe('foo');
} catch (Exception $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}
unset($client);

$client = new Mosquitto\Client;
$client->onSubscribe(function() use ($client) {
    var_dump(func_get_args());
    $client->disconnect();
});

$client->onConnect(function() use ($client) {
    $client->subscribe('#', 0);
});

$client->connect(TEST_MQTT_HOST);
$client->loopForever();

?>
--EXPECTF--
Caught error 4096 (Argument 1 passed to Mosquitto\Client::onSubscribe() must be callable, string given) in /Users/michael/Code/Mosquitto-PHP/tests/Client/onSubscribe.php on line 6
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::onSubscribe() expects parameter 1 to be a valid callback, function 'foo' not found or invalid function name
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(1)
  [2]=>
  int(0)
}
