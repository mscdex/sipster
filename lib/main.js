var EventEmitter = require('events').EventEmitter,
    inherits = require('util').inherits;

var addon = require('../build/Release/sipster.node');
addon.Account.prototype.__proto__ = EventEmitter.prototype;
addon.Call.prototype.__proto__ = EventEmitter.prototype;

module.exports = {
  init: function(cfg) {
    addon.init(JSON.stringify(cfg));
  },
  start: addon.start,
  hangupAllCalls: addon.hangupAllCalls,

  get config() {
    return JSON.parse(addon.config());
  },
  get state() {
    return addon.state();
  },
  get mediaActivePorts() {
    return addon.mediaActivePorts();
  },
  version: addon.version(),

  Transport: Transport,
  Account: addon.Account
};


function Transport(cfg) {
  if (!(this instanceof Transport))
    return new Transport(cfg);
  if (!cfg)
    cfg = { type: 'UDP' };
  else if (!cfg.type)
    cfg.type = 'UDP';
  this._id = addon.transportCreate(cfg);
  this._enabled = true;
  this._closed = false;
}
inherits(Transport, EventEmitter);

Transport.prototype.enable = function() {
  if (!this._enabled && !this._closed) {
    addon.transportSetEnable(this._id, true);
    this._enabled = true;
  }
};
Transport.prototype.disable = function() {
  if (this._enabled && !this._closed) {
    addon.transportSetEnable(this._id, false);
    this._enabled = false;
  }
};
Transport.prototype.close = function() {
  if (!this._closed) {
    addon.transportClose(this._id);
    this._closed = true;
  }
};
Object.defineProperty(Transport.prototype, 'config', {
  get: function() {
    return JSON.parse(addon.transportConfig(this._id));
  }
});
Object.defineProperty(Transport.prototype, 'enabled', {
  get: function() {
    return this._enabled;
  }
});
Object.defineProperty(Transport.prototype, 'closed', {
  get: function() {
    return this._closed;
  }
});
Object.defineProperty(Transport.prototype, 'info', {
  get: function() {
    return JSON.parse(addon.transportGetInfo(this._id));
  }
});
