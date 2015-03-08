--TEST--
Mosquitto\Message::topicMatchesSub()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
use Mosquitto\Message;
include(dirname(__DIR__) . '/setup.php');

var_dump(Message::topicMatchesSub("foo/bar", "#"));
var_dump(Message::topicMatchesSub("foo/bar", "foo/+"));
var_dump(Message::topicMatchesSub(null, null));
var_dump(Message::topicMatchesSub('', 'foo'));
var_dump(Message::topicMatchesSub('', '#'));
var_dump(Message::topicMatchesSub('foo', ''));

try {
    var_dump(Message::topicMatchesSub(new stdClass, "#"));
} catch (Exception $e) {
    var_dump($e->getMessage());
}

try {
    var_dump(Message::topicMatchesSub("foo", new stdClass));
} catch (Exception $e) {
    var_dump($e->getMessage());
}
?>
--EXPECTF--
bool(true)
bool(true)
bool(false)
bool(false)
bool(false)
bool(false)
string(83) "Mosquitto\Message::topicMatchesSub() expects parameter 1 to be string, object given"
string(83) "Mosquitto\Message::topicMatchesSub() expects parameter 2 to be string, object given"
