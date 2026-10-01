
#pragma once
#include <QObject>
#include <QString>

class QTimer;

class AudioOut : public QObject {
    Q_OBJECT
public:
    explicit AudioOut(QObject *parent = nullptr);
    ~AudioOut() override;

    void handle(const QString &json);
    void setEnabled(bool on);

    void setVolume(int percent);
    void stop();

    bool isEnabled() const { return on; }
    QString playing() const { return cur; }

signals:
    void logLine(const QString &line);

private:
    void play(const QString &path, bool loop);
    void poll();

    bool on = true;
    int vol = 70;
    bool looping = false;
    bool opened = false;
    QString cur;
    QString curPath;
    QTimer *timer = nullptr;
};
