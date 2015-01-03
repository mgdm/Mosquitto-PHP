--TEST--
Mosquitto\Client::setTlsInsecure()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client;
$client->setTlsInsecure(true);
var_dump($client);

$client->setTlsInsecure(false);
var_dump($client);

try {
$client->setTlsInsecure(new stdClass);
var_dump($client);
} catch (Mosquitto\Exception $e) {
    var_dump($e->getMessage());
}

?>
--EXPECTF--
object(Mosquitto\Client)#%d (0) {
}
object(Mosquitto\Client)#%d (0) {
}
string(82) "Mosquitto\Client::setTlsInsecure() expects parameter 1 to be boolean, object given"
