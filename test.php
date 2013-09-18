<?php

$client = new Mosquitto\Client("test");
$client->connect("localhost", 1883, 5, function($r) { echo "Got code {$r}\n"; });
$client->loop();

