#include "../src/editor/canvas.hpp"
#include "../src/editor/ui_design_tokens.hpp"
#include "../src/shell/editor_style.hpp"
#include "../src/shell/editor_window.hpp"
#include <QAbstractAnimation>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QStyle>
#include <QTest>
#include <QToolBar>
#include <QVBoxLayout>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
// The presentation contract of the precision-tool restyle: tokens, palette roles, QSS rules,
// box model, number formatting, hairlines and the two-tier grid. Every assertion names the
// measured value so a failure on a platform is a measurement, not a guess.
using namespace trafficsim;
namespace {
void require(bool ok,const std::string& message) { if(!ok)throw std::runtime_error(message); }
double luminance(const QColor& colour) {
    const auto linear=[](double channel){return channel<=.04045?channel/12.92:std::pow((channel+.055)/1.055,2.4);};
    return .2126*linear(colour.redF())+.7152*linear(colour.greenF())+.0722*linear(colour.blueF());
}
double contrast(const QColor& a,const QColor& b) {
    const double x=luminance(a),y=luminance(b);return (std::max(x,y)+.05)/(std::min(x,y)+.05);
}
// RGB spread, not HSL saturation: near-white greys such as #F9FAFB have a large HSL saturation
// from a 2/255 spread and are still neutral. The slate greys lean blue by up to 24/255 (0.094);
// the accent and semantic colours spread by 0.4 or more, so 0.15 separates them cleanly.
bool chromatic(const QColor& c) {
    return std::max({c.redF(),c.greenF(),c.blueF()})-std::min({c.redF(),c.greenF(),c.blueF()})>.15;
}

void palette() {
    const auto p=editorDesign::editorPalette();
    require(p.color(QPalette::Highlight)==QColor("#2F6FED"),"Accent must be #2F6FED");
    // Only the accent and the four semantic roles may carry hue; every other role is neutral.
    QSet<int> hued{QPalette::Highlight,QPalette::BrightText,QPalette::LinkVisited,QPalette::Link,QPalette::Shadow};
#if QT_VERSION >= QT_VERSION_CHECK(6,6,0)
    hued.insert(QPalette::Accent);
#endif
    for(int role=0;role<QPalette::NColorRoles;++role) {
        if(role==QPalette::NoRole)continue;
        require(hued.contains(role)||!chromatic(p.color(QPalette::Active,static_cast<QPalette::ColorRole>(role))),
                "Neutral role "+std::to_string(role)+" carries hue");
    }
    // Text-bearing roles meet WCAG AA against both surfaces they sit on. The accent is only ever a
    // fill or border (4.55:1 on white, 4.1:1 on the hover grey), so it is checked as a fill.
    const auto base=p.color(QPalette::Base),hover=p.color(QPalette::Midlight);
    for(const auto role:{QPalette::Text,QPalette::WindowText,QPalette::BrightText,QPalette::LinkVisited,QPalette::Link,QPalette::Shadow}) {
        require(contrast(p.color(role),base)>=4.5,"Role "+std::to_string(role)+" below 4.5:1 on Base: "+std::to_string(contrast(p.color(role),base)));
        require(contrast(p.color(role),hover)>=4.5,"Role "+std::to_string(role)+" below 4.5:1 on Midlight: "+std::to_string(contrast(p.color(role),hover)));
    }
    require(contrast(p.color(QPalette::HighlightedText),p.color(QPalette::Highlight))>=4.5,"Text on the accent below 4.5:1");
    require(contrast(p.color(QPalette::Dark),base)>=3,"Control outline below 3:1 (WCAG 1.4.11)");
    for(const auto s:{editorDesign::Semantic::error,editorDesign::Semantic::warning,editorDesign::Semantic::advisory,editorDesign::Semantic::ok})
        require(editorDesign::semantic(s).isValid(),"Semantic colour missing");
}
void stylesheet() {
    const auto qss=editorStyleSheet();
    require(!qss.contains(QRegularExpression("#[0-9a-fA-F]{3,8}(?![0-9a-zA-Z_])")),"QSS holds a hex colour literal");
    for(const char* banned:{"gradient","box-shadow","glow","transition","animation","letter-spacing","rgb("})
        require(!qss.contains(banned),std::string("QSS uses ")+banned);
    const QSet<int> sizes{11,12,13,14,18};
    auto sizeMatches=QRegularExpression("font-size:\\s*(\\d+)px").globalMatch(qss);
    int found=0;
    while(sizeMatches.hasNext()){const int px=sizeMatches.next().captured(1).toInt();++found;
        require(sizes.contains(px),"Off-scale font size "+std::to_string(px));}
    require(found>0,"No font sizes found");
    auto radii=QRegularExpression("border-radius:\\s*(\\d+)px").globalMatch(qss);
    while(radii.hasNext()){const int px=radii.next().captured(1).toInt();
        require(px<=editorDesign::maxRadius,"Radius above 3 px: "+std::to_string(px));}
    require(editorDesign::maxMotionMs<=150,"Motion budget above 150 ms");
    for(const int space:{editorDesign::space1,editorDesign::space2,editorDesign::space3,editorDesign::space4,editorDesign::space5,editorDesign::space6})
        require(space%4==0,"Spacing token off the 4 px scale");
}
void boxModel() {
    QWidget host;applyEditorStyle(&host);auto* layout=new QVBoxLayout(&host);
    auto* edit=new QLineEdit(&host);auto* spin=new QDoubleSpinBox(&host);auto* combo=new QComboBox(&host);
    auto* button=new QPushButton("Apply",&host);combo->addItem("x");
    for(QWidget* control:{static_cast<QWidget*>(edit),static_cast<QWidget*>(spin),static_cast<QWidget*>(combo),static_cast<QWidget*>(button)})layout->addWidget(control);
    host.show();QApplication::processEvents();
    for(QWidget* control:{static_cast<QWidget*>(edit),static_cast<QWidget*>(spin),static_cast<QWidget*>(combo),static_cast<QWidget*>(button)}) {
        require(control->sizeHint().height()==editorDesign::controlHeight,
                std::string(control->metaObject()->className())+" is "+std::to_string(control->sizeHint().height())+" px, not 28");
        // The invalid state swaps a 1 px border for 2 px; padding gives 1 px back so height holds.
        control->setProperty("validationState","invalid");control->style()->unpolish(control);control->style()->polish(control);
        require(control->sizeHint().height()==editorDesign::controlHeight,
                std::string(control->metaObject()->className())+" changes height when invalid: "+std::to_string(control->sizeHint().height()));
    }
    QToolBar bar;bar.setStyleSheet(editorStyleSheet());bar.setIconSize(QSize(editorDesign::iconSize,editorDesign::iconSize));
    bar.addAction(editorIcon(EditorIcon::select),"Select");bar.show();QApplication::processEvents();
    require(bar.sizeHint().height()==editorDesign::toolbarHeight,"Toolbar is "+std::to_string(bar.sizeHint().height())+" px, not 32");
    require(editorIcon(EditorIcon::select).actualSize(QSize(64,64),QIcon::Normal).width()>=editorDesign::iconSize,"Icon raster smaller than 16 px");
}
void typography() {
    const auto numeric=editorDesign::numericFont();
    require(numeric.fixedPitch()&&numeric.pixelSize()==12,"Numeric face is not 12 px fixed pitch");
    QFont english;editorDesign::styleGroupLabel(english,true);
    require(english.pixelSize()==11&&english.capitalization()==QFont::AllUppercase&&english.letterSpacingType()==QFont::AbsoluteSpacing&&
            english.letterSpacing()==editorDesign::labelTracking,"English group label is not 11 px tracked uppercase");
    QFont thai;editorDesign::styleGroupLabel(thai,false);
    require(thai.capitalization()==QFont::MixedCase&&thai.letterSpacingType()==QFont::PercentageSpacing&&thai.letterSpacing()==100.,
            "Thai group label must stay untracked");
    const QLocale german(QLocale::German,QLocale::Germany),english_(QLocale::English,QLocale::UnitedStates);
    const auto nbsp=QString(QChar(0x00A0));
    require(editorDesign::formatValue(1234.5,2,"m",german)=="1.234,50"+nbsp+"m","German format: "+editorDesign::formatValue(1234.5,2,"m",german).toStdString());
    require(editorDesign::formatValue(1234.5,2,"m",english_)=="1,234.50"+nbsp+"m","English format wrong");
    require(editorDesign::formatValue(3,3,QString(),english_)=="3.000","Fixed decimals lost");
}
void hairlines() {
    const auto pen=editorDesign::hairlinePen(QColor("#E5E7EB"),2.);
    require(pen.isCosmetic()&&std::abs(pen.widthF()-.5)<1e-12,"Hairline is not one device pixel at 2x");
    require(std::abs(editorDesign::hairlinePen(QColor(),1.).widthF()-1)<1e-12&&std::abs(editorDesign::hairlinePen(QColor(),0.).widthF()-1)<1e-12,"Hairline at 1x/invalid dpr");
    // Every snapped coordinate lands on a device-pixel centre, for either axis sign and any offset.
    for(const double scale:{4.,-4.,1.5,-.25,8.})for(const double offset:{0.,3.3,-120.7,401.})for(double scene=-50.3;scene<50;scene+=7.31) {
        const double device=editorDesign::snapHairline(scene,scale,offset)*scale+offset;
        require(std::abs(device-std::floor(device)-.5)<1e-9,"Hairline not on a device pixel centre (scale "+std::to_string(scale)+")");
    }
    require(editorDesign::snapHairline(5,0,0)==5,"Zero scale must pass the coordinate through");
}
void gridTiers() {
    require(std::abs(editorDesign::levelOfDetail(QTransform::fromScale(4,-4))-4)<1e-12,"LOD of a 4x flipped view is not 4");
    auto tiers=editorDesign::gridTiers(1,4);
    require(tiers.minor==10&&tiers.major==100,"4 px/m: minor must jump to 10 m");
    tiers=editorDesign::gridTiers(1,10);require(tiers.minor==1&&tiers.major==10,"10 px/m keeps the 1 m minor");
    tiers=editorDesign::gridTiers(1,.05);require(tiers.minor*.05>=editorDesign::minorMinPixels&&tiers.minor==1000,"Zoomed-out minor spacing below 8 px");
    tiers=editorDesign::gridTiers(.1,100);require(tiers.minor==.1,"Fine grid dropped at high zoom");
    for(const double bad:{0.,-1.,std::nan(""),std::numeric_limits<double>::infinity()}) {
        require(editorDesign::gridTiers(bad,4).minor==0,"Bad step accepted");require(editorDesign::gridTiers(1,bad).minor==0,"Bad zoom accepted");
    }
}
// An empty canvas paints only the background, so every pixel must be exactly one of the three
// neutral roles: a blended pixel means a line straddled two device pixels.
void gridIsCrisp() {
    EditorCanvas canvas;canvas.resize(420,300);canvas.show();QApplication::processEvents();
    const auto image=canvas.viewport()->grab().toImage().convertToFormat(QImage::Format_RGB32);
    const QSet<QRgb> allowed{editorDesign::role(QPalette::Base).rgb(),editorDesign::role(QPalette::Midlight).rgb(),editorDesign::role(QPalette::Mid).rgb()};
    int lines=0;
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
        const auto pixel=image.pixel(x,y)|0xFF000000u;
        require(allowed.contains(pixel),"Blended grid pixel "+QColor(pixel).name().toStdString()+" at "+std::to_string(x)+","+std::to_string(y));
        lines+=pixel!=editorDesign::role(QPalette::Base).rgb();
    }
    require(lines>0,"Grid drew nothing");
}
void window(const std::filesystem::path& data) {
    EditorWindow w{data};w.show();QApplication::processEvents();
    int buddies=0;
    for(auto* label:w.findChildren<QLabel*>())if(auto* field=label->buddy()) {
        ++buddies;require(field->focusPolicy()!=Qt::NoFocus,"Buddy field cannot take focus: "+field->objectName().toStdString());
    }
    require(buddies>5,"Inspector labels lost their buddies");
    require(w.findChildren<QAbstractAnimation*>().isEmpty(),"The editor owns an animation");
    require(w.palette().color(QPalette::Highlight)==QColor("#2F6FED"),"Window did not take the editor palette");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        require(argc>=2,"Expected data directory");
        palette();stylesheet();boxModel();typography();hairlines();gridTiers();gridIsCrisp();window(argv[1]);
        std::cout<<"Palette roles, QSS rules, 28/32 px box model, formatting, hairlines and grid tiers passed\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
