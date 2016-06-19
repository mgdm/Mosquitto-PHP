<?php

$c = new Mosquitto\Client();
$c->onConnect(function($code, $message) use ($c) {
    echo "I'm connected\n";
    $c->subscribe('#', 1);
});

$c->onMessage(function($m) {
    var_dump($m);
});

$c->connect('localhost', 1883, 60);

$read = array($c->getSocket());
$write = null;
$except = null;

while (true) {
    $changedSockets = socket_select($read, $write, $except, 30);

    if ($changedSockets === false) {
        echo "Nothing doing\n";
    } else if ($changedSockets > 0) {
        var_dump($changedSockets, $read, $write, $except);
        $c->loop(0);
    }
}

