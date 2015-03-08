--TEST--
Mosquitto\Client::unsubscribe()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

/* No params */
try {
    $client = new Mosquitto\Client;
    $client->unsubscribe();
} catch (Exception $e) {
    writeException($e);
}

/* Null param */
try {
    $client = new Mosquitto\Client;
    $client->unsubscribe(null);
} catch (Exception $e) {
    writeException($e);
}

/* One param */
try {
    $client = new Mosquitto\Client;
    $client->unsubscribe('#');
} catch (Exception $e) {
    writeException($e);
}

/* Daft params */
try {
    $client = new Mosquitto\Client;
    $client->unsubscribe(new stdClass);
} catch (Exception $e) {
    writeException($e);
}

$client = new Mosquitto\Client;

$client->onConnect(function() use ($client) {
    $client->subscribe('#', 0);
});

$client->onSubscribe(function() use ($client) {
    $client->unsubscribe('#');
});

$client->onUnsubscribe(function() use ($client) {
    var_dump(func_get_args());
    $client->disconnect();
});

$client->connect(TEST_MQTT_HOST);
$client->loopForever();
?>
--EXPECTF--
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::unsubscribe() expects exactly 1 parameter, 0 given
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::unsubscribe() expects parameter 1 to be string, object given
array(1) {
  [0]=>
  int(2)
}
