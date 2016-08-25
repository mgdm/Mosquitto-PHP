--TEST--
Mosquitto\Client::setReconnectDelay()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client;
$client->setReconnectDelay(10);
var_dump($client);

$client->setReconnectDelay(0);
var_dump($client);

$client->setReconnectDelay("100");
var_dump($client);

try {
    $client->setReconnectDelay("foo");
    var_dump($client);
} catch (Exception $e) {
    var_dump($e->getMessage());
}

try {
    $client->setReconnectDelay(new stdClass);
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
string(%d) "Mosquitto\Client::setReconnectDelay() expects parameter 1 to be %s, string given"
string(%d) "Mosquitto\Client::setReconnectDelay() expects parameter 1 to be %s, object given"
