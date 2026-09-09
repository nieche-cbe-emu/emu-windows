
#pragma once
#include <QImage>
#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QVector>

#include "core.h"

class EmuThread : public QThread {
    Q_OBJECT
public:
    explicit EmuThread(QObject *parent = nullptr);
    ~EmuThread() override;

    void requestStart(const QString &path);
    void requestStop();
    void requestQuit();
    void setKeys(unsigned mask);
    void pushTouch(int x, int y, int state);
    void setFps(int fps);

signals:
    void frameReady(const QImage &img);
    void statusChanged(const QString &title, int w, int h);
    void fpsMeasured(double fps);
    void logLine(const QString &line);
    void moduleExited();

protected:
    void run() override;

private:
    Core core;
    QMutex m;
    QString pendingPath;
    bool wantStop = false;
    bool quitting = false;
    unsigned keys = 0;
    QVector<std::array<int, 3>> touches;
    int fps = 30;
};
