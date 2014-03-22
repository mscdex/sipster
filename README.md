Description
===========

A [pjsip](http://www.pjsip.org) binding for node.js.


Requirements
============

* Non-Windows OS
* [pjsip](http://www.pjsip.org/) -- v2.2.1 or newer
* [node.js](http://nodejs.org/) -- v0.10.0 or newer


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

// set up a SIP account, we need at least one -- as requried by pjsip
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

```TODO```