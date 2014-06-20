#ifndef vaapi_host_h
#define vaapi_host_h

#define DEFINE_CLASS_ENTRY(Klass, Mime, Codec) { Mime, create##Klass<Vaapi##Klass##Codec> }

#define DEFINE_CLASS_FACTORY(Klass) \
template <class T>\
IVideo##Klass* create##Klass()\
{\
    return new T();\
}\
struct Klass##Entry\
{\
    const char* mime;\
    IVideo##Klass* (*create)();\
};

#define DEFINE_DECODER_ENTRY(Mime, Codec) DEFINE_CLASS_ENTRY(Decoder, Mime, Codec)
#define DEFINE_ENCODER_ENTRY(Mime, Codec) DEFINE_CLASS_ENTRY(Encoder, Mime, Codec)

#endif //vaapi_host_h
