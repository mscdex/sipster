#ifndef SIPSTERACCOUNT_H_
#define SIPSTERACCOUNT_H_

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

class SIPSTERAccount : public Account, public Nan::ObjectWrap {
public:
  Nan::Callback* emit;

  SIPSTERAccount();
  ~SIPSTERAccount();

  static AccountConfig genConfig(Local<Object> acct_obj);
  virtual void onRegStarted(OnRegStartedParam &prm);
  virtual void onRegState(OnRegStateParam &prm);
  virtual void onIncomingCall(OnIncomingCallParam &iprm);
  static NAN_METHOD(New);
  static NAN_METHOD(Modify);
  static NAN_GETTER(ValidGetter);
  static NAN_GETTER(DefaultGetter);
  static NAN_SETTER(DefaultSetter);
  static NAN_METHOD(GetInfo);
  static NAN_METHOD(SetRegistration);
  static NAN_METHOD(SetTransport);
  static NAN_METHOD(MakeCall);
  static NAN_METHOD(DoRef);
  static NAN_METHOD(DoUnref);
  static void Initialize(Handle<Object> target);
};

#endif
