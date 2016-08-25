--TEST--
Mosquitto\Client::onMessage()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

try {
    $client = new Mosquitto\Client;
    $client->onMessage('foo');
} catch (TypeError $e) {
    printf("Caught %s with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
} catch (Mosquitto\Exception $e) {
    printf("Caught TypeError with code %d and message: %s\n", get_class($e), $e->getCode(), $e->getMessage());
}
unset($client);

$client = new Mosquitto\Client;
var_dump($client);

$client->onMessage(function($m) {
    var_dump($m);
});

$client->connect(TEST_MQTT_HOST);
$client->subscribe('#', 1);

$client2 = new Mosquitto\Client;
$client2->connect(TEST_MQTT_HOST);
$client2->publish('test', 'test', 1);

for ($i = 0; $i < 30; $i++) {
    $client->loop();
    $client2->loop();
}

?>
--EXPECTF--
%ACaught TypeError with code 0 and message: %s
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
