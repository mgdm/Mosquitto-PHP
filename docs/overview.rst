========
Overview
========

Requirements
============

* PHP 5.3 or newer, including PHP 7+
* libmosquitto 1.2.x or later

Installation
============

If you've used a pre-built package to install Mosquitto, you need to make sure you have the development headers installed. On Red Hat-derived systems, this is probably called ``libmosquitto-devel``, and on Debian-based systems it will be ``libmosquitto-dev``.

You may obtain this package using `PECL <http://pecl.php.net>`_::

    pecl install Mosquitto-alpha

Alternatively, you can use the normal extension build process::

    phpize
    ./configure --with-mosquitto=/path/to/libmosquitto
    make
    make install

Then add ``extension=mosquitto.so`` to your ``php.ini``.

The ``--with-mosquitto`` argument is optional, and only required if your
libmosquitto install cannot be found.

General Operation
=================

The underlying library is based on callbacks and event-driven operation. As such, you have to call the ``loop()`` method of the ``Client`` frequently to permit the library to handle the messages in its queues. You can use ``loopForever()`` to ensure that the client handles this itself. Also, you should use the callback functions to ensure that you only attempt to publish after the client has connected, etc. For example, here is how you would correctly publish a QoS=2 message:

.. code-block:: php

    <?php

    $c = new Mosquitto\Client;
    $c->onConnect(function() use ($c) {
        $c->publish('mgdm/test', 'Hello', 2);
        $c->disconnect();
    });

    $c->connect('test.mosquitto.org');

    // Loop around to permit the library to do its work
    // This function will call the callback defined in `onConnect()`
    // and disconnect cleanly when the message has been sent
    $c->loopForever();

    echo "Finished\n";


