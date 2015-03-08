--TEST--
Mosquitto\Client::setWill()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

$client = new Mosquitto\Client();

try {
    $client->setWill();
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill(new stdClass);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill(new stdClass, new stdClass);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill('#');
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill('topic');
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill('topic', 'payload');
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill('topic', 'payload', new stdClass);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill('topic', 'payload', 1);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill('topic', 'payload', 1, new stdClass);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill('topic', 'payload', 1, false);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

try {
    $client->setWill('topic', 'payload', 1, true);
    echo "Done\n";
} catch (Mosquitto\Exception $e) {
    writeException($e);
}

?>
--EXPECTF--
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setWill() expects exactly 4 parameters, 0 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setWill() expects exactly 4 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setWill() expects exactly 4 parameters, 2 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setWill() expects exactly 4 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setWill() expects exactly 4 parameters, 1 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setWill() expects exactly 4 parameters, 2 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setWill() expects exactly 4 parameters, 3 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setWill() expects exactly 4 parameters, 3 given
Caught Mosquitto\Exception with code 0 and message: Mosquitto\Client::setWill() expects parameter 4 to be boolean, object given
Done
Done
