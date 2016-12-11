/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
/**
 * @defgroup MySigninggrp MySigning
 * @ingroup internals
 * @{
 *
 * @brief The signing driver provides a generic API for various signing backends to offer
 * signing of MySensors messages.
 *
 * Signing support created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
 *
 * @section MySigninggrpbackground Background and concepts
 *
 * Suppose two participants, Alice and Bob, wants to exchange a message. Alice sends a message to Bob. In MySensors “language” Alice
 * could be a gateway and Bob an actuator (light swich, electronic lock, etc). But to be generic, we will substitute the term
 * “gateway” with Alice and a “node” with Bob (although the reverse relationship is also supported).
 *
 * Alice sends a message to Bob. This message can be heard by anyone who wants to listen (and also by anyone that is within “hearing”
 * distance). Normally, this is perhaps not a big issue. Nothing Alice says to Bob may be secret or sensitive in any way.
 * However, sometimes (or perhaps always) Bob want to be sure that the message Bob receives actually came from Alice. In cryptography,
 * this is known as <i>authenticity</i>. Bob needs some way of determining that the message is authentic from Alice, when Bob receives it.
 * This prevents an eavesdropper, Eve, to trick Bob into thinking it was Alice that sent a message Eve in fact transmitted. Bob also needs
 * to know how to determine if the message has been repeated. Eve could record a message sent by Alice that Bob accepted and then send
 * the same message again. Eve could also in some way prevent Bob from receiving the message and delay it in order to permit the message
 * to arrive to Bob at a time Eve chooses, instead of Alice. Such an attack is known as a <b>replay attack</b>.<br>
 * <i>Authenticity</i> permits Bob to determine if Alice is the true sender of a message.
 * @image html MySigning/alicenfriends.png
 * @image latex MySigning/alicenfriends.eps "Alice with Eve and Bob" width=6cm
 *
 * It can also be interesting for Bob to know that the message Alice sent has not been tampered with in any way. This is the
 * <i>integrity</i> of the message. We now introduce Mallory, who could be intercepting the communication between Alice and Bob and
 * replace some parts of the message but keeping the parts that authenticate the message. That way, Bob still trusts Alice to be the
 * source, but the contents of the message was not the content Alice sent. Bob needs to be able to determine that the contents of the
 * message was not altered after Alice sent it.<br>
 * Mallory would in this case be a <b>man-in-the-middle</b> attacker.<br>
 * <i>Integrity</i> permits Bob to verify that the messages received from Alice has not been tampered with.<br>
 * This is achieved by adding a <i>signature</i> to the message, which Bob can inspect to validate that Alice is the author.
 * @image html MySigning/alicenfriends2.png
 * @image latex MySigning/alicenfriends2.eps "Alice with Mallory and Bob" width=9cm
 *
 * The signing scheme used, needs to address both these attack scenarios. Neither Eve nor Mallory must be permitted to interfere with the
 * message exchange between Alice and Bob.
 *
 * The key challenge to implementing a secure signing scheme is to ensure that every signature is different, even if the message is not.
 * If not, <b>replay attacks</b> would be very hard to prevent.<br>
 * One way of doing this is to increment some counter on the sender side and include it in the signature. This is however predictable.<br>
 * A better option would be to introduce a random number to the signature. That way, it is impossible to predict what the signature will be.
 * The problem is, that also makes it impossible for the receiver (Bob) to verify that the signature is valid.<br>
 * A solution to this is to let Bob generate the random number, keep it in memory and send it to Alice. Alice can then use the random number
 * in the signature calculation and send the signed message back to Bob who can validate the signature with the random number used.
 * This random number is in cryptography known as a <a href="https://en.wikipedia.org/wiki/Cryptographic_nonce">nonce</a> or
 * <a href="https://en.wikipedia.org/wiki/Salt_%28cryptography%29">salt</a>.
 *
 * However, Mallory might be eavesdropping on the communication and snoop up the nonce in order to generate a new valid signature for a
 * different message. To counter this, both Alice and Bob keep a secret that only they know. This secret is never transmitted over the air,
 * nor is it revealed to anybody. This secret is known as a <a href="https://en.wikipedia.org/wiki/Pre-shared_key"> pre-shared key</a> (PSK).
 *
 * If Eve or Mallory are really sophisticated, he/she might use a <b>delayed replay attack</b>. This can be done by allowing Bob to transmit
 * a nonce to Alice. But when Alice transmits the uniquely signed message, Mallory prevents Bob from receiving it, to a point when Mallory
 * decides Bob should receive it. An example of such an attack is described
 * <a href="http://spencerwhyte.blogspot.se/2014/03/delay-attack-jam-intercept-and-replay.html">here</a>.<br>
 * This needs to be addressed as well, and one way of doing this is to have Bob keep track of time between a transmitted nonce and a signed
 * message to verify. If Bob is asked for a nonce, Bob knows that a signed message is going to arrive “soon”. Bob can then decide that if the
 * signed message does not arrive within a predefined timeframe, Bob throws away the generated nonce and thus makes it impossible to verify
 * the message if it arrives late.
 *
 * The flow can be described like this:
 * @image html MySigning/alicenbob.png
 * @image latex MySigning/alicenbob.eps "Alice and Bob" width=8cm
 * The benefits for MySensors to support this are obvious. Nobody wants others to be able to control or manipulate any actuators in their home.
 *
 * @section MySigninggrpencryption Why encryption is not part of this
 *
 * Well, some could be uncomfortable with somebody being able to snoop temperatures, motion or the state changes of locks in the environment.
 * Signing does <b>not</b> address these issues. Encryption is needed to prevent this.<br>
 * It is my personal standpoint that encryption should not be part of the MySensors “protocol”. The reason is that a gateway and a node does
 * not really care about messages being readable or not by “others”. It makes more sense that such guarantees are provided by the underlying
 * transmission layer (RF solution in this case). It is the information transmitted over the air that needs to be secret (if user so desires).
 * The “trust” level on the other hand needs to go all the way into the sketches (who might have different requirements of trust depending on
 * the message participant), and for this reason, it is more important (and less complicated) to ensure authenticity and <i>integrity</i> at
 * protocol-level as message contents is still readable throughout the protocol stack. But as soon as the message leaves the “stack” it can be
 * scramble into “garbage” when transmitted over the air and then reassembled by a receiving node before being fed in “the clear” up the stack
 * at the receiving end.
 *
 * There are methods and possibilities to provide encryption also in software, but if this is done, it is my recommendation that this is done
 * after integrity- and authentication information has been provided to the message (if this is desired). Integrity and authentication is of
 * course not mandatory and some might be happy with only having encryption to cover their need for security. I, however, have only focused on
 * <i>integrity</i> and <i>authenticity</i> while at the same time keeping the current message routing mechanisms intact and therefore leave
 * the matter of <i>secrecy</i> to be implemented in the “physical” transport layer. With the <i>integrity</i> and <i>authenticity</i> handled
 * in the protocol it ought to be enough for a simple encryption (nonce-less AES with a PSK for instance) on the message as it is sent to the
 * RF backend. Atmel does provide such circuits as well but I have not investigated the matter further as it given the current size of the
 * ethernet gateway sketch is close to the size limit on an Arduino Nano, so it will be difficult to fit this into some existing gateway
 * designs.<br>
 * Also it is worth to consider that the state of a lock can just as readily be determined by simply looking at the door in question or
 * attempting to open it, so obfuscating this information will not necessarily deter an attacker in any way.<br>
 * Nevertheless, I do acknowledge that people find the fact that all information is sent “in the clear” even if it require some technical
 * effort for an intruder to obtain and inspect this information. So I do encourage the use of encrypting transport layers.<br>
 * This is however not covered by this implementation.
 *
 * @section MySigninggrphow How this is done
 *
 * There exist many forms of message signature solutions to combat Eve and Mallory.<br>
 * Most of these solutions are quite complex in term of computations, so I elected to use an algorithm that an external circuit is able to
 * process. This has the added benefit of protecting any keys and intermediate data used for calculating the signature so that even if
 * someone were to actually steal a sensor and disassembled it, they would not be able to extract the keys and other information from the
 * device.<br>
 * A common scheme for message signing (authenticity and integrity) is implemented using
 * <a href="http://en.wikipedia.org/wiki/Hash-based_message_authentication_code">HMAC</a> which in combination with a strong
 * <a href="http://en.wikipedia.org/wiki/Hash_function"> hash function</a> provides a very strong level of protection.<br>
 * The <a href="http://www.atmel.com/devices/ATSHA204A.aspx">Atmel ATSHA204A</a> is a low-cost, low-voltage/current circuit that provides
 * HMAC calculation capabilities with SHA256 hashing which is a (currently) virtually unbreakable combination. If SHA256 were to be hacked,
 * a certain <a href="http://en.wikipedia.org/wiki/Bitcoin">cryptocurrency</a> would immediately be rendered worthless.<br>
 * The ATSHA device also contain a random number generator (RNG) which enables the generation of a good nonce, as in,
 * <i>non-predictable</i>.<br>
 * As I acknowledge that some might not want to use an additional external circuit, I have also implemented a software version of the ATSHA
 * device, capable of generating the same signatures as the ATSHA device does. Because it is pure-software however, it does not provide as
 * good nonces (it uses the <a href="http://arduino.cc/en/reference/random">Arduino pseudo-random generator</a>) and the HMAC key is stored
 * in SW and is therefore readable if the memory is dumped. It also naturally claims more flash space due to the more complex software. But
 * for indoor sensors/actuators this might be good enough for most people.
 *
 * @section MySigninggrphowuse How to use this
 *
 * Before we begin with the details, I just want to emphasize that signing is completely optional and not enabled by default. If you do want
 * the additional security layer signing provides, you pick the backend of your choise in your sketch. Currently, two compatible backends are
 * supported; @ref MY_SIGNING_ATSHA204 (hardware backed) and @ref MY_SIGNING_SOFT (software backed). You can either enable these globally in
 * @ref MyConfig.h or in your sketch for sketch specific/local use.
 *
 * <b>Firstly</b>, you need to make sure to pick a backend to use as described above.
 * @code{.cpp}
 * //#define MY_SIGNING_SOFT
 * #define MY_SIGNING_ATSHA204
 * #include <MySensors.h>
 * ...
 * @endcode
 * Make sure to set the define before the inclusion of MySensors.h.
 * It is legal to mix hardware- and software-based backends in a network. They work together.
 *
 * You also need to decide if the node (or gateway) in question require and verify signatures in addition to calculating them.
 * This has to be set by at least one of the node in a "pair" or nobody will actually start calculating a signature for a message.
 * Just set the flag @ref MY_SIGNING_REQUEST_SIGNATURES and the node will inform the gateway that it expects the gateway to sign all
 * messages sent to the node. If this is set in a gateway, it will @b NOT force all nodes to sign messages to it. It will only require
 * signatures from nodes that in turn require signatures. If it is desired that the gateway should require signatures from all nodes,
 * @ref MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL can be set in the gateway sketch.<br>
 * If you want to have two nodes communicate securely directly with each other, the nodes that require signatures must send a presentation
 * message to all nodes it expect signed messages from (only the gateway is informed automatically). See @ref signerPresentation().<br>
 * A node can have three "states" with respect to signing:
 * 1. Node does not support signing in any way (neither @ref MY_SIGNING_ATSHA204 nor @ref MY_SIGNING_SOFT is set)
 * 2. Node does support signing but don't require messages sent to it to be signed (@ref MY_SIGNING_REQUEST_SIGNATURES is not set)
 * 3. Node does support signing and require messages sent to it to be signed (@ref MY_SIGNING_REQUEST_SIGNATURES is set)
 *
 * <b>Secondly</b>, you need to verify the configuration for the backend.<br>
 * For hardware backed signing it is the pin the device is connected to. In MyConfig.h there are defaults which you might need to adjust to
 * match your personal build. The setting is defined using @ref MY_SIGNING_ATSHA204_PIN and the default is to use pin A3.<br>
 * Similar to picking your backend, this can also be set in your sketch:
 * @code{.cpp}
 * #define MY_SIGNING_ATSHA204
 * #define MY_SIGNING_ATSHA204_PIN 4
 * #define MY_SIGNING_REQUEST_SIGNATURES
 * #include <MySensors.h>
 * ...
 * @endcode
 * For the software backed signingbackend, an unconnected analog pin is required to set a random seed for the pseudo-random generator.
 * It is important that the pin is floating, or the output of the pseudo-random generator will be predictable, and thus compromise the
 * signatures. The setting is defined using @ref MY_SIGNING_SOFT_RANDOMSEED_PIN and the default is to use pin A7. The same configuration
 * possibilities exist as with the other configuration options.
 *
 * <b>Thirdly</b>, if you use the software backend, you need to personalize the node (see @ref personalization).
 * @code{.cpp}
 * #define MY_SIGNING_SOFT
 * #define MY_SIGNING_SOFT_RANDOMSEED_PIN 7
 * #define MY_SIGNING_REQUEST_SIGNATURES
 * #include <MySensors.h>
 * ...
 * @endcode
 *
 * An example of a node that require signatures is available in @ref SecureActuator.ino.
 *
 * @anchor personalization If you use the “real” ATSHA204A, before any signing operations can be done, the device needs to be <i>personalized</i>.
 * This can be a daunting process as it involves irreversibly writing configurations to the device, which cannot be undone. I have however tried
 * to simplify the process as much as possibly by creating a helper-sketch specifically for this purpose in @ref SecurityPersonalizer.ino
 * Note that you also need to do personalization for software signing, but then the values are stored in EEPROM (described below).
 *
 * The process of personalizing the ATSHA204A involves
 * - Writing and locking chip configuration
 * - (optionally) Generating and (mandatory) writing HMAC key
 * - (optionally) Locking data sections
 *
 * First execute the sketch without modifications to make sure communications with the device can be established. It is highly recommended that
 * the first time this is done, a device with serial debug possibilities is used.<br>
 * When this has been confirmed, it is time to decide what type of personalization is desired.<br>
 * There are a few options here.<br>
 * Firstly, enable @ref LOCK_CONFIGURATION to allow the sketch to lock the chip configuration. The sketch will write the default settings to the
 * chip because these are fine for our purposes. This also enables the RNG which is required to allow the sketch to automatically generate a PSK
 * if this is desired.<br>
 * Now it is possible to execute the sketch to lock the configuration and enable the RNG.<br>
 * Next step is to decide if a new key should be generated or an existing key should be stored to the device. This is determined using
 * @ref USER_KEY, which, if defined, will use the data in the variable user_key_data.<br>
 * If @ref USER_KEY is disabled, the RNG will be used to generate a key. This key obviously need to be made available to you so you can use
 * it in other devices in the network, and this key is therefore also printed on the serial console when it has been generated.<br>
 * The key (generated or provided) will be written to the device unless @ref SKIP_KEY_STORAGE is set. As long as the data zone is kept unlocked
 * the key can be replaced at any time. However, Atmel suggests the data region to be locked for maximum security. On the other hand, they also
 * claim that the key is not readable from the device even if the data zone remains unlocked so the need for locking the data region is optional
 * for MySensors usage.<br>
 * For devices that does not have serial debug possibilities, it is possible to set @ref SKIP_UART_CONFIRMATION, but it is required to set
 * @ref USER_KEY if this option is enabled since a generated and potentially unknown key could be written to the device and thus rendering
 * it useless (if the data zone is also locked).<br>
 * For devices with serial debug possibilities it is recommended to not use @ref SKIP_UART_CONFIRMATION as the sketch without that setting will
 * ask user to send a ‘space’ character on the serial terminal before any locking operations are executed as an additional confirmation that
 * this irreversible operation is done. However, if a number of nodes are to undergo personalization, this option can be enabled to streamline
 * the personalization.<br>
 * This is a condensed description of settings to fully personalize and lock down a set of sensors (and gateways):
 * - Pick a “master” device with serial debug port.
 * - Set the following sketch configuration of the personalizer:
 *  -# Enable @ref LOCK_CONFIGURATION
 *  -# Disable @ref LOCK_DATA
 *  -# Enable @ref SKIP_KEY_STORAGE
 *  -# Disable @ref SKIP_UART_CONFIRMATION
 *  -# Disable @ref USER_KEY
 * - Execute the sketch on the “master” device to obtain a randomized key. Save this key to a secure location and keep it confidential so that\n
     you can retrieve it if you need to personalize more devices later on.
 * - Now reconfigure the sketch with these settings:
 *  -# Enable @ref LOCK_CONFIGURATION
 *  -# Enable @ref LOCK_DATA (if you are sure you do not need to replace/revoke the key, this is the most secure option to protect from key readout\n
 *     according to Atmel, but they also claim that key is not readable even if data region remains unlocked from the slot we are using)
 *  -# Disable @ref SKIP_KEY_STORAGE
 *  -# Enable @ref SKIP_UART_CONFIRMATION
 *  -# Enable @ref USER_KEY
 * - Put the saved key in the user_key_data variable.
 * - Now execute the sketch on all devices you want to personalize with this secret key.
 *
 * For software signing the personalization procedure looks slightly different.<br>
 * - If you do not have the ATSHA204A device and need to generate random keys:
 *  -# Enable @ref USE_SOFT_SIGNING
 * - If you do have the ATSHA204A and want to use it to generate random keys for soft signing:
 *  -# Disable @ref USE_SOFT_SIGNING
 * - If you want to review existing EEPROM configuration to determine if anything needs to be updated:
 *  -# Make sure to disable any ATSHA204A update features if you use it (enable @ref SKIP_KEY_STORAGE, disable @ref LOCK_CONFIGURATION and @ref LOCK_DATA)
 *  -# Disable @ref STORE_SOFT_KEY
 *  -# Disable @ref STORE_SOFT_SERIAL
 *  -# Disable @ref STORE_AES_KEY
 * - Execute the sketch and the EEPROM configuration will be printed on the serial console.
 * - Decide what you want to update and enable the corresponding flags (@ref STORE_SOFT_KEY, @ref STORE_SOFT_SERIAL, @ref STORE_AES_KEY).
 * - If you have existing keys and want to use them instead of having the values randomized:
 * - Enable the corresponding flag (@ref USER_SOFT_KEY, @ref USER_SOFT_SERIAL, @ref USER_AES_KEY) and also set the appropriate values
 *   (@ref user_soft_key_data, @ref user_soft_serial, @ref user_aes_key)
 *
 * The personalizer also takes care of generation and storage of the AES key used for RF traffic encryption.
 * See the software signing personalization flow for how to manage it.
 *
 * That’s it. Personalization is done and the device is ready to execute signing operations which are valid only on your personal network.
 *
 * If a node does require signing, any unsigned message sent to the node will be rejected.<br>
 * This also applies to the gateway. However, the difference is that the gateway will only require signed messages from nodes it knows in turn
 * require signed messages (unless @ref MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL is set).<br>
 * A node can also inform a different node that it expects to receive signed messages from it. This is done by transmitting an internal message
 * of type @ref I_SIGNING_PRESENTATION and provide flags as payload that inform the receiver of the signing preferences of the sender.<br>
 * All nodes and gateways in a network maintain a table where the signing preferences of all nodes are stored. This is also stored in EEPROM so
 * if the gateway reboots, the nodes does not have to retransmit a signing presentation to the gateway for the gateway to realize that the node
 * expect signed messages.<br>
 * Also, the nodes that do not require signed messages will also inform gateway of this, so if you reprogram a node to stop require signing,
 * the gateway will adhere to this as soon as the new node has presented itself to the gateway. Note however, that if the gateway sets
 * @ref MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL a node that does not support signing will be unable to send any data to the gateway.
 *
 * The following sequence diagram illustrate how messages are passed in a MySensors network with respect to signing:
 * @image html MySigning/signingsequence.png
 * @image latex MySigning/signingsequence.eps "Signing sequence diagram" width=9cm
 *
 * None of this activity is “visible” to you (as the sensor sketch implementor). All you need to do is to set your preferences in MyConfig.h or
 * in your sketch. Depending on chosen backend, do personalization or key configurations and set @ref MY_SIGNING_REQUEST_SIGNATURES on the node that
 * you want protected. That is enough to enable protection from both Eve and Mallory in your network (although if you do not also enable encryption,
 * Eve can eavesdrop, but not do anything about, your messages).
 *
 * @section MySigningwhitelisting Whitelisting and node revocation
 *
 * Consider the situation when you have set up your secure topology. We use the remotely operated garage door as an example:
 * - You have a node inside your garage (therefore secure and out of reach from prying eyes) that controls your garage door motor.\n
 *   This node requires signing since you do not want an unauthorized person sending it orders to open the door.
 * - You have a keyfob node with a signing backend that uses the same PSK as your door opener node.
 *
 * In this setup, your keyfob can securely transmit messages to your door node since the keyfob will sign the messages it sends and the door node
 * will verify that these were sent from a trusted node (since it used the correct PSK). If the keyfob does not sign the messages, the door node
 * will not accept them.
 *
 * One day your keyfob gets stolen or you lost it or it simply broke down.
 *
 * You now end up with a problem; you need some way of telling your door node that the keyfob in question cannot be trusted any more. Furthermore,
 * you maybe locked the data region in your door nodes ATSHA device and is not able to revoke/change your PSK, or you have some other reason for
 * not wanting to replace the PSK. How do you make sure that the "rogue" keyfob can be removed from the "trusted chain"?
 *
 * The answer to this is whitelisting. You let your door node keep a whitelist of all nodes it trusts. If you stop trusting a particular node, you
 * remove it from the nodes whitelist, and it will no longer be able to communicate signed messages to the door node.
 *
 * In case you want to be able to "whitelist" trusted nodes (in order to be able to revoke them in case they are lost) you also need to take note
 * of the serial number of the ATSHA device or the software value stored in EEPROM. This is unique for each device. The serial number is printed
 * in a copy+paste friendly format by the personalizer for this purpose.<br>
 * The whitelist is stored on the node that require signatures. When a received message is verified, the serial of the sender is looked up in a
 * list stored on the receiving node, and the corresponding serial stored in the list for that sender is then included in the signature verification
 * process. The list is stored as the value of the flag that enables whitelisting, @ref MY_SIGNING_NODE_WHITELISTING.<br>
 *
 * Whitelisting is achieved by 'salting' the signature with some node-unique information known to the receiver. In the case of ATSHA204A this is the
 * unique serial number programmed into the circuit. This unique number is never transmitted over the air in clear text, so Eve will not be able to
 * figure out a "trusted" serial by snooping on the traffic.<br>
 * Instead the value is hashed together with the senders NodeId into the HMAC signature to produce the final signature. The receiver will then take
 * the originating NodeId of the signed message and do the corresponding calculation with the serial it has stored in it's whitelist if it finds a
 * matching entry in it's whitelist.
 *
 * Whitelisting is an optional alternative because it adds some code and configuration options which might not be desirable for every user. So if
 * you want the ability to use whitelists, you need to enable @ref MY_SIGNING_NODE_WHITELISTING.<br>
 * The whitelist is provided as value of the flag that enable it as follows (example is a node that require signing as well):
 * @code{.cpp}
 * #define MY_SIGNING_ATSHA204
 * #define MY_SIGNING_REQUEST_SIGNATURES
 * #define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01}},{.nodeId = 2,.serial = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09}}}
 * #include <MySensors.h>
 * ...
 * @endcode
 * In this example, there are two nodes in the whitelist; the gateway, and a separate node that communicates directly with this node (with signed
 * messages). You do not need to do anything special for the sending nodes, apart from making sure they support signing.
 *
 * The "soft" backend of course also support whitelisting. Example:
 * @code{.cpp}
 * #define MY_SIGNING_SOFT
 * #define MY_SIGNING_SOFT_RANDOMSEED_PIN 7
 * #define MY_SIGNING_REQUEST_SIGNATURES
 * #define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01}},{.nodeId = 2,.serial = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09}}}
 * #include <MySensors.h>
 * ...
 * @endcode
 *
 * For a node that should transmit whitelisted messages but not receive whitelisted messages, you do not need any special configurations:
 * @code{.cpp}
 * #define MY_SIGNING_SOFT
 * #define MY_SIGNING_SOFT_RANDOMSEED_PIN 7
 * @endcode
 * Remember that you always need to select a signing backend for all nodes that communicate to a node that require whitelisting.
 * Also, note that a node that use whitelisting will not accept messages from nodes that are not present in it's whitelist.
 * And you have to personalize all nodes that use signing with a common HMAC key but different serial numbers (ATSHA204A always has unique serials).
 *
 * @section MySigningtechnical The technical stuff
 *
 * How are the messages actually affected by the signing?<br>
 * The following illustration shows what part of the message is signed, and where the signature is stored:
 * @image html MySigning/signingillustrated1.png
 * @image latex MySigning/signingillustrated1.eps "Message structure" width=11cm
 *
 * The first byte of the header is not covered by the signature, because in the network, this byte is used to track hops in the network and therefore
 * might change if the message is passing a relay node. So it cannot be part of the signature, or the signature would be invalid when it arrives to
 * its destination. The signature also carries a byte with a signing identifier to prevent false results from accidental mixing of incompatible
 * signing backends in the network. Thus, the maximum size for a payload is 29-7 bytes. Larger payloads are not possible to sign. Another thing to
 * consider is that the strength of the signature is inversely proportional to the payload size.
 *
 * As for the software backend, it turns out that the ATSHA does not do “vanilla” HMAC processing. Fortunately, Atmel has documented exactly how
 * the circuit processes the data and hashes thus making it possible to generate signatures that are identical to signatures generated by the circuit.
 *
 * The signatures are calculates in the following way:
 * @image html MySigning/signingillustrated2.png
 * @image latex MySigning/signingillustrated2.eps "HMAC processing" width=10cm
 *
 * Exactly how this is done can be reviewd in the source for the ATSHA204SOFT backend and the ATSHA204A
 * <a href="http://www.atmel.com/Images/Atmel-8885-CryptoAuth-ATSHA204A-Datasheet.pdf">datasheet</a>.
 * In the MySensors protocol, the following internal messagetypes handles signature requirements and nonce requests:<br>
 * @ref I_SIGNING_PRESENTATION <br>
 * @ref I_NONCE_REQUEST <br>
 * @ref I_NONCE_RESPONSE <br>
 *
 * Also, the version field in the header has been reduced from 3 to 2 bits in order to fit a single bit to indicate that a message is signed.
 *
 * @section MySigninglimitations Known limitations
 *
 * Due to the limiting factor of our cheapest Arduino nodes, the use of diversified keys is not implemented. That mean that all nodes in your
 * network share the same PSK (at least the ones that are supposed to exchange signed data). It is important to understand the implications of
 * this, and that is covered in the "Typical use cases" chapter below.<br>
 * Also be reminded that the strength of the signature is inversely proportional to the size of the message. The larger the message, the weaker
 * the signature.
 *
 * @section MySigningusecases Typical use cases
 *
 * "Securely located" in this context mean a node which is not physically publicly accessible. Typically at least your gateway.<br>
 * "Public" in this context mean a node that is located outside your "trusted environment". This includes sensors located outdoors, keyfobs etc.
 *
 * @subsection MySigninglock Securely located lock
 *
 * You have a securely located gateway and a lock somewhere inside your "trusted environment" (e.g. inside your house door, the door to your
 * dungeon or similar).<br>
 * You should then keep the data section of your gateway and your lock node unlocked. Locking the data (and therefore the PSK) will require you
 * to replace at least the signing circuit in case you need to revoke the PSK because some other node in your network gets compromised.<br>
 * You need to make your node require signed messages but you do not necessarily need to make your gateway require signed messsages (unless you
 * are concerned that someone might spoof the lock status of your lock).<br>
 * Configuration example for the secure lock node:<br>
 * @code{.cpp}
 * #define MY_SIGNING_ATSHA204
 * #define MY_SIGNING_REQUEST_SIGNATURES
 * #include <MySensors.h>
 * ...
 * @endcode
 * If you do also want your gateway to require signatures from your lock you just enable the same (or similar if using software signing) settings
 * in the gateway.
 *
 * @subsection MySigningpatio Patio motion sensor
 *
 * Your gateway is securely located inside your house, but your motion sensor is located outside your house. You have for some reason elected
 * that this node should sign the messages it send to your gateway. You should lock the data (PSK) in this node then, because if someone were
 * to steal your patio motion sensor, they could rewrite the firmware and spoof your gateway to use it to transmit a correctly signed message
 * to your secure lock inside your house. But if you revoke your gateway (and lock) PSK the outside sensor cannot be used for this anymore.
 * Nor can it be changed in order to do it in the future. You can also use whitelisting to revoke your lost node.<br>
 * This is an unlikely use case because there really is no reason to sign sensor values. If you for some reason want to obfuscate sensor data,
 * encryption is a better alternative.<br>
 * Configuration example for a motion sensor:<br>
 * @code{.cpp}
 * #define MY_SIGNING_SOFT
 * #define MY_SIGNING_SOFT_RANDOMSEED_PIN 7
 * #define MY_SIGNING_REQUEST_SIGNATURES
 * #include <MySensors.h>
 * ...
 * @endcode
 *
 * The gateway needs to be configured with a whitelist (and it has to have an entry for all nodes that send and/or require signed messages):<br>
 * @code{.cpp}
 * #define MY_SIGNING_SOFT
 * #define MY_SIGNING_SOFT_RANDOMSEED_PIN 7
 * #define MY_SIGNING_REQUEST_SIGNATURES
 * #define MY_SIGNING_NODE_WHITELISTING {{.nodeId = MOTION_SENSOR_ID,.serial = {0x12,0x34,0x56,0x78,0x90,0x12,0x34,0x56,0x78}}}
 * #include <MySensors.h>
 * ...
 * @endcode

 * @subsection MySigningkeyfob Keyfob for garage door opener
 *
 * Perhaps the most typical usecase for signed messages. Your keyfob should be totally locked down. If the garage door opener is secured
 * (and it should be) it can be unlocked. That way, if you loose your keyfob, you can revoke the PSK in both the opener and your gateway,
 * thus rendering the keyfob useless without having to replace your nodes. You can also use whitelisting to revoke your lost keyfob.<br>
 * Configuration example for the keyfob (keyfob will only transmit to another node and not receive anything):<br>
 * @code{.cpp}
 * #define MY_SIGNING_ATSHA204
 * #include <MySensors.h>
 * ...
 * @endcode
 *
 * Configuration example for the door controller node (should require signing from anyone who wants to control it):<br>
 * @code{.cpp}
 * #define MY_SIGNING_SOFT
 * #define MY_SIGNING_SOFT_RANDOMSEED_PIN 7
 * #define MY_SIGNING_REQUEST_SIGNATURES
 * #define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88}},{.nodeId = KEYFOB_ID,.serial = {<FROM ATSHA ON KEYFOB>}}}
 * #include <MySensors.h>
 * ...
 * @endcode
 *
 * @subsection MySigningsketches Relevant sketches
 *
 * - @ref SecureActuator.ino
 * - @ref SecurityPersonalizer.ino
 */

/**
 * @file MySigning.h
 *
 * @brief API declaration for MySigning signing backend
 */
#ifndef MySigning_h
#define MySigning_h

#include "MySensorsCore.h"
#include "drivers/ATSHA204/ATSHA204.h"

#ifdef MY_SIGNING_NODE_WHITELISTING
typedef struct {
	uint8_t nodeId;                   /**< @brief The ID of the node */
	uint8_t serial[SHA204_SERIAL_SZ]; /**< @brief Node specific serial number */
} whitelist_entry_t;
#endif

/** @brief Helper macro to determine the number of elements in a array */
#define NUM_OF(x) (sizeof(x)/sizeof(x[0]))

/**
 * @brief Initializes signing infrastructure and associated backend.
 *
 * This function makes sure that the internal states of the signing infrastructure
 * is set to a known initial state.
 * \n@b Usage: This fuction should be called before any signing related operations take place.
 */
void signerInit(void);

/**
 * @brief Does signing specific presentation for a node.
 *
 * This function makes sure any signing related presentation info is shared with the other part.
 * The presentation of the gateways signing preferences is done in @ref signerProcessInternal().
 * \n@b Usage: This function should be called by the presentation routine of the mysensors library.
 * You only need to call this directly from a sketch to set up a node to node signed message exchange.
 * If you do call this directly from a sketch, and you at some point change your sketch to go from
 * requireing signing to not requireing signatures, you need to present this change to the node at least
 * once, so it can update its requirements tables accordingly. Or it will keep believing that this node
 * require signatures and attempt to send signed messages to it.
 *
 * @param msg Message buffer to use.
 * @param destination Node ID of the destination.
 */
void signerPresentation(MyMessage &msg, uint8_t destination);

/**
 * @brief Manages internal signing message handshaking.
 *
 * This function takes care of signing related message handshaknig such as nonce exchange.
 * \n@b Usage: This function should be called by the incoming message handler before any further message
 * processing is performed on internal messages. This function should only be called for @ref C_INTERNAL class
 * messages.
 *
 * @param msg Message buffer to process.
 * @returns @c true if caller should stop further message processing.
 */
bool signerProcessInternal(MyMessage &msg);

/**
 * @brief Check timeout of verification session.
 *
 * Nonce will be purged if it takes too long for a signed message to be sent to the receiver.
 * \n@b Usage: This function should be called on regular intervals, typically within some process loop.
 *
 * @returns @c true if session is still valid.
 */
bool signerCheckTimer(void);

/**
 * @brief Get nonce from provided message and store for signing operations.
 *
 * Returns @c false if subsystem is busy processing an ongoing signing operation.<br>
 * Returns @c false if signing identifier found in message is not supported by the used backend.<br>
 * If successful, this marks the start of a signing operation at the sending side so
 * implementation is expected to do any necessary initializations within this call.
 * \n@b Usage: This function is typically called as action when receiving a @ref I_NONCE_RESPONSE
 * message.
 *
 * @param msg The message to get the nonce from.
 * @returns @c true if successful, else @c false.
 */
bool signerPutNonce(MyMessage &msg);

/**
 * @brief Signs provided message. All remaining space in message payload buffer is used for
 * signing identifier and signature.
 *
 * Nonce used for signature calculation is the one generated previously within @ref signerProcessInternal().<br>
 * Nonce will be cleared when this function is called to prevent re-use of nonce.<br>
 * Returns @c false if subsystem is busy processing an ongoing signing operation.<br>
 * Returns @c false if not two bytes or more of free payload space is left in provided message.<br>
 * This ends a signing operation at the sending side so implementation is expected to do any
 * deinitializations and enter a power saving state within this call.
 * \n@b Usage: This function is typically called as action when receiving a @ref I_NONCE_RESPONSE
 * message and after @ref signerPutNonce() has successfully been executed.
 *
 * @param msg The message to sign.
 * @returns @c true if successful, else @c false.
*/
bool signerSignMsg(MyMessage &msg);

/**
 * @brief Verifies signature in provided message.
 *
 * Nonce used for verification is the one previously set using @ref signerPutNonce().<br>
 * Nonce will be cleared when this function is called to prevent re-use of nonce.<br>
 * Returns @c false if subsystem is busy processing an ongoing signing operation.<br>
 * Returns @c false if signing identifier found in message is not supported by the used backend.<br>
 * This ends a signing operation at the receiving side so implementation is expected to do any
 * deinitializations and enter a power saving state within this call.
 * \n@b Usage: This function is typically called when receiving a message that is flagged as signed
 * and @ref MY_SIGNING_REQUEST_SIGNATURES is activated.
 *
 * @param msg The message to verify.
 * @returns @c true if successful, else @c false.
 */
bool signerVerifyMsg(MyMessage &msg);

/**
 * @brief Initialize a hash calculation session.
 *
 * Any ongoing calculation session will be terminated/reset.
 */
void signerSha256Init(void);

/**
 * @brief Add data to an ongoing hash calculation session.
 *
 * More data can be added by repeated calls to this function.
 *
 * @param data Buffer with data to add.
 * @param sz Size of data buffer.
 */
void signerSha256Update(const uint8_t* data, size_t sz);

/**
 * @brief Finalize an ongong hash calculation session and return the hash.
 *
 * The returned hash size is always 32 bytes. Buffer can be assumed to be valid
 * until @ref signerSha256Init() is called again.
 *
 * @returns Buffer with 32-byte hash.
 */
uint8_t* signerSha256Final(void);

/**
 * @brief Do a timing neutral memory comparison.
 *
 * The function behaves similar to memcmp with the difference that it will
 * always use the same number of instructions for a given number of bytes,
 * no matter how the two buffers differ and the response is either 0 or -1.
 *
 * @param a First buffer for comparison.
 * @param b Second buffer for comparison.
 * @param sz The number of bytes to compare.
 * @returns 0 if buffers match, -1 if they do not.
 */
int signerMemcmp(const void* a, const void* b, size_t sz);

#endif
/** @}*/
/**
 * @defgroup MySigningTroubleshootinggrp Troubleshooting
 * @ingroup MySigninggrp
 * @{
 *
 * @section MySigningTroubleshootingSymptoms Symptoms and solutions
 *
 * The first thing to do if you suspect signing is causing problems, is to enable the verbose debug
 * flag for the signing backend. @see MY_DEBUG_VERBOSE_SIGNING
 *
 * If you are having trouble getting signing to work, please see the following troubleshooting tips.
 *
 * @subsection MySigningTroubleshootingSymptomStFail Signing fails and logs show st=fail on transmissions
 *
 * This is actually not a signing problem, although ofthen st=fail becomes st=ok when signing is disabled.
 * This is by far the most commonly reported problem with signing, but the problems is not with signing,
 * it is with radio performence.<br>
 * This is a typical log which might look like a signing related issue but isn't:
 * @code{.unparsed}
 * 0;255;3;0;9;Skipping security for command 3 type 16
 * 0;255;3;0;9;read: 3-3-0 s=255,c=3,t=16,pt=0,l=0,sg=0:
 * 0;255;3;0;9;Signing backend: ATSHA204Soft
 * 0;255;3;0;9;SHA256: 86DEAE1DAF50D577A4E2262B33ABF9DEE05DD8FAF84F94F50900000000000000
 * 0;255;3;0;9;Skipping security for command 3 type 17
 * 0;255;3;0;9;send: 0-0-3-3 s=255,c=3,t=17,pt=6,l=25,sg=0,st=fail:86DEAE1DAF50D577A4E2262B33ABF9DEE05DD8FAF84F94F509
 * 0;255;3;0;9;Failed to transmit nonce!
 * 0;255;3;0;9;Message is not signed, but it should have been!
 * 0;255;3;0;9;verify fail
 * @endcode
 *
 * The reason for this is that when signing is used, the messages transmitted become relatively large.<br>
 * Because of this, the message is more sensitive to noise, and the chance for a message to get scrambled
 * increase with the message size. Please refer to the troubleshooting section at the MySensors forum for
 * information on how to improve radio performence.<br>
 * This is a good place to start: https://forum.mysensors.org/topic/666/debug-faq-and-how-ask-for-help
 *
 * @subsection MySigningTroubleshootingSymptomNonce Failed to generate nonce
 *
 * The signing backend failed to generate the nonce needed to sign a message. This indicate a hardware
 * problem. Please post the debug info on the forum together with a description of your hardware setup.
 *
 * @subsection MySigningTroubleshootingSymptomSign Failed to sign message
 *
 * The signing backend failed to sign the message. Typically this happens if your message is so large,
 * that there is no room left in the buffer to store a signature.
 *
 * @subsection MySigningTroubleshootingSymptomWrongSource Nonce did not come from the destination (XX) of the message to be signed! It came from YY
 *
 * This should not really happen. The reason for this message is that the signing backend is only capable
 * of handling one signed message session at any time. If for some reason multiple nodes send a nonce message to
 * the same node, only the nonce from a node that is the destination of the current message signing session will be
 * accepted. Any other nonces will be dropped. This should not happen as no node should send a nonce unless asked to,
 * and a node will only ask for a nonce to one destination for every signing session.<br>
 * If you see this message, please post the debugging details on the MySensors forum so it can be investigated further
 * together with a description of your setup.
 *
 * @subsection MySigningTroubleshootingSymptomNotSigned Message is not signed, but it should have been
 *
 * A node has failed to comply with the signing preferences of this node. Check that the node has received a
 * signing presentation message from this node. This is automatically transmitted to gateways. For other nodes,
 * you need to transmit the presentation from the sketch. @see signerPresentation
 *
 * @subsection MySigningTroubleshootingSymptomNotSignedGeneral "Messages do not appear to be signed but I think they should be..."
 *
 * Make sure you have enabled the flag to require signatures to require signatures and have enabled one of the signing
 * backends. @see MY_SIGNING_REQUEST_SIGNATURES @see MY_SIGNING_ATSHA204 @see MY_SIGNING_SOFT
 *
 * @subsection MySigningTroubleshootingSymptomNotWorkingWhitelisting Signature verification failed!
 *
 * Make sure both source and destination of the signed message has undergone @ref personalization with the same HMAC key.<br>
 * Also, if whitelisting is used, make sure the proper serial is paired with the proper node ID at the destination.
 * Whitelisting preferences are communicated with the signing presentation (done automatically from nodes to gateway but
 * has to be explicitly done by sketch for node to node communication). @see signerPresentation
 */
/** @}*/
