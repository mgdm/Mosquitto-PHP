--TEST--
Mosquitto\Client::connect()
--SKIPIF--
if (!extension_loaded('mosquitto')) die('skip - Mosquitto extension not available');
--FILE--
<?php
include(dirname(__DIR__) . '/setup.php');

/* No parameters */
try {
    $client = new Mosquitto\Client();
    $client->connect();
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* Invalid hostname */
try {
    $client = new Mosquitto\Client();
    $client->connect(false);
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* Invalid hostname */
try {
    $client = new Mosquitto\Client();
    $client->connect(":^(%^*:");
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* Invalid port */
try {
    $client = new Mosquitto\Client();
    $client->connect(TEST_MQTT_HOST, 0);
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* Invalid port */
try {
    $client = new Mosquitto\Client();
    $client->connect(TEST_MQTT_HOST, new stdClass);
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* Invalid keepalive */
try {
    $client = new Mosquitto\Client();
    $client->connect(TEST_MQTT_HOST, TEST_MQTT_PORT, new stdClass);
    var_dump($client);
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* Invalid bind address */
try {
    $client = new Mosquitto\Client();
    $client->connect(TEST_MQTT_HOST, TEST_MQTT_PORT, 0, '^(%%^&*');
    var_dump($client);
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* Zero keepalive (OK) */
try {
    $client = new Mosquitto\Client();
    $client->connect(TEST_MQTT_HOST, TEST_MQTT_PORT, 0);
    var_dump($client);
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* 10-second keepalive */
try {
    $client = new Mosquitto\Client();
    $client->connect(TEST_MQTT_HOST, TEST_MQTT_PORT, 10);
    var_dump($client);
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* Bind to 127.0.0.1 - should work if connecting to localhost */
try {
    $client = new Mosquitto\Client();
    $client->connect(TEST_MQTT_HOST, TEST_MQTT_PORT, 0, '127.0.0.1');
    var_dump($client);
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

/* Specify just the host */
try {
    $client = new Mosquitto\Client();
    $client->connect(TEST_MQTT_HOST);
    var_dump($client);
} catch (Exception $e) {
    echo $e->getMessage(), "\n";
}

?>
--EXPECTF--
Mosquitto\Client::connect() expects at least 1 parameter, 0 given
%s error.
%s error.
Invalid function arguments provided.
Mosquitto\Client::connect() expects parameter 2 to be %s, object given
Mosquitto\Client::connect() expects parameter 3 to be %s, object given
%s error.
object(Mosquitto\Client)#%d (%d) {
}
object(Mosquitto\Client)#%d (%d) {
}
object(Mosquitto\Client)#%d (%d) {
}
object(Mosquitto\Client)#%d (%d) {
}

