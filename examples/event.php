<?php

$c = new Mosquitto\Client();
$c->onConnect(function($code, $message) {
    echo "I'm connected\n";
});

$c->connect('localhost', 1883, 60);
$c->subscribe('#', 1);
$c->onMessage(function($m) {
    var_dump($m);
});

$socket = $c->getSocket();

$base = new EventBase();
$ev = new Event($base, $socket, Event::READ | Event::PERSIST, 'cb', $base);

function cb($fd, $what, $arg) {
    global $c;
    echo "Triggered\n";
    var_dump(func_get_args());
    $c->loop();
}

$ev->add();
$base->dispatch();
