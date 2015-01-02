--TEST--
Mosquitto\Client::onLog()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

function logger() {
    var_dump(func_get_args());
}

function errorHandler($errno, $errstr, $errfile, $errline) {
    printf("Caught error %d (%s) in %s on line %d\n", $errno, $errstr, $errfile, $errline);
}
set_error_handler('errorHandler');

try {
    $client = new Mosquitto\Client;
    $client->onLog('foo');
} catch (Exception $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}

$client = new Mosquitto\Client;
$client->onLog('logger');
var_dump($client);

$client->connect('localhost');
$client->loop();
$client->loop();
?>
--EXPECTF--
Caught error 4096 (Argument 1 passed to Mosquitto\Client::onLog() must be callable, string given) in %s on line %d
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::onLog() expects parameter 1 to be a valid callback, function 'foo' not found or invalid function name
object(Mosquitto\Client)#%d (%d) {
}
array(2) {
  [0]=>
  int(16)
  [1]=>
  string(%d) "Client %s sending CONNECT"
}
array(2) {
  [0]=>
  int(16)
  [1]=>
  string(%d) "Client %s received CONNACK"
}
array(2) {
  [0]=>
  int(16)
  [1]=>
  string(%d) "Client %s sending DISCONNECT"
}
