--TEST--
Mosquitto\Message::tokeniseTopic()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
use Mosquitto\Message;
include(dirname(__DIR__) . '/setup.php');

var_dump(Message::tokeniseTopic('foo/bar'));
var_dump(Message::tokeniseTopic('/foo/bar'));
var_dump(Message::tokeniseTopic('#'));
var_dump(Message::tokeniseTopic('foo/+/bar'));
var_dump(Message::tokeniseTopic(NULL));

try {
    var_dump(Message::tokeniseTopic(new stdClass));
} catch (Exception $e) {
    var_dump($e->getMessage());
}
?>
--EXPECTF--
array(2) {
  [0]=>
  string(3) "foo"
  [1]=>
  string(3) "bar"
}
array(3) {
  [0]=>
  NULL
  [1]=>
  string(3) "foo"
  [2]=>
  string(3) "bar"
}
array(1) {
  [0]=>
  string(1) "#"
}
array(3) {
  [0]=>
  string(3) "foo"
  [1]=>
  string(1) "+"
  [2]=>
  string(3) "bar"
}
array(1) {
  [0]=>
  NULL
}
string(%d) "%s expects parameter 1 to be string, object given"
