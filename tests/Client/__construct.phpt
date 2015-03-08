--TEST--
Mosquitto\Client::__construct()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client();
var_dump($client);

$client = new Mosquitto\Client("test-client");
var_dump($client);

$client = new Mosquitto\Client("test-client", true);
var_dump($client);

$client = new Mosquitto\Client("test-client", false);
var_dump($client);

$client = new Mosquitto\Client(null, true);
var_dump($client);

/* Null ID and no clean session should fail */
/* Behaviour varies between OSX and Linux in Mosquitto 1.4 */
try {
    $client = new Mosquitto\Client(null, false);
} catch (Exception $e) {
    echo "Caught exception.";
}

?>
--EXPECTF--
object(Mosquitto\Client)#%d (%d) {
}
object(Mosquitto\Client)#%d (%d) {
}
object(Mosquitto\Client)#%d (%d) {
}
object(Mosquitto\Client)#%d (%d) {
}
object(Mosquitto\Client)#%d (%d) {
}
Caught exception.
