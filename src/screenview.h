
#pragma once
#include <QImage>
#include <QWidget>

class ScreenView : public QWidget {
    Q_OBJECT
public:
    explicit ScreenView(QWidget *parent = nullptr);

    void setImage(const QImage &img);
    void setScale(int s);
    void setRotate(int deg);
    void setSmooth(bool on);
    QSize sizeHint() const override;

signals:

    void touched(int x, int y, int state);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private:
    QRect target() const;
    void emitTouch(const QPoint &p, int state);

    QImage img;
    int scale = 2;
    int rotate = 0;
    bool smooth = false;
};
