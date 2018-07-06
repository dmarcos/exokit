#include <oculus-bindings.h>

#include <nan/nan.h>

#include <node.h>

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"
#include <v8.h>
  #include <oculusmanager.h>

NAN_METHOD(Oculus_Init)
{
  // bool returnValue;
  // ovrSession session;
  // ovrGraphicsLuid luid;
  // ovrResult result = ovr_Create(&session, &luid);
  
  // ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
  // result = ovr_Initialize(&initParams);
  
  // ovrHmdDesc desc = ovr_GetHmdDesc(session);
  // if (desc.Type == ovrHmd_None) { 
  //   returnValue = false;
  // } else {
  //   returnValue = true;
  // }

  // info.GetReturnValue().Set(Nan::New<Boolean>(returnValue));

  // Wrap the resulting system in the correct wrapper and return it.
  ovrSession* session = (ovrSession*) malloc(sizeof (ovrSession));
  auto result = OculusManager::NewInstance(session);
  info.GetReturnValue().Set(result);
}

Local<Object> makeOculusVr() {
  v8::EscapableHandleScope scope(Isolate::GetCurrent());
  
  Local<Object> exports = Object::New(Isolate::GetCurrent());

  exports->Set(Nan::New("Oculus_Init").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(Oculus_Init)->GetFunction());

  return scope.Escape(exports);
}