--TEST--
Mosquitto\Client::onLog()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');
?>
--EXPECTF--
Stuff