<?php

$client = new Mosquitto\Client('asyncphp');
$client->onConnect('connect');
$client->onDisconnect('disconnect');
#$client->onSubscribe('subscribe');
$client->onMessage('message');
$client->onLog('logger');
$client->connectAsync("localhost", 1883, 60);
$client->loopStart();
$client->subscribe('#', 1);
//$client->connect("localhost", 1883, 60);

while (true) {
	echo "running\n";
	sleep(0.1);
}

function connect($r, $message) {
	echo "I got code {$r} and message {$message}\n";
}

function subscribe() {
	echo "Subscribed to a topic\n";
}

function message($message) {
	echo "Got a message on topic {$message->topic} with payload:\n{$message->payload}\n";
}

function disconnect() {
	echo "Disconnected cleanly\n";
}

function logger() {
	var_dump(func_get_args());
}
