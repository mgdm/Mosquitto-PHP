<?php

$client = new Mosquitto\Client("test");
$client->connect("localhost", 1883, 5, 'callback');
$client->loop();

function callback($r) {
	echo "I got code {$r}\n";
}
