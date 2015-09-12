#ifndef SIPSTERCALL_H_
#define SIPSTERCALL_H_

#include <node.h>
#include <nan.h>
#include <string.h>
#include <strings.h>
#include <regex.h>
#include <pjsua2.hpp>

using namespace std;
using namespace node;
using namespace v8;
using namespace pj;

class SIPSTERCall : public Call, public Nan::ObjectWrap {
public:
  Nan::Callback* emit;

  SIPSTERCall(Account &acc, int call_id=PJSUA_INVALID_ID);
  ~SIPSTERCall();

  void onCallMediaState(OnCallMediaStateParam &prm);
  virtual void onCallState(OnCallStateParam &prm);
  virtual void onDtmfDigit(OnDtmfDigitParam &prm);
  static NAN_METHOD(New);
  static NAN_METHOD(Answer);
  static NAN_METHOD(Hangup);
  static NAN_METHOD(SetHold);
  static NAN_METHOD(Reinvite);
  static NAN_METHOD(Update);
  static NAN_METHOD(DialDtmf);
  static NAN_METHOD(Transfer);
  static NAN_METHOD(DoRef);
  static NAN_METHOD(DoUnref);
  static NAN_METHOD(GetStats);
  static NAN_GETTER(ConDurationGetter);
  static NAN_GETTER(TotDurationGetter);
  static NAN_GETTER(HasMediaGetter);
  static NAN_GETTER(IsActiveGetter);
  static void Initialize(Handle<Object> target);
};

#endif
