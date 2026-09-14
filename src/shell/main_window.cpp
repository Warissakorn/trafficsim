#include "main_window.hpp"
#include "path.hpp"
#include "editor_window.hpp"
#include <QMenuBar>
#include <QAction>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QGridLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QAbstractButton>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <algorithm>

namespace trafficsim {
MainWindow::MainWindow(const std::filesystem::path& data, const std::filesystem::path& scenario, const QString& language)
    : data_(data) {
    const auto fontFile = data / "fonts/NotoSansThai.ttf";
    const int fontId = QFontDatabase::addApplicationFont(displayPath(fontFile));
    if (fontId < 0) throw std::runtime_error("Cannot load bundled Thai font");
    const auto families = QFontDatabase::applicationFontFamilies(fontId);
    if (!families.isEmpty()) setFont(QFont(families.front(), 10));
    for (const auto* code : {"en", "th"}) {
        QFile file(displayPath(data / "locales" / (std::string(code) + ".json")));
        if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot load locale " + std::string(code));
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid locale file");
        locales_[code] = document.object();
    }
    for (auto it = locales_.at("en").begin(); it != locales_.at("en").end(); ++it)
        if (!it.value().isString() || !locales_.at("th").value(it.key()).isString()) throw std::runtime_error("Locale key mismatch");
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(18, 18, 18, 18); layout->setSpacing(12);
    validation_ = new QLabel(central); validation_->setWordWrap(true); validation_->setObjectName("validation");
    validation_->setStyleSheet("background:#fff3cd; color:#614700; padding:10px; border-radius:5px;");
    layout->addWidget(validation_);
    auto* controls = new QGridLayout;
    run_ = new QPushButton(central); run_->setObjectName("run");
    step_ = new QPushButton(central); step_->setObjectName("step");
    reset_ = new QPushButton(central); reset_->setObjectName("reset");
    open_ = new QPushButton(central); open_->setObjectName("open");
    controls->addWidget(run_, 0, 0); controls->addWidget(step_, 0, 1);
    controls->addWidget(reset_, 0, 2); controls->addWidget(open_, 0, 3);
    seed_ = new QLineEdit("42", central); seed_->setObjectName("seed"); seed_->setMaxLength(10);
    speed_ = new QComboBox(central); speed_->setObjectName("speed");
    for (double speed : {0.5, 1.0, 2.0, 5.0, 10.0}) speed_->addItem(QString::number(speed) + QString::fromUtf8("×"), speed);
    speed_->setCurrentIndex(1);
    language_ = new QComboBox(central); language_->setObjectName("language");
    language_->addItem(locales_.at("en").value("english").toString(), "en");
    language_->addItem(locales_.at("en").value("thai").toString(), "th");
    language_->setCurrentIndex(language == "th" ? 1 : 0);
    int column = 0;
    for (const auto* key : {"seed", "speed", "language"}) {
        auto* label = new QLabel(central); labels_[key] = label;
        controls->addWidget(label, 1, column++);
    }
    controls->addWidget(seed_, 2, 0); controls->addWidget(speed_, 2, 1);
    controls->addWidget(language_, 2, 2, 1, 2);
    labels_["seed"]->setBuddy(seed_); labels_["speed"]->setBuddy(speed_); labels_["language"]->setBuddy(language_);
    layout->addLayout(controls);
    error_ = new QLabel(central); error_->setWordWrap(true); error_->setObjectName("error");
    error_->setStyleSheet("color:#ac263c"); error_->hide(); layout->addWidget(error_);
    view_ = new NetworkView(central); layout->addWidget(view_, 1);
    auto* stats = new QGridLayout;
    int i = 0;
    for (const auto* key : {"time", "active", "pending", "completed", "delay", "clamps"}) {
        auto* label = new QLabel(central); auto* value = new QLabel(central);
        value->setObjectName(key); value->setStyleSheet("font-size:20px; font-weight:600;");
        labels_[key] = label; values_[key] = value;
        label->setWordWrap(true);
        stats->addWidget(label, (i / 3) * 2, i % 3); stats->addWidget(value, (i / 3) * 2 + 1, i % 3); ++i;
    }
    layout->addLayout(stats);
    scope_ = new QLabel(central); scope_->setWordWrap(true); layout->addWidget(scope_);
    setCentralWidget(central); resize(1024, 800);
    connect(run_, &QPushButton::clicked, this, [this] {
        if (timer_.isActive()) pause();
        else { elapsed_.start(); credit_ = 0; timer_.start(16); refresh(); }
    });
    connect(step_, &QPushButton::clicked, this, [this] { step(); });
    connect(reset_, &QPushButton::clicked, this, [this] { reset(); });
    connect(seed_, &QLineEdit::textChanged, this, [this] { seedChanged(); });
    connect(language_, &QComboBox::currentIndexChanged, this, [this] { changeLanguage(); });
    connect(&timer_, &QTimer::timeout, this, [this] { tick(); });
    connect(open_, &QPushButton::clicked, this, [this] {
        pause();
        const auto file = QFileDialog::getOpenFileName(this, text("open"), displayPath(data_ / "scenarios"), text("jsonFilter"));
        if (file.isEmpty()) return;
        try { loadFile(nativePath(file)); error_->hide(); }
        catch (const std::exception& e) { showLoadError(e, file); }
    });
    auto* editorAction = menuBar()->addAction(text("editorTitle"));
    editorAction->setObjectName("launchEditor");
    connect(editorAction, &QAction::triggered, this, [this] { openEditor(); });
    loadFile(scenario);
    changeLanguage();
}
void MainWindow::openEditor(const QString& file) {
    pause();
    auto* editor = new EditorWindow(data_, language_->currentData().toString(), this);
    editor->setAttribute(Qt::WA_DeleteOnClose); editor->setWindowFlag(Qt::Window);
    // A file this window rejected can still fail here (a project is validated on open). Say so
    // rather than presenting an empty editor as if the drawing had loaded.
    editor->show();
    if (!file.isEmpty()) editor->openFileOrReport(file); // the editor translates its own failure
}
void MainWindow::openScenario(const std::filesystem::path& file) {
    try { loadFile(file); error_->hide(); }
    catch (const std::exception& e) { showLoadError(e, displayPath(file)); }
}
QString MainWindow::explain(const std::exception& error) const {
    // Error codes are locale keys. Every path that shows one to a user comes through here, so a
    // bare identifier like SCENARIO_IS_PROJECT can never reach a dialog untranslated.
    const auto* load = dynamic_cast<const ScenarioLoadError*>(&error);
    const auto key = load ? load->code : std::string(error.what());
    const auto translated = key.empty() ? QString{} : text(key.c_str());
    if (!translated.isEmpty()) return translated;
    return QString::fromUtf8(load ? load->detail.c_str() : error.what());
}
void MainWindow::showLoadError(const std::exception& error, const QString& file) {
    // A network drawn in the editor is a project, not a runnable scenario. Saying so — and
    // offering the window that can open it — is the whole answer; a parser message is not.
    const auto* load = dynamic_cast<const ScenarioLoadError*>(&error);
    error_->setText(text("loadError") + "\n" + file + "\n" + explain(error));
    error_->show();
    if (!load || load->code != "SCENARIO_IS_PROJECT") return;
    QMessageBox box(QMessageBox::Information, text("title"), text("SCENARIO_IS_PROJECT"), QMessageBox::Open | QMessageBox::Cancel, this);
    box.button(QMessageBox::Open)->setText(text("openInEditor"));
    box.button(QMessageBox::Cancel)->setText(text("editorCancel"));
    box.setDefaultButton(QMessageBox::Open);
    if (box.exec() == QMessageBox::Open) openEditor(file);
}
QString MainWindow::text(const char* key) const {
    const auto language = language_->currentData().toString();
    return locales_.at(language).value(key).toString();
}
void MainWindow::loadFile(const std::filesystem::path& file) {
    auto loaded = loadScenario(file, data_); // Commit only after parsing and validation succeeds.
    pause(); loaded_ = std::move(loaded);
    { const QSignalBlocker blocker(seed_); seed_->setText("42"); }
    view_->setNetwork(loaded_.network); reset();
}
void MainWindow::changeLanguage() {
    if (auto* action = findChild<QAction*>("launchEditor")) action->setText(text("editorTitle"));
    setWindowTitle(text("title") + " — " + text("subtitle"));
    validation_->setText(text("validation")); scope_->setText(text("scope"));
    for (const auto& [key, label] : labels_) label->setText(text(key.c_str()));
    step_->setText(text("step")); reset_->setText(text("reset")); open_->setText(text("open"));
    view_->setAccessibleName(text("canvas"));
    seed_->setAccessibleName(text("seed")); speed_->setAccessibleName(text("speed")); language_->setAccessibleName(text("language"));
    if (error_->property("invalidSeed").toBool()) error_->setText(text("invalidSeed"));
    refresh();
}
void MainWindow::pause() { timer_.stop(); credit_ = 0; refresh(); }
void MainWindow::seedChanged() {
    pause();
    try { (void)parseSeed(seed_->text().toStdString()); reset(); }
    catch (const std::exception&) {
        error_->setProperty("invalidSeed", true); error_->setText(text("invalidSeed")); error_->show(); refresh();
    }
}
void MainWindow::reset() {
    timer_.stop(); credit_ = 0;
    state_ = createSimulation(loaded_.scenario, parseSeed(seed_->text().toStdString()));
    summary_ = {};
    for (const auto& event : state_.events) summary_.add(event);
    error_->setProperty("invalidSeed", false); error_->hide();
    view_->setFrame(state_); refresh();
}
void MainWindow::step() {
    if (state_.tick >= totalTicks(*state_.scenario)) return;
    state_ = stepSimulation(state_);
    for (const auto& event : state_.events) summary_.add(event);
    if (state_.tick == totalTicks(*state_.scenario)) timer_.stop();
    view_->setFrame(state_); refresh();
}
void MainWindow::tick() {
    // Wall time controls playback only; dt and event timestamps stay in core.
    credit_ += std::min(0.25, elapsed_.restart() / 1000.0) * speed_->currentData().toDouble();
    while (credit_ >= state_.scenario->timeStep && timer_.isActive()) {
        credit_ -= state_.scenario->timeStep;
        step();
    }
}
void MainWindow::refresh() {
    if (!state_.scenario) return;
    const bool invalid = error_->property("invalidSeed").toBool();
    const bool done = state_.tick >= totalTicks(*state_.scenario);
    run_->setText(text(timer_.isActive() ? "pause" : "run"));
    run_->setEnabled(!invalid && !done); step_->setEnabled(!invalid && !done && !timer_.isActive());
    reset_->setEnabled(!invalid);
    values_.at("time")->setText(QString::number(state_.time, 'f', 1));
    values_.at("active")->setText(QString::number(state_.vehicles.size()));
    values_.at("pending")->setText(QString::number(pendingCount(state_)));
    values_.at("completed")->setText(QString::number(state_.completed));
    const auto summary = summary_.summary();
    values_.at("delay")->setText(summary.meanDelay ? QString::number(*summary.meanDelay, 'f', 2) : text("empty"));
    values_.at("clamps")->setText(QString::number(summary.safetyClamps));
}
}
