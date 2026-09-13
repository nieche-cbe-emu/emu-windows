#include "screenview.h"

#include <QMouseEvent>
#include <QPainter>

ScreenView::ScreenView(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void ScreenView::setImage(const QImage &frame)
{
    if (frame.isNull())
        return;
    img = frame;
    update();
}

void ScreenView::setScale(int s) { scale = s; updateGeometry(); update(); }
void ScreenView::setRotate(int d) { rotate = d; updateGeometry(); update(); }
void ScreenView::setSmooth(bool on) { smooth = on; update(); }

QSize ScreenView::sizeHint() const
{
    if (img.isNull())
        return QSize(240 * 2, 400 * 2);
    QSize s = img.size();
    if (rotate == 90 || rotate == 270)
        s.transpose();
    return scale > 0 ? s * scale : s;
}

QRect ScreenView::target() const
{
    if (img.isNull())
        return QRect();
    QSize s = img.size();
    if (rotate == 90 || rotate == 270)
        s.transpose();
    if (scale > 0) {
        s *= scale;
    } else {

        const int k = qMax(1, qMin(width() / s.width(), height() / s.height()));
        s *= k;
    }
    return QRect(QPoint((width() - s.width()) / 2, (height() - s.height()) / 2), s);
}

void ScreenView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), palette().window());
    if (img.isNull())
        return;
    p.setRenderHint(QPainter::SmoothPixmapTransform, smooth);
    const QRect t = target();
    if (rotate) {
        p.translate(t.center());
        p.rotate(rotate);
        p.translate(-t.center());
    }
    p.drawImage(t, img);
}

void ScreenView::emitTouch(const QPoint &pt, int state)
{
    const QRect t = target();
    if (img.isNull() || !t.contains(pt))
        return;

    double fx = double(pt.x() - t.left()) / t.width();
    double fy = double(pt.y() - t.top()) / t.height();
    switch (rotate) {
    case 90:  { double a = fx; fx = fy; fy = 1.0 - a; break; }
    case 180: { fx = 1.0 - fx; fy = 1.0 - fy; break; }
    case 270: { double a = fx; fx = 1.0 - fy; fy = a; break; }
    default: break;
    }
    emit touched(int(fx * img.width()), int(fy * img.height()), state);
}

void ScreenView::mousePressEvent(QMouseEvent *e) { emitTouch(e->pos(), 0); }
void ScreenView::mouseReleaseEvent(QMouseEvent *e) { emitTouch(e->pos(), 1); }
void ScreenView::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
        emitTouch(e->pos(), 2);
}
