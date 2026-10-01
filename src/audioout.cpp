#include "audioout.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>

namespace {

const wchar_t *ALIAS = L"niecheBgm";

QString mciError(MCIERROR e)
{
    wchar_t buf[256] = {0};
    if (mciGetErrorStringW(e, buf, 256))
        return QString::fromWCharArray(buf);
    return QStringLiteral("MCI 错误 %1").arg(e);
}

MCIERROR mci(const QString &cmd, QString *out = nullptr)
{
    wchar_t ret[128] = {0};
    const MCIERROR e = mciSendStringW(reinterpret_cast<const wchar_t *>(cmd.utf16()),
                                      out ? ret : nullptr, out ? 128 : 0, nullptr);
    if (out)
        *out = QString::fromWCharArray(ret);
    return e;
}

QString mciPath(const QString &native)
{
    wchar_t buf[MAX_PATH] = {0};
    const DWORD n = GetShortPathNameW(reinterpret_cast<const wchar_t *>(native.utf16()),
                                      buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH)
        return QString::fromWCharArray(buf, int(n));
    return native;
}

QString mciType(const QString &path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == QLatin1String("mid") || ext == QLatin1String("midi"))
        return QStringLiteral(" type sequencer");
    if (ext == QLatin1String("wav"))
        return QStringLiteral(" type waveaudio");
    if (ext == QLatin1String("mp3"))
        return QStringLiteral(" type mpegvideo");
    return QString();
}
}
#endif

AudioOut::AudioOut(QObject *parent) : QObject(parent)
{

    timer = new QTimer(this);
    timer->setInterval(400);
    connect(timer, &QTimer::timeout, this, &AudioOut::poll);
}

AudioOut::~AudioOut() { stop(); }

void AudioOut::setEnabled(bool v)
{
    on = v;
    if (!on)
        stop();
}

void AudioOut::setVolume(int percent)
{
    vol = qBound(0, percent, 100);
#ifdef Q_OS_WIN
    if (opened)
        mci(QStringLiteral("setaudio %1 volume to %2")
                .arg(QString::fromWCharArray(ALIAS))
                .arg(vol * 10));
#endif
}

void AudioOut::handle(const QString &json)
{
    const QJsonObject o = QJsonDocument::fromJson(json.toUtf8()).object();
    if (o.value(QStringLiteral("kind")).toString() != QLatin1String("audio"))
        return;
    const QString op = o.value(QStringLiteral("op")).toString();
    if (op == QLatin1String("stop")) {
        stop();
        return;
    }
    if (op != QLatin1String("play"))
        return;
    const QString path = o.value(QStringLiteral("path")).toString();

    if (path.isEmpty())
        return;
    cur = o.value(QStringLiteral("name")).toString();
    if (!on)
        return;
    play(path, o.value(QStringLiteral("loop")).toBool());
}

void AudioOut::play(const QString &path, bool loop)
{
    stop();
    looping = loop;
    curPath = path;
#ifdef Q_OS_WIN
    const QString alias = QString::fromWCharArray(ALIAS);
    const QString native = QDir::toNativeSeparators(path);
    MCIERROR e = mci(QStringLiteral("open \"%1\"%2 alias %3")
                         .arg(mciPath(native), mciType(path), alias));
    if (e) {
        emit logLine(QStringLiteral("[音频] 打不开 %1：%2").arg(native, mciError(e)));
        return;
    }
    opened = true;
    setVolume(vol);
    e = mci(QStringLiteral("play %1").arg(alias));
    if (e) {
        emit logLine(QStringLiteral("[音频] 放不了 %1：%2").arg(native, mciError(e)));
        stop();
        return;
    }
    timer->start();
#else
    Q_UNUSED(loop);
#endif
}

void AudioOut::poll()
{
#ifdef Q_OS_WIN
    if (!opened) {
        timer->stop();
        return;
    }
    QString mode;
    const QString alias = QString::fromWCharArray(ALIAS);
    if (mci(QStringLiteral("status %1 mode").arg(alias), &mode)) {
        stop();
        return;
    }
    if (mode != QLatin1String("stopped"))
        return;
    if (!looping) {
        stop();
        return;
    }
    mci(QStringLiteral("seek %1 to start").arg(alias));
    if (mci(QStringLiteral("play %1").arg(alias)))
        stop();
#endif
}

void AudioOut::stop()
{
    if (timer)
        timer->stop();
    looping = false;
    cur.clear();
    curPath.clear();
#ifdef Q_OS_WIN
    if (!opened)
        return;
    opened = false;
    const QString alias = QString::fromWCharArray(ALIAS);
    mci(QStringLiteral("stop %1").arg(alias));
    mci(QStringLiteral("close %1").arg(alias));
#endif
}
