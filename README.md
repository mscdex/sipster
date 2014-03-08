Description
===========

A [pjsip](http://www.pjsip.org) binding for node.js.


Requirements
============

* Non-Windows OS
* [pjsip](http://www.pjsip.org/) -- v2.2 or newer
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
var transport = new lib.Transport({ port: 5060 });

// set up a SIP account, we need at least one -- as requried by pjsip
// this sets up an account for calls coming from 192.168.100.10
var acct = new lib.Account({
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
    console.log('=== DTMF digit pressed: ' + digit);
  });

  // answer the call with a 200 status code
  call.answer(200);
});

// finalize the initialization phase ...
lib.start();
```


API
===

```TODO```