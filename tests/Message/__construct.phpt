--TEST--
Mosquitto\Message::__construct()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$message = new Mosquitto\Message;
var_dump($message);

$message->topic = "Hello";
var_dump($message);

$message->payload = "Hello world";
var_dump($message);

$message->topic = false;
var_dump($message);

$message->topic = new stdClass;
var_dump($message);

$message->payload = false;
var_dump($message);

$message->payload = new stdClass;
var_dump($message);
?>
--EXPECTF--
object(Mosquitto\Message)#1 (5) {
  ["mid"]=>
  int(%d)
  ["topic"]=>
  NULL
  ["payload"]=>
  string(0) ""
  ["qos"]=>
  int(0)
  ["retain"]=>
  bool(false)
}
object(Mosquitto\Message)#1 (5) {
  ["mid"]=>
  int(%d)
  ["topic"]=>
  string(5) "Hello"
  ["payload"]=>
  string(0) ""
  ["qos"]=>
  int(0)
  ["retain"]=>
  bool(false)
}
object(Mosquitto\Message)#1 (5) {
  ["mid"]=>
  int(%d)
  ["topic"]=>
  string(5) "Hello"
  ["payload"]=>
  string(11) "Hello world"
  ["qos"]=>
  int(0)
  ["retain"]=>
  bool(false)
}
object(Mosquitto\Message)#1 (5) {
  ["mid"]=>
  int(%d)
  ["topic"]=>
  string(0) ""
  ["payload"]=>
  string(11) "Hello world"
  ["qos"]=>
  int(0)
  ["retain"]=>
  bool(false)
}
Caught error 4096 (Object of class stdClass could not be converted to string) in %s on line 16
%Aobject(Mosquitto\Message)#1 (5) {
  ["mid"]=>
  int(%d)
  ["topic"]=>
  string(6) "Object"
  ["payload"]=>
  string(11) "Hello world"
  ["qos"]=>
  int(0)
  ["retain"]=>
  bool(false)
}
object(Mosquitto\Message)#1 (5) {
  ["mid"]=>
  int(%d)
  ["topic"]=>
  string(6) "Object"
  ["payload"]=>
  string(0) ""
  ["qos"]=>
  int(0)
  ["retain"]=>
  bool(false)
}
Caught error 4096 (Object of class stdClass could not be converted to string) in %s on line 22
%Aobject(Mosquitto\Message)#1 (5) {
  ["mid"]=>
  int(%d)
  ["topic"]=>
  string(6) "Object"
  ["payload"]=>
  string(6) "Object"
  ["qos"]=>
  int(0)
  ["retain"]=>
  bool(false)
}
