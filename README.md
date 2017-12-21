# Mosquitto-PHP

This is an extension to allow using the [Eclipse Mosquitto™ MQTT client library](http://mosquitto.org) with PHP. See the `examples/` directory for usage.

[![Build Status](https://travis-ci.org/mgdm/Mosquitto-PHP.svg?branch=master)](https://travis-ci.org/mgdm/Mosquitto-PHP)

## PHP 7 support

Thanks to [Sara Golemon](https://twitter.com/SaraMG) this extension now supports PHP 7. I would be grateful if anyone using PHP 7 could test it and let me know how it works out.

## Requirements

* PHP 5.3+
* libmosquitto 1.2.x or later
* Linux or Mac OS X. I do not have a Windows machine handy, though patches or
  pull requests are of course very welcome!

## Installation

If you've used a pre-built package to install Mosquitto, you need to make sure you have the development headers installed. On Red Hat-derived systems, this is probably called `libmosquitto-devel`, and on Debian-based systems it will be `libmosquitto-dev`.

You may obtain this package using [PECL](http://pecl.php.net):

````
pecl install Mosquitto-alpha
````

Alternatively, you can use the normal extension build process:

````
phpize
./configure --with-mosquitto=/path/to/libmosquitto
make
make install
````

Then add `extension=mosquitto.so` to your `php.ini`.

The `--with-mosquitto` argument is optional, and only required if your
libmosquitto install cannot be found.

## General operation

The underlying library is based on callbacks and asynchronous operation. As such, you have to call the `loop()` method of the `Client` frequently to permit the library to handle the messages in its queues. Also, you should use the callback functions to ensure that you only attempt to publish after the client has connected, etc. For example, here is how you would correctly publish a QoS=2 message:

```php
<?php

$c = new Mosquitto\Client;
$c->onConnect(function() use ($c) {
    $c->publish('mgdm/test', 'Hello', 0);
    $c->disconnect();
});

$c->connect('test.mosquitto.org');
$c->loopForever();

echo "Finished\n";
```

## Documentation

Full documentation is [available on ReadTheDocs](http://mosquitto-php.readthedocs.io/).

