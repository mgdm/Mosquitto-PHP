--TEST--
Mosquitto\Client::loop()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

/* Not connected */
try {
    $client = new Mosquitto\Client;
    $client->loop();
} catch (Mosquitto\Exception $e) {
    var_dump($e->getMessage());
}

/* No params */
$client = new Mosquitto\Client;
$client->connect(TEST_MQTT_HOST);

for ($i = 0; $i < 3; $i++) {
    echo "Looping\n";
    $client->loop();
}

unset ($client);

/* Zero param */
$client = new Mosquitto\Client;
$client->connect(TEST_MQTT_HOST);

for ($i = 0; $i < 3; $i++) {
    echo "Looping\n";
    $client->loop(0);
}

unset ($client);

$client = new Mosquitto\Client;
$client->connect(TEST_MQTT_HOST);

for ($i = 0; $i < 3; $i++) {
    echo "Looping\n";
    $client->loop(100);
}

?>
--EXPECTF--
string(38) "The client is not currently connected."
Looping
Looping
Looping
Looping
Looping
Looping
Looping
Looping
Looping
