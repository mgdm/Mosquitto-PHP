# Mosquitto-PHP

This is an extension to allow using the [Mosquitto MQTT library](http://mosquitto.org) with PHP. See the examples/ directory for usage.

## Requirements

* PHP 5.3+
* libmosquitto 1.2.1 (this is the only version I have tested)
* Linux or Mac OS X. I do not have a Windows machine handy, though patches or
  pull requests are of course very welcome!

## Installation

You may obtain this package using [my PECL channel](http://pecl.mgdm.net), if you prefer:

````
pecl channel-discover pecl.mgdm.net
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

## Documentation

The classes in this extension are namespaced.

### Class Mosquitto\Client

This is the actual Mosquitto client.

1. [__construct](#__construct) - create a new client
1. [setCredentials](#setCredentials) - set the credentials to use on connection
1. [setWill](#setWill) - set the client will, to be delivered if disconnected
   uncleanly
1. [clearWill](#clearWill) - clear a previously-set will
1. [setReconnectDelay](#setReconnectDelay) - set the behaviour if disconnected
   uncleanly
1. [connect](#connect) - connect to an MQTT broker
1. [disconnect](#disconnect) - disconnect from an MQTT broker
1. [onConnect](#onConnect) - set the connect callback
1. [onDisconnect](#onDisconnect) - set the disconnect callback
1. [onSubscribe](#onSubscribe) - set the subscribe callback
1. [onMessage](#onMessage) - set the callback fired when a message is received
1. [setMaxInFlightMessages](#setMaxInFlightMessages) - set the number of QoS
   1 and 2 messages that can be "in flight" at once
1. [setMessageRetry](#setMessageRetry) - set the number of seconds to wait
   before retrying messages
1. [publish](#publish) - publish a message to a broker
1. [subscribe](#subscribe) - subscribe to a topic
1. [unsubscribe](#unsubscribe) - unsubscribe from a topic
1. [loop](#loop) - The main network loop
1. [loopForever](#loopForever) - run loop() in an infinite blocking loop

#### __construct

Creates a new Client instance.

| Parameter | Type | Description |
| --- | --- | ---- |
| id | string | Client ID. Optional. If not supplied or NULL, one will be generated at random. |
| clean_session | boolean | Set to true to instruct the broker to clean all messages and subscriptions on disconnect. |

#### setCredentials

Set the username and password to use on connecting to the broker. Must be
called before connect().

| Parameter | Type | Description |
| --- | --- | ---- |
| Username | string | Username to supply to the broker |
| Password | string | Password to supply to the broker |

#### setWill

Set the client "last will and testament", which will be sent on an unclean
disconnection from the broker. Must be called before connect().

| Parameter | Type | Description |
| --- | --- | ---- |
| topic | string | The topic on which to publish the will. |
| payload | string | The data to send. |
| qos | int | Optional. Default 0. Integer 0, 1, or 2 indicating the Quality of Service to be used. |
| retain | boolean | Optional. Default false. If true, the message will be retained. |

#### clearWill

Remove a previously-set will. No parameters.

#### setReconnectDelay

Control the behaviour of the client when it has unexpectedly disconnected in
loopForever.  The default behaviour if this method is not used is to
repeatedly attempt to reconnect with a delay of 1 second until the connection
succeeds.

| Parameter | Type | Description |
| --- | --- | ---- |
| reconnect_delay | int | Set delay between successive reconnection attempts. |
| reconnect_delay | int | Set max delay between successive reconnection attempts when exponential backoff is enabled |
| exponential_backoff | bool | Enable exponential backoff |

#### connect

Connect to an MQTT broker.

| Parameter | Type | Description |
| --- | --- | ---- |
| host | string | Hostname to connect to |
| port | int | Optional. Port number to connect to. Defaults to 1883. |
| keepalive | int | Optional. Number of sections after which the broker should PING the client if no messages have been recieved. |
| interface | string | Optional. The address or hostname of a local interface to bind to for this connection. |

#### disconnect

Disconnect from the broker. No parameters.

#### onConnect

Set the connect callback.  This is called when the broker sends a CONNACK message in response to a connection.

| Parameter | Type | Description |
| --- | --- | ---- |
| callback | callback | The callback |

The callback should take parameters of the form:

| Parameter | Type | Description |
| --- | --- | ---- |
| rc | int | Response code from the broker. |

Response codes are as follows:

| Code | Meaning |
| --- | --- |
| 0 | Success |
| 1 | Connection refused (unacceptable protocol version) |
| 2 | Connection refused (identifier rejected) |
| 3 | Connection refused (broker unavailable ) |
| 4-255 | Reserved for future use |

#### onDisconnect

Set the disconnect callback. This is called when the broker has received the
DISCONNECT command and has disconnected the client.

| Parameter | Type | Description |
| --- | --- | ---- |
| callback | callback | The callback |

The callback should take parameters of the form:

| Parameter | Type | Description |
| --- | --- | ---- |
| rc | int | Reason for the disconnection. 0 means the client requested it. Any other value indicates an unexpected disconnection. |

#### onSubscribe

Set the subscribe callback.  This is called when the broker responds to
a subscription request.

| Parameter | Type | Description |
| --- | --- | ---- |
| callback | callback | The callback |

The callback should take parameters of the form:

| Parameter | Type | Description |
| --- | --- | ---- |
| mid | int | Message ID of the subscribe message |
| qos_count | int | Number of granted subscriptions |

This function needs to return the granted QoS for each subscription, but
currently cannot.

#### onUnsubscribe

Set the unsubscribe callback.  This is called when the broker responds to
a unsubscribe request.

| Parameter | Type | Description |
| --- | --- | ---- |
| callback | callback | The callback |

The callback should take parameters of the form:

| Parameter | Type | Description |
| --- | --- | ---- |
| mid | int | Message ID of the unsubscribe message |

#### onMessage

Set the message callback.  This is called when a message is received from the broker.

| Parameter | Type | Description |
| --- | --- | ---- |
| callback | callback | The callback |

The callback should take parameters of the form:

| Parameter | Type | Description |
| --- | --- | ---- |
| message | Mosquitto\Message | A Message object containing the message data |


#### setMaxInFlightMessages

Set the number of QoS 1 and 2 messages that can be “in flight” at one time.  An
in flight message is part way through its delivery flow.  Attempts to send
further messages with publish() will result in the messages being queued until
the number of in flight messages reduces.

Set to 0 for no maximum.

| Parameter | Type | Description |
| --- | --- | ---- |
| max_inflight_messages | int | The maximum |

#### setMessageRetry

Set the number of seconds to wait before retrying messages.  This applies to
publish messages with QoS>0.  May be called at any time.

| Parameter | Type | Description |
| --- | --- | ---- |
| message_retry | int | The retry period |

#### publish

Publish a message on a given topic.

| Parameter | Type | Description |
| --- | --- | ---- |
| topic | string | The topic to publish on |
| payload | string | The message payload |
| qos | int | Integer value 0, 1 or 2 indicating the QoS for this message |
| retain | boolean | If true, make this message retained |

#### subscribe

Subscribe to a topic.

| Parameter | Type | Description |
| --- | --- | ---- |
| topic | string | The topic. |
| qos | The QoS to request for this subscription |

Returns the message ID of the subscription message, so this can be matched up
in the onSubscribe callback.

#### unsubscribe

Unsubscribe from a topic.

| Parameter | Type | Description |
| --- | --- | ---- |
| topic | string | The topic. |
| qos | The QoS to request for this subscription |

Returns the message ID of the subscription message, so this can be matched up
in the onUnsubscribe callback.

#### loop

The main network loop for the client.  You must call this frequently in order
to keep communications between the client and broker working.  If incoming data
is present it will then be processed.  Outgoing commands, from e.g.  publish(),
are normally sent immediately that their function is called, but this is not
always possible. loop() will also attempt to send any remaining outgoing
messages, which also includes commands that are part of the flow for messages
with QoS>0.

| Parameter | Type | Description |
| --- | --- | ---- |
| timeout | int | Optional. Number of milliseconds to wait for network activity. Pass 0 for instant timeout. Defaults to 1000. |
| max_packets | int | Currently unused. |

#### loopForever

Call loop() in an infinite blocking loop. Callbacks will be called as required.
This will handle reconnecting if the connection is lost. Call disconnect() in
a callback to return from the loop.

| Parameter | Type | Description |
| --- | --- | ---- |
| timeout | int | Optional. Number of milliseconds to wait for network activity. Pass 0 for instant timeout. Defaults to 1000. |
| max_packets | int | Currently unused. |

### Class Mosquitto\Message

Represents a message received from a broker. All data is represented as
properties. The class has no methods.

| Property | Type | Description |
| --- | --- | --- |
| topic | string | The topic this message was delivered to. |
| payload | string | The payload of this message. |
| mid | int | The ID of this message. |
| qos | int | The QoS value applied to this message. |
| retain | bool | Whether this is a retained message or not. |

### Class Mosquitto\Exception

This is an exception that may be thrown by many of the operations in the Client
object.

## To do

* Arginfo
* Logging callbacks
* TLS support
* Tests
