--TEST--
Mosquitto\Client::exitLoop()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client;
/* Not looping - no effect */
$client->exitLoop();

$client->onConnect(function() use ($client) {
    $client->exitLoop();
});
$client->connect('localhost');
$client->loopForever();
echo "Made it\n";
?>
--EXPECTF--
Made it
