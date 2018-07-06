#include <oculusmanager.h>

#include <array>
#include <node.h>
#include <openvr.h>

using namespace v8;

//=============================================================================
NAN_MODULE_INIT(OculusManager::Init)
{
  // Create a function template that is called in JS to create this wrapper.
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);

  // Declare human-readable name for this wrapper.
  tpl->SetClassName(Nan::New("OculusManager").ToLocalChecked());

  // Declare the stored number of fields (just the wrapped C++ object).
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Set a static constructor function to reference the `New` function template.
  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
}

//=============================================================================
Local<Object> OculusManager::NewInstance(ovrSession* value)
{
  Nan::EscapableHandleScope scope;
  Local<Function> cons = Nan::New(constructor());
  Local<Value> argv[1] = { Nan::New<External>(value) };
  return scope.Escape(Nan::NewInstance(cons, 1, argv).ToLocalChecked());
}

//=============================================================================
OculusManager::OculusManager(ovrSession* self)
: self_(self)
{
  // Do nothing.
}

//=============================================================================
NAN_METHOD(OculusManager::New)
{
  if (!info.IsConstructCall())
  {
    Nan::ThrowError("Use the `new` keyword when creating a new instance.");
    return;
  }

  if (info.Length() != 1 || !info[0]->IsExternal())
  {
    Nan::ThrowTypeError("Argument[0] must be an `IVRSystem*`.");
    return;
  }

  auto wrapped_instance = static_cast<ovrSession*>(
    Local<External>::Cast(info[0])->Value());
  OculusManager *obj = new OculusManager(wrapped_instance);
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}