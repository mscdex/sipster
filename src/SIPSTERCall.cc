#include "SIPSTERCall.h"
#include "SIPSTERAccount.h"
#include "common.h"

Nan::Persistent<FunctionTemplate> SIPSTERCall_constructor;

SIPSTERCall::SIPSTERCall(Account &acc, int call_id) : Call(acc, call_id) {
  emit = NULL;
  uv_mutex_lock(&async_mutex);
  uv_ref(reinterpret_cast<uv_handle_t*>(&dumb));
  uv_mutex_unlock(&async_mutex);
}

SIPSTERCall::~SIPSTERCall() {
  if (emit)
    delete emit;
  uv_mutex_lock(&async_mutex);
  uv_unref(reinterpret_cast<uv_handle_t*>(&dumb));
  uv_mutex_unlock(&async_mutex);
}

void SIPSTERCall::onCallMediaState(OnCallMediaStateParam &prm) {
  SETUP_EVENT_NOARGS(CALLMEDIA);
  ev.call = this;

  ENQUEUE_EVENT(ev);
}

void SIPSTERCall::onCallState(OnCallStateParam &prm) {
  CallInfo ci = getInfo();

  SETUP_EVENT(CALLSTATE);
  ev.call = this;

  args->_state = ci.state;

  ENQUEUE_EVENT(ev);
}

void SIPSTERCall::onDtmfDigit(OnDtmfDigitParam &prm) {
  CallInfo ci = getInfo();

  SETUP_EVENT(CALLDTMF);
  ev.call = this;

  args->digit = prm.digit[0];

  ENQUEUE_EVENT(ev);
}

NAN_METHOD(SIPSTERCall::New) {
  Nan::HandleScope scope;

  if (!info.IsConstructCall())
    return Nan::ThrowError("Use `new` to create instances of this object.");

  SIPSTERCall* call = NULL;
  if (info.Length() > 0) {
    if (Nan::New(SIPSTERAccount_constructor)->HasInstance(info[0])) {
      Account* acct =
        Nan::ObjectWrap::Unwrap<SIPSTERAccount>(Local<Object>::Cast(info[0]));
      call = new SIPSTERCall(*acct);
    } else {
      Local<External> extCall = Local<External>::Cast(info[0]);
      call = static_cast<SIPSTERCall*>(extCall->Value());
    }
  } else
    return Nan::ThrowError("Expected callId or Account argument");

  call->Wrap(info.This());

  call->emit = new Nan::Callback(
    Local<Function>::Cast(call->handle()->Get(Nan::New(emit_symbol)))
  );

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(SIPSTERCall::Answer) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  CallOpParam prm;
  if (info.Length() > 0 && info[0]->IsUint32()) {
    prm.statusCode = static_cast<pjsip_status_code>(info[0]->Int32Value());
    if (info.Length() > 1 && info[1]->IsString()) {
      Nan::Utf8String reason_str(info[1]);
      prm.reason = string(*reason_str);
    }
  } else
    prm.statusCode = PJSIP_SC_OK;

  try {
    call->answer(prm);
  } catch(Error& err) {
    string errstr = "Call.answer() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERCall::Hangup) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  CallOpParam prm;
  if (info.Length() > 0 && info[0]->IsUint32()) {
    prm.statusCode = static_cast<pjsip_status_code>(info[0]->Int32Value());
    if (info.Length() > 1 && info[1]->IsString()) {
      Nan::Utf8String reason_str(info[1]);
      prm.reason = string(*reason_str);
    }
  }

  try {
    call->hangup(prm);
  } catch(Error& err) {
    string errstr = "Call.hangup() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERCall::SetHold) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  CallOpParam prm;
  if (info.Length() > 0 && info[0]->IsUint32()) {
    prm.statusCode = static_cast<pjsip_status_code>(info[0]->Int32Value());
    if (info.Length() > 1 && info[1]->IsString()) {
      Nan::Utf8String reason_str(info[1]);
      prm.reason = string(*reason_str);
    }
  }

  try {
    call->setHold(prm);
  } catch(Error& err) {
    string errstr = "Call.setHold() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERCall::Reinvite) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  CallOpParam prm;
  if (info.Length() > 0 && info[0]->IsUint32()) {
    prm.statusCode = static_cast<pjsip_status_code>(info[0]->Int32Value());
    if (info.Length() > 1 && info[1]->IsString()) {
      Nan::Utf8String reason_str(info[1]);
      prm.reason = string(*reason_str);
    }
  }

  try {
    call->reinvite(prm);
  } catch(Error& err) {
    string errstr = "Call.reinvite() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERCall::Update) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  CallOpParam prm;
  if (info.Length() > 0 && info[0]->IsUint32()) {
    prm.statusCode = static_cast<pjsip_status_code>(info[0]->Int32Value());
    if (info.Length() > 1 && info[1]->IsString()) {
      Nan::Utf8String reason_str(info[1]);
      prm.reason = string(*reason_str);
    }
  }

  try {
    call->update(prm);
  } catch(Error& err) {
    string errstr = "Call.update() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERCall::DialDtmf) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  if (info.Length() > 0 && info[0]->IsString()) {
    try {
      Nan::Utf8String dtmf_str(info[0]);
      call->dialDtmf(string(*dtmf_str));
    } catch(Error& err) {
      string errstr = "Call.dialDtmf() error: " + err.info();
      return Nan::ThrowError(errstr.c_str());
    }
  } else
    return Nan::ThrowTypeError("Missing DTMF string");

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERCall::Transfer) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  string dest;
  CallOpParam prm;
  if (info.Length() > 0 && info[0]->IsString()) {
    Nan::Utf8String dest_str(info[0]);
    dest = string(*dest_str);
    if (info.Length() > 1) {
      prm.statusCode = static_cast<pjsip_status_code>(info[1]->Int32Value());
      if (info.Length() > 2 && info[2]->IsString()) {
        Nan::Utf8String reason_str(info[2]);
        prm.reason = string(*reason_str);
      }
    }
  } else
    return Nan::ThrowTypeError("Missing transfer destination");

  try {
    call->xfer(dest, prm);
  } catch(Error& err) {
    string errstr = "Call.xfer() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERCall::DoRef) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  call->Ref();

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERCall::DoUnref) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  call->Unref();

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERCall::GetStats) {
  Nan::HandleScope scope;
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  bool with_media = true;
  string indent = "  ";
  if (info.Length() > 0 && info[0]->IsBoolean()) {
    with_media = info[0]->BooleanValue();
    if (info.Length() > 1 && info[1]->IsString()) {
      Nan::Utf8String indent_str(info[1]);
      indent = string(*indent_str);
    }
  }

  string stats_info;
  try {
    stats_info = call->dump(with_media, indent);
  } catch(Error& err) {
    string errstr = "Call.dump() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  info.GetReturnValue().Set(Nan::New(stats_info.c_str()).ToLocalChecked());
}

NAN_GETTER(SIPSTERCall::ConDurationGetter) {
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  CallInfo ci;

  try {
    ci = call->getInfo();
  } catch(Error& err) {
    string errstr = "Call.getInfo() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  double duration = ci.connectDuration.sec + (ci.connectDuration.msec / 1000);

  info.GetReturnValue().Set(Nan::New<Number>(duration));
}

NAN_GETTER(SIPSTERCall::TotDurationGetter) {
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  CallInfo ci;

  try {
    ci = call->getInfo();
  } catch(Error& err) {
    string errstr = "Call.getInfo() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  double duration = ci.totalDuration.sec + (ci.totalDuration.msec / 1000);

  info.GetReturnValue().Set(Nan::New<Number>(duration));
}

NAN_GETTER(SIPSTERCall::HasMediaGetter) {
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  info.GetReturnValue().Set(Nan::New(call->hasMedia()));
}

NAN_GETTER(SIPSTERCall::IsActiveGetter) {
  SIPSTERCall* call = Nan::ObjectWrap::Unwrap<SIPSTERCall>(info.This());

  info.GetReturnValue().Set(Nan::New(call->isActive()));
}

void SIPSTERCall::Initialize(Handle<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  Local<String> name = Nan::New("Call").ToLocalChecked();

  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->SetClassName(name);

  Nan::SetPrototypeMethod(tpl, "answer", Answer);
  Nan::SetPrototypeMethod(tpl, "hangup", Hangup);
  Nan::SetPrototypeMethod(tpl, "hold", SetHold);
  Nan::SetPrototypeMethod(tpl, "reinvite", Reinvite);
  Nan::SetPrototypeMethod(tpl, "update", Update);
  Nan::SetPrototypeMethod(tpl, "dtmf", DialDtmf);
  Nan::SetPrototypeMethod(tpl, "transfer", Transfer);
  Nan::SetPrototypeMethod(tpl, "ref", DoRef);
  Nan::SetPrototypeMethod(tpl, "unref", DoUnref);
  Nan::SetPrototypeMethod(tpl, "getStatsDump", GetStats);

  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("connDuration").ToLocalChecked(),
                   ConDurationGetter);
  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("totalDuration").ToLocalChecked(),
                   TotDurationGetter);
  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("hasMedia").ToLocalChecked(),
                   HasMediaGetter);
  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("isActive").ToLocalChecked(),
                   IsActiveGetter);

  Nan::Set(target, name, tpl->GetFunction());

  SIPSTERCall_constructor.Reset(tpl);
}
