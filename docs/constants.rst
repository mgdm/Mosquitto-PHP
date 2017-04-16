=================
Constants
=================

.. php:namespace:: Mosquitto

.. php:class:: LogLevel

    Log levels for use with the ``onLog`` callback.

    .. php:const:: DEBUG

        Identifies a debug-level log message.

    .. php:const:: INFO

        Identifies an info-level log message.

    .. php:const:: NOTICE

        Identifies a notice-level log message.

    .. php:const:: WARNING

        Identifies a warning-level log message.

    .. php:const:: ERR

        Identifies an error-level log message.

.. php:class:: SslVerify

    .. php:const:: NONE

        Do not verify the identity of the server, thus making the connection insecure. Do not use this in production.

    .. php:const:: INFO

        Verify the identity of the server.


.. php:class:: ProtocolVersion

    .. php:const:: V31

        Use MQTT v3.1 when connecting to the server.
    
    .. php:const:: V311

        Use MQTT v3.1.1 when connecting to the server.
