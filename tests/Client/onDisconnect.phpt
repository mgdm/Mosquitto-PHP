--TEST--
Mosquitto\Client::onDisconnect()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

try {
    $client = new Mosquitto\Client;
    $client->onDisconnect('foo');
} catch (Exception $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}
unset($client);


$client = new Mosquitto\Client;

$client->onConnect(function() use ($client) {
    echo "Triggering disconnect\n";
    $client->disconnect();
});

$client->onDisconnect(function() {
    echo "Disconnected\n";
});

$client->connect(TEST_MQTT_HOST);
$client->loopForever(50);
unset($client);

/* onDisconnect called when Client is destroyed */
$client = new Mosquitto\Client;
$loop = true;
$client->onDisconnect(function() use (&$loop) {
    $loop = false;
    echo "Disconnected\n";
});

$client->connect(TEST_MQTT_HOST);

for ($i = 0; $i < 5; $i++) {
    if (!$loop) break;
    $client->loop(50);
}

?>
--EXPECTF--
Caught error 4096 (Argument 1 passed to Mosquitto\Client::onDisconnect() must be callable, string given) in %s on line 6
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::onDisconnect() expects parameter 1 to be a valid callback, function 'foo' not found or invalid function name
Triggering disconnect
Disconnected
Disconnected
