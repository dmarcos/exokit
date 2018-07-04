#include <oculus-bindings.h>

#include <nan/nan.h>

#include <node.h>

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"
#include <v8.h>

NAN_METHOD(Oculus_Init)
{
  if (info.Length() != 1)
  {
    Nan::ThrowError("Wrong number of arguments.");
    return;
  }

  if (!info[0]->IsNumber())
  {
    Nan::ThrowTypeError("Argument[0] must be a number (EVRApplicationType).");
    return;
  }

  return;
}

Local<Object> makeOculusVr() {
  v8::EscapableHandleScope scope(Isolate::GetCurrent());
  
  Local<Object> exports = Object::New(Isolate::GetCurrent());

  exports->Set(Nan::New("Oculus_Init").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(Oculus_Init)->GetFunction());

  return scope.Escape(exports);
}