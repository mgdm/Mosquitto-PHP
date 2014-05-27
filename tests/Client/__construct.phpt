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

$client = new Mosquitto\Client(null, false);
var_dump($client);

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

Fatal error: Uncaught exception 'Mosquitto\Exception' with message 'Invalid argument' in %s
Stack trace:
#0 %s: Mosquitto\Client->__construct(NULL, false)
#1 {main}
  thrown in %s
