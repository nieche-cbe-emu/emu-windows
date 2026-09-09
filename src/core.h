
#pragma once
#include <QByteArray>
#include <QSize>
#include <QString>
#include <QStringList>
#include <cstdint>

class Core {
public:
    ~Core();

    bool load();
    QString errorString() const { return err; }
    uint32_t abiVersion() const;

    bool open(const QString &path);
    void close();
    bool booted() const { return sess != nullptr; }

    QByteArray step();
    void setKeys(uint32_t mask);

    void setTouch(int x, int y, int state);
    void softKey(int side);
    QSize size() const;
    QString name() const;
    QStringList takeEvents();

private:
    void *lib = nullptr;
    void *sess = nullptr;
    QString err;
    QByteArray buf;
};
