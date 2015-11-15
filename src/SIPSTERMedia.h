#ifndef SIPSTERMEDIA_H_
#define SIPSTERMEDIA_H_

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

class SIPSTERMedia : public Nan::ObjectWrap {
public:
  Nan::Callback* emit;
  AudioMedia* media;
  bool is_media_new;
  pjmedia_dir dir;
  string srcRTP;
  string srcRTCP;


  SIPSTERMedia();
  ~SIPSTERMedia();

  static NAN_METHOD(New);
  static NAN_METHOD(StartTransmit);
  static NAN_METHOD(StopTransmit);
  static NAN_METHOD(AdjustRxLevel);
  static NAN_METHOD(AdjustTxLevel);
  static NAN_METHOD(Close);
  static NAN_GETTER(RxLevelGetter);
  static NAN_GETTER(TxLevelGetter);
  static NAN_GETTER(DirGetter);
  static NAN_GETTER(SrcRTPGetter);
  static NAN_GETTER(SrcRTCPGetter);
  static void Initialize(Handle<Object> target);
};

#endif
