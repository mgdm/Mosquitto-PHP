--TEST--
Mosquitto\Client::onUnsubscribe()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

try {
    $client = new Mosquitto\Client;
    $client->onUnsubscribe('foo');
} catch (Exception $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}
unset($client);

$client = new Mosquitto\Client;

$client->onUnsubscribe(function() use ($client) {
    $client->disconnect();
});

$client->onConnect(function() use ($client) {
    $client->subscribe('#', 0);
});

$client->onSubscribe(function() use ($client) {
    var_dump(func_get_args());
    $client->unsubscribe('#');
});

$client->connect(TEST_MQTT_HOST);
$client->loopForever();

?>
--EXPECTF--
Caught error 4096 (Argument 1 passed to Mosquitto\Client::onUnsubscribe() must be callable, string given) in %s on line 6
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::onUnsubscribe() expects parameter 1 to be a valid callback, function 'foo' not found or invalid function name
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(1)
  [2]=>
  int(0)
}
