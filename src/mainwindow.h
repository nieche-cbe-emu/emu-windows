
#pragma once
#include <QMainWindow>

#include "core.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;
class Keypad;
class ScreenView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:

    explicit MainWindow(const QString &autoStart = QString());

protected:
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;

public:

    static void trace(const QString &msg);

private:
    void buildUi();
    void refreshLibrary();
    void startModule(const QString &path);
    void stopModule();
    void tick();
    void askFps();
    void applyKeys();

    Core core;
    QTimer *timer = nullptr;

    ScreenView *screen = nullptr;
    Keypad *pad = nullptr;
    QListWidget *library = nullptr;
    QLabel *status = nullptr;
    QLabel *fpsReal = nullptr;
    QPushButton *fpsBtn = nullptr;
    QCheckBox *fitWindow = nullptr;
    QSpinBox *scaleBox = nullptr;
    QComboBox *rotateBox = nullptr;
    QComboBox *upscaleBox = nullptr;
    QCheckBox *soundOn = nullptr;
    QSlider *volume = nullptr;
    QPlainTextEdit *log = nullptr;

    unsigned kbMask = 0;
    unsigned padMask = 0;
    int fpsTarget = 30;
    int frames = 0;
    qint64 mark = 0;
    QString title;
};
