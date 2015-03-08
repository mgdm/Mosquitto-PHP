--TEST--
Mosquitto\Client::setWill()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client();
$client->clearWill();

try {
    $client->clearWill(true);
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

?>
--EXPECTF--
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::clearWill() expects exactly 0 parameters, 1 given
