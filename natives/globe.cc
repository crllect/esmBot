#include <napi.h>
#include <list>
#include <iostream>
#include <Magick++.h>

using namespace std;
using namespace Magick;

class GlobeWorker : public Napi::AsyncWorker {
 public:
  GlobeWorker(Napi::Function& callback, string in_path, string type, int delay)
      : Napi::AsyncWorker(callback), in_path(in_path), type(type), delay(delay) {}
  ~GlobeWorker() {}

  void Execute() {
    list<Image> frames;
    list<Image> coalesced;
    list<Image> mid;
    list<Image> result;
    Image distort;
    readImages(&frames, in_path);
    distort.read("./assets/images/spheremap.png");
    coalesceImages(&coalesced, frames.begin(), frames.end());

    if (type != "GIF") {
      list<Image>::iterator it = coalesced.begin();
      for (int i = 0; i < 29; ++i) {
        coalesced.push_back(*it);
      }
    }

    int i = 0;

    for (Image &image : coalesced) {
      image.scale(Geometry("500x500!"));
      image.alphaChannel(Magick::SetAlphaChannel);
      size_t width = image.columns();
      image.roll(Geometry("+" + to_string(width * i / coalesced.size())));
      image.composite(distort, Magick::CenterGravity, Magick::DistortCompositeOp);
      image.magick("GIF");
      mid.push_back(image);
      i++;
    }

    optimizeImageLayers(&result, mid.begin(), mid.end());
    if (delay != 0) {
      for_each(result.begin(), result.end(), animationDelayImage(delay));
    } else if (type != "GIF") {
      for_each(result.begin(), result.end(), animationDelayImage(5));
    }
    writeImages(result.begin(), result.end(), &blob);
  }

  void OnOK() {
    Callback().Call({Env().Undefined(), Napi::Buffer<char>::Copy(Env(), (char *)blob.data(), blob.length())});
  }

 private:
  string in_path, type;
  int delay, wordlength, n;
  size_t bytes, type_size;
  Blob blob;
};

Napi::Value Globe(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  string in_path = info[0].As<Napi::String>().Utf8Value();
  string type = info[1].As<Napi::String>().Utf8Value();
  int delay = info[2].As<Napi::Number>().Int32Value();
  Napi::Function cb = info[3].As<Napi::Function>();

  GlobeWorker* blurWorker = new GlobeWorker(cb, in_path, type, delay);
  blurWorker->Queue();
  return env.Undefined();
}