
#pragma once
#include <QWidget>

class Keypad : public QWidget {
    Q_OBJECT
public:
    explicit Keypad(QWidget *parent = nullptr);

signals:
    void pressed(unsigned mask);
    void released(unsigned mask);
    void softKey(int side);
};
