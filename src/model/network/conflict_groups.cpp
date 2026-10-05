#include "right_of_way.hpp"
#include "conflict_surface.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
namespace trafficsim {
std::string conflictOwner(const ControlPathRef& p) { return p.connectorId.empty()?p.linkId:p.connectorId; }
namespace {
constexpr double kTouch=1e-6;
double cross(Point a,Point b,Point c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);}
bool on(Point p,Point a,Point b) {
    const double length=std::hypot(b.x-a.x,b.y-a.y);
    return std::abs(cross(a,b,p))<=kTouch*std::max(1.,length) && p.x>=std::min(a.x,b.x)-kTouch &&
        p.x<=std::max(a.x,b.x)+kTouch && p.y>=std::min(a.y,b.y)-kTouch && p.y<=std::max(a.y,b.y)+kTouch;
}
bool contains(const std::vector<Point>& poly,Point p) {
    bool inside=false;
    for(std::size_t i=0,j=poly.size()-1;i<poly.size();j=i++) {
        const auto a=poly[j],b=poly[i];if(on(p,a,b))return true;
        if((a.y>p.y)!=(b.y>p.y) && p.x<(b.x-a.x)*(p.y-a.y)/(b.y-a.y)+a.x)inside=!inside;
    }
    return inside;
}
bool touches(const std::vector<Point>& a,const std::vector<Point>& b) {
    if(a.size()<3 || b.size()<3)return false;
    for(std::size_t i=0;i<a.size();++i)for(std::size_t j=0;j<b.size();++j) {
        const auto p=a[i],q=a[(i+1)%a.size()],r=b[j],s=b[(j+1)%b.size()];
        if(on(p,r,s)||on(q,r,s)||on(r,p,q)||on(s,p,q))return true;
        const auto x=cross(p,q,r),y=cross(p,q,s),u=cross(r,s,p),v=cross(r,s,q);
        if((x<0)!=(y<0) && (u<0)!=(v<0))return true;
    }
    return contains(a,b.front())||contains(b,a.front());
}
struct Piece {std::string id,firstOwner,secondOwner;ConflictKind kind;bool automatic;std::vector<Point> first,second;};
}
std::vector<ConflictGroup> conflictGroups(const Network& n,const std::vector<AutomaticConflict>& automatic) {
    std::vector<Piece> pieces;
    const auto add=[&](const std::string& id,ConflictKind kind,const ConflictSide& a,const ConflictSide& b,bool automatic) {
        auto first=conflictOwner(a.path),second=conflictOwner(b.path);
        auto f=conflictRuntimeOutline(n,a),s=conflictRuntimeOutline(n,b);
        if(second<first){std::swap(first,second);std::swap(f,s);}
        pieces.push_back({id,first,second,kind,automatic,std::move(f),std::move(s)});
    };
    for(const auto& a:n.rightOfWay.conflictAreas)add(a.id,a.kind,a.first,a.second,false);
    for(const auto& a:automatic)add(a.key,a.kind,a.first,a.second,true);
    std::sort(pieces.begin(),pieces.end(),[](const auto& a,const auto& b){return a.id<b.id;});
    std::vector<std::size_t> roots(pieces.size());std::iota(roots.begin(),roots.end(),0);
    const auto root=[&](std::size_t i){while(roots[i]!=i)i=roots[i]=roots[roots[i]];return i;};
    for(std::size_t i=0;i<pieces.size();++i)for(std::size_t j=i+1;j<pieces.size();++j) {
        const auto& a=pieces[i];const auto& b=pieces[j];
        if(a.kind!=b.kind || a.firstOwner!=b.firstOwner || a.secondOwner!=b.secondOwner)continue;
        if(touches(a.first,b.first) && touches(a.second,b.second))roots[root(j)]=root(i);
    }
    std::vector<ConflictGroup> result;std::vector<std::size_t> owners;
    for(std::size_t i=0;i<pieces.size();++i) {
        const auto r=root(i);const auto at=std::find(owners.begin(),owners.end(),r);
        if(at==owners.end()){owners.push_back(r);result.push_back({{},pieces[i].firstOwner,pieces[i].secondOwner,pieces[i].kind,{},{}});}
        auto& g=result[static_cast<std::size_t>(std::find(owners.begin(),owners.end(),r)-owners.begin())];
        (pieces[i].automatic?g.automaticKeys:g.areaIds).push_back(pieces[i].id);
    }
    for(auto& g:result)g.key=!g.areaIds.empty()?g.areaIds.front():g.automaticKeys.front();
    // Keep authored rows before suggestions, as the existing table does across Undo/Redo.
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){
        if(a.areaIds.empty()!=b.areaIds.empty())return !a.areaIds.empty();
        return a.key<b.key;
    });return result;
}
bool conflictGroupContains(const ConflictGroup& g,const std::string& id) {
    return std::find(g.areaIds.begin(),g.areaIds.end(),id)!=g.areaIds.end() ||
        std::find(g.automaticKeys.begin(),g.automaticKeys.end(),id)!=g.automaticKeys.end();
}
}
