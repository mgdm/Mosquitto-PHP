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
} catch (TypeError $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
} catch (Mosquitto\Exception $e) {
    printf("Caught TypeError with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
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
%ACaught TypeError with code 0 and message: %s
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(1)
  [2]=>
  int(0)
}
