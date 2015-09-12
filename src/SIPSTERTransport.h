#ifndef SIPSTERTRANSPORT_H_
#define SIPSTERTRANSPORT_H_

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

class SIPSTERTransport : public Nan::ObjectWrap {
public:
  Nan::Callback* emit;
  TransportId transId;
  bool enabled;

  SIPSTERTransport();
  ~SIPSTERTransport();

  static NAN_METHOD(New);
  static NAN_METHOD(GetInfo);
  static NAN_METHOD(Enable);
  static NAN_METHOD(Disable);
  static NAN_METHOD(DoRef);
  static NAN_METHOD(DoUnref);
  static NAN_GETTER(EnabledGetter);
  static void Initialize(Handle<Object> target);
};

#endif
