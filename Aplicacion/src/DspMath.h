#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

namespace MyaDSP {
using Complex=std::complex<double>;
inline bool powerOfTwo(int n){return n>=2&&(n&(n-1))==0;}
inline void fft(std::vector<Complex>& a,bool inverse=false){
 const int n=(int)a.size();if(!powerOfTwo(n))return;
 for(int i=1,j=0;i<n;++i){int bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[(size_t)i],a[(size_t)j]);}
 const double sign=inverse?1.:-1.;
 for(int len=2;len<=n;len<<=1){Complex step=std::polar(1.,sign*2.*3.14159265358979323846/len);for(int start=0;start<n;start+=len){Complex w{1.,0.};for(int j=0;j<len/2;++j){auto u=a[(size_t)(start+j)],v=a[(size_t)(start+j+len/2)]*w;a[(size_t)(start+j)]=u+v;a[(size_t)(start+j+len/2)]=u-v;w*=step;}}}
 if(inverse)for(auto& v:a)v/=(double)n;
}
enum class Window { Rectangular, Hann, Hamming, BlackmanHarris, FlatTop };
inline double window(Window kind,int i,int n){
 if(n<=1)return 1.;double x=2.*3.14159265358979323846*i/(n-1);
 switch(kind){
  case Window::Rectangular:return 1.;
  case Window::Hann:return .5-.5*std::cos(x);
  case Window::Hamming:return .54-.46*std::cos(x);
  case Window::BlackmanHarris:return .35875-.48829*std::cos(x)+.14128*std::cos(2*x)-.01168*std::cos(3*x);
  case Window::FlatTop:return .21557895-.41663158*std::cos(x)+.277263158*std::cos(2*x)-.083578947*std::cos(3*x)+.006947368*std::cos(4*x);
 }
 return 1.;
}
inline int nextPowerOfTwo(int n){int result=2;while(result<n&&result<(1<<22))result<<=1;return result;}
}
