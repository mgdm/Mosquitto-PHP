<?php

$client = new Mosquitto\Client;
$client->onConnect(function() use ($client) {
    $client->subscribe('#', 0);
});

$client->connect('localhost');

while (true) {
    $client->loop();
    $socket = $client->getSocket();
    var_dump($socket);
    $r = $w = $e = [$socket];
    $changedSockets = socket_select($r, $w, $e, 30);
}
