#ifndef _OCULUS_BINDINGS_H_
#define _OCULUS_BINDINGS_H_

#include <v8.h>
#include <nan/nan.h>

using namespace v8;

NAN_METHOD(Oculus_Init);

Local<Object> makeOculusVr();

#endif
