<?php

$client = new Mosquitto\Client();
$client->onConnect('callback');
$client->onDisconnect('disconnect');
$client->onSubscribe('subscribe');
$client->onMessage('message');
$client->connect("localhost", 1883, 5);
$client->subscribe('/#', 1);

for ($i = 0; $i < 10; $i++) {
	$client->loop();
}
$client->disconnect();
unset($client);

function callback($r) {
	echo "I got code {$r}\n";
}

function subscribe() {
	var_dump(func_get_args());
}

function message($message) {
	var_dump($message->topic, $message->payload);
}

function disconnect() {
	echo "Disconnected cleanly\n";
}
