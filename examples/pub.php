<?php

$client = new Mosquitto\Client();
$client->onConnect('connect');
$client->onDisconnect('disconnect');
$client->onSubscribe('subscribe');
$client->onMessage('message');
$client->connect("localhost", 1883, 5);
$client->subscribe('/#', 1);

while (true) {
	$client->loop();
	$mid = $client->publish('/hello', "Hello from PHP at " . date('Y-m-d H:i:s'), 1, 0);
	echo "Sent message ID: {$mid}\n";
	$client->loop();

	sleep(2);
}

$client->disconnect();
unset($client);

function connect($r) {
	echo "I got code {$r}\n";
}

function subscribe() {
	echo "Subscribed to a topic\n";
}

function message($message) {
	printf("Got a message ID %d on topic %s with payload:\n%s\n\n", $message->mid, $message->topic, $message->payload);
}

function disconnect() {
	echo "Disconnected cleanly\n";
}
