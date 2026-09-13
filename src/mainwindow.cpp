#include "mainwindow.h"

#include "keypad.h"
#include "screenview.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

void traceImpl(const QString &msg)
{
    static QFile f;
    if (!f.isOpen()) {
        QString r = qEnvironmentVariable("NIECHE_HOME");
        if (r.isEmpty())
            r = QDir::homePath() + QStringLiteral("/.nieche-emu");
        QDir().mkpath(r);
        f.setFileName(r + QStringLiteral("/emu.log"));
        f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
    if (f.isOpen()) {
        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << "  " << msg << "\n";
        ts.flush();
        f.flush();
    }
}

unsigned keyToMask(int k)
{
    switch (k) {
    case Qt::Key_W: case Qt::Key_Up:    return (1u << 2) | (1u << 17);
    case Qt::Key_S: case Qt::Key_Down:  return (1u << 8) | (1u << 18);
    case Qt::Key_A: case Qt::Key_Left:  return (1u << 4) | (1u << 15);
    case Qt::Key_D: case Qt::Key_Right: return (1u << 6) | (1u << 16);
    case Qt::Key_J: case Qt::Key_Space: case Qt::Key_Return: return (1u << 5) | (1u << 14);
    case Qt::Key_K: return 1u << 12;
    case Qt::Key_L: return 1u << 13;
    case Qt::Key_U: return 1u << 20;
    case Qt::Key_1: return 1u << 19;
    case Qt::Key_2: return 1u << 18;
    case Qt::Key_3: return 1u << 20;
    case Qt::Key_4: return 1u << 15;
    case Qt::Key_5: return 1u << 14;
    case Qt::Key_6: return 1u << 16;
    case Qt::Key_7: return 1u << 21;
    case Qt::Key_8: return 1u << 17;
    case Qt::Key_9: return 1u << 22;
    case Qt::Key_0: return 1u << 24;
    default: return 0;
    }
}

QString dataRoot()
{
    QString r = qEnvironmentVariable("NIECHE_HOME");
    if (r.isEmpty())
        r = QDir::homePath() + QStringLiteral("/.nieche-emu");
    QDir().mkpath(r);
    return r;
}
}

void MainWindow::trace(const QString &msg) { traceImpl(msg); }

MainWindow::MainWindow(const QString &autoStart)
{
    setWindowTitle(QStringLiteral("尼彩 CBE 模拟器"));
    buildUi();

    traceImpl(QStringLiteral("=== 启动 ==="));

    emu = new EmuThread(this);
    connect(emu, &EmuThread::frameReady, this, [this](const QImage &img) {
        screen->setImage(img);
    });
    connect(emu, &EmuThread::statusChanged, this,
            [this](const QString &t, int w, int h) {
                status->setText(h > 0 ? QStringLiteral("%1  %2×%3").arg(t).arg(w).arg(h) : t);
            });
    connect(emu, &EmuThread::fpsMeasured, this, [this](double f) {
        fpsReal->setText(QStringLiteral("实测 %1").arg(f, 0, 'f', 1));
    });
    connect(emu, &EmuThread::logLine, this,
            [this](const QString &l) { log->appendPlainText(l); });
    connect(emu, &EmuThread::moduleExited, this, [this] {
        status->setText(QStringLiteral("模块已退出"));
    });
    emu->setFps(fpsTarget);
    emu->start();

    refreshLibrary();

    resize(1100, 760);

    if (!autoStart.isEmpty())
        QMetaObject::invokeMethod(
            this, [this, autoStart] { startModule(autoStart); }, Qt::QueuedConnection);
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *root = new QHBoxLayout(central);
    root->setSpacing(12);

    auto *libBox = new QVBoxLayout;
    libBox->addWidget(new QLabel(QStringLiteral("游戏库")));
    library = new QListWidget(central);
    library->setMinimumWidth(200);
    connect(library, &QListWidget::itemActivated, this, [this](QListWidgetItem *it) {
        startModule(it->data(Qt::UserRole).toString());
    });
    libBox->addWidget(library, 1);
    auto *openBtn = new QPushButton(QStringLiteral("打开 .cbe…"), central);
    openBtn->setFocusPolicy(Qt::NoFocus);
    connect(openBtn, &QPushButton::clicked, this, [this] {
        const QString f = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择模块"), QString(),
            QStringLiteral("CBE 模块 (*.cbe *.CBE);;所有文件 (*)"));
        if (!f.isEmpty())
            startModule(f);
    });
    libBox->addWidget(openBtn);
    root->addLayout(libBox);

    auto *midBox = new QVBoxLayout;
    screen = new ScreenView(central);
    connect(screen, &ScreenView::touched, this, [this](int x, int y, int s) {
        emu->pushTouch(x, y, s);
    });
    midBox->addWidget(screen, 1);
    status = new QLabel(QStringLiteral("未加载模块"), central);
    status->setStyleSheet(QStringLiteral("color: palette(mid);"));
    midBox->addWidget(status);
    root->addLayout(midBox, 1);

    auto *rightBox = new QVBoxLayout;
    auto *form = new QFormLayout;

    auto *scaleRow = new QHBoxLayout;
    fitWindow = new QCheckBox(QStringLiteral("适应窗口"), central);
    fitWindow->setFocusPolicy(Qt::NoFocus);
    scaleBox = new QSpinBox(central);
    scaleBox->setRange(1, 6);
    scaleBox->setValue(2);
    scaleBox->setSuffix(QStringLiteral("×"));
    scaleBox->setFocusPolicy(Qt::NoFocus);
    auto applyScale = [this] {
        screen->setScale(fitWindow->isChecked() ? 0 : scaleBox->value());
        scaleBox->setEnabled(!fitWindow->isChecked());
    };
    connect(fitWindow, &QCheckBox::toggled, this, applyScale);
    connect(scaleBox, &QSpinBox::valueChanged, this, applyScale);
    scaleRow->addWidget(fitWindow);
    scaleRow->addWidget(scaleBox);
    form->addRow(QStringLiteral("缩放"), scaleRow);

    rotateBox = new QComboBox(central);
    rotateBox->addItems({QStringLiteral("0°"), QStringLiteral("90°"),
                         QStringLiteral("180°"), QStringLiteral("270°")});
    rotateBox->setFocusPolicy(Qt::NoFocus);
    connect(rotateBox, &QComboBox::currentIndexChanged, this,
            [this](int i) { screen->setRotate(i * 90); });
    form->addRow(QStringLiteral("旋转"), rotateBox);

    upscaleBox = new QComboBox(central);
    upscaleBox->addItems({QStringLiteral("最近邻"), QStringLiteral("平滑")});
    upscaleBox->setFocusPolicy(Qt::NoFocus);
    connect(upscaleBox, &QComboBox::currentIndexChanged, this,
            [this](int i) { screen->setSmooth(i == 1); });
    form->addRow(QStringLiteral("放大"), upscaleBox);

    auto *soundRow = new QHBoxLayout;
    soundOn = new QCheckBox(QStringLiteral("开"), central);
    soundOn->setChecked(true);
    soundOn->setFocusPolicy(Qt::NoFocus);
    volume = new QSlider(Qt::Horizontal, central);
    volume->setRange(0, 100);
    volume->setValue(70);
    volume->setFocusPolicy(Qt::NoFocus);
    soundRow->addWidget(soundOn);
    soundRow->addWidget(volume);
    form->addRow(QStringLiteral("声音"), soundRow);

    auto *fpsRow = new QHBoxLayout;
    fpsBtn = new QPushButton(central);
    fpsBtn->setFocusPolicy(Qt::NoFocus);
    connect(fpsBtn, &QPushButton::clicked, this, &MainWindow::askFps);
    fpsReal = new QLabel(central);
    fpsReal->setStyleSheet(QStringLiteral("color: palette(mid);"));
    fpsRow->addWidget(fpsBtn);
    fpsRow->addWidget(fpsReal);
    form->addRow(QStringLiteral("帧率"), fpsRow);
    rightBox->addLayout(form);

    auto *stopBtn = new QPushButton(QStringLiteral("停止"), central);
    stopBtn->setFocusPolicy(Qt::NoFocus);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::stopModule);
    rightBox->addWidget(stopBtn);

    pad = new Keypad(central);
    connect(pad, &Keypad::pressed, this, [this](unsigned m) { padMask |= m; applyKeys(); });
    connect(pad, &Keypad::released, this, [this](unsigned m) { padMask &= ~m; applyKeys(); });
    connect(pad, &Keypad::softKey, this, [this](int s) {

        emu->pushTouch(-1, -1, 10 + s);
    });
    rightBox->addWidget(pad);

    rightBox->addWidget(new QLabel(QStringLiteral("模块日志")));
    log = new QPlainTextEdit(central);
    log->setReadOnly(true);
    log->setMaximumBlockCount(400);
    log->setFocusPolicy(Qt::NoFocus);
    log->setMinimumHeight(90);
    rightBox->addWidget(log);

    auto *rightWrap = new QWidget(central);
    rightWrap->setLayout(rightBox);
    rightWrap->setFixedWidth(300);
    root->addWidget(rightWrap);

    setCentralWidget(central);

    QSettings s(QStringLiteral("nieche"), QStringLiteral("emu"));
    fpsTarget = s.value(QStringLiteral("fps"), 30).toInt();
    fpsBtn->setText(QStringLiteral("%1 fps").arg(fpsTarget));
    applyScale();
}

void MainWindow::refreshLibrary()
{
    library->clear();
    QDir d(dataRoot() + QStringLiteral("/games"));
    d.mkpath(QStringLiteral("."));
    for (const QFileInfo &fi : d.entryInfoList({QStringLiteral("*.cbe"), QStringLiteral("*.CBE")},
                                               QDir::Files, QDir::Name)) {
        auto *it = new QListWidgetItem(fi.fileName(), library);
        it->setData(Qt::UserRole, fi.absoluteFilePath());
    }
}

void MainWindow::startModule(const QString &path)
{
    traceImpl(QStringLiteral("startModule %1").arg(path));
    emu->requestStart(path);
}

void MainWindow::stopModule()
{
    emu->requestStop();
    kbMask = padMask = 0;
    emu->setKeys(0);
}

void MainWindow::applyKeys() { emu->setKeys(kbMask | padMask); }

void MainWindow::askFps()
{
    bool ok = false;

    const int v = QInputDialog::getInt(
        this, QStringLiteral("帧率"),
        QStringLiteral("这就是游戏速度：模块按帧推进，跑多快游戏就多快。\n"
                       "真机上这些游戏大概只有 10-15fps。"),
        fpsTarget, 1, 240, 1, &ok);
    if (!ok)
        return;
    fpsTarget = v;
    fpsBtn->setText(QStringLiteral("%1 fps").arg(fpsTarget));
    QSettings(QStringLiteral("nieche"), QStringLiteral("emu"))
        .setValue(QStringLiteral("fps"), fpsTarget);
    emu->setFps(fpsTarget);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    const unsigned m = keyToMask(e->key());
    if (m && !e->isAutoRepeat()) {
        kbMask |= m;
        applyKeys();
        return;
    }
    QMainWindow::keyPressEvent(e);
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    const unsigned m = keyToMask(e->key());
    if (m && !e->isAutoRepeat()) {
        kbMask &= ~m;
        applyKeys();
        return;
    }
    QMainWindow::keyReleaseEvent(e);
}
