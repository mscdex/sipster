#include "SIPSTERTransport.h"
#include "common.h"

Nan::Persistent<FunctionTemplate> SIPSTERTransport_constructor;

SIPSTERTransport::SIPSTERTransport() : transId(-1), enabled(false) {}
SIPSTERTransport::~SIPSTERTransport() {
  if (emit)
    delete emit;
  if (transId > -1) {
    try {
      ep->transportClose(transId);
      uv_mutex_lock(&async_mutex);
      uv_unref(reinterpret_cast<uv_handle_t*>(&dumb));
      uv_mutex_unlock(&async_mutex);
    } catch(Error& err) {
      string errstr = "transportClose error: " + err.info();
      Nan::ThrowError(errstr.c_str());
    }
  }
}

NAN_METHOD(SIPSTERTransport::New) {
  Nan::HandleScope scope;

  if (!info.IsConstructCall())
    return Nan::ThrowError("Use `new` to create instances of this object.");

  TransportConfig tp_cfg;
  string errstr;

  Local<Value> val;
  if (info.Length() > 0 && info[0]->IsObject()) {
    Local<Object> obj = info[0]->ToObject();
    JS2PJ_UINT(obj, port, tp_cfg);
    JS2PJ_UINT(obj, portRange, tp_cfg);
    JS2PJ_STR(obj, publicAddress, tp_cfg);
    JS2PJ_STR(obj, boundAddress, tp_cfg);
    JS2PJ_ENUM(obj, qosType, pj_qos_type, tp_cfg);

    val = obj->Get(Nan::New("qosParams").ToLocalChecked());
    if (val->IsObject()) {
      pj_qos_params qos_params;
      Local<Object> qos_obj = val->ToObject();
      Local<Value> flags_val = qos_obj->Get(Nan::New("flags").ToLocalChecked());
      Local<Value> dscp_val = qos_obj->Get(Nan::New("dscp_val").ToLocalChecked());
      Local<Value> so_prio_val = qos_obj->Get(Nan::New("so_prio").ToLocalChecked());
      Local<Value> wmm_prio_val = qos_obj->Get(Nan::New("wmm_prio").ToLocalChecked());
      if (flags_val->IsUint32())
        qos_params.flags = static_cast<pj_uint8_t>(flags_val->Uint32Value());
      if (dscp_val->IsUint32()) {
        qos_params.dscp_val =
          static_cast<pj_uint8_t>(dscp_val->Uint32Value());
      }
      if (so_prio_val->IsUint32()) {
        qos_params.so_prio =
          static_cast<pj_uint8_t>(so_prio_val->Uint32Value());
      }
      if (wmm_prio_val->IsUint32()) {
        qos_params.wmm_prio =
          static_cast<pj_qos_wmm_prio>(wmm_prio_val->Uint32Value());
      }
      tp_cfg.qosParams = qos_params;
    }

    val = obj->Get(Nan::New("tlsConfig").ToLocalChecked());
    if (val->IsObject()) {
      Local<Object> tls_obj = val->ToObject();
      JS2PJ_STR(tls_obj, CaListFile, tp_cfg.tlsConfig);
      JS2PJ_STR(tls_obj, certFile, tp_cfg.tlsConfig);
      JS2PJ_STR(tls_obj, privKeyFile, tp_cfg.tlsConfig);
      JS2PJ_STR(tls_obj, password, tp_cfg.tlsConfig);
      JS2PJ_ENUM(tls_obj, method, pjsip_ssl_method, tp_cfg.tlsConfig);

      val = tls_obj->Get(Nan::New("ciphers").ToLocalChecked());
      if (val->IsArray()) {
        const Local<Array> arr_obj = Local<Array>::Cast(val);
        const uint32_t arr_length = arr_obj->Length();
        if (arr_length > 0) {
          vector<int> ciphers;
          for (uint32_t i = 0; i < arr_length; ++i) {
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

      val = tls_obj->Get(Nan::New("qosParams").ToLocalChecked());
      if (val->IsObject()) {
        pj_qos_params qos_params;
        Local<Object> qos_obj = val->ToObject();
        Local<Value> flags_val = qos_obj->Get(Nan::New("flags").ToLocalChecked());
        Local<Value> dscp_val = qos_obj->Get(Nan::New("dscp_val").ToLocalChecked());
        Local<Value> so_prio_val = qos_obj->Get(Nan::New("so_prio").ToLocalChecked());
        Local<Value> wmm_prio_val = qos_obj->Get(Nan::New("wmm_prio").ToLocalChecked());
        if (flags_val->IsUint32()) {
          qos_params.flags =
            static_cast<pj_uint8_t>(flags_val->Uint32Value());
        }
        if (dscp_val->IsUint32()) {
          qos_params.dscp_val =
            static_cast<pj_uint8_t>(dscp_val->Uint32Value());
        }
        if (so_prio_val->IsUint32()) {
          qos_params.so_prio =
            static_cast<pj_uint8_t>(so_prio_val->Uint32Value());
        }
        if (wmm_prio_val->IsUint32()) {
          qos_params.wmm_prio =
            static_cast<pj_qos_wmm_prio>(wmm_prio_val->Uint32Value());
        }
        tp_cfg.tlsConfig.qosParams = qos_params;
      }

      JS2PJ_BOOL(tls_obj, qosIgnoreError, tp_cfg.tlsConfig);
    }
  }

  val.Clear();
  pjsip_transport_type_e transportType = PJSIP_TRANSPORT_UDP;
  if (info.Length() > 0 && info[0]->IsString())
    val = info[0];
  else if (info.Length() > 1 && info[1]->IsString())
    val = info[1];
  if (!val.IsEmpty()) {
    Nan::Utf8String type_str(val);
    const char* typecstr = *type_str;
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
    else
      return Nan::ThrowError("Unsupported transport type");
  }

  TransportId tid;
  try {
    tid = ep->transportCreate(transportType, tp_cfg);
  } catch(Error& err) {
    errstr = "transportCreate error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  uv_mutex_lock(&async_mutex);
  uv_ref(reinterpret_cast<uv_handle_t*>(&dumb));
  uv_mutex_unlock(&async_mutex);

  SIPSTERTransport* trans = new SIPSTERTransport();

  trans->Wrap(info.This());
  trans->Ref();

  trans->transId = tid;
  trans->enabled = true;
  trans->emit = new Nan::Callback(
    Local<Function>::Cast(trans->handle()->Get(Nan::New(emit_symbol)))
  );

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(SIPSTERTransport::GetInfo) {
  Nan::HandleScope scope;
  SIPSTERTransport* trans =
    Nan::ObjectWrap::Unwrap<SIPSTERTransport>(info.This());

  TransportInfo ti;

  try {
    ti = ep->transportGetInfo(trans->transId);
  } catch(Error& err) {
    string errstr = "transportGetInfo error: " + err.info();
    return Nan::ThrowError(errstr.c_str());
  }

  Local<Object> info_obj = Nan::New<Object>();
  Nan::Set(info_obj,
           Nan::New("type").ToLocalChecked(),
           Nan::New(ti.typeName.c_str()).ToLocalChecked());
  Nan::Set(info_obj,
           Nan::New("info").ToLocalChecked(),
           Nan::New(ti.info.c_str()).ToLocalChecked());
  Nan::Set(info_obj,
           Nan::New("flags").ToLocalChecked(),
           Nan::New(ti.flags));
  Nan::Set(info_obj,
           Nan::New("localAddress").ToLocalChecked(),
           Nan::New(ti.localAddress.c_str()).ToLocalChecked());
  Nan::Set(info_obj,
           Nan::New("localName").ToLocalChecked(),
           Nan::New(ti.localName.c_str()).ToLocalChecked());
  Nan::Set(info_obj,
           Nan::New("usageCount").ToLocalChecked(),
           Nan::New(ti.usageCount));

  info.GetReturnValue().Set(info_obj);
}

NAN_METHOD(SIPSTERTransport::Enable) {
  Nan::HandleScope scope;
  SIPSTERTransport* trans =
    Nan::ObjectWrap::Unwrap<SIPSTERTransport>(info.This());

  if (!trans->enabled) {
    try {
      ep->transportSetEnable(trans->transId, true);
    } catch(Error& err) {
      string errstr = "transportSetEnable error: " + err.info();
      return Nan::ThrowError(errstr.c_str());
    }
    trans->enabled = true;
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERTransport::Disable) {
  Nan::HandleScope scope;
  SIPSTERTransport* trans =
    Nan::ObjectWrap::Unwrap<SIPSTERTransport>(info.This());

  if (trans->enabled) {
    try {
      ep->transportSetEnable(trans->transId, false);
    } catch(Error& err) {
      string errstr = "transportSetEnable error: " + err.info();
      return Nan::ThrowError(errstr.c_str());
    }
    trans->enabled = false;
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERTransport::DoRef) {
  Nan::HandleScope scope;
  SIPSTERTransport* trans =
    Nan::ObjectWrap::Unwrap<SIPSTERTransport>(info.This());

  trans->Ref();

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(SIPSTERTransport::DoUnref) {
  Nan::HandleScope scope;
  SIPSTERTransport* trans =
    Nan::ObjectWrap::Unwrap<SIPSTERTransport>(info.This());

  trans->Unref();

  info.GetReturnValue().SetUndefined();
}

NAN_GETTER(SIPSTERTransport::EnabledGetter) {
  SIPSTERTransport* trans =
    Nan::ObjectWrap::Unwrap<SIPSTERTransport>(info.This());

  info.GetReturnValue().Set(Nan::New(trans->enabled));
}

void SIPSTERTransport::Initialize(Handle<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  Local<String> name = Nan::New("Transport").ToLocalChecked();

  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->SetClassName(name);

  Nan::SetPrototypeMethod(tpl, "getInfo", GetInfo);
  Nan::SetPrototypeMethod(tpl, "enable", Enable);
  Nan::SetPrototypeMethod(tpl, "disable", Disable);
  Nan::SetPrototypeMethod(tpl, "ref", DoRef);
  Nan::SetPrototypeMethod(tpl, "unref", DoUnref);

  Nan::SetAccessor(tpl->PrototypeTemplate(),
                   Nan::New("enabled").ToLocalChecked(),
                   EnabledGetter);

  Nan::Set(target, name, tpl->GetFunction());

  SIPSTERTransport_constructor.Reset(tpl);
}
