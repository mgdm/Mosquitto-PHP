==================
Mosquitto\\Message
==================

.. php:namespace:: Mosquitto

.. php:class:: Message

    Represents a message received from a broker. All data is represented as properties.

    .. php:attr:: $topic

        (*string*) The topic this message was delivered to.

    .. php:attr:: $payload

        (*string*) The payload of this message.

    .. php:attr:: $mid

        (*int*) The ID of this message.

    .. php:attr:: $qos

        (*int*) The QoS value applied to this message.

    .. php:attr:: $retain

        (*boolean*) Whether this is a retained message or not.

    This class has two static methods.

    .. php:staticmethod:: topicMatchesSub()

        Returns true if the supplied topic matches the supplied description, and otherwise false.

        :param string $topic: The topic to match
        :param string $subscription: The subscription to match

    .. php:staticmethod:: tokeniseTopic()

        Tokenise a topic or subscription string into an array of strings representing the topic hierarchy.

        :param string $topic: The topic to tokenise
