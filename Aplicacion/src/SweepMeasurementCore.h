#pragma once
#include "TransferFunctionCore.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <optional>
#include <vector>

namespace SweepMeasurementCore {
using Complex=std::complex<double>;
struct Result {std::vector<Complex> transfer;std::vector<bool> valid;double sampleRate=0;};
inline void fft(std::vector<Complex>& a){size_t n=a.size();for(size_t i=1,j=0;i<n;++i){size_t bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}for(size_t length=2;length<=n;length<<=1){auto step=std::polar(1.,-2.*3.14159265358979323846/length);for(size_t start=0;start<n;start+=length){Complex w{1,0};for(size_t j=0;j<length/2;++j){auto u=a[start+j],v=a[start+j+length/2]*w;a[start+j]=u+v;a[start+j+length/2]=u-v;w*=step;}}}}
inline std::optional<Result> analyze(const std::vector<float>& mic,const std::vector<float>& ref,double rate,double startHz,double endHz){
 if(mic.size()!=ref.size()||mic.size()<4096||rate<8000||startHz<=0||endHz<=startHz)return {};
 size_t size=1;while(size<mic.size())size<<=1;if(size>1u<<22)return {};
 std::vector<Complex> m(size),r(size);for(size_t i=0;i<mic.size();++i){m[i]={mic[i],0};r[i]={ref[i],0};}fft(m);fft(r);
 double maximum=0;for(size_t k=1;k<size/2;++k)maximum=std::max(maximum,std::norm(r[k]));if(maximum<1.e-12)return {};
 Result out;out.sampleRate=rate;out.transfer.resize(TransferFunctionCore::bins);out.valid.resize(TransferFunctionCore::bins);
 for(int k=1;k<TransferFunctionCore::bins;++k){double hz=k*rate/TransferFunctionCore::fftSize;size_t bin=(size_t)std::llround(hz*size/rate);if(hz<startHz||hz>endHz||bin>=size/2)continue;double power=std::norm(r[bin]);if(power<maximum*1.e-7)continue;out.transfer[(size_t)k]=m[bin]*std::conj(r[bin])/(power+maximum*1.e-9);out.valid[(size_t)k]=true;}
 return out;
}
}
