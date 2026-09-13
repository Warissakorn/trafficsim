#include "main_window.hpp"
#include "editor_window.hpp"
#include "path.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("TrafficSim");
    QCoreApplication::setApplicationVersion(TRAFFICSIM_VERSION);
    QCommandLineParser parser;
    parser.addOption({"editor", "Open the native network editor"});
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"data-dir", "Data and locale directory", "directory"});
    parser.addOption({"scenario", "M0 authoring scenario JSON", "file"});
    parser.addOption({"language", "Interface language: en or th", "code", "en"});
    parser.process(app);
    try {
        const auto data = parser.isSet("data-dir") ? trafficsim::nativePath(parser.value("data-dir")) :
            trafficsim::findDataDirectory(trafficsim::nativePath(QCoreApplication::applicationFilePath()));
        const auto file = parser.isSet("scenario") ? trafficsim::nativePath(parser.value("scenario")) :
                                                    data / "scenarios/crossing.json";
        if (parser.value("language") != "en" && parser.value("language") != "th") throw std::invalid_argument("Unknown language");
        if (parser.isSet("editor")) {
            trafficsim::EditorWindow editor(data, parser.value("language"));
            if (parser.isSet("scenario")) editor.openFile(parser.value("scenario"));
            editor.show(); return app.exec();
        }
        trafficsim::MainWindow window(data, file, parser.value("language"));
        window.show();
        return app.exec();
    } catch (const std::exception& error) {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(), QString::fromUtf8(error.what()));
        return 1;
    }
}
