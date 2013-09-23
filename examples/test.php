<?php

$client = new Mosquitto\Client("test");
$client->onConnect('callback');
$client->onSubscribe('subscribe');
$client->onMessage('message');
$client->connect("localhost", 1883, 5);
$client->subscribe('/#', 1);

while (true) {
	$client->loop();
}

function callback($r) {
	echo "I got code {$r}\n";
}

function subscribe() {
	var_dump(func_get_args());
}

function message($message) {
	var_dump($message->topic, $message->payload);
}
