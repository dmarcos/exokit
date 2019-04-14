#include <canvascontext/include/canvas-context.h>

#include "gl/GrGLInterface.h"
#include "GrBackendSurface.h"
#include "GrContext.h"

#include <windowsystem.h>
#include "gl/GrGLAssembleInterface.h"

#include <exout>

using namespace v8;
using namespace node;

#ifdef __APPLE__
#define MAC_FLUSH() glFlush()
#else
#define MAC_FLUSH() 
#endif

template<NAN_METHOD(F)>
NAN_METHOD(ctxCallWrap) {
  Local<Object> ctxObj = info.This();
  CanvasRenderingContext2D *ctx = ObjectWrap::Unwrap<CanvasRenderingContext2D>(ctxObj);
  if (ctx->live) {
    windowsystem::SetCurrentWindowContext(ctx->windowHandle);

    F(info);
  } else {
    info.GetReturnValue().Set(JS_STR(""));
  }
}

bool isImageValue(Local<Value> arg) {
  if (JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("HTMLCanvasElement"))) {
    Local<Value> otherContextObj = JS_OBJ(arg)->Get(JS_STR("_context"));
    return otherContextObj->IsObject() && JS_OBJ(JS_OBJ(otherContextObj)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("CanvasRenderingContext2D"));
  } else {
    return arg->IsObject() && (
      JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("CanvasRenderingContext2D")) ||
      JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("HTMLImageElement")) ||
      JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("ImageData")) ||
      JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("ImageBitmap"))
    );
  }
}

/* void flipCanvasY(SkCanvas *canvas, float height) {
  canvas->translate(0, canvas->imageInfo().height());
  canvas->scale(1.0, -1.0);
} */

void rgbaToString(std::string& str, uint32_t c) {
  uint32_t a = 0xFF & ((uint32_t)c >> (8 * 3));
  uint32_t r = 0xFF & ((uint32_t)c >> (8 * 2));
  uint32_t g = 0xFF & ((uint32_t)c >> (8 * 1));
  uint32_t b = 0xFF & ((uint32_t)c >> (8 * 0));
  if (a == 0xFF) {
    char out[4096];
    sprintf(out, "#%02x%02x%02x", r, g, b);
    str = out;
  } else {
    double alpha = a / 255.0;
    char outAlpha[4096];
    sprintf(outAlpha, "%.15f", alpha);
    char* end = outAlpha + strlen(outAlpha);
    while (end > outAlpha && *(end - 1) == '0') {
      end--;
      if (end > outAlpha && *(end - 1) != '.') {
        *end = '\0';
      }
    } 
    char out[4096];
    sprintf(out, "rgba(%d, %d, %d, %s)", r, g, b, (strcmp(outAlpha, "0.0") == 0) ? "0" : outAlpha);
    str = out;
  }
}

void ConfigureLooper(CanvasRenderingContext2D *context) {
    canvas::web_color c = canvas::web_color::from_string(context->shadowColor.c_str());
    uint32_t rgba = ((uint32_t)c.a << (8 * 3)) | ((uint32_t)c.r << (8 * 2)) | ((uint32_t)c.g << (8 * 1)) | ((uint32_t)c.b << (8 * 0));
    rgbaToString(context->shadowColor, rgba);
    if (rgba == 0x00000000) {
      context->fillPaint.setDrawLooper(nullptr);
    } else {
      context->fillPaint.setDrawLooper(SkBlurDrawLooper::Make(rgba, context->shadowBlur, context->shadowOffsetX, context->shadowOffsetY));
    }
}

Local<Object> CanvasRenderingContext2D::Initialize(Isolate *isolate, Local<Value> imageDataCons, Local<Value> canvasGradientCons, Local<Value> canvasPatternCons) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("CanvasRenderingContext2D"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();

  Nan::SetMethod(proto, "scale", ctxCallWrap<Scale>);
  Nan::SetMethod(proto, "rotate", ctxCallWrap<Rotate>);
  Nan::SetMethod(proto, "translate", ctxCallWrap<Translate>);
  Nan::SetMethod(proto, "transform", ctxCallWrap<Transform>);
  Nan::SetMethod(proto, "setTransform", ctxCallWrap<SetTransform>);
  Nan::SetMethod(proto, "resetTransform", ctxCallWrap<ResetTransform>);
  Nan::SetMethod(proto, "measureText", ctxCallWrap<MeasureText>);
  Nan::SetMethod(proto, "beginPath", ctxCallWrap<BeginPath>);
  Nan::SetMethod(proto, "closePath", ctxCallWrap<ClosePath>);
  Nan::SetMethod(proto, "clip", ctxCallWrap<Clip>);
  Nan::SetMethod(proto, "stroke", ctxCallWrap<Stroke>);
  Nan::SetMethod(proto, "fill", ctxCallWrap<Fill>);
  Nan::SetMethod(proto, "moveTo", ctxCallWrap<MoveTo>);
  Nan::SetMethod(proto, "lineTo", ctxCallWrap<LineTo>);
  Nan::SetMethod(proto, "arc", ctxCallWrap<Arc>);
  Nan::SetMethod(proto, "arcTo", ctxCallWrap<ArcTo>);
  Nan::SetMethod(proto, "quadraticCurveTo", ctxCallWrap<QuadraticCurveTo>);
  Nan::SetMethod(proto, "bezierCurveTo", ctxCallWrap<BezierCurveTo>);
  Nan::SetMethod(proto, "rect", ctxCallWrap<Rect>);
  Nan::SetMethod(proto, "fillRect", ctxCallWrap<FillRect>);
  Nan::SetMethod(proto, "strokeRect", ctxCallWrap<StrokeRect>);
  Nan::SetMethod(proto, "clearRect", ctxCallWrap<ClearRect>);
  Nan::SetMethod(proto, "fillText", ctxCallWrap<FillText>);
  Nan::SetMethod(proto, "strokeText", ctxCallWrap<StrokeText>);
  Nan::SetMethod(proto, "createLinearGradient", ctxCallWrap<CreateLinearGradient>);
  Nan::SetMethod(proto, "createRadialGradient", ctxCallWrap<CreateRadialGradient>);
  Nan::SetMethod(proto, "createPattern", ctxCallWrap<CreatePattern>);
  Nan::SetMethod(proto, "resize", ctxCallWrap<Resize>);
  Nan::SetMethod(proto, "drawImage", ctxCallWrap<DrawImage>);
  Nan::SetMethod(proto, "save", ctxCallWrap<Save>);
  Nan::SetMethod(proto, "restore", ctxCallWrap<Restore>);
  Nan::SetMethod(proto, "toArrayBuffer", ctxCallWrap<ToArrayBuffer>);
  Nan::SetMethod(proto, "createImageData", ctxCallWrap<CreateImageData>);
  Nan::SetMethod(proto, "getImageData", ctxCallWrap<GetImageData>);
  Nan::SetMethod(proto, "putImageData", ctxCallWrap<PutImageData>);
  
  Nan::SetMethod(proto, "setTexture", ctxCallWrap<SetTexture>);

  Nan::SetMethod(proto, "destroy", Destroy);
  Nan::SetMethod(proto, "getWindowHandle", GetWindowHandle);
  Nan::SetMethod(proto, "setWindowHandle", SetWindowHandle);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();
  ctorFn->Set(JS_STR("ImageData"), imageDataCons);
  ctorFn->Set(JS_STR("CanvasGradient"), canvasGradientCons);
  ctorFn->Set(JS_STR("CanvasPattern"), canvasPatternCons);

  return scope.Escape(ctorFn);
}

unsigned int CanvasRenderingContext2D::GetWidth() {
  return surface->width();
}

unsigned int CanvasRenderingContext2D::GetHeight() {
  return surface->height();
}

unsigned int CanvasRenderingContext2D::GetNumChannels() {
  return 4;
}

void CanvasRenderingContext2D::Scale(float x, float y) {
  surface->getCanvas()->scale(x, y);
}

void CanvasRenderingContext2D::Rotate(float angle) {
  surface->getCanvas()->rotate(angle);
}

void CanvasRenderingContext2D::Translate(float x, float y) {
  surface->getCanvas()->translate(x, y);
}

void CanvasRenderingContext2D::Transform(float a, float b, float c, float d, float e, float f) {
  SkScalar affine[] = {a, b, c, d, e, f};
  SkMatrix m;
  m.setAffine(affine);
  surface->getCanvas()->setMatrix(m);
}

void CanvasRenderingContext2D::SetTransform(float a, float b, float c, float d, float e, float f) {
  SkScalar affine[] = {a, b, c, d, e, f};
  SkMatrix m;
  m.setAffine(affine);
  surface->getCanvas()->setMatrix(m);
}

void CanvasRenderingContext2D::ResetTransform() {
  surface->getCanvas()->resetMatrix();
}

float CanvasRenderingContext2D::MeasureText(const std::string &text) {
  SkRect bounds;
  strokePaint.measureText(text.c_str(), text.length(), &bounds);
  return bounds.width();
}

void CanvasRenderingContext2D::BeginPath() {
  path.reset();
}

void CanvasRenderingContext2D::ClosePath() {
  path.close();
}

void CanvasRenderingContext2D::Clip() {
  surface->getCanvas()->clipPath(path);
}

void CanvasRenderingContext2D::Stroke() {
  surface->getCanvas()->drawPath(path, strokePaint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::Stroke(const Path2D &path) {
  surface->getCanvas()->drawPath(path.path, strokePaint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::Fill() {
  surface->getCanvas()->drawPath(path, fillPaint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::Fill(const Path2D &path) {
  surface->getCanvas()->drawPath(path.path, fillPaint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::MoveTo(float x, float y) {
  path.moveTo(x, y);
}

void CanvasRenderingContext2D::LineTo(float x, float y) {
  path.lineTo(x, y);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::Arc(float x, float y, float radius, float startAngle, float endAngle, float anticlockwise) {
  startAngle = startAngle / M_PI * 360;
  endAngle = endAngle / M_PI * 360;
  if (anticlockwise) {
    endAngle -= 360;
  }
  path.addArc(SkRect::MakeLTRB(x - radius, y - radius, x + radius, y + radius), startAngle, endAngle);
}

void CanvasRenderingContext2D::ArcTo(float x1, float y1, float x2, float y2, float radius) {
  path.arcTo(x1, y1, x2 - x1, y2 - y1, radius);
}

void CanvasRenderingContext2D::QuadraticCurveTo(float cpx, float cpy, float x, float y) {
  path.quadTo(cpx, cpy, x, y);
}

void CanvasRenderingContext2D::BezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) {
  path.cubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
}

void CanvasRenderingContext2D::Rect(float x, float y, float w, float h) {
  path.addRect(SkRect::MakeXYWH(x, y, w, h));
}

void CanvasRenderingContext2D::FillRect(float x, float y, float w, float h) {
  SkPath path;
  path.addRect(SkRect::MakeXYWH(x, y, w, h));
  surface->getCanvas()->drawPath(path, fillPaint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::StrokeRect(float x, float y, float w, float h) {
  SkPath path;
  path.addRect(SkRect::MakeXYWH(x, y, w, h));
  surface->getCanvas()->drawPath(path, strokePaint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::ClearRect(float x, float y, float w, float h) {
  SkPath path;
  path.addRect(SkRect::MakeXYWH(x, y, w, h));
  surface->getCanvas()->drawPath(path, clearPaint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

float getFontBaseline(const SkPaint &paint, const TextBaseline &textBaseline, float lineHeight) {
  SkPaint::FontMetrics fontMetrics;
  paint.getFontMetrics(&fontMetrics);

  // If the font is so tiny that the lroundf operations result in two
  // different types of text baselines to return the same baseline, use
  // floating point metrics (crbug.com/338908).
  // If you changed the heuristic here, for consistency please also change it
  // in SimpleFontData::platformInit().
  // TODO(fserb): revisit this.
  switch (textBaseline) {
    case TextBaseline::TOP:
      return fontMetrics.fAscent;
    case TextBaseline::HANGING:
      // According to
      // http://wiki.apache.org/xmlgraphics-fop/LineLayout/AlignmentHandling
      // "FOP (Formatting Objects Processor) puts the hanging baseline at 80% of
      // the ascender height"
      return fontMetrics.fAscent * 80.0f / 100.0f;
    case TextBaseline::BOTTOM:
    case TextBaseline::IDEOGRAPHIC:
      return -fontMetrics.fDescent;
    case TextBaseline::MIDDLE:
      return -fontMetrics.fDescent + fontMetrics.fCapHeight * lineHeight / 2.0f;
    case TextBaseline::ALPHABETIC:
    default:
      // Do nothing.
      break;
  }
  return 0;
}

void CanvasRenderingContext2D::FillText(const std::string &text, float x, float y) {
  // surface->getCanvas()->drawText(text.c_str(), text.length(), x, y - getFontBaseline(fillPaint, textBaseline, lineHeight), fillPaint);
  surface->getCanvas()->drawText(text.c_str(), text.length(), x, y, fillPaint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::StrokeText(const std::string &text, float x, float y) {
  // surface->getCanvas()->drawText(text.c_str(), text.length(), x, y - getFontBaseline(strokePaint, textBaseline, lineHeight), strokePaint);
  surface->getCanvas()->drawText(text.c_str(), text.length(), x, y, strokePaint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::Resize(unsigned int w, unsigned int h) {
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  
  grContext->resetContext();
  
  GrGLTextureInfo glTexInfo;
  glTexInfo.fID = tex;
  glTexInfo.fTarget = GL_TEXTURE_2D;
  glTexInfo.fFormat = GL_RGBA8;
  
  GrBackendTexture backendTex(w, h, GrMipMapped::kNo, glTexInfo);
  
  surface = SkSurface::MakeFromBackendTexture(grContext.get(), backendTex, kBottomLeft_GrSurfaceOrigin, 0, SkColorType::kRGBA_8888_SkColorType, nullptr, nullptr);
  if (!surface) {
    exerr << "Failed to resize CanvasRenderingContext2D surface" << std::endl;
    abort();
  }
}

void CanvasRenderingContext2D::DrawImage(const SkImage *image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh) {
  SkPaint paint;
  paint.setColor(0xFFFFFFFF);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setBlendMode(SkBlendMode::kSrcOver);
  surface->getCanvas()->drawImageRect(image, SkRect::MakeXYWH(sx, sy, sw, sh), SkRect::MakeXYWH(dx, dy, dw, dh), &paint);
  surface->getCanvas()->flush();
  MAC_FLUSH();
}

void CanvasRenderingContext2D::Save() {
  surface->getCanvas()->save();
}

void CanvasRenderingContext2D::Restore() {
  surface->getCanvas()->restore();
}

NAN_METHOD(CanvasRenderingContext2D::New) {
  CanvasRenderingContext2D *context = new CanvasRenderingContext2D();

  Local<Object> ctxObj = info.This();
  context->Wrap(ctxObj);

  Nan::SetAccessor(ctxObj, JS_STR("width"), WidthGetter);
  Nan::SetAccessor(ctxObj, JS_STR("height"), HeightGetter);
  Nan::SetAccessor(ctxObj, JS_STR("texture"), TextureGetter);
  Nan::SetAccessor(ctxObj, JS_STR("lineWidth"), LineWidthGetter, LineWidthSetter);
  Nan::SetAccessor(ctxObj, JS_STR("strokeStyle"), StrokeStyleGetter, StrokeStyleSetter);
  Nan::SetAccessor(ctxObj, JS_STR("fillStyle"), FillStyleGetter, FillStyleSetter);
  Nan::SetAccessor(ctxObj, JS_STR("shadowColor"), ShadowColorGetter, ShadowColorSetter);
  Nan::SetAccessor(ctxObj, JS_STR("shadowBlur"), ShadowBlurGetter, ShadowBlurSetter);
  Nan::SetAccessor(ctxObj, JS_STR("shadowOffsetX"), ShadowOffsetXGetter, ShadowOffsetXSetter);
  Nan::SetAccessor(ctxObj, JS_STR("shadowOffsetY"), ShadowOffsetYGetter, ShadowOffsetYSetter);
  Nan::SetAccessor(ctxObj, JS_STR("font"), FontGetter, FontSetter);
  Nan::SetAccessor(ctxObj, JS_STR("fontFamily"), FontFamilyGetter, FontFamilySetter);
  Nan::SetAccessor(ctxObj, JS_STR("fontSize"), FontSizeGetter, FontSizeSetter);
  Nan::SetAccessor(ctxObj, JS_STR("fontVariant"), FontVariantGetter, FontVariantSetter);
  Nan::SetAccessor(ctxObj, JS_STR("fontWeight"), FontWeightGetter, FontWeightSetter);
  Nan::SetAccessor(ctxObj, JS_STR("lineHeight"), LineHeightGetter, LineHeightSetter);
  Nan::SetAccessor(ctxObj, JS_STR("fontStyle"), FontStyleGetter, FontStyleSetter);
  Nan::SetAccessor(ctxObj, JS_STR("textAlign"), TextAlignGetter, TextAlignSetter);
  Nan::SetAccessor(ctxObj, JS_STR("textBaseline"), TextBaselineGetter, TextBaselineSetter);
  Nan::SetAccessor(ctxObj, JS_STR("direction"), DirectionGetter, DirectionSetter);

  ctxObj->Set(JS_STR("font"), JS_STR("10px sans-serif"));

  info.GetReturnValue().Set(ctxObj);
}

NAN_GETTER(CanvasRenderingContext2D::WidthGetter) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  info.GetReturnValue().Set(JS_INT(context->GetWidth()));
}

NAN_GETTER(CanvasRenderingContext2D::HeightGetter) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  info.GetReturnValue().Set(JS_INT(context->GetHeight()));
}

NAN_GETTER(CanvasRenderingContext2D::TextureGetter) {
  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  if (context->tex != 0) {
    Local<Object> texObj = Nan::New<Object>();
    texObj->Set(JS_STR("id"), JS_INT(context->tex));
    info.GetReturnValue().Set(texObj);
  } else {
    info.GetReturnValue().Set(Nan::Null());
  }
}

NAN_GETTER(CanvasRenderingContext2D::LineWidthGetter) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  info.GetReturnValue().Set(JS_FLOAT(context->strokePaint.getStrokeWidth()));
}

NAN_SETTER(CanvasRenderingContext2D::LineWidthSetter) {
  // Nan::HandleScope scope;

  if (value->IsNumber()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    float lineWidth = TO_FLOAT(value);

    context->strokePaint.setStrokeWidth(lineWidth);
    context->fillPaint.setStrokeWidth(lineWidth);
  } else {
    Nan::ThrowError("lineWidth: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::StrokeStyleGetter) {
  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  info.GetReturnValue().Set(Nan::New(context->jsStrokeStyle));
}

NAN_SETTER(CanvasRenderingContext2D::StrokeStyleSetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    Nan::Utf8String text(value);
    std::string strokeStyle(*text, text.length());

    canvas::web_color webColor = canvas::web_color::from_string(strokeStyle.c_str());
    uint32_t rgba = ((uint32_t)webColor.a << (8 * 3)) | ((uint32_t)webColor.r << (8 * 2)) | ((uint32_t)webColor.g << (8 * 1)) | ((uint32_t)webColor.b << (8 * 0));
    context->strokePaint.setColor(rgba);
    context->strokePaint.setShader(nullptr);
    rgbaToString(strokeStyle, rgba);
    context->jsStrokeStyle.Reset(JS_STR(strokeStyle.c_str()));
  } else if (value->IsObject() && JS_OBJ(JS_OBJ(value)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("CanvasGradient"))) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    CanvasGradient *canvasGradient = ObjectWrap::Unwrap<CanvasGradient>(Local<Object>::Cast(value));
    context->strokePaint.setShader(canvasGradient->getShader());
    context->jsStrokeStyle.Reset(value);
  } else if (value->IsObject() && JS_OBJ(JS_OBJ(value)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("CanvasPattern"))) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    CanvasPattern *canvasPattern = ObjectWrap::Unwrap<CanvasPattern>(Local<Object>::Cast(value));
    context->strokePaint.setShader(canvasPattern->getShader());
    context->jsStrokeStyle.Reset(value);
  } else {
    Nan::ThrowError("strokeStyle: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::ShadowColorGetter) {
  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  info.GetReturnValue().Set(JS_STR(context->shadowColor.c_str()));
}

NAN_SETTER(CanvasRenderingContext2D::ShadowColorSetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    Nan::Utf8String text(value);
    std::string shadowColor(*text, text.length());

    if (shadowColor != context->shadowColor) {
      context->shadowColor = shadowColor;
      ConfigureLooper(context);
    }
  } else {
     Nan::ThrowError("shadowColor: invalid arguments");
  }
}


NAN_GETTER(CanvasRenderingContext2D::ShadowBlurGetter) {
  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  info.GetReturnValue().Set(JS_NUM(context->shadowBlur));
}

NAN_SETTER(CanvasRenderingContext2D::ShadowBlurSetter) {
  // Nan::HandleScope scope;

  if (value->IsNumber()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
    if (context->shadowBlur != TO_FLOAT(value)) {
      context->shadowBlur = TO_FLOAT(value);
      ConfigureLooper(context);
    }
  } else {
     Nan::ThrowError("shadowBlur: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::ShadowOffsetXGetter) {
  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  info.GetReturnValue().Set(JS_NUM(context->shadowOffsetX));
}

NAN_SETTER(CanvasRenderingContext2D::ShadowOffsetXSetter) {
  // Nan::HandleScope scope;

  if (value->IsNumber()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
    if (context->shadowOffsetX != TO_FLOAT(value)) {
      context->shadowOffsetX = TO_FLOAT(value);
      ConfigureLooper(context);
    }
  } else {
     Nan::ThrowError("shadowOffsetX: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::ShadowOffsetYGetter) {
  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  info.GetReturnValue().Set(JS_NUM(context->shadowOffsetY));
}

NAN_SETTER(CanvasRenderingContext2D::ShadowOffsetYSetter) {
  // Nan::HandleScope scope;

  if (value->IsNumber()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
    if (context->shadowOffsetY != TO_FLOAT(value)) {
      context->shadowOffsetY = TO_FLOAT(value);
      ConfigureLooper(context);
    }
  } else {
     Nan::ThrowError("shadowOffsetY: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::FillStyleGetter) {
  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  info.GetReturnValue().Set(Nan::New(context->jsFillStyle));
}

NAN_SETTER(CanvasRenderingContext2D::FillStyleSetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    Nan::Utf8String text(value);
    std::string fillStyle(*text, text.length());

    canvas::web_color webColor = canvas::web_color::from_string(fillStyle.c_str());
    uint32_t rgba = ((uint32_t)webColor.a << (8 * 3)) | ((uint32_t)webColor.r << (8 * 2)) | ((uint32_t)webColor.g << (8 * 1)) | ((uint32_t)webColor.b << (8 * 0));
    context->fillPaint.setColor(rgba);
    context->fillPaint.setShader(nullptr);
    rgbaToString(fillStyle, rgba);
    context->jsFillStyle.Reset(JS_STR(fillStyle.c_str()));
  } else if (value->IsObject() && JS_OBJ(JS_OBJ(value)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("CanvasGradient"))) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    CanvasGradient *canvasGradient = ObjectWrap::Unwrap<CanvasGradient>(Local<Object>::Cast(value));
    context->fillPaint.setShader(canvasGradient->getShader());
    context->jsFillStyle.Reset(value);
  } else if (value->IsObject() && JS_OBJ(JS_OBJ(value)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("CanvasPattern"))) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    CanvasPattern *canvasPattern = ObjectWrap::Unwrap<CanvasPattern>(Local<Object>::Cast(value));
    context->fillPaint.setShader(canvasPattern->getShader());
    context->jsFillStyle.Reset(value);
  } else {
     Nan::ThrowError("fillStyle: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::FontGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::FontSetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    Local<Object> contextObj = info.This();

    Nan::Utf8String text(value);
    std::string font(*text, text.length());

    canvas::FontDeclaration declaration = canvas::parse_short_font(font);

    contextObj->Set(JS_STR("fontFamily"), JS_STR(declaration.fontFamily));
    contextObj->Set(JS_STR("fontStyle"), JS_STR(declaration.fontStyle));
    contextObj->Set(JS_STR("fontVariant"), JS_STR(declaration.fontVariant));
    contextObj->Set(JS_STR("fontWeight"), JS_STR(declaration.fontWeight));
    contextObj->Set(JS_STR("fontSize"), JS_STR(declaration.fontSize));
    contextObj->Set(JS_STR("lineHeight"), JS_STR(declaration.lineHeight));
  } else {
    Nan::ThrowError("font: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::FontFamilyGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::FontFamilySetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    Nan::Utf8String text(value);
    std::string fontFamily(*text, text.length());

    SkTypeface *typeface = context->strokePaint.getTypeface();
    SkFontStyle fontStyle = typeface ? typeface->fontStyle() : SkFontStyle();
    context->strokePaint.setTypeface(SkTypeface::MakeFromName(fontFamily.c_str(), fontStyle));
    context->fillPaint.setTypeface(SkTypeface::MakeFromName(fontFamily.c_str(), fontStyle));
  } else {
    Nan::ThrowError("fontFamily: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::FontSizeGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::FontSizeSetter) {
  // Nan::HandleScope scope;

  if (value->IsNumber() || value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    double fontSize = TO_DOUBLE(value);

    context->strokePaint.setTextSize(fontSize);
    context->fillPaint.setTextSize(fontSize);
  } else {
    Nan::ThrowError("fontSize: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::FontWeightGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::FontWeightSetter) {
  // Nan::HandleScope scope;

  if (value->IsNumber() || value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    Nan::Utf8String text(value);
    std::string fontStyleString(*text, text.length());

    unsigned int fontWeight;
    if (fontStyleString == "normal") {
      fontWeight = 400;
    } else if (fontStyleString == "bold") {
      fontWeight = 700;
    } else {
      fontWeight = value->IsNumber() ? TO_UINT32(value) : 400;
    }

    SkTypeface *typeface = context->strokePaint.getTypeface();
    SkFontStyle oldFontStyle = typeface ? typeface->fontStyle() : SkFontStyle();
    SkFontStyle fontStyle(fontWeight, oldFontStyle.width(), oldFontStyle.slant());

    const char *familyNameString;
    if (typeface) {
      SkString familyName;
      typeface->getFamilyName(&familyName);
      familyNameString = familyName.c_str();
    } else {
      familyNameString = nullptr;
    }
    context->strokePaint.setTypeface(SkTypeface::MakeFromName(familyNameString, fontStyle));
    context->fillPaint.setTypeface(SkTypeface::MakeFromName(familyNameString, fontStyle));
  } else {
    Nan::ThrowError("fontWeight: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::LineHeightGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::LineHeightSetter) {
  // Nan::HandleScope scope;

  if (value->IsNumber() || value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

    double lineHeight = TO_DOUBLE(value);

    context->lineHeight = lineHeight;
  } else {
    Nan::ThrowError("lineHeight: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::FontStyleGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::FontStyleSetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
    Nan::Utf8String text(value);
    std::string fontStyleString(*text, text.length());

    SkFontStyle::Slant slant;
    if (fontStyleString == "normal") {
      slant = SkFontStyle::Slant::kUpright_Slant;
    } else if (fontStyleString == "italic") {
      slant = SkFontStyle::Slant::kItalic_Slant;
    } else if (fontStyleString == "oblique") {
      slant = SkFontStyle::Slant::kOblique_Slant;
    } else {
      slant = SkFontStyle::Slant::kUpright_Slant;
    }

    SkTypeface *typeface = context->strokePaint.getTypeface();
    SkFontStyle oldFontStyle = typeface ? typeface->fontStyle() : SkFontStyle();
    SkFontStyle fontStyle(oldFontStyle.weight(), oldFontStyle.width(), slant);

    const char *familyNameString;
    if (typeface) {
      SkString familyName;
      typeface->getFamilyName(&familyName);
      familyNameString = familyName.c_str();
    } else {
      familyNameString = nullptr;
    }
    context->strokePaint.setTypeface(SkTypeface::MakeFromName(familyNameString, fontStyle));
    context->fillPaint.setTypeface(SkTypeface::MakeFromName(familyNameString, fontStyle));
  } else {
    Nan::ThrowError("fontStyle: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::FontVariantGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::FontVariantSetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    // TODO
  } else {
    Nan::ThrowError("fontStyle: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::TextAlignGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::TextAlignSetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
    Nan::Utf8String text(value);
    std::string textAlignString(*text, text.length());

    if (textAlignString == "left") {
      context->strokePaint.setTextAlign(SkPaint::kLeft_Align);
      context->fillPaint.setTextAlign(SkPaint::kLeft_Align);
    } else if (textAlignString == "right") {
      context->strokePaint.setTextAlign(SkPaint::kRight_Align);
      context->fillPaint.setTextAlign(SkPaint::kRight_Align);
    } else if (textAlignString == "center") {
      context->strokePaint.setTextAlign(SkPaint::kCenter_Align);
      context->fillPaint.setTextAlign(SkPaint::kCenter_Align);
    } else if (textAlignString == "start") {
      if (context->direction == Direction::LEFT_TO_RIGHT) {
        context->strokePaint.setTextAlign(SkPaint::kLeft_Align);
        context->fillPaint.setTextAlign(SkPaint::kLeft_Align);
      } else {
        context->strokePaint.setTextAlign(SkPaint::kRight_Align);
        context->fillPaint.setTextAlign(SkPaint::kRight_Align);
      }
    } else if (textAlignString == "end") {
      if (context->direction == Direction::LEFT_TO_RIGHT) {
        context->strokePaint.setTextAlign(SkPaint::kRight_Align);
        context->fillPaint.setTextAlign(SkPaint::kRight_Align);
      } else {
        context->strokePaint.setTextAlign(SkPaint::kLeft_Align);
        context->fillPaint.setTextAlign(SkPaint::kLeft_Align);
      }
    } else {
      context->textAlign = "";
    }
  } else {
    Nan::ThrowError("textAlign: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::TextBaselineGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::TextBaselineSetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
    Nan::Utf8String text(value);
    std::string textBaselineString(*text, text.length());

    if (textBaselineString == "top") {
      context->textBaseline = TextBaseline::TOP;
    } else if (textBaselineString == "hanging") {
      context->textBaseline = TextBaseline::HANGING;
    } else if (textBaselineString == "alphabetic") {
      context->textBaseline = TextBaseline::ALPHABETIC;
    } else if (textBaselineString == "ideographic") {
      context->textBaseline = TextBaseline::IDEOGRAPHIC;
    } else if (textBaselineString == "bottom") {
      context->textBaseline = TextBaseline::BOTTOM;
    } else {
      context->textBaseline = TextBaseline::ALPHABETIC;
    }
  } else {
    Nan::ThrowError("textBaseline: invalid arguments");
  }
}

NAN_GETTER(CanvasRenderingContext2D::DirectionGetter) {
  // nothing
}

NAN_SETTER(CanvasRenderingContext2D::DirectionSetter) {
  // Nan::HandleScope scope;

  if (value->IsString()) {
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
    Nan::Utf8String text(value);
    std::string direction(*text, text.length());

    if (direction == "ltr") {
      context->direction = Direction::LEFT_TO_RIGHT;
    } else if (direction == "rtl") {
      context->direction = Direction::RIGHT_TO_LEFT;
    } else {
      context->direction = Direction::LEFT_TO_RIGHT;
    }
  } else {
    Nan::ThrowError("direction: invalid arguments");
  }
}

NAN_METHOD(CanvasRenderingContext2D::Scale) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x = TO_DOUBLE(info[0]);
  double y = TO_DOUBLE(info[1]);
  context->Scale(x, y);
}

NAN_METHOD(CanvasRenderingContext2D::Rotate) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double angle = TO_DOUBLE(info[0]);
  context->Rotate(angle);
}

NAN_METHOD(CanvasRenderingContext2D::Translate) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x = TO_DOUBLE(info[0]);
  double y = TO_DOUBLE(info[1]);
  context->Translate(x, y);
}

NAN_METHOD(CanvasRenderingContext2D::Transform) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double a = TO_DOUBLE(info[0]);
  double b = TO_DOUBLE(info[1]);
  double c = TO_DOUBLE(info[2]);
  double d = TO_DOUBLE(info[3]);
  double e = TO_DOUBLE(info[4]);
  double f = TO_DOUBLE(info[5]);
  context->Transform(a, b, c, d, e, f);
}

NAN_METHOD(CanvasRenderingContext2D::SetTransform) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double a = TO_DOUBLE(info[0]);
  double b = TO_DOUBLE(info[1]);
  double c = TO_DOUBLE(info[2]);
  double d = TO_DOUBLE(info[3]);
  double e = TO_DOUBLE(info[4]);
  double f = TO_DOUBLE(info[5]);
  context->SetTransform(a, b, c, d, e, f);
}

NAN_METHOD(CanvasRenderingContext2D::ResetTransform) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  context->ResetTransform();
}

NAN_METHOD(CanvasRenderingContext2D::MeasureText) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  Nan::Utf8String textUtf8(info[0]);
  std::string text(*textUtf8, textUtf8.length());

  Local<Object> result = Object::New(Isolate::GetCurrent());
  result->Set(JS_STR("width"), JS_FLOAT(context->MeasureText(text)));

  info.GetReturnValue().Set(result);
}

NAN_METHOD(CanvasRenderingContext2D::BeginPath) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  context->BeginPath();
}

NAN_METHOD(CanvasRenderingContext2D::ClosePath) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  context->ClosePath();
}

NAN_METHOD(CanvasRenderingContext2D::Clip) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  context->Clip();
}

NAN_METHOD(CanvasRenderingContext2D::Stroke) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  if (TO_BOOL(info[0]) && JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("Path2D"))) {
    Path2D *path2d = ObjectWrap::Unwrap<Path2D>(Local<Object>::Cast(info[0]));
    context->Stroke(*path2d);
  } else {
    context->Stroke();
  }
}

NAN_METHOD(CanvasRenderingContext2D::Fill) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

  if (TO_BOOL(info[0]) && JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("Path2D"))) {
    Path2D *path2d = ObjectWrap::Unwrap<Path2D>(Local<Object>::Cast(info[0]));
    context->Fill(*path2d);
  } else {
    context->Fill();
  }
}

NAN_METHOD(CanvasRenderingContext2D::MoveTo) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x = TO_DOUBLE(info[0]);
  double y = TO_DOUBLE(info[1]);

  context->MoveTo(x, y);
}

NAN_METHOD(CanvasRenderingContext2D::LineTo) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x = TO_DOUBLE(info[0]);
  double y = TO_DOUBLE(info[1]);

  context->LineTo(x, y);
}

NAN_METHOD(CanvasRenderingContext2D::Arc) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x = TO_DOUBLE(info[0]);
  double y = TO_DOUBLE(info[1]);
  double radius = TO_DOUBLE(info[2]);
  double startAngle = TO_DOUBLE(info[3]);
  double endAngle = TO_DOUBLE(info[4]);
  double anticlockwise = TO_DOUBLE(info[5]);

  context->Arc(x, y, radius, startAngle, endAngle, anticlockwise);
}

NAN_METHOD(CanvasRenderingContext2D::ArcTo) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x1 = TO_DOUBLE(info[0]);
  double y1 = TO_DOUBLE(info[1]);
  double x2 = TO_DOUBLE(info[2]);
  double y2 = TO_DOUBLE(info[3]);
  double radius = TO_DOUBLE(info[4]);

  context->ArcTo(x1, y1, x2, y2, radius);
}

NAN_METHOD(CanvasRenderingContext2D::QuadraticCurveTo) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x1 = TO_DOUBLE(info[0]);
  double y1 = TO_DOUBLE(info[1]);
  double x2 = TO_DOUBLE(info[2]);
  double y2 = TO_DOUBLE(info[3]);

  context->QuadraticCurveTo(x1, y1, x2, y2);
}

NAN_METHOD(CanvasRenderingContext2D::BezierCurveTo) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x1 = TO_DOUBLE(info[0]);
  double y1 = TO_DOUBLE(info[1]);
  double x2 = TO_DOUBLE(info[2]);
  double y2 = TO_DOUBLE(info[3]);
  double x = TO_DOUBLE(info[4]);
  double y = TO_DOUBLE(info[5]);

  context->BezierCurveTo(x1, y1, x2, y2, x, y);
}

NAN_METHOD(CanvasRenderingContext2D::Rect) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x = TO_DOUBLE(info[0]);
  double y = TO_DOUBLE(info[1]);
  double w = TO_DOUBLE(info[2]);
  double h = TO_DOUBLE(info[3]);

  context->Rect(x, y, w, h);

  // info.GetReturnValue().Set(JS_INT(image->GetHeight()));
}

NAN_METHOD(CanvasRenderingContext2D::FillRect) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x = TO_DOUBLE(info[0]);
  double y = TO_DOUBLE(info[1]);
  double w = TO_DOUBLE(info[2]);
  double h = TO_DOUBLE(info[3]);

  context->FillRect(x, y, w, h);

  // info.GetReturnValue().Set(JS_INT(image->GetHeight()));
}

NAN_METHOD(CanvasRenderingContext2D::StrokeRect) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x = TO_DOUBLE(info[0]);
  double y = TO_DOUBLE(info[1]);
  double w = TO_DOUBLE(info[2]);
  double h = TO_DOUBLE(info[3]);

  context->StrokeRect(x, y, w, h);

  // info.GetReturnValue().Set(JS_INT(image->GetHeight()));
}

NAN_METHOD(CanvasRenderingContext2D::ClearRect) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  double x = TO_DOUBLE(info[0]);
  double y = TO_DOUBLE(info[1]);
  double w = TO_DOUBLE(info[2]);
  double h = TO_DOUBLE(info[3]);

  context->ClearRect(x, y, w, h);

  // info.GetReturnValue().Set(JS_INT(image->GetHeight()));
}

NAN_METHOD(CanvasRenderingContext2D::FillText) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  Nan::Utf8String text(info[0]);
  std::string string(*text, text.length());
  double x = TO_DOUBLE(info[1]);
  double y = TO_DOUBLE(info[2]);

  context->FillText(string, x, y);

  // info.GetReturnValue().Set(JS_INT(image->GetHeight()));
}

NAN_METHOD(CanvasRenderingContext2D::StrokeText) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  Nan::Utf8String text(info[0]);
  std::string string(*text, text.length());
  double x = TO_DOUBLE(info[1]);
  double y = TO_DOUBLE(info[2]);

  context->StrokeText(string, x, y);

  // info.GetReturnValue().Set(JS_INT(image->GetHeight()));
}

NAN_METHOD(CanvasRenderingContext2D::CreateLinearGradient) {
  if (info[0]->IsNumber() && info[1]->IsNumber() && info[2]->IsNumber() && info[3]->IsNumber()) {
    Local<Object> contextObj = Local<Object>::Cast(info.This());
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(contextObj);

    Local<Function> canvasGradientCons = Local<Function>::Cast(JS_OBJ(contextObj->Get(JS_STR("constructor")))->Get(JS_STR("CanvasGradient")));
    Local<Value> argv[] = {
      info[0],
      info[1],
      info[2],
      info[3],
    };
    Local<Object> canvasGradientObj = canvasGradientCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();
    info.GetReturnValue().Set(canvasGradientObj);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_METHOD(CanvasRenderingContext2D::CreateRadialGradient) {
  if (info[0]->IsNumber() && info[1]->IsNumber() && info[2]->IsNumber() && info[3]->IsNumber() && info[4]->IsNumber() && info[5]->IsNumber()) {
    Local<Object> contextObj = Local<Object>::Cast(info.This());
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(contextObj);

    Local<Function> canvasGradientCons = Local<Function>::Cast(JS_OBJ(contextObj->Get(JS_STR("constructor")))->Get(JS_STR("CanvasGradient")));
    Local<Value> argv[] = {
      info[0],
      info[1],
      info[2],
      info[3],
      info[4],
      info[5],
    };
    Local<Object> canvasGradientObj = canvasGradientCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();
    info.GetReturnValue().Set(canvasGradientObj);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_METHOD(CanvasRenderingContext2D::CreatePattern) {
  if (isImageValue(info[0])) {
    Local<Object> contextObj = Local<Object>::Cast(info.This());
    CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(contextObj);

    Local<Function> canvasPatternCons = Local<Function>::Cast(JS_OBJ(contextObj->Get(JS_STR("constructor")))->Get(JS_STR("CanvasPattern")));
    Local<Value> argv[] = {
      info[0],
      info[1],
    };
    Local<Object> canvasPatternObj = canvasPatternCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();
    info.GetReturnValue().Set(canvasPatternObj);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_METHOD(CanvasRenderingContext2D::Resize) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  unsigned int w = TO_UINT32(info[0]);
  unsigned int h = TO_UINT32(info[1]);
  
  context->Resize(w, h);
}

NAN_METHOD(CanvasRenderingContext2D::DrawImage) {
  // Nan::HandleScope scope;

  if (isImageType(info[0])) {
    sk_sp<SkImage> image = getImage(info[0]);
    if (image) {
      CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());

      int x = TO_INT32(info[1]);
      int y = TO_INT32(info[2]);

      if (info.Length() > 3) {
        if (info.Length() > 5) {
          unsigned int sw = TO_UINT32(info[3]);
          unsigned int sh = TO_UINT32(info[4]);
          unsigned int dx = TO_UINT32(info[5]);
          unsigned int dy = TO_UINT32(info[6]);
          unsigned int dw = TO_UINT32(info[7]);
          unsigned int dh = TO_UINT32(info[8]);

          context->DrawImage(image.get(), x, y, sw, sh, dx, dy, dw, dh);
        } else {
          unsigned int dw = TO_UINT32(info[3]);
          unsigned int dh = TO_UINT32(info[4]);
          unsigned int sw = image->width();
          unsigned int sh = image->height();

          context->DrawImage(image.get(), 0, 0, sw, sh, x, y, dw, dh);
        }
      } else {
        unsigned int sw = image->width();
        unsigned int sh = image->height();
        unsigned int dw = sw;
        unsigned int dh = sh;

        context->DrawImage(image.get(), 0, 0, sw, sh, x, y, dw, dh);
      }
    }
  } else {
    Nan::ThrowError("drawImage: invalid arguments");
  }
}

NAN_METHOD(CanvasRenderingContext2D::CreateImageData) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(Local<Object>::Cast(info.This()));
  double w = TO_DOUBLE(info[0]);
  double h = TO_DOUBLE(info[1]);

  Local<Function> imageDataCons = Local<Function>::Cast(
    JS_OBJ(Local<Object>::Cast(info.This())->Get(JS_STR("constructor")))->Get(JS_STR("ImageData"))
  );
  Local<Value> argv[] = {
    Number::New(Isolate::GetCurrent(), w),
    Number::New(Isolate::GetCurrent(), h),
  };
  Local<Object> imageDataObj = imageDataCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  info.GetReturnValue().Set(imageDataObj);
}

NAN_METHOD(CanvasRenderingContext2D::GetImageData) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(Local<Object>::Cast(info.This()));
  int x = TO_INT32(info[0]);
  int y = TO_INT32(info[1]);
  unsigned int w = TO_UINT32(info[2]);
  unsigned int h = TO_UINT32(info[3]);

  Local<Function> imageDataCons = Local<Function>::Cast(
    JS_OBJ(Local<Object>::Cast(info.This())->Get(JS_STR("constructor")))->Get(JS_STR("ImageData"))
  );
  Local<Value> argv[] = {
    Number::New(Isolate::GetCurrent(), w),
    Number::New(Isolate::GetCurrent(), h),
  };
  Local<Object> imageDataObj = imageDataCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();
  ImageData *imageData = ObjectWrap::Unwrap<ImageData>(imageDataObj);

  bool ok = context->surface->getCanvas()->readPixels(imageData->bitmap, x, y);
  if (ok) {
    return info.GetReturnValue().Set(imageDataObj);
  } else {
    return Nan::ThrowError("Failed to get pixels");
  }
}

NAN_METHOD(CanvasRenderingContext2D::PutImageData) {
  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(Local<Object>::Cast(info.This()));

  ImageData *imageData = ObjectWrap::Unwrap<ImageData>(Local<Object>::Cast(info[0]));
  int x = TO_INT32(info[1]);
  int y = TO_INT32(info[2]);

  if (info.Length() > 3) {
    int dirtyX = TO_INT32(info[3]);
    int dirtyY = TO_INT32(info[4]);
    unsigned int dirtyWidth = TO_UINT32(info[5]);
    unsigned int dirtyHeight = TO_UINT32(info[6]);
    unsigned int dw = imageData->GetWidth();
    unsigned int dh = imageData->GetHeight();

    sk_sp<SkImage> image = SkImage::MakeFromBitmap(imageData->bitmap);
    context->DrawImage(image.get(), dirtyX, dirtyY, dirtyWidth, dirtyHeight, x, y, dw, dh);
  } else {
    unsigned int sw = imageData->GetWidth();
    unsigned int sh = imageData->GetHeight();
    unsigned int dw = sw;
    unsigned int dh = sh;

    sk_sp<SkImage> image = SkImage::MakeFromBitmap(imageData->bitmap);
    context->DrawImage(image.get(), 0, 0, sw, sh, x, y, dw, dh);
  }
}

NAN_METHOD(CanvasRenderingContext2D::Save) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  context->Save();
}

NAN_METHOD(CanvasRenderingContext2D::Restore) {
  // Nan::HandleScope scope;

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  context->Restore();
}

NAN_METHOD(CanvasRenderingContext2D::ToArrayBuffer) {
  // Nan::HandleScope scope;

  std::string type;
  if (info[0]->IsString()) {
    Nan::Utf8String utf8Value(Local<String>::Cast(info[0]));
    type = *utf8Value;
  }
  SkEncodedImageFormat format;
  if (type == "image/png") {
    format = SkEncodedImageFormat::kPNG;
  } else if (type == "image/jpeg") {
    format = SkEncodedImageFormat::kJPEG;
  } else {
    type = "image/png";
    format = SkEncodedImageFormat::kPNG;
  }

  int quality = 90;
  if (info[1]->IsNumber()) {
    double d = std::min<double>(std::max<double>(TO_DOUBLE(info[1]), 0), 1);
    quality = static_cast<int>(d * 100);
  }

  CanvasRenderingContext2D *context = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  sk_sp<SkImage> image = getImageFromContext(context);
  sk_sp<SkData> data = image->encodeToData(format, quality);

  Local<ArrayBuffer> result = ArrayBuffer::New(Isolate::GetCurrent(), data->size());
  memcpy(result->GetContents().Data(), data->data(), data->size());
  result->Set(JS_STR("type"), JS_STR(type));
  info.GetReturnValue().Set(result);
}

NAN_METHOD(CanvasRenderingContext2D::Destroy) {
  CanvasRenderingContext2D *ctx = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  ctx->live = false;
}

NAN_METHOD(CanvasRenderingContext2D::GetWindowHandle) {
  CanvasRenderingContext2D *ctx = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  if (ctx->windowHandle) {
    info.GetReturnValue().Set(pointerToArray(ctx->windowHandle));
  } else {
    info.GetReturnValue().Set(Nan::Null());
  }
}

NAN_METHOD(CanvasRenderingContext2D::SetWindowHandle) {
  CanvasRenderingContext2D *ctx = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  if (info[0]->IsArray()) {
    ctx->windowHandle = (NATIVEwindow *)arrayToPointer(Local<Array>::Cast(info[0]));
    
    windowsystem::SetCurrentWindowContext(ctx->windowHandle);
    
    // You've already created your OpenGL context and bound it.
    // Leaving interface as null makes Skia extract pointers to OpenGL functions for the current
    // context in a platform-specific way. Alternatively, you may create your own GrGLInterface and
    // initialize it however you like to attach to an alternate OpenGL implementation or intercept
    // Skia's OpenGL calls.
    // const GrGLInterface *interface = nullptr;
    ctx->grContext = GrContext::MakeGL(nullptr);
  } else {
    ctx->windowHandle = nullptr;
  }
}

NAN_METHOD(CanvasRenderingContext2D::SetTexture) {
  CanvasRenderingContext2D *ctx = ObjectWrap::Unwrap<CanvasRenderingContext2D>(info.This());
  if (info[0]->IsNumber() && info[1]->IsNumber() && info[2]->IsNumber()) {
    GLuint tex = TO_UINT32(info[0]);
    int width = TO_INT32(info[1]);
    int height = TO_INT32(info[2]);
    
    ctx->tex = tex;

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    GrGLTextureInfo glTexInfo;
    glTexInfo.fID = tex;
    glTexInfo.fTarget = GL_TEXTURE_2D;
    glTexInfo.fFormat = GL_RGBA8;
    
    GrBackendTexture backendTex(width, height, GrMipMapped::kNo, glTexInfo);
    
    ctx->surface = SkSurface::MakeFromBackendTexture(ctx->grContext.get(), backendTex, kBottomLeft_GrSurfaceOrigin, 0, SkColorType::kRGBA_8888_SkColorType, nullptr, nullptr);
    if (!ctx->surface) {
      exerr << "Failed to create CanvasRenderingContext2D surface" << std::endl;
      abort();
    }
  } else {
    Nan::ThrowError("CanvasRenderingContext2D: invalid arguments");
  }
}

bool CanvasRenderingContext2D::isImageType(Local<Value> arg) {
  Local<Value> constructorName = JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"));

  Nan::Utf8String utf8Value(constructorName);
  std::string stringValue(*utf8Value, utf8Value.length());

  return
    stringValue == "HTMLImageElement" ||
    stringValue == "HTMLVideoElement" ||
    stringValue == "ImageData" ||
    stringValue == "ImageBitmap" ||
    stringValue == "HTMLCanvasElement";
}

sk_sp<SkImage> CanvasRenderingContext2D::getImageFromContext(CanvasRenderingContext2D *ctx) {
  SkCanvas *canvas = ctx->surface->getCanvas();
  SkBitmap bitmap;
  bool ok = bitmap.tryAllocPixels(canvas->imageInfo()) && canvas->readPixels(bitmap, 0, 0);
  if (ok) {
    bitmap.setImmutable();
    return SkImage::MakeFromBitmap(bitmap);
  } else {
    return nullptr;
  }
}

sk_sp<SkImage> CanvasRenderingContext2D::getImage(Local<Value> arg) {
  if (JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("HTMLImageElement"))) {
    Image *image = ObjectWrap::Unwrap<Image>(Local<Object>::Cast(JS_OBJ(arg)->Get(JS_STR("image"))));
    return image->image;
  } else if (JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("HTMLVideoElement"))) {
    auto video = JS_OBJ(arg)->Get(JS_STR("video"));
    if (video->IsObject()) {
      return getImage(JS_OBJ(video)->Get(JS_STR("imageData")));
    }
    return nullptr;
  } else if (JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("ImageData"))) {
    ImageData *imageData = ObjectWrap::Unwrap<ImageData>(Local<Object>::Cast(arg));
    return SkImage::MakeFromBitmap(imageData->bitmap);
  } else if (JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("ImageBitmap"))) {
    ImageBitmap *imageBitmap = ObjectWrap::Unwrap<ImageBitmap>(Local<Object>::Cast(arg));
    return SkImage::MakeFromBitmap(imageBitmap->bitmap);
  } else if (JS_OBJ(JS_OBJ(arg)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("HTMLCanvasElement"))) {
    Local<Value> otherContextObj = JS_OBJ(arg)->Get(JS_STR("_context"));
    if (otherContextObj->IsObject() && JS_OBJ(JS_OBJ(otherContextObj)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("CanvasRenderingContext2D"))) {
      CanvasRenderingContext2D *otherContext = ObjectWrap::Unwrap<CanvasRenderingContext2D>(Local<Object>::Cast(otherContextObj));
      return getImageFromContext(otherContext);
    } else if (otherContextObj->IsObject() && (
      JS_OBJ(JS_OBJ(otherContextObj)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("WebGLRenderingContext")) ||
      JS_OBJ(JS_OBJ(otherContextObj)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("WebGL2RenderingContext"))
    )) {
      WebGLRenderingContext *gl = ObjectWrap::Unwrap<WebGLRenderingContext>(Local<Object>::Cast(otherContextObj));

      int w, h;
      windowsystem::GetWindowSize(gl->windowHandle, &w, &h);

      const SkImageInfo &info = SkImageInfo::Make(w, h, SkColorType::kRGBA_8888_SkColorType, SkAlphaType::kPremul_SkAlphaType);
      SkBitmap bitmap;
      bool ok = bitmap.tryAllocPixels(info);
      if (ok) {
        windowsystem::SetCurrentWindowContext(gl->windowHandle);

        unique_ptr<char[]> pixels(new char[w * h * 4]);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());

        flipImageData((char *)bitmap.getPixels(), pixels.get(), w, h, 4);

        bitmap.setImmutable();
        return SkImage::MakeFromBitmap(bitmap);
      } else {
        return nullptr;
      }
    } else {
      return nullptr;
    }
  } else {
    return nullptr;
  }
}

CanvasRenderingContext2D::CanvasRenderingContext2D() :
  live(true),
  tex(0),
  lineHeight(1),
  shadowBlur(0),
  shadowOffsetX(0),
  shadowOffsetY(0)
{
  // flipCanvasY(surface->getCanvas());

  jsStrokeStyle.Reset(JS_STR("#000000"));
  strokePaint.setTextSize(12);
  strokePaint.setColor(0xff000000);
  strokePaint.setStyle(SkPaint::kStroke_Style);
  strokePaint.setBlendMode(SkBlendMode::kSrcOver);

  jsFillStyle.Reset(JS_STR("#0000ff"));
  fillPaint.setTextSize(12);
  fillPaint.setColor(0xff0000ff);
  fillPaint.setStyle(SkPaint::kFill_Style);
  fillPaint.setBlendMode(SkBlendMode::kSrcOver);

  clearPaint.setTextSize(12);
  clearPaint.setColor(0x0);
  clearPaint.setStyle(SkPaint::kFill_Style);
  clearPaint.setBlendMode(SkBlendMode::kSrc);

  shadowColor = "rgba(0, 0, 0, 0)";
}

CanvasRenderingContext2D::~CanvasRenderingContext2D () {}
