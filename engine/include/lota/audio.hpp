#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
namespace lota {
// PCM frames use native signed 16-bit samples; no platform library types escape.
struct AudioClip { int rate=22050, channels=1; std::vector<int16_t> samples; };
inline AudioClip decodeWavBytes(const std::vector<uint8_t>& b) {
 auto need=[](bool ok,const char* error){if(!ok)throw std::runtime_error(error);};
 need(b.size()>=44&&b.size()<=4*1024*1024,"WAV size must be 44 bytes to 4 MiB");
 auto u16=[&](size_t p){need(p+2<=b.size(),"Truncated WAV");return uint32_t(b[p])|(uint32_t(b[p+1])<<8);};
 auto u32=[&](size_t p){need(p+4<=b.size(),"Truncated WAV");return u16(p)|(u16(p+2)<<16);};
 auto tag=[&](size_t p,const char* s){return p+4<=b.size()&&std::string(b.begin()+p,b.begin()+p+4)==s;};
 need(tag(0,"RIFF")&&tag(8,"WAVE")&&uint64_t(u32(4))+8==b.size(),"Invalid RIFF/WAVE");
 bool fmt=false,data=false;size_t at=12,offset=0,length=0;uint32_t format=0,bits=0,align=0,byteRate=0;AudioClip clip;
 while(at+8<=b.size()) {size_t n=u32(at+4),end=at+8+n;need(end<=b.size(),"Truncated WAV chunk");
  if(tag(at,"fmt ")) {need(!fmt&&n>=16,"Invalid WAV format chunk");fmt=true;format=u16(at+8);clip.channels=int(u16(at+10));clip.rate=int(u32(at+12));byteRate=u32(at+16);align=u16(at+20);bits=u16(at+22);}
  if(tag(at,"data")) {need(!data,"Duplicate WAV data chunk");data=true;offset=at+8;length=n;}at=end+(n%2);
 }
 need(at==b.size()&&fmt&&data&&format==1&&bits==16&&(clip.channels==1||clip.channels==2)&&clip.rate>=8000&&clip.rate<=48000&&align==uint32_t(clip.channels*2)&&byteRate==uint32_t(clip.rate)*align&&length>0&&length%align==0,"Use PCM 16-bit WAV, mono/stereo, 8–48 kHz");
 need(double(length)/byteRate<=60,"WAV clips must be at most 60 seconds");
 clip.samples.reserve(length/2);for(size_t p=offset;p<offset+length;p+=2){auto n=u16(p);clip.samples.push_back(int16_t(n>=32768?int(n)-65536:int(n)));}return clip;
}
inline std::vector<uint8_t> decodeBase64(const std::string& s) {
 if(s.empty()||s.size()>5592408||s.size()%4)throw std::runtime_error("Invalid WAV base64 size");
 auto val=[](char c){if(c>='A'&&c<='Z')return int(c-'A');if(c>='a'&&c<='z')return int(c-'a'+26);if(c>='0'&&c<='9')return int(c-'0'+52);if(c=='+')return 62;if(c=='/')return 63;return -1;};
 std::vector<uint8_t> out;out.reserve(s.size()/4*3);
 for(size_t i=0;i<s.size();i+=4){int a=val(s[i]),b=val(s[i+1]),c=val(s[i+2]),d=val(s[i+3]);bool pad2=s[i+2]=='=',pad1=s[i+3]=='=';
  if(a<0||b<0||(!pad2&&c<0)||(!pad1&&d<0)||(pad2&&!pad1)||((pad1||pad2)&&i+4!=s.size()))throw std::runtime_error("Invalid WAV base64");
  uint32_t n=(uint32_t(a)<<18)|(uint32_t(b)<<12)|(uint32_t(c<0?0:c)<<6)|uint32_t(d<0?0:d);out.push_back(uint8_t(n>>16));if(!pad2)out.push_back(uint8_t(n>>8));if(!pad1)out.push_back(uint8_t(n));
 }return out;
}
inline AudioClip decodeWav(const std::string& base64) {return decodeWavBytes(decodeBase64(base64));}
}
