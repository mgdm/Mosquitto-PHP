--TEST--
Mosquitto\Client::setMessageRetry()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client;
$client->setMessageRetry(10);
var_dump($client);

$client->setMessageRetry(0);
var_dump($client);

$client->setMessageRetry("100");
var_dump($client);

try {
    $client->setMessageRetry("foo");
    var_dump($client);
} catch (Exception $e) {
    var_dump($e->getMessage());
}

try {
    $client->setMessageRetry(new stdClass);
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
string(83) "Mosquitto\Client::setMessageRetry() expects parameter 1 to be integer, string given"
string(83) "Mosquitto\Client::setMessageRetry() expects parameter 1 to be integer, object given"
