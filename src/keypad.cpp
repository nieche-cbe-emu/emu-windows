#include "keypad.h"

#include <QGridLayout>
#include <QPushButton>

namespace keys {
constexpr unsigned LSK = 1u << 12;
constexpr unsigned RSK = 1u << 13;
constexpr unsigned CALL = 1u << 20;
constexpr unsigned UP = (1u << 2) | (1u << 17);
constexpr unsigned DOWN = (1u << 8) | (1u << 18);
constexpr unsigned LEFT = (1u << 4) | (1u << 15);
constexpr unsigned RIGHT = (1u << 6) | (1u << 16);
constexpr unsigned OK = (1u << 5) | (1u << 14);
constexpr unsigned K1 = 1u << 19;
constexpr unsigned K2 = 1u << 18;
constexpr unsigned K3 = 1u << 20;
constexpr unsigned K4 = 1u << 15;
constexpr unsigned K5 = 1u << 14;
constexpr unsigned K6 = 1u << 16;
constexpr unsigned K7 = 1u << 21;
constexpr unsigned K8 = 1u << 17;
constexpr unsigned K9 = 1u << 22;
constexpr unsigned STAR = 1u << 23;
constexpr unsigned K0 = 1u << 24;
constexpr unsigned POUND = 1u << 25;
}

Keypad::Keypad(QWidget *parent) : QWidget(parent)
{
    auto *g = new QGridLayout(this);
    g->setSpacing(3);
    g->setContentsMargins(0, 0, 0, 0);

    struct Item { const char *label; unsigned mask; int r, c, rs, cs; int soft; };
    static const Item items[] = {
        {"左软键", keys::LSK, 0, 0, 1, 3, 0},
        {"右软键", keys::RSK, 0, 3, 1, 3, 1},
        {"呼叫", keys::CALL, 1, 0, 1, 2, -1},
        {"▲", keys::UP, 1, 2, 1, 2, -1},
        {"挂断", 0, 1, 4, 1, 2, -1},
        {"◀", keys::LEFT, 2, 0, 1, 2, -1},
        {"OK", keys::OK, 2, 2, 1, 2, -1},
        {"▶", keys::RIGHT, 2, 4, 1, 2, -1},
        {"▼", keys::DOWN, 3, 2, 1, 2, -1},
        {"1", keys::K1, 4, 0, 1, 2, -1}, {"2", keys::K2, 4, 2, 1, 2, -1}, {"3", keys::K3, 4, 4, 1, 2, -1},
        {"4", keys::K4, 5, 0, 1, 2, -1}, {"5", keys::K5, 5, 2, 1, 2, -1}, {"6", keys::K6, 5, 4, 1, 2, -1},
        {"7", keys::K7, 6, 0, 1, 2, -1}, {"8", keys::K8, 6, 2, 1, 2, -1}, {"9", keys::K9, 6, 4, 1, 2, -1},
        {"✱", keys::STAR, 7, 0, 1, 2, -1}, {"0", keys::K0, 7, 2, 1, 2, -1}, {"#", keys::POUND, 7, 4, 1, 2, -1},
    };
    for (const Item &it : items) {
        auto *b = new QPushButton(QString::fromUtf8(it.label), this);
        b->setFocusPolicy(Qt::NoFocus);
        b->setMinimumHeight(26);
        const unsigned m = it.mask;
        const int soft = it.soft;
        connect(b, &QPushButton::pressed, this, [this, m, soft] {
            if (soft >= 0)
                emit softKey(soft);
            if (m)
                emit pressed(m);
        });
        connect(b, &QPushButton::released, this, [this, m] {
            if (m)
                emit released(m);
        });
        g->addWidget(b, it.r, it.c, it.rs, it.cs);
    }
}
