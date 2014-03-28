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
class SIPSTERMedia;
class SIPSTERLogWriter;

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
#define REGSTATE_FIELDS                                             \
  X(REGSTATE, bool, active, Boolean, active)                        \
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
#define N_CALLDTMF_FIELDS 1
#define CALLDTMF_FIELDS                                             \
  X(CALLDTMF, char, digit, String, digit[0])
struct EV_ARGS_CALLDTMF {
#define X(kind, ctype, name, v8type, valconv) ctype name;
  CALLDTMF_FIELDS
#undef X
};
static Persistent<String> CALLDTMF_DTMF0_symbol;
static Persistent<String> CALLDTMF_DTMF1_symbol;
static Persistent<String> CALLDTMF_DTMF2_symbol;
static Persistent<String> CALLDTMF_DTMF3_symbol;
static Persistent<String> CALLDTMF_DTMF4_symbol;
static Persistent<String> CALLDTMF_DTMF5_symbol;
static Persistent<String> CALLDTMF_DTMF6_symbol;
static Persistent<String> CALLDTMF_DTMF7_symbol;
static Persistent<String> CALLDTMF_DTMF8_symbol;
static Persistent<String> CALLDTMF_DTMF9_symbol;
static Persistent<String> CALLDTMF_DTMFSTAR_symbol;
static Persistent<String> CALLDTMF_DTMFPOUND_symbol;
static Persistent<String> CALLDTMF_DTMFA_symbol;
static Persistent<String> CALLDTMF_DTMFB_symbol;
static Persistent<String> CALLDTMF_DTMFC_symbol;
static Persistent<String> CALLDTMF_DTMFD_symbol;
// =============================================================================

// Reg/Unreg starting event ====================================================
#define N_REGSTARTING_FIELDS 1
#define REGSTARTING_FIELDS                                          \
  X(REGSTARTING, bool, renew, Boolean, renew)
struct EV_ARGS_REGSTARTING {
#define X(kind, ctype, name, v8type, valconv) ctype name;
  REGSTARTING_FIELDS
#undef X
};
// =============================================================================

#define X(kind, ctype, name, v8type, valconv)                       \
  static Persistent<String> kind##_##name##_symbol;
  INCALL_FIELDS
  CALLDTMF_FIELDS
  REGSTATE_FIELDS
  REGSTARTING_FIELDS
#undef X
#define EVENT_TYPES                                                 \
  X(INCALL)                                                         \
  X(CALLSTATE)                                                      \
  X(CALLDTMF)                                                       \
  X(REGSTATE)                                                       \
  X(CALLMEDIA)                                                      \
  X(PLAYEREOF)                                                      \
  X(REGSTARTING)
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
  X(REGSTATE, state)                                                \
  X(CALLMEDIA, media)                                               \
  X(PLAYEREOF, eof)                                                 \
  X(REGSTARTING, registering)                                       \
  X(REGSTARTING, unregistering)

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
  SIPSTERMedia* media;
  void* args;
};
struct CustomLogEntry {
  int level;
  string msg;
  double threadId;
  string threadName;
};
static list<SIPEventInfo> event_queue;
static list<CustomLogEntry> log_queue;
static uv_mutex_t event_mutex;
static uv_mutex_t log_mutex;
static uv_mutex_t async_mutex;
// =============================================================================

// start PJSUA2-specific definitions ===========================================
static Endpoint* ep = new Endpoint;
static bool ep_init = false;
static bool ep_create = false;
static bool ep_start = false;
static EpConfig ep_cfg;
static SIPSTERLogWriter* logger = NULL;
// =============================================================================

static Persistent<FunctionTemplate> SIPSTERCall_constructor;
static Persistent<FunctionTemplate> SIPSTERAccount_constructor;
static Persistent<FunctionTemplate> SIPSTERMedia_constructor;
static Persistent<FunctionTemplate> SIPSTERTransport_constructor;
static Persistent<String> emit_symbol;
static Persistent<String> media_dir_none_symbol;
static Persistent<String> media_dir_outbound_symbol;
static Persistent<String> media_dir_inbound_symbol;
static Persistent<String> media_dir_bidi_symbol;
static Persistent<String> media_dir_unknown_symbol;
static uv_async_t dumb;
static uv_async_t logging;
static Persistent<Object> global;

class SIPSTERPlayer : public AudioMediaPlayer {
public:
  SIPSTERMedia* media;
  unsigned options;
  bool skip;

  SIPSTERPlayer() : skip(false) {}
  ~SIPSTERPlayer() {}

  virtual bool onEof() {
    if (skip)
      return false;
    SETUP_EVENT_NOARGS(PLAYEREOF);
    ev.media = media;

    ENQUEUE_EVENT(ev);
    if (options & PJMEDIA_FILE_NO_LOOP) {
      skip = true;
      return false;
    }
    return true;
  }
};

class SIPSTERMedia : public ObjectWrap {
public:
  Persistent<Function> emit;
  AudioMedia* media;
  pjmedia_dir dir;

  SIPSTERMedia() {}
  ~SIPSTERMedia() {
    emit.Dispose();
    emit.Clear();
    if (media) {
      delete media;
      media = NULL;
    }
  }

  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;

    if (!args.IsConstructCall()) {
      return ThrowException(Exception::TypeError(
        String::New("Use `new` to create instances of this object."))
      );
    }

    SIPSTERMedia* med = new SIPSTERMedia();

    med->Wrap(args.This());

    med->emit = Persistent<Function>::New(
      Local<Function>::Cast(med->handle_->Get(emit_symbol))
    );

    return args.This();
  }

  static Handle<Value> StartTransmit(const Arguments& args) {
    HandleScope scope;
    SIPSTERMedia* src = ObjectWrap::Unwrap<SIPSTERMedia>(args.This());
    if (args.Length() == 0 || !SIPSTERMedia_constructor->HasInstance(args[0])) {
      return ThrowException(Exception::TypeError(
        String::New("Expected Media object"))
      );
    }
    Local<Object> inst = Local<Object>(Object::Cast(*args[0]));
    SIPSTERMedia* dest = ObjectWrap::Unwrap<SIPSTERMedia>(inst);

    if (!src->media) {
      return ThrowException(Exception::TypeError(
        String::New("Invalid source"))
      );
    } else if (!dest->media) {
      return ThrowException(Exception::TypeError(
        String::New("Invalid destination"))
      );
    }

    try {
      src->media->startTransmit(*dest->media);
    } catch(Error& err) {
      string errstr = "AudioMedia.startTransmit() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static Handle<Value> StopTransmit(const Arguments& args) {
    HandleScope scope;
    SIPSTERMedia* src = ObjectWrap::Unwrap<SIPSTERMedia>(args.This());
    if (args.Length() == 0 || !SIPSTERMedia_constructor->HasInstance(args[0])) {
      return ThrowException(Exception::TypeError(
        String::New("Expected Media object"))
      );
    }
    Local<Object> inst = Local<Object>(Object::Cast(*args[0]));
    SIPSTERMedia* dest = ObjectWrap::Unwrap<SIPSTERMedia>(inst);

    if (!src->media) {
      return ThrowException(Exception::TypeError(
        String::New("Invalid source"))
      );
    } else if (!dest->media) {
      return ThrowException(Exception::TypeError(
        String::New("Invalid destination"))
      );
    }

    try {
      src->media->stopTransmit(*dest->media);
    } catch(Error& err) {
      string errstr = "AudioMedia.stopTransmit() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static Handle<Value> DirGetter(Local<String> property,
                                 const AccessorInfo& info) {
    HandleScope scope;
    SIPSTERMedia* med = ObjectWrap::Unwrap<SIPSTERMedia>(info.This());
    Handle<String> str;
    switch (med->dir) {
      case PJMEDIA_DIR_NONE:
        str = media_dir_none_symbol;
      break;
      case PJMEDIA_DIR_ENCODING:
        str = media_dir_outbound_symbol;
      break;
      case PJMEDIA_DIR_DECODING:
        str = media_dir_inbound_symbol;
      break;
      case PJMEDIA_DIR_ENCODING_DECODING:
        str = media_dir_bidi_symbol;
      break;
      default:
        str = media_dir_unknown_symbol;
    }
    return scope.Close(str);
  }

  static void Initialize(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    Local<String> name = String::NewSymbol("Media");

    SIPSTERMedia_constructor = Persistent<FunctionTemplate>::New(tpl);
    SIPSTERMedia_constructor->InstanceTemplate()->SetInternalFieldCount(1);
    SIPSTERMedia_constructor->SetClassName(name);

    NODE_SET_PROTOTYPE_METHOD(SIPSTERMedia_constructor,
                              "startTransmitTo",
                              StartTransmit);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERMedia_constructor,
                              "stopTransmitTo",
                              StopTransmit);

    SIPSTERMedia_constructor->PrototypeTemplate()
                            ->SetAccessor(String::NewSymbol("dir"), DirGetter);

    target->Set(name, SIPSTERMedia_constructor->GetFunction());
  }
};

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

  void onCallMediaState(OnCallMediaStateParam &prm) {
    SETUP_EVENT_NOARGS(CALLMEDIA);
    ev.call = this;

    ENQUEUE_EVENT(ev);
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

    args->digit = prm.digit[0];

    ENQUEUE_EVENT(ev);
  }

  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;

    if (!args.IsConstructCall()) {
      return ThrowException(Exception::TypeError(
        String::New("Use `new` to create instances of this object."))
      );
    }

    SIPSTERCall* call = NULL;
    if (args.Length() > 0) {
      if (SIPSTERAccount_constructor->HasInstance(args[0])) {
        HandleScope scope;
        Local<Object> acct_inst = Local<Object>(Object::Cast(*args[0]));
        Account* acct = ObjectWrap::Unwrap<Account>(acct_inst);
        call = new SIPSTERCall(*acct);
      } else
        call = static_cast<SIPSTERCall*>(External::Unwrap(args[0]));
    } else {
      return ThrowException(Exception::TypeError(
        String::New("Expected callId or Account argument"))
      );
    }

    call->Wrap(args.This());
    call->handle_->SetPointerInInternalField(0, call);

    call->emit = Persistent<Function>::New(
      Local<Function>::Cast(call->handle_->Get(emit_symbol))
    );

    return args.This();
  }

  static Handle<Value> Answer(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    CallOpParam prm;
    if (args.Length() > 0 && args[0]->IsUint32()) {
      prm.statusCode = static_cast<pjsip_status_code>(args[0]->Int32Value());
      if (args.Length() > 1 && args[1]->IsString())
        prm.reason = string(*String::Utf8Value(args[1]->ToString()));
    } else
      prm.statusCode = PJSIP_SC_OK;

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
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

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
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

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
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

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
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

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
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    if (args.Length() > 0 && args[0]->IsString()) {
      try {
        call->dialDtmf(string(*String::AsciiValue(args[0]->ToString())));
      } catch(Error& err) {
        string errstr = "Call.dialDtmf() error: " + err.info();
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
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

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

  static Handle<Value> DoRef(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    call->Ref();

    return Undefined();
  }

  static Handle<Value> DoUnref(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    call->Unref();

    return Undefined();
  }

  static Handle<Value> ConDurationGetter(Local<String> property,
                                         const AccessorInfo& info) {
    HandleScope scope;
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(info.This());

    CallInfo ci;

    try {
      ci = call->getInfo();
    } catch(Error& err) {
      string errstr = "Call.getInfo() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    double duration = ci.connectDuration.sec + (ci.connectDuration.msec / 1000);

    return scope.Close(Number::New(duration));
  }

  static Handle<Value> TotDurationGetter(Local<String> property,
                                         const AccessorInfo& info) {
    HandleScope scope;
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(info.This());

    CallInfo ci;

    try {
      ci = call->getInfo();
    } catch(Error& err) {
      string errstr = "Call.getInfo() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    double duration = ci.totalDuration.sec + (ci.totalDuration.msec / 1000);

    return scope.Close(Number::New(duration));
  }

  static Handle<Value> HasMediaGetter(Local<String> property,
                                      const AccessorInfo& info) {
    HandleScope scope;
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(info.This());

    return scope.Close(Boolean::New(call->hasMedia()));
  }

  static Handle<Value> IsActiveGetter(Local<String> property,
                                      const AccessorInfo& info) {
    HandleScope scope;
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(info.This());

    return scope.Close(Boolean::New(call->isActive()));
  }

  static Handle<Value> GetStats(const Arguments& args) {
    HandleScope scope;
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(args.This());

    bool with_media = true;
    string indent = "  ";
    if (args.Length() > 0 && args[0]->IsBoolean()) {
      with_media = args[0]->BooleanValue();
      if (args.Length() > 1 && args[1]->IsString())
        indent = string(*String::Utf8Value(args[1]->ToString()));
    }

    string info;
    try {
      info = call->dump(with_media, indent);
    } catch(Error& err) {
      string errstr = "Call.dump() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return scope.Close(String::New(info.c_str()));
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
    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "ref", DoRef);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "unref", DoUnref);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERCall_constructor, "getStatsDump", GetStats);

    SIPSTERCall_constructor->PrototypeTemplate()
                           ->SetAccessor(String::NewSymbol("connDuration"),
                                         ConDurationGetter);
    SIPSTERCall_constructor->PrototypeTemplate()
                           ->SetAccessor(String::NewSymbol("totalDuration"),
                                         TotDurationGetter);
    SIPSTERCall_constructor->PrototypeTemplate()
                           ->SetAccessor(String::NewSymbol("hasMedia"),
                                         HasMediaGetter);
    SIPSTERCall_constructor->PrototypeTemplate()
                           ->SetAccessor(String::NewSymbol("isActive"),
                                         IsActiveGetter);

    target->Set(name, SIPSTERCall_constructor->GetFunction());
  }
};

class SIPSTERTransport : public ObjectWrap {
public:
  Persistent<Function> emit;
  TransportId transId;
  bool enabled;

  SIPSTERTransport() : transId(-1), enabled(false) {}
  ~SIPSTERTransport() {
    emit.Dispose();
    emit.Clear();
    if (transId > -1) {
      try {
        ep->transportClose(transId);
        uv_mutex_lock(&async_mutex);
        uv_unref(reinterpret_cast<uv_handle_t*>(&dumb));
        uv_mutex_unlock(&async_mutex);
      } catch(Error& err) {
        string errstr = "transportClose error: " + err.info();
        ThrowException(Exception::Error(String::New(errstr.c_str())));
      }
    }
  }

  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;

    if (!args.IsConstructCall()) {
      return ThrowException(Exception::TypeError(
        String::New("Use `new` to create instances of this object."))
      );
    }

    TransportConfig tp_cfg;
    string errstr;

    Local<Value> val;
    if (args.Length() > 0 && args[0]->IsObject()) {
      HandleScope scope;
      Local<Object> obj = args[0]->ToObject();
      JS2PJ_UINT(obj, port, tp_cfg);
      JS2PJ_UINT(obj, portRange, tp_cfg);
      JS2PJ_STR(obj, publicAddress, tp_cfg);
      JS2PJ_STR(obj, boundAddress, tp_cfg);
      JS2PJ_ENUM(obj, qosType, pj_qos_type, tp_cfg);

      val = obj->Get(String::New("qosParams"));
      if (val->IsObject()) {
        HandleScope scope;
        pj_qos_params qos_params;
        Local<Object> qos_obj = val->ToObject();
        Local<Value> flags_val = qos_obj->Get(String::New("flags"));
        Local<Value> dscp_val = qos_obj->Get(String::New("dscp_val"));
        Local<Value> so_prio_val = qos_obj->Get(String::New("so_prio"));
        Local<Value> wmm_prio_val = qos_obj->Get(String::New("wmm_prio"));
        if (flags_val->IsUint32())
          qos_params.flags = static_cast<pj_uint8_t>(flags_val->Uint32Value());
        if (dscp_val->IsUint32())
          qos_params.dscp_val = static_cast<pj_uint8_t>(dscp_val->Uint32Value());
        if (so_prio_val->IsUint32())
          qos_params.so_prio = static_cast<pj_uint8_t>(so_prio_val->Uint32Value());
        if (wmm_prio_val->IsUint32())
          qos_params.wmm_prio = static_cast<pj_qos_wmm_prio>(wmm_prio_val->Uint32Value());
        tp_cfg.qosParams = qos_params;
      }

      val = obj->Get(String::New("tlsConfig"));
      if (val->IsObject()) {
        HandleScope scope;
        Local<Object> tls_obj = val->ToObject();
        JS2PJ_STR(tls_obj, CaListFile, tp_cfg.tlsConfig);
        JS2PJ_STR(tls_obj, certFile, tp_cfg.tlsConfig);
        JS2PJ_STR(tls_obj, privKeyFile, tp_cfg.tlsConfig);
        JS2PJ_STR(tls_obj, password, tp_cfg.tlsConfig);
        JS2PJ_ENUM(tls_obj, method, pjsip_ssl_method, tp_cfg.tlsConfig);

        val = tls_obj->Get(String::New("ciphers"));
        if (val->IsArray()) {
          HandleScope scope;
          const Local<Array> arr_obj = Local<Array>::Cast(val);
          const uint32_t arr_length = arr_obj->Length();
          if (arr_length > 0) {
            vector<int> ciphers;
            for (uint32_t i = 0; i < arr_length; ++i) {
              HandleScope scope;
              const Local<Value> value = arr_obj->Get(i);
              ciphers.push_back(value->Int32Value());
            }
            tp_cfg.tlsConfig.ciphers = ciphers;
          }
        }

        JS2PJ_BOOL(tls_obj, verifyServer, tp_cfg.tlsConfig);
        JS2PJ_BOOL(tls_obj, verifyClient, tp_cfg.tlsConfig);
        JS2PJ_BOOL(tls_obj, requireClientCert, tp_cfg.tlsConfig);
        JS2PJ_UINT(tls_obj, msecTimeout, tp_cfg.tlsConfig);
        JS2PJ_ENUM(tls_obj, qosType, pj_qos_type, tp_cfg.tlsConfig);

        val = tls_obj->Get(String::New("qosParams"));
        if (val->IsObject()) {
          HandleScope scope;
          pj_qos_params qos_params;
          Local<Object> qos_obj = val->ToObject();
          Local<Value> flags_val = qos_obj->Get(String::New("flags"));
          Local<Value> dscp_val = qos_obj->Get(String::New("dscp_val"));
          Local<Value> so_prio_val = qos_obj->Get(String::New("so_prio"));
          Local<Value> wmm_prio_val = qos_obj->Get(String::New("wmm_prio"));
          if (flags_val->IsUint32())
            qos_params.flags = static_cast<pj_uint8_t>(flags_val->Uint32Value());
          if (dscp_val->IsUint32())
            qos_params.dscp_val = static_cast<pj_uint8_t>(dscp_val->Uint32Value());
          if (so_prio_val->IsUint32())
            qos_params.so_prio = static_cast<pj_uint8_t>(so_prio_val->Uint32Value());
          if (wmm_prio_val->IsUint32())
            qos_params.wmm_prio = static_cast<pj_qos_wmm_prio>(wmm_prio_val->Uint32Value());
          tp_cfg.tlsConfig.qosParams = qos_params;
        }

        JS2PJ_BOOL(tls_obj, qosIgnoreError, tp_cfg.tlsConfig);
      }
    }

    val.Clear();
    pjsip_transport_type_e transportType = PJSIP_TRANSPORT_UDP;
    if (args.Length() > 0 && args[0]->IsString())
      val = args[0];
    else if (args.Length() > 1 && args[1]->IsString())
      val = args[1];
    if (!val.IsEmpty()) {
      const char* typecstr = *String::AsciiValue(val->ToString());
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
        return ThrowException(
          Exception::Error(String::New("Unsupported transport type"))
        );
      }
    }

    TransportId tid;
    try {
      tid = ep->transportCreate(transportType, tp_cfg);
    } catch(Error& err) {
      errstr = "transportCreate error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    uv_mutex_lock(&async_mutex);
    uv_ref(reinterpret_cast<uv_handle_t*>(&dumb));
    uv_mutex_unlock(&async_mutex);

    SIPSTERTransport* trans = new SIPSTERTransport();

    trans->Wrap(args.This());
    trans->Ref();

    trans->transId = tid;
    trans->enabled = true;
    trans->emit = Persistent<Function>::New(
      Local<Function>::Cast(trans->handle_->Get(emit_symbol))
    );

    return args.This();
  }

  static Handle<Value> GetInfo(const Arguments& args) {
    HandleScope scope;
    SIPSTERTransport* trans = ObjectWrap::Unwrap<SIPSTERTransport>(args.This());

    TransportInfo ti;

    try {
      ti = ep->transportGetInfo(trans->transId);
    } catch(Error& err) {
      string errstr = "transportGetInfo error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    Local<Object> info_obj = Object::New();
    info_obj->Set(String::New("type"), String::New(ti.typeName.c_str()));
    info_obj->Set(String::New("info"), String::New(ti.info.c_str()));
    info_obj->Set(String::New("flags"), Integer::NewFromUnsigned(ti.flags));
    info_obj->Set(String::New("localAddress"),
                  String::New(ti.localAddress.c_str()));
    info_obj->Set(String::New("localName"),
                  String::New(ti.localName.c_str()));
    info_obj->Set(String::New("usageCount"),
                  Integer::NewFromUnsigned(ti.usageCount));

    return scope.Close(info_obj);
  }

  static Handle<Value> Enable(const Arguments& args) {
    HandleScope scope;
    SIPSTERTransport* trans = ObjectWrap::Unwrap<SIPSTERTransport>(args.This());

    if (!trans->enabled) {
      try {
        ep->transportSetEnable(trans->transId, true);
      } catch(Error& err) {
        string errstr = "transportSetEnable error: " + err.info();
        return ThrowException(Exception::Error(String::New(errstr.c_str())));
      }
      trans->enabled = true;
    }

    return Undefined();
  }

  static Handle<Value> Disable(const Arguments& args) {
    HandleScope scope;
    SIPSTERTransport* trans = ObjectWrap::Unwrap<SIPSTERTransport>(args.This());

    if (trans->enabled) {
      try {
        ep->transportSetEnable(trans->transId, false);
      } catch(Error& err) {
        string errstr = "transportSetEnable error: " + err.info();
        return ThrowException(Exception::Error(String::New(errstr.c_str())));
      }
      trans->enabled = false;
    }

    return Undefined();
  }

  static Handle<Value> DoRef(const Arguments& args) {
    HandleScope scope;
    SIPSTERTransport* trans = ObjectWrap::Unwrap<SIPSTERTransport>(args.This());

    trans->Ref();

    return Undefined();
  }

  static Handle<Value> DoUnref(const Arguments& args) {
    HandleScope scope;
    SIPSTERTransport* trans = ObjectWrap::Unwrap<SIPSTERTransport>(args.This());

    trans->Unref();

    return Undefined();
  }

  static Handle<Value> EnabledGetter(Local<String> property,
                                     const AccessorInfo& info) {
    HandleScope scope;
    SIPSTERTransport* trans = ObjectWrap::Unwrap<SIPSTERTransport>(info.This());

    return scope.Close(Boolean::New(trans->enabled));
  }

  static void Initialize(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    Local<String> name = String::NewSymbol("Transport");

    SIPSTERTransport_constructor = Persistent<FunctionTemplate>::New(tpl);
    SIPSTERTransport_constructor->InstanceTemplate()->SetInternalFieldCount(1);
    SIPSTERTransport_constructor->SetClassName(name);

    NODE_SET_PROTOTYPE_METHOD(SIPSTERTransport_constructor,
                              "getInfo",
                              GetInfo);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERTransport_constructor,
                              "enable",
                              Enable);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERTransport_constructor,
                              "disable",
                              Disable);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERTransport_constructor,
                              "ref",
                              DoRef);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERTransport_constructor,
                              "unref",
                              DoUnref);

    SIPSTERTransport_constructor->PrototypeTemplate()
                                ->SetAccessor(String::NewSymbol("enabled"),
                                              EnabledGetter);

    target->Set(name, SIPSTERTransport_constructor->GetFunction());
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

  static AccountConfig genConfig(Local<Object> acct_obj) {
    HandleScope scope;

    AccountConfig acct_cfg;

    Local<Value> val;

    JS2PJ_INT(acct_obj, priority, acct_cfg);
    JS2PJ_STR(acct_obj, idUri, acct_cfg);

    val = acct_obj->Get(String::New("regConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      AccountRegConfig regConfig;
      Local<Object> reg_obj = val->ToObject();
      JS2PJ_STR(reg_obj, registrarUri, regConfig);
      JS2PJ_BOOL(reg_obj, registerOnAdd, regConfig);

      val = reg_obj->Get(String::New("headers"));
      if (val->IsObject()) {
        HandleScope scope;
        const Local<Object> hdr_obj = val->ToObject();
        const Local<Array> hdr_props = hdr_obj->GetPropertyNames();
        const uint32_t hdr_length = hdr_props->Length();
        if (hdr_length > 0) {
          vector<SipHeader> sipheaders;
          for (uint32_t i = 0; i < hdr_length; ++i) {
            HandleScope scope;
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

      acct_cfg.regConfig = regConfig;
    }
    val = acct_obj->Get(String::New("sipConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      AccountSipConfig sipConfig;
      Local<Object> sip_obj = val->ToObject();

      val = sip_obj->Get(String::New("authCreds"));
      if (val->IsArray()) {
        HandleScope scope;
        const Local<Array> arr_obj = Local<Array>::Cast(val);
        const uint32_t arr_length = arr_obj->Length();
        if (arr_length > 0) {
          vector<AuthCredInfo> creds;
          for (uint32_t i = 0; i < arr_length; ++i) {
            HandleScope scope;
            const Local<Value> cred_value = arr_obj->Get(i);
            if (cred_value->IsObject()) {
              HandleScope scope;
              const Local<Object> auth_obj = cred_value->ToObject();
              AuthCredInfo credinfo;
              Local<Value> scheme_val = auth_obj->Get(String::New("scheme"));
              Local<Value> realm_val = auth_obj->Get(String::New("realm"));
              Local<Value> username_val = auth_obj->Get(String::New("username"));
              Local<Value> dataType_val = auth_obj->Get(String::New("dataType"));
              Local<Value> data_val = auth_obj->Get(String::New("data"));
              if (scheme_val->IsString()
                  && realm_val->IsString()
                  && username_val->IsString()
                  && dataType_val->IsInt32()
                  && data_val->IsString()) {
                credinfo.scheme = string(*String::Utf8Value(scheme_val->ToString()));
                credinfo.realm = string(*String::Utf8Value(realm_val->ToString()));
                credinfo.username = string(*String::Utf8Value(username_val->ToString()));
                credinfo.dataType = dataType_val->Int32Value();
                credinfo.data = string(*String::Utf8Value(data_val->ToString()));
              }
            }
          }
          if (creds.size() > 0)
            sipConfig.authCreds = creds;
        }
      }

      val = sip_obj->Get(String::New("proxies"));
      if (val->IsArray()) {
        HandleScope scope;
        const Local<Array> arr_obj = Local<Array>::Cast(val);
        const uint32_t arr_length = arr_obj->Length();
        if (arr_length > 0) {
          vector<string> proxies;
          for (uint32_t i = 0; i < arr_length; ++i) {
            HandleScope scope;
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

      // deviates from the pjsip config structure to accept a Transport instance
      // instead of a transport id since that information is made available to
      // JS land
      val = sip_obj->Get(String::New("transport"));
      if (SIPSTERTransport_constructor->HasInstance(val)) {
        HandleScope scope;
        Local<Object> inst_obj = Local<Object>(Object::Cast(*val));
        SIPSTERTransport* trans = ObjectWrap::Unwrap<SIPSTERTransport>(inst_obj);
        sipConfig.transportId = trans->transId;
      }

      acct_cfg.sipConfig = sipConfig;
    }
    val = acct_obj->Get(String::New("callConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      AccountCallConfig callConfig;
      Local<Object> call_obj = val->ToObject();
      JS2PJ_ENUM(call_obj, holdType, pjsua_call_hold_type, callConfig);
      JS2PJ_ENUM(call_obj, prackUse, pjsua_100rel_use, callConfig);
      JS2PJ_ENUM(call_obj, timerUse, pjsua_sip_timer_use, callConfig);
      JS2PJ_UINT(call_obj, timerMinSESec, callConfig);
      JS2PJ_UINT(call_obj, timerSessExpiresSec, callConfig);

      acct_cfg.callConfig = callConfig;
    }
    val = acct_obj->Get(String::New("presConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      AccountPresConfig presConfig;
      Local<Object> pres_obj = val->ToObject();

      val = pres_obj->Get(String::New("headers"));
      if (val->IsObject()) {
        HandleScope scope;
        const Local<Object> hdr_obj = val->ToObject();
        const Local<Array> hdr_props = hdr_obj->GetPropertyNames();
        const uint32_t hdr_length = hdr_props->Length();
        if (hdr_length > 0) {
          vector<SipHeader> sipheaders;
          for (uint32_t i = 0; i < hdr_length; ++i) {
            HandleScope scope;
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

      acct_cfg.presConfig = presConfig;
    }
    val = acct_obj->Get(String::New("mwiConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      AccountMwiConfig mwiConfig;
      Local<Object> mwi_obj = val->ToObject();
      JS2PJ_BOOL(mwi_obj, enabled, mwiConfig);
      JS2PJ_UINT(mwi_obj, expirationSec, mwiConfig);

      acct_cfg.mwiConfig = mwiConfig;
    }
    val = acct_obj->Get(String::New("natConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      AccountNatConfig natConfig;
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

      acct_cfg.natConfig = natConfig;
    }
    val = acct_obj->Get(String::New("mediaConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      AccountMediaConfig mediaConfig;
      Local<Object> media_obj = val->ToObject();

      val = media_obj->Get(String::New("transportConfig"));
      if (val->IsObject()) {
        HandleScope scope;
        TransportConfig transportConfig;
        Local<Object> obj = val->ToObject();
        JS2PJ_UINT(obj, port, transportConfig);
        JS2PJ_UINT(obj, portRange, transportConfig);
        JS2PJ_STR(obj, publicAddress, transportConfig);
        JS2PJ_STR(obj, boundAddress, transportConfig);
        JS2PJ_ENUM(obj, qosType, pj_qos_type, transportConfig);

        val = obj->Get(String::New("qosParams"));
        if (val->IsObject()) {
          HandleScope scope;
          pj_qos_params qos_params;
          Local<Object> qos_obj = val->ToObject();
          Local<Value> flags_val = qos_obj->Get(String::New("flags"));
          Local<Value> dscp_val = qos_obj->Get(String::New("dscp_val"));
          Local<Value> so_prio_val = qos_obj->Get(String::New("so_prio"));
          Local<Value> wmm_prio_val = qos_obj->Get(String::New("wmm_prio"));
          if (flags_val->IsUint32())
            qos_params.flags = static_cast<pj_uint8_t>(flags_val->Uint32Value());
          if (dscp_val->IsUint32())
            qos_params.dscp_val = static_cast<pj_uint8_t>(dscp_val->Uint32Value());
          if (so_prio_val->IsUint32())
            qos_params.so_prio = static_cast<pj_uint8_t>(so_prio_val->Uint32Value());
          if (wmm_prio_val->IsUint32())
            qos_params.wmm_prio = static_cast<pj_qos_wmm_prio>(wmm_prio_val->Uint32Value());
          transportConfig.qosParams = qos_params;
        }

        val = obj->Get(String::New("tlsConfig"));
        if (val->IsObject()) {
          HandleScope scope;
          TlsConfig tlsConfig;
          Local<Object> tls_obj = val->ToObject();
          JS2PJ_STR(tls_obj, CaListFile, tlsConfig);
          JS2PJ_STR(tls_obj, certFile, tlsConfig);
          JS2PJ_STR(tls_obj, privKeyFile, tlsConfig);
          JS2PJ_STR(tls_obj, password, tlsConfig);
          JS2PJ_ENUM(tls_obj, method, pjsip_ssl_method, tlsConfig);

          val = tls_obj->Get(String::New("ciphers"));
          if (val->IsArray()) {
            HandleScope scope;
            const Local<Array> arr_obj = Local<Array>::Cast(val);
            const uint32_t arr_length = arr_obj->Length();
            if (arr_length > 0) {
              vector<int> ciphers;
              for (uint32_t i = 0; i < arr_length; ++i) {
                HandleScope scope;
                const Local<Value> value = arr_obj->Get(i);
                ciphers.push_back(value->Int32Value());
              }
              tlsConfig.ciphers = ciphers;
            }
          }

          JS2PJ_BOOL(tls_obj, verifyServer, tlsConfig);
          JS2PJ_BOOL(tls_obj, verifyClient, tlsConfig);
          JS2PJ_BOOL(tls_obj, requireClientCert, tlsConfig);
          JS2PJ_UINT(tls_obj, msecTimeout, tlsConfig);
          JS2PJ_ENUM(tls_obj, qosType, pj_qos_type, tlsConfig);

          val = tls_obj->Get(String::New("qosParams"));
          if (val->IsObject()) {
            HandleScope scope;
            pj_qos_params qos_params;
            Local<Object> qos_obj = val->ToObject();
            Local<Value> flags_val = qos_obj->Get(String::New("flags"));
            Local<Value> dscp_val = qos_obj->Get(String::New("dscp_val"));
            Local<Value> so_prio_val = qos_obj->Get(String::New("so_prio"));
            Local<Value> wmm_prio_val = qos_obj->Get(String::New("wmm_prio"));
            if (flags_val->IsUint32())
              qos_params.flags = static_cast<pj_uint8_t>(flags_val->Uint32Value());
            if (dscp_val->IsUint32())
              qos_params.dscp_val = static_cast<pj_uint8_t>(dscp_val->Uint32Value());
            if (so_prio_val->IsUint32())
              qos_params.so_prio = static_cast<pj_uint8_t>(so_prio_val->Uint32Value());
            if (wmm_prio_val->IsUint32())
              qos_params.wmm_prio = static_cast<pj_qos_wmm_prio>(wmm_prio_val->Uint32Value());
            tlsConfig.qosParams = qos_params;
          }

          JS2PJ_BOOL(tls_obj, qosIgnoreError, tlsConfig);

          transportConfig.tlsConfig = tlsConfig;
        }

        mediaConfig.transportConfig = transportConfig;
      }

      JS2PJ_BOOL(media_obj, lockCodecEnabled, mediaConfig);
      JS2PJ_BOOL(media_obj, streamKaEnabled, mediaConfig);
      JS2PJ_ENUM(media_obj, srtpUse, pjmedia_srtp_use, mediaConfig);
      JS2PJ_INT(media_obj, srtpSecureSignaling, mediaConfig);
      JS2PJ_ENUM(media_obj, ipv6Use, pjsua_ipv6_use, mediaConfig);

      acct_cfg.mediaConfig = mediaConfig;
    }
    val = acct_obj->Get(String::New("videoConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      AccountVideoConfig videoConfig;
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

      acct_cfg.videoConfig = videoConfig;
    }

    return acct_cfg;
  }

  virtual void onRegStarted(OnRegStartedParam &prm) {
    SETUP_EVENT(REGSTARTING);
    ev.acct = this;

    args->renew = prm.renew;

    ENQUEUE_EVENT(ev);
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
    SIPSTERCall* call = new SIPSTERCall(*this, iprm.callId);
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

    SIPSTERAccount* acct = new SIPSTERAccount();

    AccountConfig acct_cfg;
    string errstr;
    bool isDefault = false;
    if (args.Length() > 0 && args[0]->IsObject()) {
      acct_cfg = genConfig(args[0]->ToObject());

      if (args.Length() > 1 && args[1]->IsBoolean())
        isDefault = args[1]->BooleanValue();
    } else if (args.Length() > 0 && args[0]->IsBoolean())
      isDefault = args[0]->BooleanValue();

    try {
      acct->create(acct_cfg, isDefault);
    } catch(Error& err) {
      delete acct;
      errstr = "Account.create() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    acct->Wrap(args.This());
    acct->Ref();

    acct->emit = Persistent<Function>::New(
      Local<Function>::Cast(acct->handle_->Get(emit_symbol))
    );

    return args.This();
  }

  static Handle<Value> Modify(const Arguments& args) {
    HandleScope scope;
    SIPSTERAccount* acct = ObjectWrap::Unwrap<SIPSTERAccount>(args.This());

    AccountConfig acct_cfg;
    if (args.Length() > 0 && args[0]->IsObject())
      acct_cfg = genConfig(args[0]->ToObject());
    else {
      return ThrowException(
        Exception::Error(String::New("Missing renew argument"))
      );
    }

    try {
      acct->modify(acct_cfg);
    } catch(Error& err) {
      string errstr = "Account->modify() error: " + err.info();
      ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static Handle<Value> ValidGetter(Local<String> property,
                                   const AccessorInfo& info) {
    HandleScope scope;
    SIPSTERAccount* acct = ObjectWrap::Unwrap<SIPSTERAccount>(info.This());
    return scope.Close(Boolean::New(acct->isValid()));
  }

  static Handle<Value> DefaultGetter(Local<String> property,
                                     const AccessorInfo& info) {
    HandleScope scope;
    SIPSTERAccount* acct = ObjectWrap::Unwrap<SIPSTERAccount>(info.This());
    return scope.Close(Boolean::New(acct->isDefault()));
  }

  static void DefaultSetter(Local<String> property,
                                     Local<Value> value,
                                     const AccessorInfo& info) {
    HandleScope scope;
    SIPSTERAccount* acct = ObjectWrap::Unwrap<SIPSTERAccount>(info.This());

    if (value->BooleanValue()) {
      try {
        acct->setDefault();
      } catch(Error& err) {
        string errstr = "Account->setDefault() error: " + err.info();
        ThrowException(Exception::Error(String::New(errstr.c_str())));
      }
    }
  }

  static Handle<Value> GetInfo(const Arguments& args) {
    HandleScope scope;
    SIPSTERAccount* acct = ObjectWrap::Unwrap<SIPSTERAccount>(args.This());

    AccountInfo ai;
    try {
      ai = acct->getInfo();
    } catch(Error& err) {
      string errstr = "Account->getInfo() error: " + err.info();
      ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    Local<Object> info_obj = Object::New();
    info_obj->Set(String::New("uri"), String::New(ai.uri.c_str()));
    info_obj->Set(String::New("regIsConfigured"), Boolean::New(ai.regIsConfigured));
    info_obj->Set(String::New("regIsActive"), Boolean::New(ai.regIsActive));
    info_obj->Set(String::New("regExpiresSec"), Integer::New(ai.regExpiresSec));
    // TODO: onlineStatus*, regStatus*, regLastErr?

    return scope.Close(info_obj);
  }

  static Handle<Value> SetRegistration(const Arguments& args) {
    HandleScope scope;
    SIPSTERAccount* acct = ObjectWrap::Unwrap<SIPSTERAccount>(args.This());

    bool renew;
    if (args.Length() > 0 && args[0]->IsBoolean())
      renew = args[0]->BooleanValue();
    else {
      return ThrowException(
        Exception::Error(String::New("Missing renew argument"))
      );
    }

    try {
      acct->setRegistration(renew);
    } catch(Error& err) {
      string errstr = "Account->setRegistration() error: " + err.info();
      ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
  }

  static Handle<Value> SetTransport(const Arguments& args) {
    HandleScope scope;
    SIPSTERAccount* acct = ObjectWrap::Unwrap<SIPSTERAccount>(args.This());

    TransportId tid;
    if (args.Length() > 0
        && SIPSTERTransport_constructor->HasInstance(args[0])) {
      HandleScope scope;
      Local<Object> inst_obj = Local<Object>(Object::Cast(*args[0]));
      SIPSTERTransport* trans = ObjectWrap::Unwrap<SIPSTERTransport>(inst_obj);
      tid = trans->transId;
    } else {
      return ThrowException(
        Exception::Error(String::New("Missing Transport instance"))
      );
    }

    try {
      acct->setTransport(tid);
    } catch(Error& err) {
      string errstr = "Account->setTransport() error: " + err.info();
      ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return Undefined();
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
    SIPSTERCall* call = ObjectWrap::Unwrap<SIPSTERCall>(call_obj);

    try {
      call->makeCall(dest, prm);
    } catch(Error& err) {
      string errstr = "Call.makeCall() error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }

    return call_obj;
  }

  static Handle<Value> DoRef(const Arguments& args) {
    HandleScope scope;
    SIPSTERAccount* acct = ObjectWrap::Unwrap<SIPSTERAccount>(args.This());

    acct->Ref();

    return Undefined();
  }

  static Handle<Value> DoUnref(const Arguments& args) {
    HandleScope scope;
    SIPSTERAccount* acct = ObjectWrap::Unwrap<SIPSTERAccount>(args.This());

    acct->Unref();

    return Undefined();
  }

  static void Initialize(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    Local<String> name = String::NewSymbol("Account");

    SIPSTERAccount_constructor = Persistent<FunctionTemplate>::New(tpl);
    SIPSTERAccount_constructor->InstanceTemplate()->SetInternalFieldCount(1);
    SIPSTERAccount_constructor->SetClassName(name);

    NODE_SET_PROTOTYPE_METHOD(SIPSTERAccount_constructor, "modify", Modify);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERAccount_constructor, "makeCall", MakeCall);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERAccount_constructor, "getInfo", GetInfo);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERAccount_constructor,
                              "setRegistration",
                              SetRegistration);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERAccount_constructor,
                              "setTransport",
                              SetTransport);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERAccount_constructor, "ref", DoRef);
    NODE_SET_PROTOTYPE_METHOD(SIPSTERAccount_constructor, "unref", DoUnref);

    SIPSTERAccount_constructor->PrototypeTemplate()
                              ->SetAccessor(String::NewSymbol("valid"),
                                            ValidGetter);
    SIPSTERAccount_constructor->PrototypeTemplate()
                              ->SetAccessor(String::NewSymbol("default"),
                                            DefaultGetter,
                                            DefaultSetter);

    target->Set(name, SIPSTERAccount_constructor->GetFunction());
  }
};

class SIPSTERLogWriter : public LogWriter {
public:
  Persistent<Function> func;

  SIPSTERLogWriter() {}
  ~SIPSTERLogWriter() {
    if (!func.IsEmpty()) {
      func.Dispose();
      func.Clear();
    }
  }

  virtual void write(const LogEntry& entry) {
    CustomLogEntry log;
    log.level = entry.level;
    log.msg = entry.msg;
    log.threadId = static_cast<double>(entry.threadId);
    log.threadName = entry.threadName;

    uv_mutex_lock(&log_mutex);
    log_queue.push_back(log);
    uv_mutex_unlock(&log_mutex);
    uv_async_send(&logging);
  }
};

// start event processing-related definitions ==================================
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
        HandleScope scope;
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
        HandleScope scope;
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
        HandleScope scope;
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
          HandleScope scope;
          SIPSTERCall* call = ev.call;
          Handle<Value> emit_argv[1] = { ev_name };
          call->emit->Call(call->handle_, 1, emit_argv);
          Handle<Value> emit_catchall_argv[2] = { ev_CALLSTATE_state_symbol, ev_name };
          call->emit->Call(call->handle_, 2, emit_catchall_argv);
        }
        delete args;
      }
      break;
      case EVENT_CALLMEDIA: {
        HandleScope scope;
        SIPSTERCall* call = ev.call;

        Local<Array> medias = Array::New();
        CallInfo ci = call->getInfo();
        AudioMedia* media = NULL;
        for (unsigned i = 0, m = 0; i < ci.media.size(); ++i) {
          if (ci.media[i].type==PJMEDIA_TYPE_AUDIO
              && (media = static_cast<AudioMedia*>(call->getMedia(i)))) {
            HandleScope scope;
            Local<Object> med_obj;
            med_obj = SIPSTERMedia_constructor->GetFunction()
                                              ->NewInstance(0, NULL);
            SIPSTERMedia* med = ObjectWrap::Unwrap<SIPSTERMedia>(med_obj);
            med->media = media;
            med->dir = ci.media[i].dir;
            medias->Set(m++, med_obj);
          }
        }
        if (medias->Length() > 0) {
          HandleScope scope;
          Handle<Value> emit_argv[2] = { ev_CALLMEDIA_media_symbol, medias };
          call->emit->Call(call->handle_, 2, emit_argv);
        }
      }
      break;
      case EVENT_CALLDTMF: {
        HandleScope scope;
        EV_ARGS_CALLDTMF* args = reinterpret_cast<EV_ARGS_CALLDTMF*>(ev.args);
        SIPSTERCall* call = ev.call;
        Handle<Value> dtmf_char;
        switch (args->digit) {
          case '0':
            dtmf_char = CALLDTMF_DTMF0_symbol;
          break;
          case '1':
            dtmf_char = CALLDTMF_DTMF1_symbol;
          break;
          case '2':
            dtmf_char = CALLDTMF_DTMF2_symbol;
          break;
          case '3':
            dtmf_char = CALLDTMF_DTMF3_symbol;
          break;
          case '4':
            dtmf_char = CALLDTMF_DTMF4_symbol;
          break;
          case '5':
            dtmf_char = CALLDTMF_DTMF5_symbol;
          break;
          case '6':
            dtmf_char = CALLDTMF_DTMF6_symbol;
          break;
          case '7':
            dtmf_char = CALLDTMF_DTMF7_symbol;
          break;
          case '8':
            dtmf_char = CALLDTMF_DTMF8_symbol;
          break;
          case '9':
            dtmf_char = CALLDTMF_DTMF9_symbol;
          break;
          case '*':
            dtmf_char = CALLDTMF_DTMFSTAR_symbol;
          break;
          case '#':
            dtmf_char = CALLDTMF_DTMFPOUND_symbol;
          break;
          case 'A':
            dtmf_char = CALLDTMF_DTMFA_symbol;
          break;
          case 'B':
            dtmf_char = CALLDTMF_DTMFB_symbol;
          break;
          case 'C':
            dtmf_char = CALLDTMF_DTMFC_symbol;
          break;
          case 'D':
            dtmf_char = CALLDTMF_DTMFD_symbol;
          break;
          default:
            const char digit[1] = { args->digit };
            dtmf_char = String::New(digit, 1);
        }
        Handle<Value> emit_argv[2] = { ev_CALLDTMF_dtmf_symbol, dtmf_char };
        call->emit->Call(call->handle_, 2, emit_argv);
        delete args;
      }
      break;
      case EVENT_PLAYEREOF: {
        HandleScope scope;
        SIPSTERMedia* media = ev.media;
        Handle<Value> emit_argv[1] = { ev_PLAYEREOF_eof_symbol };
        media->emit->Call(media->handle_, 1, emit_argv);
      }
      break;
      case EVENT_REGSTARTING: {
        HandleScope scope;
        EV_ARGS_REGSTARTING* args = reinterpret_cast<EV_ARGS_REGSTARTING*>(ev.args);
        SIPSTERAccount* acct = ev.acct;

        Handle<Value> emit_argv[1] = {
          (args->renew
           ? ev_REGSTARTING_registering_symbol
           : ev_REGSTARTING_unregistering_symbol)
        };
        acct->emit->Call(acct->handle_, 1, emit_argv);
        delete args;
      }
      break;
    }
    if (try_catch.HasCaught())
      FatalException(try_catch);
  }
  uv_mutex_unlock(&event_mutex);
}

void logging_close_cb(uv_handle_t* handle) {}

void logging_cb(uv_async_t* handle, int status) {
  HandleScope scope;
  Handle<Value> log_argv[4];
  while (true) {
    uv_mutex_lock(&log_mutex);
    if (log_queue.empty())
      break;
    const CustomLogEntry log = log_queue.front();
    log_queue.pop_front();
    uv_mutex_unlock(&log_mutex);

    HandleScope scope;

    log_argv[0] = Integer::New(log.level);
    log_argv[1] = String::New(log.msg.c_str());
    log_argv[2] = Number::New(log.threadId);
    log_argv[3] = String::New(log.threadName.c_str());

    TryCatch try_catch;
    logger->func->Call(global, 4, log_argv);
    if (try_catch.HasCaught())
      FatalException(try_catch);
  }
}
// =============================================================================

// static methods ==============================================================

static Handle<Value> CreateRecorder(const Arguments& args) {
  HandleScope scope;

  string dest;
  unsigned fmt = PJMEDIA_FILE_WRITE_ULAW;
  pj_ssize_t max_size = 0;
  if (args.Length() > 0 && args[0]->IsString()) {
    dest = string(*String::AsciiValue(args[0]->ToString()));
    if (args.Length() > 1 && args[1]->IsString()) {
      const char* fmt_str = *String::AsciiValue(args[1]->ToString());
      if (strcasecmp(fmt_str, "pcm") == 0)
        fmt = PJMEDIA_FILE_WRITE_PCM;
      else if (strcasecmp(fmt_str, "alaw") == 0)
        fmt = PJMEDIA_FILE_WRITE_ALAW;
      else if (strcasecmp(fmt_str, "ulaw") != 0) {
        return ThrowException(
          Exception::Error(String::New("Invalid media format"))
        );
      }
    }
    if (args.Length() > 2 && args[2]->IsInt32()) {
      pj_ssize_t size = static_cast<pj_ssize_t>(args[2]->Int32Value());
      if (size >= -1)
        max_size = size;
    }
  } else {
    return ThrowException(
      Exception::Error(String::New("Missing destination filename"))
    );
  }

  AudioMediaRecorder* recorder = new AudioMediaRecorder();
  try {
    recorder->createRecorder(dest, 0, max_size, fmt);
  } catch(Error& err) {
    delete recorder;
    string errstr = "recorder->createRecorder() error: " + err.info();
    return ThrowException(Exception::Error(String::New(errstr.c_str())));
  }

  Local<Object> med_obj;
  med_obj = SIPSTERMedia_constructor->GetFunction()
                                    ->NewInstance(0, NULL);
  SIPSTERMedia* med = ObjectWrap::Unwrap<SIPSTERMedia>(med_obj);
  med->media = recorder;

  return scope.Close(med_obj);
}

static Handle<Value> CreatePlayer(const Arguments& args) {
  HandleScope scope;

  string src;
  unsigned opts = 0;
  if (args.Length() > 0 && args[0]->IsString()) {
    src = string(*String::AsciiValue(args[0]->ToString()));
    if (args.Length() > 1 && args[1]->IsBoolean() && args[1]->BooleanValue())
      opts = PJMEDIA_FILE_NO_LOOP;
  } else {
    return ThrowException(
      Exception::Error(String::New("Missing source filename"))
    );
  }

  SIPSTERPlayer* player = new SIPSTERPlayer();
  try {
    player->createPlayer(src, opts);
  } catch(Error& err) {
    delete player;
    string errstr = "player->createPlayer() error: " + err.info();
    return ThrowException(Exception::Error(String::New(errstr.c_str())));
  }

  Local<Object> med_obj;
  med_obj = SIPSTERMedia_constructor->GetFunction()
                                    ->NewInstance(0, NULL);
  SIPSTERMedia* med = ObjectWrap::Unwrap<SIPSTERMedia>(med_obj);
  med->media = player;
  player->media = med;
  player->options = opts;

  return scope.Close(med_obj);
}

static Handle<Value> CreatePlaylist(const Arguments& args) {
  HandleScope scope;

  unsigned opts = 0;
  vector<string> playlist;
  if (args.Length() > 0 && args[0]->IsArray()) {
    HandleScope scope;
    const Local<Array> arr_obj = Local<Array>::Cast(args[0]);
    const uint32_t arr_length = arr_obj->Length();
    if (arr_length == 0) {
      return ThrowException(
        Exception::Error(String::New("Nothing to add to playlist"))
      );
    }
    playlist.reserve(arr_length);
    for (uint32_t i = 0; i < arr_length; ++i) {
      playlist.push_back(string(*String::AsciiValue(arr_obj->Get(i)
                                                           ->ToString())));
    }
    if (args.Length() > 1 && args[1]->IsBoolean() && args[1]->BooleanValue())
      opts = PJMEDIA_FILE_NO_LOOP;
  } else {
    return ThrowException(
      Exception::Error(String::New("Missing source filenames"))
    );
  }

  SIPSTERPlayer* player = new SIPSTERPlayer();
  try {
    player->createPlaylist(playlist, "", opts);
  } catch(Error& err) {
    delete player;
    string errstr = "player->createPlayer() error: " + err.info();
    return ThrowException(Exception::Error(String::New(errstr.c_str())));
  }

  Local<Object> med_obj;
  med_obj = SIPSTERMedia_constructor->GetFunction()
                                    ->NewInstance(0, NULL);
  SIPSTERMedia* med = ObjectWrap::Unwrap<SIPSTERMedia>(med_obj);
  med->media = player;
  player->media = med;
  player->options = opts;

  return scope.Close(med_obj);
}

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

  if (!ep_create) {
    try {
      ep->libCreate();
      ep_create = true;
    } catch(Error& err) {
      errstr = "libCreate error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
    }
  }

  Local<Value> val;
  if (args.Length() > 0 && args[0]->IsObject()) {
    HandleScope scope;
    Local<Object> cfg_obj = args[0]->ToObject();
    val = cfg_obj->Get(String::New("uaConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      UaConfig uaConfig;
      Local<Object> ua_obj = val->ToObject();
      JS2PJ_UINT(ua_obj, maxCalls, uaConfig);
      JS2PJ_UINT(ua_obj, threadCnt, uaConfig);
      JS2PJ_BOOL(ua_obj, mainThreadOnly, uaConfig);

      val = ua_obj->Get(String::New("nameserver"));
      if (val->IsArray()) {
        HandleScope scope;
        const Local<Array> arr_obj = Local<Array>::Cast(val);
        const uint32_t arr_length = arr_obj->Length();
        if (arr_length > 0) {
          vector<string> nameservers;
          for (uint32_t i = 0; i < arr_length; ++i) {
            HandleScope scope;
            const Local<Value> arr_val = arr_obj->Get(i);
            if (arr_val->IsString())
              nameservers.push_back(string(*String::AsciiValue(arr_val->ToString())));
          }
          if (nameservers.size() > 0)
            uaConfig.nameserver = nameservers;
        }
      }

      JS2PJ_STR(ua_obj, userAgent, uaConfig);

      val = ua_obj->Get(String::New("stunServer"));
      if (val->IsArray()) {
        HandleScope scope;
        const Local<Array> arr_obj = Local<Array>::Cast(val);
        const uint32_t arr_length = arr_obj->Length();
        if (arr_length > 0) {
          vector<string> stunServers;
          for (uint32_t i = 0; i < arr_length; ++i) {
            HandleScope scope;
            const Local<Value> arr_val = arr_obj->Get(i);
            if (arr_val->IsString())
              stunServers.push_back(string(*String::AsciiValue(arr_val->ToString())));
          }
          if (stunServers.size() > 0)
            uaConfig.stunServer = stunServers;
        }
      }

      JS2PJ_BOOL(ua_obj, stunIgnoreFailure, uaConfig);
      JS2PJ_INT(ua_obj, natTypeInSdp, uaConfig);
      JS2PJ_BOOL(ua_obj, mwiUnsolicitedEnabled, uaConfig);

      ep_cfg.uaConfig = uaConfig;
    }

    val = cfg_obj->Get(String::New("logConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      LogConfig logConfig;
      Local<Object> log_obj = val->ToObject();
      JS2PJ_UINT(log_obj, msgLogging, logConfig);
      JS2PJ_UINT(log_obj, level, logConfig);
      JS2PJ_UINT(log_obj, consoleLevel, logConfig);
      JS2PJ_UINT(log_obj, decor, logConfig);
      JS2PJ_STR(log_obj, filename, logConfig);
      JS2PJ_UINT(log_obj, fileFlags, logConfig);

      val = log_obj->Get(String::New("writer"));
      if (val->IsFunction()) {
        HandleScope scope;
        if (logger) {
          delete logger;
          uv_close(reinterpret_cast<uv_handle_t*>(&logging), logging_close_cb);
        }
        logger = new SIPSTERLogWriter();
        logger->func = Persistent<Function>::New(Local<Function>::Cast(val));
        logConfig.writer = logger;
        uv_async_init(uv_default_loop(), &logging, logging_cb);
      }

      ep_cfg.logConfig = logConfig;
    }

    val = cfg_obj->Get(String::New("medConfig"));
    if (val->IsObject()) {
      HandleScope scope;
      MediaConfig medConfig;
      Local<Object> med_obj = val->ToObject();
      JS2PJ_UINT(med_obj, clockRate, medConfig);
      JS2PJ_UINT(med_obj, sndClockRate, medConfig);
      JS2PJ_UINT(med_obj, channelCount, medConfig);
      JS2PJ_UINT(med_obj, audioFramePtime, medConfig);
      JS2PJ_UINT(med_obj, maxMediaPorts, medConfig);
      JS2PJ_BOOL(med_obj, hasIoqueue, medConfig);
      JS2PJ_UINT(med_obj, threadCnt, medConfig);
      JS2PJ_UINT(med_obj, quality, medConfig);
      JS2PJ_UINT(med_obj, ptime, medConfig);
      JS2PJ_BOOL(med_obj, noVad, medConfig);
      JS2PJ_UINT(med_obj, ilbcMode, medConfig);
      JS2PJ_UINT(med_obj, txDropPct, medConfig);
      JS2PJ_UINT(med_obj, rxDropPct, medConfig);
      JS2PJ_UINT(med_obj, ecOptions, medConfig);
      JS2PJ_UINT(med_obj, ecTailLen, medConfig);
      JS2PJ_UINT(med_obj, sndRecLatency, medConfig);
      JS2PJ_UINT(med_obj, sndPlayLatency, medConfig);
      JS2PJ_INT(med_obj, jbInit, medConfig);
      JS2PJ_INT(med_obj, jbMinPre, medConfig);
      JS2PJ_INT(med_obj, jbMaxPre, medConfig);
      JS2PJ_INT(med_obj, jbMax, medConfig);
      JS2PJ_INT(med_obj, sndAutoCloseTime, medConfig);
      JS2PJ_BOOL(med_obj, vidPreviewEnableNative, medConfig);

      ep_cfg.medConfig = medConfig;
    }
  }

  try {
    ep->libInit(ep_cfg);
    ep_init = true;
  } catch(Error& err) {
    errstr = "libInit error: " + err.info();
    return ThrowException(Exception::Error(String::New(errstr.c_str())));
  }

  uv_async_init(uv_default_loop(), &dumb, dumb_cb);

  Endpoint::instance().audDevManager().setNullDev();

  if ((args.Length() == 1 && args[0]->IsBoolean() && args[0]->BooleanValue())
      || (args.Length() > 1 && args[1]->IsBoolean() && args[1]->BooleanValue())) {
    if (ep_start)
      return ThrowException(Exception::Error(String::New("Already started")));
    try {
      ep->libStart();
      ep_start = true;
    } catch(Error& err) {
      string errstr = "libStart error: " + err.info();
      return ThrowException(Exception::Error(String::New(errstr.c_str())));
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
    return ThrowException(Exception::Error(String::New(errstr.c_str())));
  }

  try {
    return scope.Close(String::New(wdoc.saveString().c_str()));
  } catch(Error& err) {
    errstr = "JsonDocument.saveString error: " + err.info();
    return ThrowException(Exception::Error(String::New(errstr.c_str())));
  }
}

static Handle<Value> EPGetState(const Arguments& args) {
  HandleScope scope;

  try {
    pjsua_state st = ep->libGetState();
    string state;
    switch (st) {
      case PJSUA_STATE_CREATED:
        state = "created";
      break;
      case PJSUA_STATE_INIT:
        state = "init";
      break;
      case PJSUA_STATE_STARTING:
        state = "starting";
      break;
      case PJSUA_STATE_RUNNING:
        state = "running";
      break;
      case PJSUA_STATE_CLOSING:
        state = "closing";
      break;
      default:
        return Null();
    }
    return scope.Close(String::New(state.c_str()));
  } catch(Error& err) {
    string errstr = "libGetState error: " + err.info();
    return ThrowException(Exception::Error(String::New(errstr.c_str())));
  }
}

static Handle<Value> EPHangupAllCalls(const Arguments& args) {
  HandleScope scope;

  try {
    ep->hangupAllCalls();
  } catch(Error& err) {
    string errstr = "hangupAllCalls error: " + err.info();
    return ThrowException(Exception::Error(String::New(errstr.c_str())));
  }
  return Undefined();
}

static Handle<Value> EPMediaActivePorts(const Arguments& args) {
  HandleScope scope;

  return scope.Close(Integer::NewFromUnsigned(ep->mediaActivePorts()));
}

static Handle<Value> EPMediaMaxPorts(const Arguments& args) {
  HandleScope scope;

  return scope.Close(Integer::NewFromUnsigned(ep->mediaMaxPorts()));
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
  REGSTATE_FIELDS
  REGSTARTING_FIELDS
#undef X

    CALLDTMF_DTMF0_symbol = NODE_PSYMBOL("0");
    CALLDTMF_DTMF1_symbol = NODE_PSYMBOL("1");
    CALLDTMF_DTMF2_symbol = NODE_PSYMBOL("2");
    CALLDTMF_DTMF3_symbol = NODE_PSYMBOL("3");
    CALLDTMF_DTMF4_symbol = NODE_PSYMBOL("4");
    CALLDTMF_DTMF5_symbol = NODE_PSYMBOL("5");
    CALLDTMF_DTMF6_symbol = NODE_PSYMBOL("6");
    CALLDTMF_DTMF7_symbol = NODE_PSYMBOL("7");
    CALLDTMF_DTMF8_symbol = NODE_PSYMBOL("8");
    CALLDTMF_DTMF9_symbol = NODE_PSYMBOL("9");
    CALLDTMF_DTMFSTAR_symbol = NODE_PSYMBOL("*");
    CALLDTMF_DTMFPOUND_symbol = NODE_PSYMBOL("#");
    CALLDTMF_DTMFA_symbol = NODE_PSYMBOL("A");
    CALLDTMF_DTMFB_symbol = NODE_PSYMBOL("B");
    CALLDTMF_DTMFC_symbol = NODE_PSYMBOL("C");
    CALLDTMF_DTMFD_symbol = NODE_PSYMBOL("D");
    media_dir_none_symbol = NODE_PSYMBOL("none");
    media_dir_outbound_symbol = NODE_PSYMBOL("outbound");
    media_dir_inbound_symbol = NODE_PSYMBOL("inbound");
    media_dir_bidi_symbol = NODE_PSYMBOL("bidirectional");
    media_dir_unknown_symbol = NODE_PSYMBOL("unknown");
    emit_symbol = NODE_PSYMBOL("emit");

    uv_mutex_init(&event_mutex);
    uv_mutex_init(&log_mutex);
    uv_mutex_init(&async_mutex);

    SIPSTERAccount::Initialize(target);
    SIPSTERCall::Initialize(target);
    SIPSTERMedia::Initialize(target);
    SIPSTERTransport::Initialize(target);

    global = Persistent<Object>::New(Context::GetCurrent()->Global());

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
    target->Set(String::NewSymbol("mediaMaxPorts"),
                FunctionTemplate::New(EPMediaMaxPorts)->GetFunction());

    target->Set(String::NewSymbol("createRecorder"),
                FunctionTemplate::New(CreateRecorder)->GetFunction());
    target->Set(String::NewSymbol("createPlayer"),
                FunctionTemplate::New(CreatePlayer)->GetFunction());
    target->Set(String::NewSymbol("createPlaylist"),
                FunctionTemplate::New(CreatePlaylist)->GetFunction());
  }

  NODE_MODULE(sipster, init);
}
