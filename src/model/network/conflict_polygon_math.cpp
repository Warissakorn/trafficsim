#include "conflict_polygon_math.hpp"
#include <algorithm>
#include <cmath>
namespace trafficsim {
namespace {
Point sub(Point a,Point b){return {a.x-b.x,a.y-b.y};}
double cross(Point a,Point b){return a.x*b.y-a.y*b.x;}
double turn(Point a,Point b,Point c){return cross(sub(b,a),sub(c,a));}
}
double polygonSignedArea(const std::vector<Point>& p) {
    double sum=0;for(std::size_t i=0;i<p.size();++i)sum+=cross(p[i],p[(i+1)%p.size()]);return sum/2;
}
std::vector<Point> intersectConvexPolygons(std::vector<Point> subject, const std::vector<Point>& by) {
    for (std::size_t e = 0; e < by.size() && !subject.empty(); ++e) {
        const auto a = by[e], edge = sub(by[(e + 1) % by.size()], a);
        const auto input = std::move(subject);
        subject.clear();
        for (std::size_t i = 0; i < input.size(); ++i) {
            const auto p = input[i], q = input[(i + 1) % input.size()];
            const double sp = cross(edge, sub(p, a)), sq = cross(edge, sub(q, a));
            if (sp >= 0) subject.push_back(p);
            if ((sp >= 0) != (sq >= 0)) {
                const double t = sp / (sp - sq);
                subject.push_back({p.x + t * (q.x - p.x), p.y + t * (q.y - p.y)});
            }
        }
    }
    return subject;
}
double stationInLaneQuad(Point l0,Point l1,Point r0,Point r1,Point p,double start,double length) {
    const auto e = sub(l1, l0), g = sub(r0, l0), h = sub(p, l0), k = sub(sub(r1, r0), e);
    const double a = -cross(k, e), b = cross(k, h) - cross(g, e), c = cross(g, h);
    double t;
    if (std::abs(a) <= 1e-12 * (std::abs(b) + std::abs(c)) || a == 0) t = b == 0 ? 0 : -c / b;
    else {
        const double root = std::sqrt(std::max(0.0, b * b - 4 * a * c));
        const double t1 = (-b + root) / (2 * a), t2 = (-b - root) / (2 * a);
        const auto off = [](double x) { return x < 0 ? -x : x > 1 ? x - 1 : 0; };
        t = off(t1) <= off(t2) ? t1 : t2;
    }
    return start + std::clamp(t, 0.0, 1.0) * length;
}
ConflictPolygons triangulatePolygon(std::vector<Point> p) {
    constexpr double eps=1e-10;
    bool changed=true;
    while(changed && p.size()>2) {
        changed=false;
        for(std::size_t i=0;i<p.size();++i) {
            const auto a=p[(i+p.size()-1)%p.size()],b=p[i],c=p[(i+1)%p.size()];
            if(std::hypot(b.x-a.x,b.y-a.y)<eps || std::abs(turn(a,b,c))<eps) {
                p.erase(p.begin()+static_cast<std::ptrdiff_t>(i));changed=true;break;
            }
        }
    }
    if(p.size()<3 || std::abs(polygonSignedArea(p))<1e-6)return {};
    if(polygonSignedArea(p)<0)std::reverse(p.begin(),p.end());
    // Reject folded polygons instead of assigning an ambiguous interior.
    for(std::size_t i=0;i<p.size();++i)for(std::size_t j=i+2;j<p.size();++j) {
        if(i==0 && j+1==p.size())continue;
        const auto a=p[i],b=p[(i+1)%p.size()],c=p[j],d=p[(j+1)%p.size()];
        if(turn(a,b,c)*turn(a,b,d)<-eps && turn(c,d,a)*turn(c,d,b)<-eps)return {};
    }
    ConflictPolygons result;
    while(p.size()>3) {
        bool removed=false;
        for(std::size_t i=0;i<p.size();++i) {
            const auto prev=(i+p.size()-1)%p.size(),next=(i+1)%p.size();
            const auto a=p[prev],b=p[i],c=p[next];if(turn(a,b,c)<=eps)continue;
            bool occupied=false;
            for(std::size_t j=0;j<p.size();++j)if(j!=prev && j!=i && j!=next)
                if(turn(a,b,p[j])>=-eps && turn(b,c,p[j])>=-eps && turn(c,a,p[j])>=-eps)occupied=true;
            if(occupied)continue;
            result.push_back({a,b,c});p.erase(p.begin()+static_cast<std::ptrdiff_t>(i));removed=true;break;
        }
        if(!removed)return {};
    }
    result.push_back(std::move(p));return result;
}
}
