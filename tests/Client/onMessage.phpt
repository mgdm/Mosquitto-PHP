--TEST--
Mosquitto\Client::onMessage()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

function errorHandler($errno, $errstr, $errfile, $errline) {
    printf("Caught error %d (%s) in %s on line %d\n", $errno, $errstr, $errfile, $errline);
}
set_error_handler('errorHandler');

try {
    $client = new Mosquitto\Client;
    $client->onMessage('foo');
} catch (Exception $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}
unset($client);

$client = new Mosquitto\Client;
var_dump($client);

$client->onMessage(function($m) {
    var_dump($m);
});

$client->connect('localhost');
$client->subscribe('#', 1);

$client2 = new Mosquitto\Client;
$client2->connect('localhost');
$client2->publish('test', 'test', 1);

for ($i = 0; $i < 3; $i++) {
    $client->loop();
    $client2->loop();
}

?>
--EXPECTF--
Caught error 4096 (Argument 1 passed to Mosquitto\Client::onMessage() must be callable, string given) in %s on line %d
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::onMessage() expects parameter 1 to be a valid callback, function 'foo' not found or invalid function name
object(Mosquitto\Client)#%d (%d) {
}
object(Mosquitto\Message)#%d (%d) {
  ["mid"]=>
  int(%d)
  ["topic"]=>
  string(4) "test"
  ["payload"]=>
  string(4) "test"
  ["qos"]=>
  int(1)
  ["retain"]=>
  bool(false)
}
