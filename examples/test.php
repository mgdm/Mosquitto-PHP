<?php

$client = new Mosquitto\Client();
$client->onConnect('connect');
$client->onDisconnect('disconnect');
$client->onSubscribe('subscribe');
$client->onMessage('message');
$client->connect("localhost", 1883, 5);
$client->onLog('logger');
$client->subscribe('#', 1);

for ($i = 0; $i < 10; $i++) {
    $client->loop();
}

$client->unsubscribe('#');

for ($i = 0; $i < 10; $i++) {
    $client->loop();
}

function connect($r, $message) {
	echo "I got code {$r} and message {$message}\n";
}

function subscribe() {
	echo "Subscribed to a topic\n";
}

function unsubscribe() {
	echo "Unsubscribed from a topic\n";
}

function message($message) {
	printf("Got a message on topic %s with payload:\n%s\n", $message->topic, $message->payload);
}

function disconnect() {
	echo "Disconnected cleanly\n";
}

function logger() {
	var_dump(func_get_args());
}
