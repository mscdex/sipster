Description
===========

A [pjsip](http://www.pjsip.org) (or more accurately a [pjsua2](http://www.pjsip.org/docs/book-latest/html/index.html)) binding for node.js.

Familiarity with pjsip/pjsua2 is a plus when using this binding.


Requirements
============

* Non-Windows OS
* [pjsip](http://www.pjsip.org/) -- v2.2.1 or newer
* [node.js](http://nodejs.org/) -- v0.10.x


Install
=======

    npm install sipster


Examples
========

* UAS set up as a SIP trunk (no registration):

```javascript
var sipster = require('sipster');

// initialize pjsip
sipster.init();

// set up a transport to listen for incoming connections, defaults to UDP
var transport = new sipster.Transport({ port: 5060 });

// set up a SIP account, we need at least one -- as required by pjsip.
// this sets up an account for calls coming from 192.168.100.10
var acct = new sipster.Account({
  idUri: 'sip:192.168.100.10'
});

// watch for incoming calls
acct.on('call', function(info, call) {
  console.log('=== Incoming call from ' + info.remoteContact);

  // watch for call state changes
  call.on('state', function(state) {
    console.log('=== Call state is now: ' + state.toUpperCase());
  });

  // listen for DTMF digits
  call.on('dtmf', function(digit) {
    console.log('=== DTMF digit received: ' + digit);
  });

  // audio stream(s) available
  call.on('media', function(medias) {
    // play looping .wav file to the first audio stream
    var player = sipster.createPlayer('sound.wav');
    player.startTransmitTo(medias[0]);

    // record the audio of the other side, this will not include the audio from
    // the player above.
    var recorder = sipster.createRecorder('call.wav');
    medias[0].startTransmitTo(recorder);
    // to include the player audio, you can mix the sources together simply
    // by transmitting to the same recorder:
    //   player.startTransmitTo(recorder);
  });

  // answer the call (with default 200 OK)
  call.answer();
});

// finalize the pjsip initialization phase ...
sipster.start();
```


API
===

Exported static methods
-----------------------

* **init**([< _object_ >endpointCfg]) - _(void)_ - Starts the initializion of the pjsip library (`libInit()`). This is to be done only once. `endpointCfg` is an [EpConfig](http://www.pjsip.org/pjsip/docs/html/structpj_1_1EpConfig.htm)-like object for if you need to change any global options from the library defaults.

* **start**() - _(void)_ - Finalizes the initialization of the pjsip library (`libStart()`). This is generally called once you've got everything configured and set up.

* **hangupAllCalls**() - _(void)_ - Hangs up all existing calls.

* **createRecorder**(< _string_ >filename[, < _string_ >format[, < _integer_ >maxSize]]) - _Media_ - Creates an audio recorder that writes to the given `filename`. `format` can be one of 'ulaw', 'alaw', or 'pcm' (default is 'ulaw'). `maxSize` is the maximum file size (default is no limit).

* **createPlayer**(< _string_ >filename[, < _boolean_ >noLoop]) - _Media_ - Creates an audio player that reads from `filename`. Set `noLoop` to true to disable looping of the audio. When `noLoop` is true, an 'eof' event will be emitted on the Media object when it reaches the end of playback.

* **createPlaylist**(< _array_ >filenames[, < _boolean_ >noLoop]) - _Media_ - Creates an audio player that sequentially reads from the list of `filenames`. Set `noLoop` to true to disable looping of the playlist. When `noLoop` is true, an 'eof' event will be emitted on the Media object when it reaches the end of the playlist.


Exported properties
-------------------

* **version** - _object_ - (Read-only) Contains information about the pjsip library version (`libVersion()`):
    * **major** - _integer_ - The major number.
    * **minor** - _integer_ - The minor number.
    * **rev** - _integer_ - The additional revision number.
    * **suffix** - _string_ - The version suffix (e.g. '-svn').
    * **full** - _string_ - The concatenation of `major`, `minor`, `rev`, and `suffix` (e.g. '2.2.1-svn').
    * **numeric** - _integer_ - The `major`, `minor`, and `rev` as a single integer in the form 0xMMIIRR00 where MM is `major`, II is `minor`, and RR is `rev`.

* **config** - _object_ - (Read-only) Returns the **entire** current (EpConfig) config for pjsip.

* **state** - _string_ - (Read-only) Returns the state of the library/endpoint (`libGetState()`). For example: 'created', 'init', 'starting', 'running', or 'closing'.

* **mediaActivePorts** - _integer_ - (Read-only) Returns the total number of active Media ports.

* **mediaMaxPorts** - _integer_ - (Read-only) Returns the maximum number of Media ports permitted.

Additionally any needed pjsip library constants (may be needed when creating and passing in config objects) are exported as well.


Exported types
--------------

* **Transport** - Represents an underlying (network) interface that Calls and Accounts use.

* **Account** - An entity used for identification purposes for incoming or outgoing requests.


Transport methods
-----------------

* **(constructor)**([< _object_ >transportConfig]) - Creates and returns a new, enabled Transport instance. `transportConfig` is a [TransportConfig](http://www.pjsip.org/pjsip/docs/html/structpj_1_1TransportConfig.htm)-like object for if you need to change any transport options from the library defaults.

* **unref**() - _(void)_ - Detaches the Transport from the event loop.

* **ref**() - _(void)_ - Attaches the Transport to the event loop (default upon instantiation).

* **getInfo**() - _object_ - Returns information (`TransportInfo`) about the transport:
    * **type** - _string_ - Transport type name.
    * **info** - _string_ - Transport string info/description.
    * **flags** - _integer_ - Transport flags (e.g. PJSIP_TRANSPORT_RELIABLE, PJSIP_TRANSPORT_SECURE, PJSIP_TRANSPORT_DATAGRAM).
    * **localAddress** - _string_ - Local/bound address.
    * **localName** - _string_ - Published address.
    * **usageCount** - _integer_ - Current number of objects currently referencing this transport.

* **disable**() - _(void)_ - Disables the transport. Disabling a transport does not necessarily close the socket, it will only discard incoming messages and prevent the transport from being used to send outgoing messages.

* **enable**() - _(void)_ - Enables the transport. Transports are automatically enabled upon creation, so you don't need to call this method unless you explicitly disable the transport first.


Transport properties
--------------------

* **enabled** - _boolean_ - (Read-only) Indicates if the transport is currently enabled or not.


Account methods
---------------

* **(constructor)**(< _object_ >accountConfig) - Creates and returns a new Account instance. `accountConfig` is an [AccountConfig](http://www.pjsip.org/pjsip/docs/html/structpj_1_1AccountConfig.htm)-like object.

* **unref**() - _(void)_ - Detaches the Account from the event loop.

* **ref**() - _(void)_ - Attaches the Account to the event loop (default upon instantiation).

* **modify**(< _object_ >accountConfig) - _(void)_ - Reconfigure the Account with the given `accountConfig`.

* **getInfo**() - _object_ - Returns information (`AccountInfo`) about the account:
    * **uri** - _string_ - The account's URI.
    * **regIsConfigured** - _boolean_ - Flag to tell whether this account has registration setting (reg_uri is not empty).
    * **regIsActive** - _boolean_ - Flag to tell whether this account is currently registered (has active registration session).
    * **regExpiresSec** - _integer_ - An up to date expiration interval for account registration session.

* **setRegistration**(< _boolean_ >renew) - _(void)_ - Update registration or perform unregistration. You only need to call this method if you want to manually update the registration or want to unregister from the server. If `renew` is false, this will begin the unregistration process.

* **setTransport**(< _Transport_ >trans) - _(void)_ - Lock/bind the given transport to this account. Normally you shouldn't need to do this, as transports will be selected automatically by the library according to the destination. When an account is locked/bound to a specific transport, all outgoing requests from this account will use the specified transport (this includes SIP registration, dialog (call and event subscription), and out-of-dialog requests such as MESSAGE).

* **makeCall**(< _string_ >destination) - _Call_ - Start a new SIP call to `destination`.


Account properties
------------------

* **valid** - _boolean_ - (Read-only) Is the Account still valid?

* **default** - _boolean_ - (Read/Write) Is this the default Account for when no other Account matches a request?


Account events
--------------

* **registering**() - The registration process has started.

* **unregistering**() - The unregistration process has started.

* **registered**() - The registration process has completed.

* **unregistered**() - The unregistration process has completed.

* **state**(< _boolean_ >active, < _integer_ >statusCode) - The account state has changed. `active` indicates if registration is active. `statusCode` refers to the relevant SIP status code.

* **call**(< _object_ >info, < _Call_ >call) - An incoming call request. `info` contains:
    * **srcAddress** - _string_ - The ip (and port) of the request.
    * **localUri** - _string_ - Local SIP URI.
    * **localContact** - _string_ - Local Contact field.
    * **remoteUri** - _string_ - Remote SIP URI.
    * **remoteContact** - _string_ - Remote Contact field.
    * **callId** - _string_ - The Call-ID field.


Call methods
------------

* **answer**([< _integer_ >statusCode[, < _string_ >reason]]) - _(void)_ - For incoming calls, this responds to the INVITE with an optional `statusCode` (defaults to 200) and optional `reason` phrase.

* **hangup**([< _integer_ >statusCode[, < _string_ >reason]]) - _(void)_ - Hangs up the call with an optional `statusCode` (defaults to 603) and optional `reason` phrase. This function is different than answering the call with 3xx-6xx response (with answer()), in that this function will hangup the call regardless of the state and role of the call, while answer() only works with incoming calls on EARLY state.

* **hold**() - _(void)_ - Puts the call on hold.

* **reinvite**() - _(void)_ - Releases a hold.

* **update**() - _(void)_ - Sends an UPDATE request.

* **transfer**(< _string_ >destination) - _(void)_ - Transfers the call to `destination`.

* **dtmf**(< _string_ >digits) - _(void)_ - Sends DTMF digits to the remote end using the RFC 2833 payload format.

* **unref**() - _(void)_ - Detaches the Call from the event loop (default).

* **ref**() - _(void)_ - Attaches the Call to the event loop.

* **getStatsDump**([< _boolean_ >inclMediaStats[, < _string_ >indent]]) - _string_ - Returns formatted statistics about the call. If `inclMediaStats` is true, then statistics about the Call's media is included (default is true). `indent` is the string to use for indenting (default is two spaces).


Call properties
---------------

* **connDuration** - _double_ - (Read-only) Call connected duration (zero when call is not established).

* **totalDuration** - _double_ - (Read-only) Total call duration, including set-up time.

* **hasMedia** - _boolean_ - (Read-only) True if the Call has active media.

* **isActive** - _boolean_ - (Read-only) True if the call has an active INVITE session and the INVITE session has not been disconnected.


Call events
-----------

* **state**(< _string_ >state) - The call state has changed. `state` is one of: 'calling', 'incoming', 'early', 'connecting', 'confirmed', or 'disconnected'.

* **dtmf**(< _string_ >digit) - A DTMF digit has been received from the remote end.

* **media**(< _array_ >medias) - The list of Medias associated with this call have changed and the current list is available in `medias`.


Media methods
-------------

* **startTransmitTo**(< _Media_ >sink) - _(void)_ - Starts transmitting to `sink`.

* **stopTransmitTo**(< _Media_ >sink) - _(void)_ - Stops transmitting to `sink`.


Media properties
----------------

* **dir** - _string_ - Returns the direction of the media from our perspective. The value is one of: 'none', 'inbound', 'outbound', 'bidirectional', or 'unknown'.

* **rtpAddr** - _string_ - Returns the remote address (and port) of where the RTP originates.

* **rtcpAddr** - _string_ - Returns the remote address (and port) of where the RTCP originates.


Media events
------------

* **eof**() - This is only applicable to player or playlist Media objects and indicates that the end of the file or end of playlist has been reached.

