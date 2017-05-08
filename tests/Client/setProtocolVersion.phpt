--TEST--
Mosquitto\Client::setProtocolVersion()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client;
$client->setProtocolVersion(Mosquitto\ProtocolVersion::V31);
var_dump($client);

$client->setProtocolVersion(Mosquitto\ProtocolVersion::V311);
var_dump($client);

try {
    $client->setProtocolVersion(100);
    var_dump($client);
} catch (Exception $e) {
    var_dump($e->getMessage());
}

try {
    $client->setProtocolVersion("foo");
    var_dump($client);
} catch (Exception $e) {
    var_dump($e->getMessage());
}

try {
    $client->setProtocolVersion(new stdClass);
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
string(%d) "Invalid function arguments provided."
string(%d) "Mosquitto\Client::setProtocolVersion() expects parameter 1 to be %s, string given"
string(%d) "Mosquitto\Client::setProtocolVersion() expects parameter 1 to be %s, object given"
