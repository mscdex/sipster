#include "SIPSTERMedia.h"
#include "common.h"

Nan::Persistent<FunctionTemplate> SIPSTERMedia_constructor;

static Nan::Persistent<String> media_dir_none_symbol;
static Nan::Persistent<String> media_dir_outbound_symbol;
static Nan::Persistent<String> media_dir_inbound_symbol;
static Nan::Persistent<String> media_dir_bidi_symbol;
static Nan::Persistent<String> media_dir_unknown_symbol;

SIPSTERMedia::SIPSTERMedia() : emit(NULL), media(NULL), is_media_new(false) {}
SIPSTERMedia::~SIPSTERMedia() {
  if (media && is_media_new) {
    delete media;
  }
  if (emit) {
    delete emit;
  }
  is_media_new = false;
}

NAN_METHOD(SIPSTERMedia::New) {
  Nan::HandleScope scope;

  if (!info.IsConstructCall())
    return Nan::ThrowError("Use `new` to create instances of this object.");

  SIPSTERMedia* med = new SIPSTERMedia();

  med->Wrap(info.This());

  med->emit = new Nan::Callback(
    Local<Function>::Cast(med->handle()->Get(Nan::New(emit_symbol)))
  );

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(SIPSTERMedia::StartTransmit) {
  Nan::HandleScope scope;
  SIPSTERMedia* src = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());

  if (info.Length() == 0
      || !Nan::New(SIPSTERMedia_constructor)->HasInstance(info[0]))
    return Nan::ThrowTypeError("Expected Media object");

  SIPSTERMedia* dest =
    Nan::ObjectWrap::Unwrap<SIPSTERMedia>(Local<Object>::Cast(info[0]));

  if (!src->media)
    return Nan::ThrowError("Invalid source");
  else if (!dest->media)
    return Nan::ThrowError("Invalid destination");

  try {
    src->media->startTransmit(*dest->media);
  } catch(Error& err) {
    string errstr = "AudioMedia.startTransmit() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERMedia::StopTransmit) {
  Nan::HandleScope scope;
  SIPSTERMedia* src = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());

  if (info.Length() == 0
      || !Nan::New(SIPSTERMedia_constructor)->HasInstance(info[0]))
    return Nan::ThrowTypeError("Expected Media object");

  SIPSTERMedia* dest =
    Nan::ObjectWrap::Unwrap<SIPSTERMedia>(Local<Object>::Cast(info[0]));

  if (!src->media)
    return Nan::ThrowError("Invalid source");
  else if (!dest->media)
    return Nan::ThrowError("Invalid destination");

  try {
    src->media->stopTransmit(*dest->media);
  } catch(Error& err) {
    string errstr = "AudioMedia.stopTransmit() error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERMedia::AdjustRxLevel) {
  Nan::HandleScope scope;
  SIPSTERMedia* med = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());

  if (med->media) {
    if (info.Length() > 0 && info[0]->IsNumber()) {
      try {
        med->media->adjustRxLevel(static_cast<float>(info[0]->NumberValue()));
      } catch(Error& err) {
        string errstr = "AudioMedia.adjustRxLevel() error: " + err.info();
        return Nan::ThrowError(errstr.c_str());
      }
    } else
      return Nan::ThrowTypeError("Missing signal level");
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERMedia::AdjustTxLevel) {
  Nan::HandleScope scope;
  SIPSTERMedia* med = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());

  if (med->media) {
    if (info.Length() > 0 && info[0]->IsNumber()) {
      try {
        med->media->adjustTxLevel(static_cast<float>(info[0]->NumberValue()));
      } catch(Error& err) {
        string errstr = "AudioMedia.adjustTxLevel() error: " + err.info();
        return Nan::ThrowError(errstr.c_str());
      }
    } else
      return Nan::ThrowTypeError("Missing signal level");
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERMedia::Close) {
  Nan::HandleScope scope;
  SIPSTERMedia* med = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());

  if (med->media && med->is_media_new) {
    delete med->media;
    med->media = NULL;
  }

  info.GetReturnValue().SetUndefined();
}

NAN_GETTER(SIPSTERMedia::RxLevelGetter) {
  SIPSTERMedia* med = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());

  unsigned level = 0;

  if (med->media) {
    try {
      level = med->media->getRxLevel();
    } catch(Error& err) {
      string errstr = "AudioMedia.getRxLevel() error: " + err.info();
      return Nan::ThrowError(errstr.c_str());
    }
  }

  info.GetReturnValue().Set(Nan::New(level));
}

NAN_GETTER(SIPSTERMedia::TxLevelGetter) {
  SIPSTERMedia* med = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());

  unsigned level = 0;

  if (med->media) {
    try {
      level = med->media->getTxLevel();
    } catch(Error& err) {
      string errstr = "AudioMedia.getTxLevel() error: " + err.info();
      return Nan::ThrowError(errstr.c_str());
    }
  }

  info.GetReturnValue().Set(Nan::New(level));
}

NAN_GETTER(SIPSTERMedia::DirGetter) {
  SIPSTERMedia* med = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());
  Local<String> str;
  switch (med->dir) {
    case PJMEDIA_DIR_NONE:
      str = Nan::New(media_dir_none_symbol);
    break;
    case PJMEDIA_DIR_ENCODING:
      str = Nan::New(media_dir_outbound_symbol);
    break;
    case PJMEDIA_DIR_DECODING:
      str = Nan::New(media_dir_inbound_symbol);
    break;
    case PJMEDIA_DIR_ENCODING_DECODING:
      str = Nan::New(media_dir_bidi_symbol);
    break;
    default:
      str = Nan::New(media_dir_unknown_symbol);
  }
  info.GetReturnValue().Set(str);
}

NAN_GETTER(SIPSTERMedia::SrcRTPGetter) {
  SIPSTERMedia* med = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());

  info.GetReturnValue().Set(Nan::New(med->srcRTP.c_str()).ToLocalChecked());
}

NAN_GETTER(SIPSTERMedia::SrcRTCPGetter) {
  SIPSTERMedia* med = Nan::ObjectWrap::Unwrap<SIPSTERMedia>(info.This());

  info.GetReturnValue().Set(Nan::New(med->srcRTCP.c_str()).ToLocalChecked());
}

void SIPSTERMedia::Initialize(Handle<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  Local<String> name = Nan::New("Media").ToLocalChecked();

  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->SetClassName(name);

  media_dir_none_symbol.Reset(Nan::New("none").ToLocalChecked());
  media_dir_outbound_symbol.Reset(Nan::New("outbound").ToLocalChecked());
  media_dir_inbound_symbol.Reset(Nan::New("inbound").ToLocalChecked());
  media_dir_bidi_symbol.Reset(Nan::New("bidirectional").ToLocalChecked());
  media_dir_unknown_symbol.Reset(Nan::New("unknown").ToLocalChecked());

  Nan::SetPrototypeMethod(tpl, "startTransmitTo", StartTransmit);
  Nan::SetPrototypeMethod(tpl, "stopTransmitTo", StopTransmit);
  Nan::SetPrototypeMethod(tpl, "adjustRxLevel", AdjustRxLevel);
  Nan::SetPrototypeMethod(tpl, "adjustTxLevel", AdjustTxLevel);
  Nan::SetPrototypeMethod(tpl, "close", Close);

  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("dir").ToLocalChecked(),
                   DirGetter);
  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("rtpAddr").ToLocalChecked(),
                   SrcRTPGetter);
  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("rtcpAddr").ToLocalChecked(),
                   SrcRTCPGetter);
  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("rxLevel").ToLocalChecked(),
                   RxLevelGetter);
  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("txLevel").ToLocalChecked(),
                   TxLevelGetter);

  Nan::Set(target, name, tpl->GetFunction());

  SIPSTERMedia_constructor.Reset(tpl);
}
