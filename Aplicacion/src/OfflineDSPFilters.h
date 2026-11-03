#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <vector>

namespace OfflineDSPFilters {
using Complex = std::complex<double>;
constexpr double pi = 3.14159265358979323846;

struct Section {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    Complex response(double f, double fs) const {
        const auto z = std::polar(1.0, -2.0 * pi * f / fs);
        return (b0 + b1*z + b2*z*z) / (1.0 + a1*z + a2*z*z);
    }
    bool stable() const {
        if (!std::isfinite(b0+b1+b2+a1+a2)) return false;
        if (a2 == 0.0) return std::abs(a1) < 1.0;
        const Complex discriminant = Complex(a1*a1-4*a2, 0);
        const auto root = std::sqrt(discriminant);
        return std::abs((-a1+root)*0.5) < 1.0-1.e-10 && std::abs((-a1-root)*0.5) < 1.0-1.e-10;
    }
};

enum class Family { None, Butterworth, LinkwitzRiley, Bessel };
struct Crossover { Family family=Family::None; int order=2; double frequency=1000; };
enum class EQType { Bell, LowShelf, HighShelf };
struct EQBand { bool enabled=false; EQType type=EQType::Bell; double frequency=1000, gainDb=0, q=1; };
struct AllPass { bool enabled=false; int order=1; double frequency=1000, q=0.7071067811865476; };
struct Chingonizer { bool enabled=false; double lowFrequency=120, highFrequency=5000, lowGainDb=0, highGainDb=0; };

inline bool frequencyValid(double f, double fs) { return std::isfinite(f)&&std::isfinite(fs)&&fs>0&&f>=10&&f<fs*0.49; }
inline bool valid(const Crossover& x,double fs) {
    return x.family==Family::None || (frequencyValid(x.frequency,fs) && x.order>=1 && x.order<=8 &&
        (x.family!=Family::LinkwitzRiley || (x.order%2==0)));
}
inline bool valid(const EQBand& x,double fs) {
    return frequencyValid(x.frequency,fs)&&std::isfinite(x.gainDb)&&x.gainDb>=-15&&x.gainDb<=15&&
        std::isfinite(x.q)&&x.q>=0.1&&x.q<=128;
}
inline bool valid(const AllPass& x,double fs) {
    return (x.order==1||x.order==2)&&frequencyValid(x.frequency,fs)&&std::isfinite(x.q)&&x.q>=0.1&&x.q<=128;
}
inline bool valid(const Chingonizer& x,double fs) {
    return frequencyValid(x.lowFrequency,fs)&&frequencyValid(x.highFrequency,fs)&&x.lowFrequency<x.highFrequency&&
        std::isfinite(x.lowGainDb)&&std::isfinite(x.highGainDb)&&
        std::abs(x.lowGainDb)<=15&&std::abs(x.highGainDb)<=15;
}

inline std::vector<Complex> besselPoles(int order) {
    std::vector<double> c((size_t)order+1);
    auto factorial=[](int n){double result=1;for(int i=2;i<=n;++i)result*=i;return result;};
    for(int k=0;k<=order;++k)c[(size_t)k]=factorial(2*order-k)/(std::pow(2.0,order-k)*factorial(k)*factorial(order-k));
    std::vector<Complex> roots((size_t)order);
    for(int i=0;i<order;++i)roots[(size_t)i]=std::polar(3.0,2*pi*(i+0.25)/order);
    for(int iteration=0;iteration<2000;++iteration){
        double movement=0;
        for(int i=0;i<order;++i){
            Complex value=c.back(), denominator=1;
            for(int k=order-1;k>=0;--k)value=value*roots[(size_t)i]+c[(size_t)k];
            for(int j=0;j<order;++j)if(j!=i)denominator*=roots[(size_t)i]-roots[(size_t)j];
            if(std::abs(denominator)<1.e-18)denominator=Complex(1.e-18,1.e-18);
            const auto delta=value/denominator;roots[(size_t)i]-=delta;movement=std::max(movement,std::abs(delta));
        }
        if(movement<1.e-12)break;
    }
    return roots;
}

inline double besselThreeDb(int order,const std::vector<Complex>& poles){
    auto magnitude=[&](double w){double result=1;for(auto p:poles)result*=std::abs(-p)/std::abs(Complex(0,w)-p);return result;};
    double low=0,high=100;
    for(int i=0;i<80;++i){const double mid=(low+high)*0.5;if(magnitude(mid)>std::sqrt(0.5))low=mid;else high=mid;}
    (void)order;
    return (low+high)*0.5;
}
inline double besselHighPassScale(const std::vector<Complex>& poles){
    auto magnitude=[&](double scale){double result=1;for(auto p:poles)result/=std::abs(Complex(0,1)-p*scale);return result;};
    double low=0,high=100;
    for(int i=0;i<80;++i){const double mid=(low+high)*0.5;if(magnitude(mid)>std::sqrt(0.5))low=mid;else high=mid;}
    return (low+high)*0.5;
}

inline std::vector<Section> crossover(const Crossover& x,bool highPass,double fs){
    if(x.family==Family::None)return {};
    const int prototypeOrder=x.family==Family::LinkwitzRiley?x.order/2:x.order;
    std::vector<Complex> poles;
    if(x.family==Family::Bessel)poles=besselPoles(prototypeOrder);
    else for(int k=0;k<prototypeOrder;++k)
        poles.push_back(std::polar(1.0,pi*(2*k+1+prototypeOrder)/(2*prototypeOrder)));
    const double normalization=x.family==Family::Bessel?
        (highPass?besselHighPassScale(poles):1.0/besselThreeDb(prototypeOrder,poles)):1.0;
    const double warped=2*fs*std::tan(pi*x.frequency/fs)*normalization;
    for(auto& p:poles)p=(2*fs+p*warped)/(2*fs-p*warped);
    std::vector<Section> sections;
    std::vector<bool> used(poles.size());
    for(size_t i=0;i<poles.size();++i){
        if(used[i])continue;
        used[i]=true;
        Section section;
        const double zero=highPass?1.0:-1.0;
        if(std::abs(poles[i].imag())<1.e-7){
            section.a1=-poles[i].real();section.b0=1;section.b1=-zero;
        }else{
            size_t pair=i+1;
            while(pair<poles.size()&&(used[pair]||std::abs(poles[pair]-std::conj(poles[i]))>1.e-5))++pair;
            if(pair==poles.size())return {};
            used[pair]=true;
            section.a1=-2*poles[i].real();section.a2=std::norm(poles[i]);
            section.b0=1;section.b1=-2*zero;section.b2=1;
        }
        const double eval=highPass?-1.0:1.0;
        const double numerator=section.b0+section.b1*eval+section.b2*eval*eval;
        const double denominator=1+section.a1*eval+section.a2*eval*eval;
        section.b0*=denominator/numerator;section.b1*=denominator/numerator;section.b2*=denominator/numerator;
        sections.push_back(section);
    }
    if(x.family==Family::LinkwitzRiley){auto copy=sections;sections.insert(sections.end(),copy.begin(),copy.end());}
    return sections;
}

inline Section eq(const EQBand& x,double fs){
    const double A=std::pow(10.0,x.gainDb/40),w=2*pi*x.frequency/fs,cs=std::cos(w),sn=std::sin(w);
    const double alpha=sn/(2*x.q);
    double b0,b1,b2,a0,a1,a2;
    if(x.type==EQType::Bell){b0=1+alpha*A;b1=-2*cs;b2=1-alpha*A;a0=1+alpha/A;a1=-2*cs;a2=1-alpha/A;}
    else{
        const double beta=2*std::sqrt(A)*alpha;
        if(x.type==EQType::LowShelf){
            b0=A*((A+1)-(A-1)*cs+beta);b1=2*A*((A-1)-(A+1)*cs);b2=A*((A+1)-(A-1)*cs-beta);
            a0=(A+1)+(A-1)*cs+beta;a1=-2*((A-1)+(A+1)*cs);a2=(A+1)+(A-1)*cs-beta;
        }else{
            b0=A*((A+1)+(A-1)*cs+beta);b1=-2*A*((A-1)+(A+1)*cs);b2=A*((A+1)+(A-1)*cs-beta);
            a0=(A+1)-(A-1)*cs+beta;a1=2*((A-1)-(A+1)*cs);a2=(A+1)-(A-1)*cs-beta;
        }
    }
    return {b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};
}
inline Section allPass(const AllPass& x,double fs){
    const double w=2*pi*x.frequency/fs;
    if(x.order==1){const double a=(std::tan(w/2)-1)/(std::tan(w/2)+1);return {a,1,0,a,0};}
    const double alpha=std::sin(w)/(2*x.q),a0=1+alpha,a1=-2*std::cos(w),a2=1-alpha;
    return {a2/a0,a1/a0,1,a1/a0,a2/a0};
}
inline bool allStable(const std::vector<Section>& sections){return std::all_of(sections.begin(),sections.end(),[](const Section& s){return s.stable();});}
inline Complex response(const std::vector<Section>& sections,double f,double fs){Complex result{1,0};for(const auto& s:sections)result*=s.response(f,fs);return result;}
}
