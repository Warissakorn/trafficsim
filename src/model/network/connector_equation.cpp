#include "network.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
namespace trafficsim {
Point equationPoint(const ConnectorEquation& c,double t) {
    t=std::clamp(t,0.,1.);if(t==0)return c.controls[0];if(t==1)return c.controls[3];
    const double u=1-t;const auto& p=c.controls;
    return {u*u*u*p[0].x+3*u*u*t*p[1].x+3*u*t*t*p[2].x+t*t*t*p[3].x,
            u*u*u*p[0].y+3*u*u*t*p[1].y+3*u*t*t*p[2].y+t*t*t*p[3].y};
}
Point equationDerivative(const ConnectorEquation& c,double t) {
    const double u=1-t;const auto& p=c.controls;
    return {3*(u*u*(p[1].x-p[0].x)+2*u*t*(p[2].x-p[1].x)+t*t*(p[3].x-p[2].x)),
            3*(u*u*(p[1].y-p[0].y)+2*u*t*(p[2].y-p[1].y)+t*t*(p[3].y-p[2].y))};
}
Point equationSecondDerivative(const ConnectorEquation& c,double t) {
    const auto& p=c.controls;
    return {6*((1-t)*(p[2].x-2*p[1].x+p[0].x)+t*(p[3].x-2*p[2].x+p[1].x)),
            6*((1-t)*(p[2].y-2*p[1].y+p[0].y)+t*(p[3].y-2*p[2].y+p[1].y))};
}
namespace {
double speed(const ConnectorEquation& c,double t){const auto d=equationDerivative(c,t);return std::hypot(d.x,d.y);}
// Gauss-Legendre quadrature integrates |B'(t)|, never chords between B(t) samples.
double gauss(const ConnectorEquation& c,double a,double b) {
    constexpr std::array<double,4> x{.18343464249564980494,.52553240991632898582,.79666647741362673959,.96028985649753623168};
    constexpr std::array<double,4> w{.36268378337836198297,.31370664587788728734,.22238103445337447054,.10122853629037625915};
    const double mid=std::midpoint(a,b),half=(b-a)/2;double sum=0;
    for(std::size_t i=0;i<x.size();++i)sum+=w[i]*(speed(c,mid-half*x[i])+speed(c,mid+half*x[i]));
    return half*sum;
}
double integral(const ConnectorEquation& c,double a,double b,int depth=0) {
    if(a==b)return 0;
    const double mid=std::midpoint(a,b),whole=gauss(c,a,b),halves=gauss(c,a,mid)+gauss(c,mid,b);
    if(depth==12 || std::abs(halves-whole)<=1e-12*std::max(1.,halves))return halves;
    return integral(c,a,mid,depth+1)+integral(c,mid,b,depth+1);
}
double polynomial(const std::vector<double>& a,double x){double v=0;for(auto i=a.rbegin();i!=a.rend();++i)v=v*x+*i;return v;}
// Critical roots partition a polynomial into monotone intervals. Recursion handles repeated
// roots; fixed bisection isolates each sign-changing root without geometric point sampling.
std::vector<double> roots(std::vector<double> a) {
    while(a.size()>1 && a.back()==0)a.pop_back();
    if(a.size()<2)return {};
    if(a.size()==2){const double t=-a[0]/a[1];return t>=0 && t<=1?std::vector<double>{t}:std::vector<double>{};}
    std::vector<double> derivative;for(std::size_t i=1;i<a.size();++i)derivative.push_back(i*a[i]);
    auto cuts=roots(derivative);cuts.insert(cuts.begin(),0);cuts.push_back(1);
    double scale=0;for(double v:a)scale+=std::abs(v);const double tolerance=1e-13*std::max(1.,scale);
    std::vector<double> result;
    for(double t:cuts)if(std::abs(polynomial(a,t))<=tolerance)result.push_back(t);
    for(std::size_t i=1;i<cuts.size();++i) {
        double lo=cuts[i-1],hi=cuts[i],f=polynomial(a,lo);const double g=polynomial(a,hi);
        if((f<0)==(g<0) || f==0 || g==0)continue;
        for(int k=0;k<56;++k){const double mid=std::midpoint(lo,hi),v=polynomial(a,mid);if((v<0)==(f<0)){lo=mid;f=v;}else hi=mid;}
        result.push_back(std::midpoint(lo,hi));
    }
    std::sort(result.begin(),result.end());return result;
}
double authoredParameter(const std::vector<Point>& g,double station) {
    if(g.size()<2)return 0;
    const double length=polylineLength(g);
    if(station<=0)return 0;
    if(station>=length)return 1;
    double at=0;
    for(std::size_t j=1;j<g.size();++j){const double length=std::hypot(g[j].x-g[j-1].x,g[j].y-g[j-1].y);
        if(length>0 && station<=at+length)return (j-1+(station-at)/length)/(g.size()-1);
        at+=length;
    }
    return 1;
}
double authoredStation(const std::vector<Point>& g,double t) {
    if(g.size()<2)return 0;
    const double index=std::clamp(t,0.,1.)*(g.size()-1);double at=0;
    for(std::size_t j=1;j<g.size();++j){const double length=std::hypot(g[j].x-g[j-1].x,g[j].y-g[j-1].y);
        if(index<=j)return at+(index-(j-1))*length;
        at+=length;
    }
    return at;
}
}
ConnectorEquation makeConnectorEquation(const std::array<Point,4>& points) {
    for(const auto p:points)if(!std::isfinite(p.x) || !std::isfinite(p.y))throw std::invalid_argument("INVALID_GEOMETRY");
    ConnectorEquation c{points,{}};
    for(std::size_t j=1;j<c.arcStations.size();++j)c.arcStations[j]=c.arcStations[j-1]+integral(c,(j-1)/16.,j/16.);
    if(!std::isfinite(c.arcStations.back()))throw std::invalid_argument("INVALID_GEOMETRY");
    return c;
}
double equationStation(const ConnectorEquation& c,double t) {
    t=std::clamp(t,0.,1.);if(t==1)return c.arcStations.back();
    const auto j=std::min<std::size_t>(15,static_cast<std::size_t>(t*16));
    return c.arcStations[j]+integral(c,j/16.,t);
}
double equationParameter(const ConnectorEquation& c,double station) {
    station=std::clamp(station,0.,c.arcStations.back());if(station==0)return 0;if(station==c.arcStations.back())return 1;
    const auto i=std::min<std::size_t>(15,std::distance(c.arcStations.begin(),std::upper_bound(c.arcStations.begin(),c.arcStations.end(),station))-1);
    double lo=i/16.,hi=(i+1)/16.,t=lo+(hi-lo)*(station-c.arcStations[i])/(c.arcStations[i+1]-c.arcStations[i]);
    const double tolerance=1e-11*std::max(1.,c.arcStations.back());
    for(int k=0;k<48;++k){const double error=equationStation(c,t)-station;if(std::abs(error)<=tolerance)return t;
        if(error<0)lo=t;else hi=t;const double v=speed(c,t),next=v>1e-12?t-error/v:std::midpoint(lo,hi);
        t=next>lo && next<hi?next:std::midpoint(lo,hi);}
    return t;
}
double equationClosestStation(const ConnectorEquation& c,Point target) {
    const auto& p=c.controls;
    const std::array<Point,4> a{{{p[0].x-target.x,p[0].y-target.y},{3*(p[1].x-p[0].x),3*(p[1].y-p[0].y)},
        {3*(p[2].x-2*p[1].x+p[0].x),3*(p[2].y-2*p[1].y+p[0].y)},
        {p[3].x-3*p[2].x+3*p[1].x-p[0].x,p[3].y-3*p[2].y+3*p[1].y-p[0].y}}};
    std::vector<double> derivative(6);
    for(std::size_t i=0;i<4;++i)for(std::size_t j=1;j<4;++j)derivative[i+j-1]+=j*(a[i].x*a[j].x+a[i].y*a[j].y);
    auto candidates=roots(derivative);candidates.push_back(0);candidates.push_back(1);
    double best=0,distance=INFINITY;
    for(double t:candidates){const auto q=equationPoint(c,t);const double d=std::hypot(q.x-target.x,q.y-target.y);if(d<distance){distance=d;best=t;}}
    return equationStation(c,best);
}
double connectorPathLength(const ConnectorPath& p){return p.equation?p.equation->arcStations.back():polylineLength(p.geometry);}
Point connectorPathPoint(const ConnectorPath& p,double s){return p.equation?equationPoint(*p.equation,equationParameter(*p.equation,s)):pointAlong(p.geometry,s);}
Point connectorPathDirection(const ConnectorPath& p,double s){return p.equation?equationDerivative(*p.equation,equationParameter(*p.equation,s)):directionAlong(p.geometry,s,true);}
double connectorRuntimeStation(const Connector& c,const ConnectorPath& p,double s){return p.equation?equationStation(*p.equation,authoredParameter(c.geometry,s)):matchedStation(c.geometry,p.geometry,s);}
double connectorAuthoringStation(const Connector& c,const ConnectorPath& p,double s){return p.equation?authoredStation(c.geometry,equationParameter(*p.equation,s)):matchedStation(p.geometry,c.geometry,s);}
}
