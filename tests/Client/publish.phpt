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
    $client->publish('topic', 'payload', 1, true);
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
    $client->publish('topic', 'payload');
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

$client2 = new Mosquitto\Client();
$looping = true;

$client2->onConnect(function() use ($client2) {
    $client2->subscribe('publish', 0);
    $client2->publish('publish', 'hello', 0);
});

$client2->onMessage(function($m) use ($client2, &$looping) {
    var_dump($m);
    $client2->disconnect();
    $looping = false;
});

$client2->connect(TEST_MQTT_HOST);

for ($i = 0; $i < 10; $i++) {
    if (!$looping) break;
    $client2->loop(50);
}

?>
--EXPECTF--
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects at least 2 parameters, 0 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects at least 2 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects parameter 1 to be string, object given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects at least 2 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects at least 2 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects parameter 3 to be %s, object given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::publish() expects parameter 4 to be boolean, object given
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
Caught Mosquitto\Exception with code 0 and message: The client is not currently connected.
object(Mosquitto\Message)#%d (5) {
  ["mid"]=>
  int(%d)
  ["topic"]=>
  string(7) "publish"
  ["payload"]=>
  string(5) "hello"
  ["qos"]=>
  int(0)
  ["retain"]=>
  bool(false)
}
