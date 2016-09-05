=================
Mosquitto\\Client
=================

.. php:namespace:: Mosquitto

.. php:class:: Client

    This is the main Mosquitto client.

    .. php:method:: __construct([$id = null, $cleanSession = true])

        Construct a new Client instance.

        :param string $id: The client ID. If omitted or ``null``, one will be generated at random.
        :param boolean $cleanSession: Set to ``true`` to instruct the broker to clean all messages and subscriptions on disconnect. Must be true if the ``$id`` parameter is ``null``.


    .. php:method:: setCredentials($username, $password)

        Set the username and password to use on connecting to the broker. Must be called before ``connect()``.

        :param string $username: Username to supply to the broker
        :param string $password: Password to supply to the broker

    .. php:method:: setTlsCertificates($caPath, $certFile, $keyFile, $password)

        Configure the client for certificate based SSL/TLS support.  Must be called before connect(). Cannot be used in conjunction with setTlsPSK().

        Define the Certificate Authority certificates to be trusted (ie. the server certificate must be signed with one of these certificates) using cafile.  If the server you are connecting to requires clients to provide a certificate, define certfile and keyfile with your client certificate and private key.  If your private key is encrypted, provide the password as the fourth parameter, or you will have to enter the password at the command line.

        :param string $username: Path to the PEM encoded trusted CA certificate files, or to a directory containing them.
        :param string $certFile: Path to the PEM encoded certificate file for this client. Optional.
        :param string $keyFile: Path to a file containing the PEM encoded private key for this client. Required if certfile is set.
        :param string $password: The password for the keyfile, if it is encrypted. If null, the password will be asked for on the command line.

    .. php:method:: setTlsInsecure($value)

        Configure verification of the server hostname in the server certificate.  If value is set to true, it is impossible to guarantee that the host you are connecting to is not impersonating your server. Do not use this function in a real system. Must be called before connect().

        :param boolean $value: If set to false, the default, certificate hostname checking is performed.  If set to true, no hostname checking is performed and the connection is insecure.

    .. php:method:: setTlsOptions($certReqs, $tlsVersion, $ciphers)

        Set advanced SSL/TLS options.  Must be called before ``connect()``.

        :param int $certReqs: Whether or not to verify the server. Can be ``Mosquitto\\Client::SSL_VERIFY_NONE``, to disable certificate verification, or ``Mosquitto\Client::SSL_VERIFY_PEER`` (the default), to verify the server certificate.
        :param string $tlsVersion: The TLS version to use. If ``null``, a default is used. The default value depends on the version of OpenSSL the library was compiled against. Available options on OpenSSL >= 1.0.1 are 'tlsv1.2', 'tlsv1.1' and 'tlsv1'.
        :param string $cipers: A string describing the ciphers available for use. See the ``openssl ciphers`` tool for more information. If ``null``, the default set will be used.

    .. php:method:: setTlsPSK()

        Configure the client for pre-shared-key based TLS support.  Must be called before connect(). Cannot be used in conjunction with setTlsCertificates.

        :param string $psk: The pre-shared key in hex format with no leading "0x".
        :param string $identity: The identity of this client. May be used as the username depending on server settings.
        :param string $cipers: Optional. A string describing the ciphers available for use. See the ``openssl ciphers`` tool for more information. If ``null``, the default set will be used.

    .. php:method:: setWill()

        Set the client "last will and testament", which will be sent on an unclean disconnection from the broker. Must be called before connect().

        :param string $topic: The topic on which to publish the will.
        :param string $payload: The data to send.
        :param int $qos: Optional. Default 0. Integer 0, 1, or 2 indicating the Quality of Service to be used.
        :param boolean $retain: Optional. Default false. If true, the message will be retained.

    .. php:method:: clearWill()

        Remove a previously-set will. No parameters.

    .. php:method:: setReconnectDelay()

        Control the behaviour of the client when it has unexpectedly disconnected in loopForever.  The default behaviour if this method is not used is to repeatedly attempt to reconnect with a delay of 1 second until the connection succeeds.

        :param int $reconnect_delay: Set delay between successive reconnection attempts.
        :param int $reconnect_delay: Set max delay between successive reconnection attempts when exponential backoff is enabled
        :param bool $exponential_backoff: Enable exponential backoff

    .. php:method:: connect()

        Connect to an MQTT broker.

        :param string $host: Hostname to connect to
        :param int $port: Optional. Port number to connect to. Defaults to 1883.
        :param int $keepalive: Optional. Number of sections after which the broker should PING the client if no messages have been recieved.
        :param string $interface: Optional. The address or hostname of a local interface to bind to for this connection.

    .. php:method:: disconnect()

        Disconnect from the broker. No parameters.

    .. php:method:: onConnect()

        Set the connect callback.  This is called when the broker sends a CONNACK message in response to a connection.

        :param callback $callback: The callback

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

    .. php:method:: onDisconnect()

        Set the disconnect callback. This is called when the broker has received the DISCONNECT command and has disconnected the client.

        :param callback $callback: The callback

        The callback should take parameters of the form:

        :param int $rc: Reason for the disconnection. 0 means the client requested it. Any other value indicates an unexpected disconnection.

    .. php:method:: onLog()

        Set the logging callback.

        :param callback $callback: The callback

        The callback should take parameters of the form:

        :param int $level: The log message level from the values below
        :param string $str: The message string.

        The level can be one of:

        * ``Mosquitto\Client::LOG_DEBUG``
        * ``Mosquitto\Client::LOG_INFO``
        * ``Mosquitto\Client::LOG_NOTICE``
        * ``Mosquitto\Client::LOG_WARNING``
        * ``Mosquitto\Client::LOG_ERR``

    .. php:method:: onSubscribe()

        Set the subscribe callback.  This is called when the broker responds to a subscription request.

        :param callback $callback: The callback

        The callback should take parameters of the form:

        :param int $mid: Message ID of the subscribe message
        :param int $qos_count: Number of granted subscriptions

        This function needs to return the granted QoS for each subscription, but currently cannot.

    .. php:method:: onUnsubscribe()

        Set the unsubscribe callback.  This is called when the broker responds to a unsubscribe request.

        :param callback $callback: The callback

        The callback should take parameters of the form:

        :param int $mid: Message ID of the unsubscribe message

    .. php:method:: onMessage()

        Set the message callback.  This is called when a message is received from the broker.

        :param callback $callback: The callback

        The callback should take parameters of the form:

        :param Mosquitto\Message $message: A Message object containing the message data

    .. php:method:: onPublish()

        Set the publish callback. This is called when a message is published by the client itself.

        **Warning**: this may be called before the method ``Mosquitto\Client::publish()`` returns the message id, so, you need to create a queue to deal with the MID list.

        :param callback $callback: The callback

        The callback should take parameters of the form:

        :param int $mid: the message id returned by ``Mosquitto\Client::publish()``

    .. php:method:: setMaxInFlightMessages()

        Set the number of QoS 1 and 2 messages that can be “in flight” at one time.  An in flight message is part way through its delivery flow.  Attempts to send further messages with publish() will result in the messages being queued until the number of in flight messages reduces.

        Set to 0 for no maximum.

        :param int $maxInFlightMessages: The maximum

    .. php:method:: setMessageRetry($messageRetryPeriod)

        Set the number of seconds to wait before retrying messages.  This applies to publish messages with QoS>0.  May be called at any time.

        :param int $messageRetryPeriod: The retry period

    .. php:method:: publish($topic, $payload, $qos = 0, $retain = false)

        Publish a message on a given topic.

        :param string $topic: The topic to publish on
        :param string $payload: The message payload
        :param int $qos: Integer value 0, 1 or 2 indicating the QoS for this message
        :param boolean $retain: If true, make this message retained
        :returntype: int
        :returns: The message ID returned by the broker. **Warning**: the message ID is not unique.

    .. php:method:: subscribe()

        Subscribe to a topic.

        :param string $topic: The topic.
        :param int $qos: The QoS to request for this subscription

        Returns the message ID of the subscription message, so this can be matched up in the onSubscribe callback.

    .. php:method:: unsubscribe()

        Unsubscribe from a topic.

        :param string $topic: The topic.
        :param int $qos: The QoS to request for this subscription

        Returns the message ID of the subscription message, so this can be matched up in the onUnsubscribe callback.

    .. php:method:: loop()

        The main network loop for the client.  You must call this frequently in order to keep communications between the client and broker working.  If incoming data is present it will then be processed.  Outgoing commands, from e.g.  publish(), are normally sent immediately that their function is called, but this is not always possible. loop() will also attempt to send any remaining outgoing messages, which also includes commands that are part of the flow for messages with QoS>0.  

        :param int $timeout: Optional. Number of milliseconds to wait for network activity. Pass 0 for instant timeout. Defaults to 1000.
        :param int $max_packets: Currently unused.

    .. php:method:: loopForever()

        Call loop() in an infinite blocking loop. Callbacks will be called as required.  This will handle reconnecting if the connection is lost. Call disconnect() in a callback to return from the loop.

        Note: exceptions thrown in callbacks do not currently cause the loop to exit. To work around this, use loop() and wrap your own loop structure around it such as a while().  

        :param int $timeout: Optional. Number of milliseconds to wait for network activity. Pass 0 for instant timeout. Defaults to 1000.
        :param int $max_packets: Currently unused.
        
