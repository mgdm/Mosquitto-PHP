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
} catch (TypeError $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
} catch (Mosquitto\Exception $e) {
    printf("Caught TypeError with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
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

for ($i = 0; $i < 10; $i++) {
    if (!$loop) break;
    $client->loop();
}

?>
--EXPECTF--
%ACaught TypeError with code 0 and message: %s
Triggering disconnect
Disconnected
Disconnected
