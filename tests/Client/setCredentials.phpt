--TEST--
Mosquitto\Client::setCredentials()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client;

$client->setCredentials('foo', 'bar');
var_dump($client);

$client->setCredentials(null, null);
var_dump($client);

$client->setCredentials('foo', null);
var_dump($client);

$client->setCredentials(null, 'foo');
var_dump($client);

try {
    $client->setCredentials(new stdClass, 'foo');
    var_dump($client);
} catch (Exception $e) {
    var_dump($e->getMessage());
}

try {
    $client->setCredentials('foo', new stdClass);
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
object(Mosquitto\Client)#%d (0) {
}
string(81) "Mosquitto\Client::setCredentials() expects parameter 1 to be string, object given"
string(81) "Mosquitto\Client::setCredentials() expects parameter 2 to be string, object given"
