--TEST--
Mosquitto\Client::publish()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client();

try {
    $client->publish();
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish(new stdClass);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish(new stdClass, new stdClass);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish('#');
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish('topic');
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish('topic', 'payload', new stdClass);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish('topic', 'payload', 1, new stdClass);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish('topic', 'payload', 1);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish('topic', 'payload', 1, false);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish('topic', 'payload', 1, true);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->publish('topic', 'payload');
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

unset ($client);
$client = new Mosquitto\Client;
$looping = true;

$client->onConnect(function() use ($client) {
    $client->subscribe('#', 0);
    $client->publish('foo', 'hello', 0);
});

$client->onMessage(function($m) use ($client, &$looping) {
    var_dump($m);
    $client->disconnect();
    $looping = false;
});

$client->connect(TEST_MQTT_HOST);

for ($i = 0; $i < 10; $i++) {
    if (!$looping) break;
    $client->loop(50);
}

?>
--EXPECTF--
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects at least 2 parameters, 0 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects at least 2 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects parameter 1 to be string, object given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects at least 2 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects at least 2 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects parameter 3 to be long, object given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects parameter 4 to be boolean, object given
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
object(Mosquitto\Message)#%d (5) {
  ["mid"]=>
  int(0)
  ["topic"]=>
  string(5) "topic"
  ["payload"]=>
  string(7) "payload"
  ["qos"]=>
  int(0)
  ["retain"]=>
  bool(true)
}
