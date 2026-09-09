#include "core.h"

#include <QFileInfo>
#include <QLibrary>
#include <QSize>

extern "C" {
typedef uint32_t (*fn_abi)();
typedef void *(*fn_open)(const char *);
typedef void (*fn_close)(void *);
typedef int32_t (*fn_boot)(void *);
typedef void (*fn_stop)(void *);
typedef void (*fn_size)(void *, uint32_t *, uint32_t *);
typedef size_t (*fn_step)(void *, uint8_t *, size_t);
typedef void (*fn_keys)(void *, uint32_t);
typedef void (*fn_touch)(void *, int32_t, int32_t, int32_t);
typedef void (*fn_soft)(void *, int32_t);
typedef size_t (*fn_name)(void *, uint8_t *, size_t);
typedef size_t (*fn_events)(void *, uint8_t *, size_t);
}

namespace {
fn_abi p_abi = nullptr;
fn_open p_open = nullptr;
fn_close p_close = nullptr;
fn_boot p_boot = nullptr;
fn_stop p_stop = nullptr;
fn_size p_size = nullptr;
fn_step p_step = nullptr;
fn_keys p_keys = nullptr;
fn_touch p_touch = nullptr;
fn_soft p_soft = nullptr;
fn_name p_name = nullptr;
fn_events p_events = nullptr;
QLibrary qlib;
}

Core::~Core() { close(); }

bool Core::load()
{
    if (lib)
        return true;

    const QStringList cands = {
        QStringLiteral("nieche"),
        QStringLiteral("./nieche"),
    };
    for (const QString &c : cands) {
        qlib.setFileName(c);
        if (qlib.load())
            break;
    }
    if (!qlib.isLoaded()) {
        err = QStringLiteral("找不到 nieche.dll：%1").arg(qlib.errorString());
        return false;
    }
    auto R = [&](const char *n) { return qlib.resolve(n); };
    p_abi = (fn_abi)R("nieche_abi_version");
    p_open = (fn_open)R("nieche_open");
    p_close = (fn_close)R("nieche_close");
    p_boot = (fn_boot)R("nieche_boot");
    p_stop = (fn_stop)R("nieche_stop");
    p_size = (fn_size)R("nieche_size");
    p_step = (fn_step)R("nieche_step");
    p_keys = (fn_keys)R("nieche_set_keys");
    p_touch = (fn_touch)R("nieche_set_touch");
    p_soft = (fn_soft)R("nieche_soft_key");
    p_name = (fn_name)R("nieche_name");
    p_events = (fn_events)R("nieche_take_events");
    if (!p_abi || !p_open || !p_step) {
        err = QStringLiteral("nieche.dll 里缺少必要的符号");
        return false;
    }
    if (p_abi() != 2) {
        err = QStringLiteral("核心 ABI 是 %1，本程序要求 2").arg(p_abi());
        return false;
    }
    lib = &qlib;
    return true;
}

uint32_t Core::abiVersion() const { return p_abi ? p_abi() : 0; }

bool Core::open(const QString &path)
{
    close();
    const QByteArray p = path.toUtf8();
    sess = p_open(p.constData());
    if (!sess) {
        err = QStringLiteral("打不开模块");
        return false;
    }
    if (p_boot(sess) != 1) {
        err = QStringLiteral("引导失败");
        close();
        return false;
    }
    return true;
}

void Core::close()
{
    if (sess) {
        if (p_stop)
            p_stop(sess);
        p_close(sess);
        sess = nullptr;
    }
}

QByteArray Core::step()
{
    if (!sess)
        return {};

    const size_t need = p_step(sess, nullptr, 0);
    if (buf.size() < (int)need)
        buf.resize((int)need);
    const size_t n = p_step(sess, (uint8_t *)buf.data(), buf.size());
    return QByteArray::fromRawData(buf.constData(), (int)n);
}

void Core::setKeys(uint32_t m)
{
    if (sess)
        p_keys(sess, m);
}
void Core::setTouch(int x, int y, int s)
{
    if (sess)
        p_touch(sess, x, y, s);
}
void Core::softKey(int side)
{
    if (sess && p_soft)
        p_soft(sess, side);
}

QSize Core::size() const
{
    uint32_t w = 240, h = 400;
    if (sess)
        p_size(sess, &w, &h);
    return QSize((int)w, (int)h);
}

QString Core::name() const
{
    if (!sess || !p_name)
        return {};
    const size_t n = p_name(sess, nullptr, 0);
    QByteArray b((int)n, 0);
    p_name(sess, (uint8_t *)b.data(), b.size());
    return QString::fromUtf8(b);
}

QStringList Core::takeEvents()
{
    if (!sess || !p_events)
        return {};
    const size_t n = p_events(sess, nullptr, 0);
    if (!n)
        return {};
    QByteArray b((int)n, 0);
    p_events(sess, (uint8_t *)b.data(), b.size());
    return QString::fromUtf8(b).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
}
