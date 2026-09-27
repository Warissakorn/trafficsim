#pragma once
#include "../src/model/network/connector_surface.hpp"
#include <cmath>
#include <numbers>

namespace surface_fixture {
inline trafficsim::Network arrival(double degrees) {
    using namespace trafficsim;
    const double angle=degrees*std::numbers::pi/180;
    const double acute=std::acos(std::abs(std::cos(angle)));
    const double a=2*std::tan(acute/2);
    const Point centre{-a,2},u{std::cos(angle),std::sin(angle)};
    const Point feedEnd{centre.x-40*u.x,centre.y-40*u.y};
    Network n;n.id="four-point-mouth";
    Link main;main.id="main";main.geometry={{-100,2},{100,2}};main.lanes={{"main-1",4}};
    Link feed;feed.id="feed";feed.geometry={{feedEnd.x-25*u.x,feedEnd.y-25*u.y},feedEnd};feed.lanes={{"feed-1",4}};
    n.links={main,feed};
    Connector c;c.id="c";c.from={"feed","feed-1",{}};c.to={"main","main-1",centre.x+100};
    for(int i=4;i>=0;--i)c.geometry.push_back({centre.x-i*10*u.x,centre.y-i*10*u.y});
    n.connectors={c};return n;
}
}
