=================
Mosquitto\\Client
=================

.. php:namespace:: Mosquitto

.. php:class:: Client

    This is the main Mosquitto client.

    .. php:const:: LOG_DEBUG

        Identifies a debug-level log message

    .. php:const:: LOG_INFO

        Identifies an info-level log message

    .. php:const:: LOG_NOTICE

        Identifies a notice-level log message

    .. php:const:: LOG_WARNING

        Identifies a warning-level log message

    .. php:const:: LOG_ERR

        Identifies an error-level log message

    .. php:const:: SSL_VERIFY_NONE

        Used with :php:meth:`~Client::setTlsInsecure`. Do not verify the identity of the server, thus making the connection insecure.

    .. php:const:: SSL_VERIFY_PEER

        Used with :php:meth:`~Client::setTlsInsecure`. Verify the identity of the server.

    .. php:method:: __construct([$id = null, $cleanSession = true])

        Construct a new Client instance.

        :param string $id: The client ID. If omitted or ``null``, one will be generated at random.
        :param boolean $cleanSession: Set to ``true`` to instruct the broker to clean all messages and subscriptions on disconnect. Must be ``true`` if the ``$id`` parameter is ``null``.


    .. php:method:: setCredentials($username, $password)

        Set the username and password to use on connecting to the broker. Must be called before :php:meth:`~Client::connect`.

        :param string $username: Username to supply to the broker
        :param string $password: Password to supply to the broker

    .. php:method:: setTlsCertificates($caPath[, $certFile, $keyFile, $password])

        Configure the client for certificate based SSL/TLS support.  Must be called before :php:meth:`~Client::connect`. Cannot be used in conjunction with :php:meth:`~Client::setTlsPSK`.

        Define the Certificate Authority certificates to be trusted (ie. the server certificate must be signed with one of these certificates) using ``$caFile``.  If the server you are connecting to requires clients to provide a certificate, define ``$certFile`` and ``$keyFile`` with your client certificate and private key.  If your private key is encrypted, provide the password as the fourth parameter.

        :param string $caPath: Path to the PEM encoded trusted CA certificate files, or to a directory containing them.
        :param string $certFile: Path to the PEM encoded certificate file for this client. Optional.
        :param string $keyFile: Path to a file containing the PEM encoded private key for this client. Required if certfile is set.
        :param string $password: The password for the keyfile, if it is encrypted. If null, the password will be asked for on the command line.

    .. php:method:: setTlsInsecure($value)

        Configure verification of the server hostname in the server certificate.  If ``$value`` is ``true``, it is impossible to guarantee that the host you are connecting to is not impersonating your server. Do not use this function in a real system. Must be called before :php:meth:`~Client::connect`.

        :param boolean $value: If set to false, the default, certificate hostname checking is performed.  If set to ``true``, no hostname checking is performed and the connection is insecure.

    .. php:method:: setTlsOptions($certReqs, $tlsVersion, $ciphers)

        Set advanced SSL/TLS options.  Must be called before :php:meth:`~Client::connect`.

        :param int $certReqs: Whether or not to verify the server. Can be ``Mosquitto\\Client::SSL_VERIFY_NONE``, to disable certificate verification, or ``Mosquitto\Client::SSL_VERIFY_PEER`` (the default), to verify the server certificate.
        :param string $tlsVersion: The TLS version to use. If ``null``, a default is used. The default value depends on the version of OpenSSL the library was compiled against. Available options on OpenSSL >= 1.0.1 are ``tlsv1.2``, ``tlsv1.1`` and ``tlsv1``.
        :param string $cipers: A string describing the ciphers available for use. See the ``openssl ciphers`` tool for more information. If ``null``, the default set will be used.

    .. php:method:: setTlsPSK($psk, $identity[, $ciphers])

        Configure the client for pre-shared-key based TLS support.  Must be called before :php:meth:`~Client::connect`. Cannot be used in conjunction with setTlsCertificates.

        :param string $psk: The pre-shared key in hex format with no leading "0x".
        :param string $identity: The identity of this client. May be used as the username depending on server settings.
        :param string $cipers: Optional. A string describing the ciphers available for use. See the ``openssl ciphers`` tool for more information. If ``null``, the default set will be used.

    .. php:method:: setWill($topic, $payload[, $qos = 0, $retain = false])

        Set the client "last will and testament", which will be sent on an unclean disconnection from the broker. Must be called before :php:meth:`~Client::connect`.

        :param string $topic: The topic on which to publish the will.
        :param string $payload: The data to send.
        :param int $qos: Optional. Default 0. Integer 0, 1, or 2 indicating the Quality of Service to be used.
        :param boolean $retain: Optional. Default false. If ``true``, the message will be retained.

    .. php:method:: clearWill()

        Remove a previously-set will. No parameters.

    .. php:method:: setReconnectDelay($reconnectDelay, $exponentialDelay, $exponentialBackoff)

        Control the behaviour of the client when it has unexpectedly disconnected in :php:meth:`Client::loopForever`.  The default behaviour if this method is not used is to repeatedly attempt to reconnect with a delay of 1 second until the connection succeeds.

        :param int $reconnectDelay: Set delay between successive reconnection attempts.
        :param int $exponentialDelay: Set max delay between successive reconnection attempts when exponential backoff is enabled
        :param bool $exponentialBackoff: Pass ``true`` to enable exponential backoff

    .. php:method:: connect($host[, $port = 1883, $keepalive = 60, $interface = null])

        Connect to an MQTT broker.

        :param string $host: Hostname to connect to
        :param int $port: Optional. Port number to connect to. Defaults to 1883.
        :param int $keepalive: Optional. Number of sections after which the broker should PING the client if no messages have been recieved.
        :param string $interface: Optional. The address or hostname of a local interface to bind to for this connection.

    .. php:method:: disconnect()

        Disconnect from the broker. No parameters.

    .. php:method:: onConnect($callback)

        Set the connect callback.  This is called when the broker sends a CONNACK message in response to a connection.

        :param callable $callback: The callback

        The callback should take parameters of the form:

        :param int $rc: Response code from the broker.
        :param string $message: String description of the response code.

        Response codes are as follows:
        
        =====    ====
        Code     Meaning
        -----    ----
        0        Success
        1        Connection refused (unacceptable protocol version)
        2        Connection refused (identifier rejected)
        3        Connection refused (broker unavailable )
        4-255    Reserved for future use
        =====    ====

    .. php:method:: onDisconnect($callback)

        Set the disconnect callback. This is called when the broker has received the DISCONNECT command and has disconnected the client.

        :param callable $callback: The callback

        The callback should take parameters of the form:

        :param int $rc: Reason for the disconnection. 0 means the client requested it. Any other value indicates an unexpected disconnection.

    .. php:method:: onLog($callback)

        Set the logging callback.

        :param callable $callback: The callback

        The callback should take parameters of the form:

        :param int $level: The log message level from the values below
        :param string $str: The message string.

        The level can be one of:

        * :php:const:`Client::LOG_DEBUG`
        * :php:const:`Client::LOG_INFO`
        * :php:const:`Client::LOG_NOTICE`
        * :php:const:`Client::LOG_WARNING`
        * :php:const:`Client::LOG_ERR`

    .. php:method:: onSubscribe($callback)

        Set the subscribe callback.  This is called when the broker responds to a subscription request.

        :param callable $callback: The callback

        The callback should take parameters of the form:

        :param int $mid: Message ID of the subscribe message
        :param int $qosCount: Number of granted subscriptions

        This function needs to return the granted QoS for each subscription, but currently cannot.

    .. php:method:: onUnsubscribe($callback)

        Set the unsubscribe callback.  This is called when the broker responds to a unsubscribe request.

        :param callable $callback: The callback

        The callback should take parameters of the form:

        :param int $mid: Message ID of the unsubscribe message

    .. php:method:: onMessage($callback)

        Set the message callback.  This is called when a message is received from the broker.

        :param callable $callback: The callback

        The callback should take parameters of the form:

        :param :php:class:`Message` $message: A :php:class:`Message` object containing the message data

    .. php:method:: onPublish($callback)

        Set the publish callback. This is called when a message is published by the client itself.

        **Warning**: this may be called before the method :php:meth:`~Client::publish` returns the message id, so, you need to create a queue to deal with the MID list.

        :param callable $callback: The callback

        The callback should take parameters of the form:

        :param int $mid: the message id returned by :php:meth:`~Client::publish`

    .. php:method:: setMaxInFlightMessages($maxInFlightMessages)

        Set the number of QoS 1 and 2 messages that can be “in flight” at one time.  An in flight message is part way through its delivery flow.  Attempts to send further messages with :php:meth:`~Client::publish` will result in the messages being queued until the number of in flight messages reduces.

        Set to 0 for no maximum.

        :param int $maxInFlightMessages: The maximum

    .. php:method:: setMessageRetry($messageRetryPeriod)

        Set the number of seconds to wait before retrying messages.  This applies to publishing messages with QoS>0.  May be called at any time.

        :param int $messageRetryPeriod: The retry period

    .. php:method:: publish($topic, $payload[, $qos = 0, $retain = false])

        Publish a message on a given topic.

        :param string $topic: The topic to publish on
        :param string $payload: The message payload
        :param int $qos: Integer value 0, 1 or 2 indicating the QoS for this message
        :param boolean $retain: If ``true``, retain this message
        :returns: The message ID returned by the broker. **Warning**: the message ID is not unique.
        :returntype: int

    .. php:method:: subscribe($topic, $qos)

        Subscribe to a topic.

        :param string $topic: The topic.
        :param int $qos: The QoS to request for this subscription

        :returns: The message ID of the subscription message, so this can be matched up in the :php:meth:`~Client::onSubscribe` callback.
        :returntype: int

    .. php:method:: unsubscribe()

        Unsubscribe from a topic.

        :param string $topic: The topic.
        :param int $qos: The QoS to request for this subscription

        :returns: the message ID of the subscription message, so this can be matched up in the :php:meth:`~Client::onUnsubscribe` callback.
        :returntype: int

    .. php:method:: loop([$timeout = 1000])

        The main network loop for the client.  You must call this frequently in order to keep communications between the client and broker working.  If incoming data is present it will then be processed.  Outgoing commands, from e.g.  :php:meth:`~Client::publish`, are normally sent immediately that their function is called, but this is not always possible. :php:meth:`~Client::loop` will also attempt to send any remaining outgoing messages, which also includes commands that are part of the flow for messages with QoS>0.  

        :param int $timeout: Optional. Number of milliseconds to wait for network activity. Pass 0 for instant timeout. Defaults to 1000.

    .. php:method:: loopForever([$timeout = 1000])

        Call loop() in an infinite blocking loop. Callbacks will be called as required.  This will handle reconnecting if the connection is lost. Call :php:meth:`~Client::disconnect` in a callback to disconnect and return from the loop. Alternatively, call :php:meth:`~Client::exitLoop` to exit the loop without disconnecting. You will need to re-enter the loop again afterwards to maintain the connection.

        :param int $timeout: Optional. Number of milliseconds to wait for network activity. Pass 0 for instant timeout. Defaults to 1000.
        
    .. php:method:: exitLoop()

       Exit the :php:meth:`~Client::loopForever` event loop without disconnecting. You will need to re-enter the loop afterwards in order to maintain the connection.
