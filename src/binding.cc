#include <node.h>
#include <string.h>
#include <strings.h>
#include <pjsua2.hpp>

using namespace std;
using namespace node;
using namespace v8;
using namespace pj;

class SIPSTERAccount;
class SIPSTERCall;

#define JS2PJ_INT(js, prop, pj) do {                                \
  val = js->Get(String::New(#prop));                                \
  if (val->IsInt32()) {                                             \
    pj.prop = val->Int32Value();                                    \
  }                                                                 \
} while(0)
#define JS2PJ_UINT(js, prop, pj) do {                               \
  val = js->Get(String::New(#prop));                                \
  if (val->IsUint32()) {                                            \
    pj.prop = val->Uint32Value();                                   \
  }                                                                 \
} while(0)
#define JS2PJ_ENUM(js, prop, type, pj) do {                         \
  val = js->Get(String::New(#prop));                                \
  if (val->IsInt32()) {                                             \
    pj.prop = static_cast<type>(val->Int32Value());                 \
  }                                                                 \
} while(0)
#define JS2PJ_STR(js, prop, pj) do {                                \
  val = js->Get(String::New(#prop));                                \
  if (val->IsString()) {                                            \
    pj.prop = string(*String::AsciiValue(val->ToString()));         \
  }                                                                 \
} while(0)
#define JS2PJ_BOOL(js, prop, pj) do {                               \
  val = js->Get(String::New(#prop));                                \
  if (val->IsBoolean()) {                                           \
    pj.prop = val->BooleanValue();                                  \
  }                                                                 \
} while(0)

#define PJ2JS_INT(pj, prop, js) do {                                \
  obj->Set(String::New(#prop), Integer::New(pj.prop));              \
} while(0)
#define PJ2JS_UINT(pj, prop, js) do {                               \
  obj->Set(String::New(#prop), Integer::NewFromUnsigned(pj.prop));  \
} while(0)
#define PJ2JS_STR(obj, prop, dest) do {                             \
  obj->Set(String::New(#prop), String::New(pj.prop.c_str()));       \
} while(0)
#define PJ2JS_BOOL(obj, prop, dest) do {                            \
  obj->Set(String::New(#prop), Boolean::New(pj.prop.c_str()));      \
} while(0)

#define ENQUEUE_EVENT(ev) do {                                      \
  uv_mutex_lock(&event_mutex);                                      \
  event_queue.push_back(ev);                                        \
  uv_mutex_unlock(&event_mutex);                                    \
  uv_async_send(&dumb);                                             \
} while(0)

#define SETUP_EVENT(name)                                           \
  SIPEventInfo ev;                                                  \
  EV_ARGS_##name* args = new EV_ARGS_##name;                        \
  ev.type = EVENT_##name;                                           \
  ev.args = reinterpret_cast<void*>(args)

#define SETUP_EVENT_NOARGS(name)                                    \
  SIPEventInfo ev;                                                  \
  ev.type = EVENT_##name

// registration change event(s) ================================================
#define N_REGSTATE_FIELDS 2
#define REGSTATE_FIELDS                                               \
  X(REGSTATE, bool, active, Boolean, active)                          \
  X(REGSTATE, int, statusCode, Integer, statusCode)
struct EV_ARGS_REGSTATE {
#define X(kind, ctype, name, v8type, valconv) ctype name;
  REGSTATE_FIELDS
#undef X
};
// =============================================================================

// incoming call event =========================================================
#define N_INCALL_FIELDS 7
#define INCALL_FIELDS                                               \
  X(INCALL, string, srcAddress, String, srcAddress.c_str())         \
  X(INCALL, string, localUri, String, localUri.c_str())             \
  X(INCALL, string, localContact, String, localContact.c_str())     \
  X(INCALL, string, remoteUri, String, remoteUri.c_str())           \
  X(INCALL, string, remoteContact, String, remoteContact.c_str())   \
  X(INCALL, string, callId, String, callId.c_str())
struct EV_ARGS_INCALL {
#define X(kind, ctype, name, v8type, valconv) ctype name;
  INCALL_FIELDS
#undef X
};
// =============================================================================

// call state change event(s) ==================================================
#define N_CALLSTATE_FIELDS 1
#define CALLSTATE_FIELDS                                            \
  X(CALLSTATE, pjsip_inv_state, _state, Integer, _state)
struct EV_ARGS_CALLSTATE {
#define X(kind, ctype, name, v8type, valconv) ctype name;
  CALLSTATE_FIELDS
#undef X
};
// =============================================================================

// DTMF event ==================================================================
#define N_CALLDTMF_FIELDS 2
#define CALLDTMF_FIELDS                                             \
  X(CALLDTMF, string, digit, String, digit.c_str())
struct EV_ARGS_CALLDTMF {
#define X(kind, ctype, name, v8type, valconv) ctype name;
  CALLDTMF_FIELDS
#undef X
};
// =============================================================================

#define X(kind, ctype, name, v8type, valconv)                       \
  static Persistent<String> kind##_##name##_symbol;
  INCALL_FIELDS
  CALLDTMF_FIELDS
  REGSTATE_FIELDS
#undef X
#define EVENT_TYPES                                                 \
  X(INCALL)                                                         \
  X(CALLSTATE)                                                      \
  X(REGSTATE)
  X(CALLDTMF)                                                       \
#define EVENT_SYMBOLS                                               \
  X(INCALL, call)                                                   \
  X(CALLSTATE, calling)                                             \
  X(CALLSTATE, incoming)                                            \
  X(CALLSTATE, early)                                               \
  X(CALLSTATE, connecting)                                          \
  X(CALLSTATE, confirmed)                                           \
  X(CALLSTATE, disconnected)                                        \
  X(CALLSTATE, state)                                               \
  X(CALLDTMF, dtmf)                                                 \
  X(REGSTATE, registered)                                           \
  X(REGSTATE, unregistered)                                         \
  X(REGSTATE, state)

// start generic event-related definitions =====================================
#define X(kind, literal)                                            \
  static Persistent<String> ev_##kind##_##literal##_symbol;
  EVENT_SYMBOLS
#undef X

enum SIPEvent {
#define X(kind)                                                     \
  EVENT_##kind,
  EVENT_TYPES
#undef X
};
struct SIPEventInfo {
  SIPEvent type;
  SIPSTERCall* call;
  SIPSTERAccount* acct;
  void* args;
};
static list<SIPEventInfo> event_queue;
static uv_mutex_t event_mutex;
static uv_mutex_t async_mutex;
// =============================================================================

// start PJSUA2-specific definitions ===========================================
static Endpoint *ep = new Endpoint;
static bool ep_init = false;
static bool ep_start = false;
static EpConfig ep_cfg;
// =============================================================================

static Persistent<FunctionTemplate> SIPSTERCall_constructor;
static Persistent<FunctionTemplate> SIPSTERAccount_constructor;
static Persistent<String> emit_symbol;
static uv_async_t dumb;

class SIPSTERCall : public Call, public ObjectWrap {
public:
  Persistent<Function> emit;

  SIPSTERCall(Account &acc, int call_id=PJSUA_INVALID_ID) : Call(acc, call_id) {
    uv_mutex_lock(&async_mutex);
    uv_ref(reinterpret_cast<uv_handle_t*>(&dumb));
    uv_mutex_unlock(&async_mutex);
  }
  ~SIPSTERCall() {
    emit.Dispose();
    emit.Clear();
    uv_mutex_lock(&async_mutex);
    uv_unref(reinterpret_cast<uv_handle_t*>(&dumb));
    uv_mutex_unlock(&async_mutex);
  }

  virtual void onCallState(OnCallStateParam &prm) {
    CallInfo ci = getInfo();

    SETUP_EVENT(CALLSTATE);
    ev.call = this;

    args->_state = ci.state;

    ENQUEUE_EVENT(ev);
  }

  virtual void onDtmfDigit(OnDtmfDigitParam &prm) {
    CallInfo ci = getInfo();

    SETUP_EVENT(CALLDTMF);
    ev.call = this;

    args->digit = prm.digit;

    ENQUEUE_EVENT(ev);
  }

  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;

    if (!args.IsConstructCall()) {
      return ThrowException(Exception::TypeError(
        String::New("Use `new` to create instances of this object."))
      );
    }

    SIPSTERCall *call = NULL;
    if (args.Length() > 0) {
      if (SIPSTERAccount_constructor->HasInstance(args[0])) {
        Local<Object> acct_inst = Local<Object>(Object::Cast(*args[0]));
        Account *acct = ObjectWrap::Unwrap<Account>(acct_inst);
        call = new SIPSTERCall(*acct);
      } else
        call = static_cast<SIPSTERCall*>(External::Unwrap(args[0]));
    } else {
      return ThrowException(Exception::TypeError(
        String::New("Expected callId or Account argument"))
      );
    }

    call->Wrap(args.This());

    call->emit = Persistent<Function>::New(
      Local<Function>::Cast(call->handle_->Get(emit_symbol))
    );

    return args.This();
  }

  static Handle<Value> Answer(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall *call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    CallOpParam prm;
    if (args.Length() > 0 && args[0]->IsUint32()) {
      prm.statusCode = static_cast<pjsip_status_code>(args[0]->Int32Value());
      if (args.Length() > 1 && args[1]->IsString())
        prm.reason = string(*String::Utf8Value(args[1]->ToString()));
    }

    try {
      call->answer(prm);
    } catch(Error& err) {
      string errstr = "Call.answer() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static Handle<Value> Hangup(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall *call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    CallOpParam prm;
    if (args.Length() > 0 && args[0]->IsUint32()) {
      prm.statusCode = static_cast<pjsip_status_code>(args[0]->Int32Value());
      if (args.Length() > 1 && args[1]->IsString())
        prm.reason = string(*String::Utf8Value(args[1]->ToString()));
    }

    try {
      call->hangup(prm);
    } catch(Error& err) {
      string errstr = "Call.hangup() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static Handle<Value> SetHold(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall *call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    CallOpParam prm;
    if (args.Length() > 0 && args[0]->IsUint32()) {
      prm.statusCode = static_cast<pjsip_status_code>(args[0]->Int32Value());
      if (args.Length() > 1 && args[1]->IsString())
        prm.reason = string(*String::Utf8Value(args[1]->ToString()));
    }

    try {
      call->setHold(prm);
    } catch(Error& err) {
      string errstr = "Call.setHold() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static Handle<Value> Reinvite(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall *call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    CallOpParam prm;
    if (args.Length() > 0 && args[0]->IsUint32()) {
      prm.statusCode = static_cast<pjsip_status_code>(args[0]->Int32Value());
      if (args.Length() > 1 && args[1]->IsString())
        prm.reason = string(*String::Utf8Value(args[1]->ToString()));
    }

    try {
      call->reinvite(prm);
    } catch(Error& err) {
      string errstr = "Call.reinvite() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static Handle<Value> Update(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall *call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    CallOpParam prm;
    if (args.Length() > 0 && args[0]->IsUint32()) {
      prm.statusCode = static_cast<pjsip_status_code>(args[0]->Int32Value());
      if (args.Length() > 1 && args[1]->IsString())
        prm.reason = string(*String::Utf8Value(args[1]->ToString()));
    }

    try {
      call->update(prm);
    } catch(Error& err) {
      string errstr = "Call.update() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static Handle<Value> DialDtmf(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall *call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    if (args.Length() > 0 && args[0]->IsString()) {
      try {
        call->dialDtmf(string(*String::AsciiValue(args[0]->ToString())));
      } catch(Error& err) {
        string errstr = "Call.update() error: " + err.info();
        return ThrowException(Exception::Error(String::New(errstr.c_str())));
      }
    } else {
      return ThrowException(
        Exception::Error(String::New("Missing DTMF string"))
      );
    }

    return Undefined();
  }

  static Handle<Value> Transfer(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall *call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    string dest;
    CallOpParam prm;
    if (args.Length() > 0 && args[0]->IsString()) {
      dest = string(*String::AsciiValue(args[0]->ToString()));
      if (args.Length() > 1) {
        prm.statusCode = static_cast<pjsip_status_code>(args[1]->Int32Value());
        if (args.Length() > 2 && args[2]->IsString())
          prm.reason = string(*String::Utf8Value(args[1]->ToString()));
      }
    } else {
      return ThrowException(
        Exception::Error(String::New("Missing transfer destination"))
      );
    }

    try {
      call->xfer(dest, prm);
    } catch(Error& err) {
      string errstr = "Call.xfer() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static void Initialize(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    Local<String> name = String::NewSymbol("Call");

    SIPSTERCall_constructor = Persistent<FunctionTemplate>::New(tpl);
    SIPSTERCall_constructor->InstanceTemplate()->SetInternalFieldCount(1);
    SIPSTERCall_constructor->SetClassName(name);

    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "answer", Answer);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "hangup", Hangup);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "hold", SetHold);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "reinvite", Reinvite);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "update", Update);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "dtmf", DialDtmf);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "transfer", Transfer);

    target->Set(name, SIPSTERCall_constructor->GetFunction());
  }
};

class SIPSTERAccount : public Account, public ObjectWrap {
public:
  Persistent<Function> emit;

  SIPSTERAccount() {}
  ~SIPSTERAccount() {
    emit.Dispose();
    emit.Clear();
  }

  virtual void onRegState(OnRegStateParam &prm) {
    AccountInfo ai = getInfo();

    SETUP_EVENT(REGSTATE);
    ev.acct = this;

    args->active = ai.regIsActive;
    args->statusCode = prm.code;

    ENQUEUE_EVENT(ev);
  }

  virtual void onIncomingCall(OnIncomingCallParam &iprm) {
    SIPSTERCall *call = new SIPSTERCall(*this, iprm.callId);
    CallInfo ci = call->getInfo();

    SETUP_EVENT(INCALL);
    ev.call = call;
    ev.acct = this;

    args->localUri = ci.localUri;
    args->localContact = ci.localContact;
    args->remoteUri = ci.remoteUri;
    args->remoteContact = ci.remoteContact;
    args->callId = ci.callIdString;
    args->srcAddress = iprm.rdata.srcAddress;

    ENQUEUE_EVENT(ev);
  }

  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;

    if (!args.IsConstructCall()) {
      return ThrowException(Exception::TypeError(
        String::New("Use `new` to create instances of this object."))
      );
    }

    SIPSTERAccount *acct = new SIPSTERAccount();

    AccountConfig acct_cfg;
    string errstr;
    Local<Value> val;
    if (args.Length() > 0 && args[0]->IsObject()) {
      Local<Object> acct_obj = args[0]->ToObject();
      JS2PJ_INT(acct_obj, priority, acct_cfg);
      JS2PJ_STR(acct_obj, idUri, acct_cfg);

      val = acct_obj->Get(String::New("regConfig"));
      if (val->IsObject()) {
        AccountRegConfig regConfig = acct_cfg.regConfig;
        Local<Object> reg_obj = val->ToObject();
        JS2PJ_STR(reg_obj, registrarUri, regConfig);
        JS2PJ_BOOL(reg_obj, registerOnAdd, regConfig);

        val = reg_obj->Get(String::New("headers"));
        if (val->IsObject()) {
          const Local<Object> hdr_obj = val->ToObject();
          const Local<Array> hdr_props = hdr_obj->GetPropertyNames();
          const uint32_t hdr_length = hdr_props->Length();
          if (hdr_length > 0) {
            vector<SipHeader> sipheaders;
            for (uint32_t i = 0 ; i < hdr_length ; ++i) {
              const Local<Value> key = hdr_props->Get(i);
              const Local<Value> value = hdr_obj->Get(key);
              SipHeader hdr;
              hdr.hName = string(*String::AsciiValue(key->ToString()));
              hdr.hValue = string(*String::AsciiValue(value->ToString()));
              sipheaders.push_back(hdr);
            }
            regConfig.headers = sipheaders;
          }
        }

        JS2PJ_UINT(reg_obj, timeoutSec, regConfig);
        JS2PJ_UINT(reg_obj, retryIntervalSec, regConfig);
        JS2PJ_UINT(reg_obj, firstRetryIntervalSec, regConfig);
        JS2PJ_UINT(reg_obj, delayBeforeRefreshSec, regConfig);
        JS2PJ_BOOL(reg_obj, dropCallsOnFail, regConfig);
        JS2PJ_UINT(reg_obj, unregWaitSec, regConfig);
        JS2PJ_UINT(reg_obj, proxyUse, regConfig);
      }
      val = acct_obj->Get(String::New("sipConfig"));
      if (val->IsObject()) {
        AccountSipConfig sipConfig = acct_cfg.sipConfig;
        Local<Object> sip_obj = val->ToObject();

        val = sip_obj->Get(String::New("authCreds"));
        if (val->IsArray()) {
          const Local<Array> arr_obj = Local<Array>::Cast(val);
          const uint32_t arr_length = arr_obj->Length();
          if (arr_length > 0) {
            vector<AuthCredInfo> creds;
            for (uint32_t i = 0 ; i < arr_length ; ++i) {
              const Local<Value> cred_value = arr_obj->Get(i);

              if (cred_value->IsObject()) {
                const Local<Object> auth_obj = cred_value->ToObject();
                const Local<Array> auth_props = auth_obj->GetPropertyNames();
                const uint32_t auth_length = auth_props->Length();
                if (auth_length > 0) {
                  AuthCredInfo credinfo;
                  credinfo.dataType = -1;
                  for (uint32_t i = 0 ; i < auth_length ; ++i) {
                    const Local<Value> key = auth_props->Get(i);
                    const Local<Value> value = auth_obj->Get(key);
                    const string keystr(*String::AsciiValue(key->ToString()));
                    const string valstr(*String::AsciiValue(value->ToString()));
                    if (keystr == "scheme")
                      credinfo.scheme = valstr;
                    else if (keystr == "realm")
                      credinfo.realm = valstr;
                    else if (keystr == "username")
                      credinfo.username = valstr;
                    else if (keystr == "dataType") {
                      if (valstr == "digest")
                        credinfo.dataType =  PJSIP_CRED_DATA_DIGEST;
                      else
                        credinfo.dataType = PJSIP_CRED_DATA_PLAIN_PASSWD;
                    } else if (keystr == "data")
                      credinfo.data = valstr;
                  }
                  if (credinfo.scheme.length() > 0
                      && credinfo.realm.length() > 0
                      && credinfo.username.length() > 0
                      && credinfo.dataType >= 0)
                    creds.push_back(credinfo);
                }
              }
            }
            if (creds.size() > 0)
              sipConfig.authCreds = creds;
          }
        }

        val = sip_obj->Get(String::New("proxies"));
        if (val->IsArray()) {
          const Local<Array> arr_obj = Local<Array>::Cast(val);
          const uint32_t arr_length = arr_obj->Length();
          if (arr_length > 0) {
            vector<string> proxies;
            for (uint32_t i = 0 ; i < arr_length ; ++i) {
              const Local<Value> value = arr_obj->Get(i);
              proxies.push_back(string(*String::AsciiValue(value->ToString())));
            }
            sipConfig.proxies = proxies;
          }
        }

        JS2PJ_STR(sip_obj, contactForced, sipConfig);
        JS2PJ_STR(sip_obj, contactParams, sipConfig);
        JS2PJ_STR(sip_obj, contactUriParams, sipConfig);
        JS2PJ_BOOL(sip_obj, authInitialEmpty, sipConfig);
        JS2PJ_STR(sip_obj, authInitialAlgorithm, sipConfig);
        // TODO: transportId
      }
      val = acct_obj->Get(String::New("callConfig"));
      if (val->IsObject()) {
        AccountCallConfig callConfig = acct_cfg.callConfig;
        Local<Object> call_obj = val->ToObject();
        JS2PJ_ENUM(call_obj, holdType, pjsua_call_hold_type, callConfig);
        JS2PJ_ENUM(call_obj, prackUse, pjsua_100rel_use, callConfig);
        JS2PJ_ENUM(call_obj, timerUse, pjsua_sip_timer_use, callConfig);
        JS2PJ_UINT(call_obj, timerMinSESec, callConfig);
        JS2PJ_UINT(call_obj, timerSessExpiresSec, callConfig);
      }
      val = acct_obj->Get(String::New("presConfig"));
      if (val->IsObject()) {
        AccountPresConfig presConfig = acct_cfg.presConfig;
        Local<Object> pres_obj = val->ToObject();

        val = pres_obj->Get(String::New("headers"));
        if (val->IsObject()) {
          const Local<Object> hdr_obj = val->ToObject();
          const Local<Array> hdr_props = hdr_obj->GetPropertyNames();
          const uint32_t hdr_length = hdr_props->Length();
          if (hdr_length > 0) {
            vector<SipHeader> sipheaders;
            for (uint32_t i = 0 ; i < hdr_length ; ++i) {
              const Local<Value> key = hdr_props->Get(i);
              const Local<Value> value = hdr_obj->Get(key);
              SipHeader hdr;
              hdr.hName = string(*String::AsciiValue(key->ToString()));
              hdr.hValue = string(*String::AsciiValue(value->ToString()));
              sipheaders.push_back(hdr);
            }
            presConfig.headers = sipheaders;
          }
        }

        JS2PJ_BOOL(pres_obj, publishEnabled, presConfig);
        JS2PJ_BOOL(pres_obj, publishQueue, presConfig);
        JS2PJ_UINT(pres_obj, publishShutdownWaitMsec, presConfig);
        JS2PJ_STR(pres_obj, pidfTupleId, presConfig);
      }
      val = acct_obj->Get(String::New("mwiConfig"));
      if (val->IsObject()) {
        AccountMwiConfig mwiConfig = acct_cfg.mwiConfig;
        Local<Object> mwi_obj = val->ToObject();
        JS2PJ_BOOL(mwi_obj, enabled, mwiConfig);
        JS2PJ_UINT(mwi_obj, expirationSec, mwiConfig);
      }
      val = acct_obj->Get(String::New("natConfig"));
      if (val->IsObject()) {
        AccountNatConfig natConfig = acct_cfg.natConfig;
        Local<Object> nat_obj = val->ToObject();
        JS2PJ_ENUM(nat_obj, sipStunUse, pjsua_stun_use, natConfig);
        JS2PJ_ENUM(nat_obj, mediaStunUse, pjsua_stun_use, natConfig);
        JS2PJ_BOOL(nat_obj, iceEnabled, natConfig);
        JS2PJ_INT(nat_obj, iceMaxHostCands, natConfig);
        JS2PJ_BOOL(nat_obj, iceAggressiveNomination, natConfig);
        JS2PJ_UINT(nat_obj, iceNominatedCheckDelayMsec, natConfig);
        JS2PJ_INT(nat_obj, iceWaitNominationTimeoutMsec, natConfig);
        JS2PJ_BOOL(nat_obj, iceNoRtcp, natConfig);
        JS2PJ_BOOL(nat_obj, iceAlwaysUpdate, natConfig);
        JS2PJ_BOOL(nat_obj, turnEnabled, natConfig);
        JS2PJ_STR(nat_obj, turnServer, natConfig);
        JS2PJ_ENUM(nat_obj, turnConnType, pj_turn_tp_type, natConfig);
        JS2PJ_STR(nat_obj, turnUserName, natConfig);
        JS2PJ_INT(nat_obj, turnPasswordType, natConfig);
        JS2PJ_STR(nat_obj, turnPassword, natConfig);
        JS2PJ_INT(nat_obj, contactRewriteUse, natConfig);
        JS2PJ_INT(nat_obj, contactRewriteMethod, natConfig);
        JS2PJ_INT(nat_obj, viaRewriteUse, natConfig);
        JS2PJ_INT(nat_obj, sdpNatRewriteUse, natConfig);
        JS2PJ_INT(nat_obj, sipOutboundUse, natConfig);
        JS2PJ_STR(nat_obj, sipOutboundInstanceId, natConfig);
        JS2PJ_STR(nat_obj, sipOutboundRegId, natConfig);
        JS2PJ_UINT(nat_obj, udpKaIntervalSec, natConfig);
        JS2PJ_STR(nat_obj, udpKaData, natConfig);
      }
      val = acct_obj->Get(String::New("mediaConfig"));
      if (val->IsObject()) {
        AccountMediaConfig mediaConfig = acct_cfg.mediaConfig;
        Local<Object> media_obj = val->ToObject();

        val = media_obj->Get(String::New("transportConfig"));
        if (val->IsObject()) {
          TransportConfig transportConfig = mediaConfig.transportConfig;
          Local<Object> obj = val->ToObject();
          JS2PJ_UINT(obj, port, transportConfig);
          JS2PJ_UINT(obj, portRange, transportConfig);
          JS2PJ_STR(obj, publicAddress, transportConfig);
          JS2PJ_STR(obj, boundAddress, transportConfig);
          JS2PJ_ENUM(obj, qosType, pj_qos_type, transportConfig);
          //JS2PJ_INT(obj, qosParams, acct_cfg.transportConfig);

          val = obj->Get(String::New("tlsConfig"));
          if (val->IsObject()) {
            TlsConfig tlsConfig = transportConfig.tlsConfig;
            Local<Object> tls_obj = val->ToObject();
            JS2PJ_STR(tls_obj, CaListFile, tlsConfig);
            JS2PJ_STR(tls_obj, certFile, tlsConfig);
            JS2PJ_STR(tls_obj, privKeyFile, tlsConfig);
            JS2PJ_STR(tls_obj, password, tlsConfig);
            JS2PJ_ENUM(tls_obj, method, pjsip_ssl_method, tlsConfig);
            // TODO: ciphers
            JS2PJ_BOOL(tls_obj, verifyServer, tlsConfig);
            JS2PJ_BOOL(tls_obj, verifyClient, tlsConfig);
            JS2PJ_BOOL(tls_obj, requireClientCert, tlsConfig);
            JS2PJ_UINT(tls_obj, msecTimeout, tlsConfig);
            JS2PJ_ENUM(tls_obj, qosType, pj_qos_type, tlsConfig);
            //JS2PJ_INT(tls_obj, qosParams, tlsConfig);
            JS2PJ_BOOL(tls_obj, qosIgnoreError, tlsConfig);
          }
        }

        JS2PJ_BOOL(media_obj, lockCodecEnabled, mediaConfig);
        JS2PJ_BOOL(media_obj, streamKaEnabled, mediaConfig);
        JS2PJ_ENUM(media_obj, srtpUse, pjmedia_srtp_use, mediaConfig);
        JS2PJ_INT(media_obj, srtpSecureSignaling, mediaConfig);
        JS2PJ_ENUM(media_obj, ipv6Use, pjsua_ipv6_use, mediaConfig);
      }
      val = acct_obj->Get(String::New("videoConfig"));
      if (val->IsObject()) {
        AccountVideoConfig videoConfig = acct_cfg.videoConfig;
        Local<Object> vid_obj = val->ToObject();
        JS2PJ_BOOL(vid_obj, autoShowIncoming, videoConfig);
        JS2PJ_BOOL(vid_obj, autoTransmitOutgoing, videoConfig);
        JS2PJ_UINT(vid_obj, windowFlags, videoConfig);
        JS2PJ_INT(vid_obj, defaultCaptureDevice, videoConfig);
        JS2PJ_INT(vid_obj, defaultRenderDevice, videoConfig);
        JS2PJ_ENUM(vid_obj,
                   rateControlMethod,
                   pjmedia_vid_stream_rc_method,
                   videoConfig);
        JS2PJ_UINT(vid_obj, rateControlBandwidth, videoConfig);
      }
    }

    bool isDefault = false;
    if (args.Length() > 1 && args[1]->IsBoolean())
      isDefault = args[1]->BooleanValue();
    else if (args.Length() > 0 && args[0]->IsBoolean())
      isDefault = args[0]->BooleanValue();

    try {
      acct->create(acct_cfg, isDefault);
    } catch(Error& err) {
      delete acct;
      errstr = "Account.create() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    acct->Wrap(args.This());
    //acct->Ref();

    acct->emit = Persistent<Function>::New(
      Local<Function>::Cast(acct->handle_->Get(emit_symbol))
    );

    return args.This();
  }

  static Handle<Value> MakeCall(const Arguments& args) {
    HandleScope scope;

    string dest;
    CallOpParam prm;
    if (args.Length() > 0 && args[0]->IsString()) {
      dest = string(*String::AsciiValue(args[0]->ToString()));
      if (args.Length() > 1) {
        prm.statusCode = static_cast<pjsip_status_code>(args[1]->Int32Value());
        if (args.Length() > 2 && args[2]->IsString())
          prm.reason = string(*String::Utf8Value(args[1]->ToString()));
      }
    } else
      return ThrowException(Exception::Error(String::New("Missing call destination")));

    Handle<Value> new_call_args[1] = { args.This() };
    Local<Object> call_obj = SIPSTERCall_constructor->GetFunction()->NewInstance(1, new_call_args);
    SIPSTERCall *call = ObjectWrap::Unwrap<SIPSTERCall>(call_obj);

    try {
      call->makeCall(dest, prm);
    } catch(Error& err) {
      string errstr = "Call.makeCall() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return call_obj;
  }

  static void Initialize(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    Local<String> name = String::NewSymbol("Account");

    SIPSTERAccount_constructor = Persistent<FunctionTemplate>::New(tpl);
    SIPSTERAccount_constructor->InstanceTemplate()->SetInternalFieldCount(1);
    SIPSTERAccount_constructor->SetClassName(name);

    NODE_SET_PROTOTYPE_METHOD(SIPSTERAccount_constructor, "makeCall", MakeCall);

    target->Set(name, SIPSTERAccount_constructor->GetFunction());
  }
};

// start event processing-related definitions ==================================
static Persistent<Object> global_context;
void dumb_cb(uv_async_t* handle, int status) {
  while (true) {
    uv_mutex_lock(&event_mutex);
    if (event_queue.empty())
      break;
    const SIPEventInfo ev = event_queue.front();
    event_queue.pop_front();
    uv_mutex_unlock(&event_mutex);

    HandleScope scope;
    TryCatch try_catch;
    switch (ev.type) {
      case EVENT_INCALL: {
        Local<Object> obj = Object::New();
        EV_ARGS_INCALL* args = reinterpret_cast<EV_ARGS_INCALL*>(ev.args);
#define X(kind, ctype, name, v8type, valconv) \
        obj->Set(kind##_##name##_symbol, v8type::New(args->valconv));
        INCALL_FIELDS
#undef X
        SIPSTERAccount* acct = ev.acct;
        SIPSTERCall* call = ev.call;
        Local<Value> new_call_args[1] = { External::Wrap(call) };
        Local<Object> call_obj;
        call_obj = SIPSTERCall_constructor->GetFunction()
                                          ->NewInstance(1, new_call_args);
        Handle<Value> emit_argv[3] = { ev_INCALL_call_symbol, obj, call_obj };
        call->emit->Call(acct->handle_, 3, emit_argv);
        delete args;
      }
      break;
      case EVENT_REGSTATE: {
        EV_ARGS_REGSTATE* args = reinterpret_cast<EV_ARGS_REGSTATE*>(ev.args);
        SIPSTERAccount* acct = ev.acct;
        Handle<Value> emit_argv[1] = {
          args->active
          ? ev_REGSTATE_registered_symbol
          : ev_REGSTATE_unregistered_symbol
        };
        acct->emit->Call(acct->handle_, 1, emit_argv);
        Handle<Value> emit_catchall_argv[N_REGSTATE_FIELDS + 1] = {
          ev_CALLSTATE_state_symbol,
#define X(kind, ctype, name, v8type, valconv) \
          v8type::New(args->valconv),
          REGSTATE_FIELDS
#undef X
        };
        acct->emit->Call(acct->handle_,
                         N_REGSTATE_FIELDS + 1,
                         emit_catchall_argv);
        delete args;
      }
      break;
      case EVENT_CALLSTATE: {
        EV_ARGS_CALLSTATE* args = reinterpret_cast<EV_ARGS_CALLSTATE*>(ev.args);
        Handle<Value> ev_name;
        switch (args->_state) {
          case PJSIP_INV_STATE_CALLING:
            ev_name = ev_CALLSTATE_calling_symbol;
          break;
          case PJSIP_INV_STATE_INCOMING:
            ev_name = ev_CALLSTATE_incoming_symbol;
          break;
          case PJSIP_INV_STATE_EARLY:
            ev_name = ev_CALLSTATE_early_symbol;
          break;
          case PJSIP_INV_STATE_CONNECTING:
            ev_name = ev_CALLSTATE_connecting_symbol;
          break;
          case PJSIP_INV_STATE_CONFIRMED:
            ev_name = ev_CALLSTATE_confirmed_symbol;
          break;
          case PJSIP_INV_STATE_DISCONNECTED:
            ev_name = ev_CALLSTATE_disconnected_symbol;
          break;
          default:
          break;
        }
        if (!ev_name.IsEmpty()) {
          SIPSTERCall* call = ev.call;
          Handle<Value> emit_argv[1] = { ev_name };
          call->emit->Call(call->handle_, 1, emit_argv);
          Handle<Value> emit_catchall_argv[2] = { ev_CALLSTATE_state_symbol, ev_name };
          call->emit->Call(call->handle_, 2, emit_catchall_argv);
        }
        delete args;
      }
      break;
      case EVENT_CALLDTMF: {
        HandleScope scope;
        EV_ARGS_CALLDTMF* args = reinterpret_cast<EV_ARGS_CALLDTMF*>(ev.args);
        SIPSTERCall* call = ev.call;
        // TODO: make a symbol for each DTMF digit and use that instead?
        Local<Value> dtmf_char = String::New(args->digit.c_str());
        Handle<Value> emit_argv[2] = { ev_CALLDTMF_dtmf_symbol, dtmf_char };
        call->emit->Call(call->handle_, 2, emit_argv);
        delete args;
      }
      break;
    }
    if (try_catch.HasCaught())
      FatalException(try_catch);
  }
  uv_mutex_unlock(&event_mutex);
}
// =============================================================================

// static methods ==============================================================

static Handle<Value> EPVersion(const Arguments& args) {
  HandleScope scope;

  pj::Version v = ep->libVersion();
  Local<Object> vinfo = Object::New();
  vinfo->Set(String::New("major"), Integer::New(v.major));
  vinfo->Set(String::New("minor"), Integer::New(v.minor));
  vinfo->Set(String::New("rev"), Integer::New(v.rev));
  vinfo->Set(String::New("suffix"), String::New(v.suffix.c_str()));
  vinfo->Set(String::New("full"), String::New(v.full.c_str()));
  vinfo->Set(String::New("numeric"), Integer::New(v.numeric));

  return scope.Close(vinfo);
}

static Handle<Value> EPInit(const Arguments& args) {
  HandleScope scope;
  string errstr;

  if (ep_init)
    return ThrowException(Exception::Error(String::New("Already initialized")));

  try {
    ep->libCreate();
  } catch(Error& err) {
    errstr = "libCreate error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
    return Undefined();
  }

  /*if (args.Length() == 1 && args[0]->IsString()) {
    JsonDocument rdoc;
    String::Utf8Value str(args[0]->ToString());
    try {
      rdoc.loadString(reinterpret_cast<const char*>(*str));
    } catch(Error& err) {
      errstr = "JsonDocument.loadString error: " + err.info();
      ThrowException(Exception::Error(String::New(errstr.c_str())));
      return Undefined();
    }
    try {
      rdoc.readObject(ep_cfg);
    } catch(Error& err) {
      errstr = "JsonDocument.readObject(EpConfig) error: " + err.info();
      ThrowException(Exception::Error(String::New(errstr.c_str())));
      return Undefined();
    }
  }*/

  /*ep_cfg.logConfig.msgLogging = PJ_LOG_HAS_NEWLINE | PJ_LOG_HAS_COLOR | PJ_LOG_HAS_LEVEL_TEXT;
  ep_cfg.logConfig.level = 5;
  ep_cfg.logConfig.consoleLevel = 5;
  ep_cfg.logConfig.decor = PJ_LOG_HAS_NEWLINE | PJ_LOG_HAS_COLOR | PJ_LOG_HAS_LEVEL_TEXT;*/

  try {
    ep->libInit(ep_cfg);
    ep_init = true;
  } catch(Error& err) {
    errstr = "libCreate error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
    return Undefined();
  }

  if ((args.Length() == 1 && args[0]->IsBoolean() && args[0]->BooleanValue())
      || (args.Length() > 1 && args[1]->IsBoolean() && args[1]->BooleanValue())) {
    if (ep_start)
      return ThrowException(Exception::Error(String::New("Already started")));
    try {
      ep->libStart();
      ep_start = true;
    } catch(Error& err) {
      string errstr = "libStart error: " + err.info();
      ThrowException(Exception::Error(String::New(errstr.c_str())));
      return Undefined();
    }
  }
  return Undefined();
}

static Handle<Value> EPStart(const Arguments& args) {
  HandleScope scope;

  if (ep_start)
    return ThrowException(Exception::Error(String::New("Already started")));
  else if (!ep_init)
    return ThrowException(Exception::Error(String::New("Not initialized yet")));

  try {
    ep->libStart();
    ep_start = true;
  } catch(Error& err) {
    string errstr = "libStart error: " + err.info();
    return ThrowException(Exception::Error(String::New(errstr.c_str())));
  }
  return Undefined();
}

static Handle<Value> EPGetConfig(const Arguments& args) {
  HandleScope scope;
  string errstr;

  JsonDocument wdoc;

  try {
    wdoc.writeObject(ep_cfg);
  } catch(Error& err) {
    errstr = "JsonDocument.writeObject(EpConfig) error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
    return Undefined();
  }

  try {
    return scope.Close(String::New(wdoc.saveString().c_str()));
  } catch(Error& err) {
    errstr = "JsonDocument.saveString error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
    return Undefined();
  }
}

static Handle<Value> EPGetState(const Arguments& args) {
  HandleScope scope;

  try {
    pjsua_state st = ep->libGetState();
    string state;
    switch (st) {
      case PJSUA_STATE_CREATED:
        state = "CREATED";
      break;
      case PJSUA_STATE_INIT:
        state = "INIT";
      break;
      case PJSUA_STATE_STARTING:
        state = "STARTING";
      break;
      case PJSUA_STATE_RUNNING:
        state = "RUNNING";
      break;
      case PJSUA_STATE_CLOSING:
        state = "CLOSING";
      break;
      default:
        return Null();
    }
    return scope.Close(String::New(state.c_str()));
  } catch(Error& err) {
    string errstr = "libGetState error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
    return Undefined();
  }
}

static Handle<Value> EPHangupAllCalls(const Arguments& args) {
  HandleScope scope;

  try {
    ep->hangupAllCalls();
  } catch(Error& err) {
    string errstr = "hangupAllCalls error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
  }
  return Undefined();
}

static Handle<Value> EPMediaActivePorts(const Arguments& args) {
  HandleScope scope;

  return scope.Close(Integer::NewFromUnsigned(ep->mediaActivePorts()));
}

static Handle<Value> EPTransportSetEnable(const Arguments& args) {
  HandleScope scope;

  try {
    ep->transportSetEnable(args[0]->Int32Value(), args[1]->BooleanValue());
  } catch(Error& err) {
    string errstr = "transportSetEnable error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
  }
  return Undefined();
}

static Handle<Value> EPTransportCreate(const Arguments& args) {
  HandleScope scope;
  TransportConfig tp_cfg;
  string errstr;

  Local<Value> val;
  if (args.Length() > 0 && args[0]->IsObject()) {
    Local<Object> obj = args[0]->ToObject();
    JS2PJ_UINT(obj, port, tp_cfg);
    JS2PJ_UINT(obj, portRange, tp_cfg);
    JS2PJ_STR(obj, publicAddress, tp_cfg);
    JS2PJ_STR(obj, boundAddress, tp_cfg);
    JS2PJ_ENUM(obj, qosType, pj_qos_type, tp_cfg);
    //JS2PJ_INT(obj, qosParams, tp_cfg);

    val = obj->Get(String::New("tlsConfig"));
    if (val->IsObject()) {
      Local<Object> tls_obj = val->ToObject();
      JS2PJ_STR(tls_obj, CaListFile, tp_cfg.tlsConfig);
      JS2PJ_STR(tls_obj, certFile, tp_cfg.tlsConfig);
      JS2PJ_STR(tls_obj, privKeyFile, tp_cfg.tlsConfig);
      JS2PJ_STR(tls_obj, password, tp_cfg.tlsConfig);
      JS2PJ_ENUM(tls_obj, method, pjsip_ssl_method, tp_cfg.tlsConfig);
      // TODO: ciphers
      JS2PJ_BOOL(tls_obj, verifyServer, tp_cfg.tlsConfig);
      JS2PJ_BOOL(tls_obj, verifyClient, tp_cfg.tlsConfig);
      JS2PJ_BOOL(tls_obj, requireClientCert, tp_cfg.tlsConfig);
      JS2PJ_UINT(tls_obj, msecTimeout, tp_cfg.tlsConfig);
      JS2PJ_ENUM(tls_obj, qosType, pj_qos_type, tp_cfg.tlsConfig);
      //JS2PJ_INT(tls_obj, qosParams, tp_cfg.tlsConfig);
      JS2PJ_BOOL(tls_obj, qosIgnoreError, tp_cfg.tlsConfig);
    }
  }

  pjsip_transport_type_e transportType = PJSIP_TRANSPORT_UDP;
  /*String::AsciiValue typestr(args[0]->ToString());
  const char* typecstr = *typestr;
  if (strcasecmp(typecstr, "udp") == 0)
    transportType = PJSIP_TRANSPORT_UDP;
  else if (strcasecmp(typecstr, "tcp") == 0)
    transportType = PJSIP_TRANSPORT_TCP;
#if defined(PJ_HAS_IPV6) && PJ_HAS_IPV6!=0
  else if (strcasecmp(typecstr, "udp6") == 0)
    transportType = PJSIP_TRANSPORT_UDP6;
  else if (strcasecmp(typecstr, "tcp6") == 0)
    transportType = PJSIP_TRANSPORT_TCP6;
#endif
#if defined(PJSIP_HAS_TLS_TRANSPORT) && PJSIP_HAS_TLS_TRANSPORT!=0
  else if (strcasecmp(typecstr, "tls") == 0)
    transportType = PJSIP_TRANSPORT_TLS;
# if defined(PJ_HAS_IPV6) && PJ_HAS_IPV6!=0
  else if (strcasecmp(typecstr, "tls6") == 0)
    transportType = PJSIP_TRANSPORT_TLS6;
# endif
#endif
  else {
    ThrowException(Exception::Error(String::New("Unsupported transport type")));
    return Undefined();
  }*/

  int tid;
  try {
    tid = ep->transportCreate(transportType, tp_cfg);
  } catch(Error& err) {
    errstr = "transportCreate error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
    return Undefined();
  }

  uv_mutex_lock(&async_mutex);
  uv_ref(reinterpret_cast<uv_handle_t*>(&dumb));
  uv_mutex_unlock(&async_mutex);
  return scope.Close(Integer::New(tid));
}

static Handle<Value> EPTransportClose(const Arguments& args) {
  HandleScope scope;

  try {
    ep->transportClose(args[0]->Int32Value());
    uv_mutex_lock(&async_mutex);
    uv_unref(reinterpret_cast<uv_handle_t*>(&dumb));
    uv_mutex_unlock(&async_mutex);
  } catch(Error& err) {
    string errstr = "transportClose error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
  }
  return Undefined();
}

static Handle<Value> EPTransportGetInfo(const Arguments& args) {
  HandleScope scope;

  try {
    TransportInfo tp_info = ep->transportGetInfo(args[0]->Int32Value());
    Local<Object> info = Object::New();
    Local<Array> flags = Array::New();

    string type;
    switch (tp_info.type) {
      case PJSIP_TRANSPORT_UDP:
        type = "udp";
      break;
      case PJSIP_TRANSPORT_TCP:
        type = "tcp";
      break;
      case PJSIP_TRANSPORT_TLS:
        type = "tls";
      break;
      case PJSIP_TRANSPORT_UDP6:
        type = "udp6";
      break;
      case PJSIP_TRANSPORT_TCP6:
        type = "tcp6";
      break;
      case PJSIP_TRANSPORT_TLS6:
        type = "tls6";
      break;
      default:
        type = "unspecified";
    }

    int i = 0;
    if (tp_info.flags & PJSIP_TRANSPORT_RELIABLE)
      flags->Set(i++, String::New("reliable"));
    if (tp_info.flags & PJSIP_TRANSPORT_SECURE)
      flags->Set(i++, String::New("secure"));
    if (tp_info.flags & PJSIP_TRANSPORT_DATAGRAM)
      flags->Set(i++, String::New("datagram"));

    info->Set(String::New("type"), String::New(type.c_str()));
    info->Set(String::New("typeName"), String::New(tp_info.typeName.c_str()));
    info->Set(String::New("info"), String::New(tp_info.info.c_str()));
    info->Set(String::New("flags"), flags);
    info->Set(String::New("localAddress"), String::New(tp_info.localAddress.c_str()));
    info->Set(String::New("localName"), String::New(tp_info.localName.c_str()));
    info->Set(String::New("usageCount"), Integer::NewFromUnsigned(tp_info.usageCount));

    return scope.Close(info);
  } catch(Error& err) {
    string errstr = "transportGetInfo error: " + err.info();
    ThrowException(Exception::Error(String::New(errstr.c_str())));
    return Undefined();
  }
}

extern "C" {
  void init(Handle<Object> target) {
    HandleScope scope;

#define X(kind, literal) ev_##kind##_##literal##_symbol = NODE_PSYMBOL(#literal);
  EVENT_SYMBOLS
#undef X
#define X(kind, ctype, name, v8type, valconv)              \
    kind##_##name##_symbol = NODE_PSYMBOL(#name);
  INCALL_FIELDS
  CALLDTMF_FIELDS
#undef X

    uv_async_init(uv_default_loop(), &dumb, dumb_cb);
    uv_mutex_init(&event_mutex);
    uv_mutex_init(&async_mutex);
    global_context = Persistent<Object>::New(Context::GetCurrent()->Global());
    emit_symbol = NODE_PSYMBOL("emit");

    SIPSTERAccount::Initialize(target);
    SIPSTERCall::Initialize(target);

    target->Set(String::NewSymbol("version"),
                FunctionTemplate::New(EPVersion)->GetFunction());
    target->Set(String::NewSymbol("state"),
                FunctionTemplate::New(EPGetState)->GetFunction());
    target->Set(String::NewSymbol("config"),
                FunctionTemplate::New(EPGetConfig)->GetFunction());
    target->Set(String::NewSymbol("init"),
                FunctionTemplate::New(EPInit)->GetFunction());
    target->Set(String::NewSymbol("start"),
                FunctionTemplate::New(EPStart)->GetFunction());
    target->Set(String::NewSymbol("hangupAllCalls"),
                FunctionTemplate::New(EPHangupAllCalls)->GetFunction());
    target->Set(String::NewSymbol("mediaActivePorts"),
                FunctionTemplate::New(EPMediaActivePorts)->GetFunction());
    target->Set(String::NewSymbol("transportSetEnable"),
                FunctionTemplate::New(EPTransportSetEnable)->GetFunction());
    target->Set(String::NewSymbol("transportCreate"),
                FunctionTemplate::New(EPTransportCreate)->GetFunction());
    target->Set(String::NewSymbol("transportClose"),
                FunctionTemplate::New(EPTransportClose)->GetFunction());
    target->Set(String::NewSymbol("transportGetInfo"),
                FunctionTemplate::New(EPTransportGetInfo)->GetFunction());
  }

  NODE_MODULE(sipster, init);
}
