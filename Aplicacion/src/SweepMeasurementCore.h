#pragma once
#include "TransferFunctionCore.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <optional>
#include <vector>

namespace SweepMeasurementCore {
using Complex=std::complex<double>;
struct Result {std::vector<Complex> transfer;std::vector<bool> valid;std::vector<float> impulse;double sampleRate=0,syncDelayMs=0;};
inline void fft(std::vector<Complex>& a){size_t n=a.size();for(size_t i=1,j=0;i<n;++i){size_t bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}for(size_t length=2;length<=n;length<<=1){auto step=std::polar(1.,-2.*3.14159265358979323846/length);for(size_t start=0;start<n;start+=length){Complex w{1,0};for(size_t j=0;j<length/2;++j){auto u=a[start+j],v=a[start+j+length/2]*w;a[start+j]=u+v;a[start+j+length/2]=u-v;w*=step;}}}}
inline void ifft(std::vector<Complex>& a){for(auto& value:a)value=std::conj(value);fft(a);double scale=1./a.size();for(auto& value:a)value=std::conj(value)*scale;}
inline std::optional<Result> analyze(const std::vector<float>& mic,const std::vector<float>& ref,double rate,double startHz,double endHz,int displayFftSize=TransferFunctionCore::fftSize,bool synchronize=false){
 if(mic.size()!=ref.size()||mic.size()<4096||rate<8000||startHz<=0||endHz<=startHz)return {};
 if(displayFftSize<128||displayFftSize>TransferFunctionCore::maxFftSize||(displayFftSize&(displayFftSize-1))!=0)return {};
 size_t size=1;while(size<mic.size())size<<=1;if(size>1u<<22)return {};
 Result out;out.sampleRate=rate;
 if(synchronize){size_t prefix=std::min(mic.size(),(size_t)std::llround(rate*2.));size_t lag=(size_t)std::llround(rate*.15),n=1;while(n<prefix+lag)n<<=1;std::vector<Complex> cm(n),cr(n);for(size_t i=0;i<prefix;++i){cm[i]={mic[i],0};cr[i]={ref[i],0};}fft(cm);fft(cr);for(size_t i=0;i<n;++i)cm[i]*=std::conj(cr[i]);ifft(cm);size_t best=0;double peak=0;for(size_t i=0;i<=lag;++i)if(std::abs(cm[i])>peak){peak=std::abs(cm[i]);best=i;}if(peak>1.e-9)out.syncDelayMs=1000.*best/rate;}
 std::vector<Complex> m(size),r(size);for(size_t i=0;i<mic.size();++i){m[i]={mic[i],0};r[i]={ref[i],0};}fft(m);fft(r);
 double maximum=0;for(size_t k=1;k<size/2;++k)maximum=std::max(maximum,std::norm(r[k]));if(maximum<1.e-12)return {};
 out.transfer.resize((displayFftSize/2+1));out.valid.resize((displayFftSize/2+1));
 for(int k=1;k<(displayFftSize/2+1);++k){double hz=k*rate/displayFftSize;size_t bin=(size_t)std::llround(hz*size/rate);if(hz<startHz||hz>endHz||bin>=size/2)continue;double power=std::norm(r[bin]);if(power<maximum*1.e-7)continue;out.transfer[(size_t)k]=m[bin]*std::conj(r[bin])/(power+maximum*1.e-9)*std::polar(1.,2.*3.14159265358979323846*hz*out.syncDelayMs/1000.);out.valid[(size_t)k]=true;}
 for(size_t k=1;k<size/2;++k){double hz=k*rate/size,power=std::norm(r[k]);Complex h{};if(hz>=startHz&&hz<=endHz&&power>=maximum*1.e-7)h=m[k]*std::conj(r[k])/(power+maximum*1.e-9)*std::polar(1.,2.*3.14159265358979323846*hz*out.syncDelayMs/1000.);m[k]=h;m[size-k]=std::conj(h);}m[0]={};m[size/2]={};ifft(m);out.impulse.resize(size);for(size_t i=0;i<size;++i)out.impulse[i]=(float)m[i].real();
 return out;
}
}
