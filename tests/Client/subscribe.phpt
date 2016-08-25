--TEST--
Mosquitto\Client::subscribe()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

/* No params */
try {
    $client = new Mosquitto\Client;
    $client->subscribe();
} catch (Exception $e) {
    writeException($e);
}

/* Null param */
try {
    $client = new Mosquitto\Client;
    $client->subscribe(null);
} catch (Exception $e) {
    writeException($e);
}

/* Only one param */
try {
    $client = new Mosquitto\Client;
    $client->subscribe('#');
} catch (Exception $e) {
    writeException($e);
}

/* Not connected */
try {
    $client = new Mosquitto\Client;
    $client->subscribe('#', 0);
} catch (Exception $e) {
    writeException($e);
}

/* Daft params */
try {
    $client = new Mosquitto\Client;
    $client->subscribe(new stdClass, 0);
} catch (Exception $e) {
    writeException($e);
}

try {
    $client = new Mosquitto\Client;
    $client->subscribe('#', new stdClass);
} catch (Exception $e) {
    writeException($e);
}

$client = new Mosquitto\Client;

$client->onConnect(function() use ($client) {
    $client->subscribe('#', 0);
});

$client->onSubscribe(function() use ($client) {
    var_dump(func_get_args());
    $client->disconnect();
});

$client->connect(TEST_MQTT_HOST);
$client->loopForever();
?>
--EXPECTF--
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::subscribe() expects exactly 2 parameters, 0 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::subscribe() expects exactly 2 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::subscribe() expects exactly 2 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::subscribe() expects parameter 1 to be string, object given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::subscribe() expects parameter 2 to be %s, object given
array(3) {
  [0]=>
  int(%d)
  [1]=>
  int(1)
  [2]=>
  int(0)
}
