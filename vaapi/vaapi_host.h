#ifndef vaapi_host_h
#define vaapi_host_h

#define DEFINE_CLASS_ENTRY(Klass, Mime, Codec) { Mime, create##Klass<YamiMediaCodec::Vaapi##Klass##Codec> }

#define DEFINE_CLASS_FACTORY(Klass) \
template <class T>\
YamiMediaCodec::IVideo##Klass* create##Klass()\
{\
    return new T();\
}\
struct Klass##Entry\
{\
    const char* mime;\
    YamiMediaCodec::IVideo##Klass* (*create)();\
};

#define DEFINE_DECODER_ENTRY(Mime, Codec) DEFINE_CLASS_ENTRY(Decoder, Mime, Codec)
#define DEFINE_ENCODER_ENTRY(Mime, Codec) DEFINE_CLASS_ENTRY(Encoder, Mime, Codec)

#endif //vaapi_host_h
