--TEST--
Mosquitto\Client::loopForever()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

try {
    $client = new Mosquitto\Client;
    $client->loopForever();
} catch (Mosquitto\Exception $e) {
    var_dump($e->getMessage());
}

$client = new Mosquitto\Client;
$client->onConnect(function() use ($client) {
    echo "Exiting loop\n";
    $client->exitLoop();
});

$client->connect('localhost');
$client->loopForever();

?>
--EXPECTF--
string(38) "The client is not currently connected."
Exiting loop
