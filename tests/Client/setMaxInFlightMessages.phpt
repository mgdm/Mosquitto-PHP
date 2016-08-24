--TEST--
Mosquitto\Client::setMaxInFlightMessages()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client;
$client->setMaxInFlightMessages(10);
var_dump($client);

$client->setMaxInFlightMessages(0);
var_dump($client);

$client->setMaxInFlightMessages("100");
var_dump($client);

try {
    $client->setMaxInFlightMessages("foo");
    var_dump($client);
} catch (Exception $e) {
    var_dump($e->getMessage());
}

try {
    $client->setMaxInFlightMessages(new stdClass);
    var_dump($client);
} catch (Exception $e) {
    var_dump($e->getMessage());
}
?>
--EXPECTF--
object(Mosquitto\Client)#%d (0) {
}
object(Mosquitto\Client)#%d (0) {
}
object(Mosquitto\Client)#%d (0) {
}
string(%d) "Mosquitto\Client::setMaxInFlightMessages() expects parameter 1 to be %s, string given"
string(%d) "Mosquitto\Client::setMaxInFlightMessages() expects parameter 1 to be %s, object given"
