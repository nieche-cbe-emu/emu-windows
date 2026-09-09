#include "emuthread.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QFileInfo>

#include "mainwindow.h"

EmuThread::EmuThread(QObject *parent) : QThread(parent) {}

EmuThread::~EmuThread()
{
    requestQuit();
    wait(3000);
}

void EmuThread::requestStart(const QString &path)
{
    QMutexLocker g(&m);
    pendingPath = path;
}
void EmuThread::requestStop()
{
    QMutexLocker g(&m);
    wantStop = true;
}
void EmuThread::requestQuit()
{
    QMutexLocker g(&m);
    quitting = true;
}
void EmuThread::setKeys(unsigned mask)
{
    QMutexLocker g(&m);
    keys = mask;
}
void EmuThread::pushTouch(int x, int y, int state)
{
    QMutexLocker g(&m);

    touches.append({x, y, state});
}
void EmuThread::setFps(int v)
{
    QMutexLocker g(&m);
    fps = qBound(1, v, 240);
}

void EmuThread::run()
{
    core.setTracer(&MainWindow::trace);
    MainWindow::trace(QStringLiteral("[emu] 线程启动"));
    if (!core.load()) {
        emit logLine(core.errorString());
        return;
    }
    emit logLine(QStringLiteral("核心已加载（ABI %1）").arg(core.abiVersion()));

    QElapsedTimer clock;
    clock.start();
    qint64 nextAt = 0;
    int frames = 0;
    qint64 mark = clock.elapsed();
    QString title;

    forever {
        QString toStart;
        bool doStop = false, doQuit = false;
        int curFps;
        unsigned curKeys;
        QVector<std::array<int, 3>> ts;
        {
            QMutexLocker g(&m);
            toStart = pendingPath;
            pendingPath.clear();
            doStop = wantStop;
            wantStop = false;
            doQuit = quitting;
            curFps = fps;
            curKeys = keys;
            ts.swap(touches);
        }
        if (doQuit)
            break;
        if (doStop) {
            core.close();
            emit statusChanged(QStringLiteral("已停止"), 0, 0);
        }
        if (!toStart.isEmpty()) {
            MainWindow::trace(QStringLiteral("[emu] 准备打开 %1").arg(toStart));
            if (core.open(toStart)) {
                title = QFileInfo(toStart).fileName();
                const QSize s = core.size();
                emit statusChanged(title, s.width(), s.height());
                frames = 0;
                mark = clock.elapsed();
                nextAt = clock.elapsed();
            } else {
                emit logLine(core.errorString());
                emit statusChanged(core.errorString(), 0, 0);
            }
        }

        if (!core.booted()) {
            msleep(20);
            continue;
        }

        const qint64 now = clock.elapsed();
        if (now < nextAt) {
            msleep(qMin<qint64>(5, nextAt - now));
            continue;
        }
        nextAt = now + qMax(1, 1000 / curFps);

        core.setKeys(curKeys);
        for (const auto &t : ts) {

            if (t[2] >= 10)
                core.softKey(t[2] - 10);
            else
                core.setTouch(t[0], t[1], t[2]);
        }

        const QByteArray px = core.step();
        const QSize s = core.size();
        if (!px.isEmpty() && s.width() > 0) {

            QImage img((const uchar *)px.constData(), s.width(), s.height(),
                       s.width() * 2, QImage::Format_RGB16);
            emit frameReady(img.copy());
        }

        bool bye = false;
        for (const QString &e : core.takeEvents()) {
            if (e.contains(QStringLiteral("\"exit\"")))
                bye = true;
            else if (e.contains(QStringLiteral("\"log\"")))
                emit logLine(e);
        }
        if (bye) {
            core.close();
            emit moduleExited();
            continue;
        }

        ++frames;
        const qint64 el = clock.elapsed() - mark;
        if (el >= 1000) {
            emit fpsMeasured(frames * 1000.0 / el);
            frames = 0;
            mark = clock.elapsed();
        }
    }
    core.close();
}
