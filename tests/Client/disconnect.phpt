--TEST--
Mosquitto\Client::disconnect()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

try {
    $client = new Mosquitto\Client();
    var_dump($client->disconnect());
} catch (Exception $e) {
    var_dump($e->getMessage());
}

$client = new Mosquitto\Client();

$client->onConnect(function() use ($client) {
    echo "Connected\n";
    $client->disconnect();
});

$client->onDisconnect(function() {
    echo "Disconnected\n";
});

$client->connect(TEST_MQTT_HOST);
$client->loopForever();

?>
--EXPECTF--
string(38) "The client is not currently connected."
Connected
Disconnected

