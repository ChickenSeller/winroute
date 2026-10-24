#include "Icons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <cmath>
#include <functional>

namespace icons {

namespace {

using Painter = std::function<void(QPainter &, double s)>;

// Renders the drawing at several sizes; `s` is the edge length in pixels.
QIcon render(const Painter &draw)
{
    QIcon icon;
    for (int size : {16, 20, 24, 32, 40, 48, 64, 128, 256}) {
        QPixmap pm(size, size);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        draw(p, size);
        p.end();
        icon.addPixmap(pm);
    }
    return icon;
}

void drawGlobe(QPainter &p, const QRectF &r, const QColor &base)
{
    QRadialGradient g(r.center() - QPointF(r.width() * 0.2, r.height() * 0.2), r.width() * 0.7);
    g.setColorAt(0, base.lighter(150));
    g.setColorAt(1, base.darker(120));
    p.setPen(QPen(base.darker(160), qMax(1.0, r.width() / 24)));
    p.setBrush(g);
    p.drawEllipse(r);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(255, 255, 255, 150), qMax(1.0, r.width() / 28)));
    p.drawEllipse(r.adjusted(r.width() * 0.3, 0, -r.width() * 0.3, 0));
    p.drawLine(QPointF(r.left(), r.center().y()), QPointF(r.right(), r.center().y()));
    p.drawLine(QPointF(r.center().x(), r.top()), QPointF(r.center().x(), r.bottom()));
}

void drawArrow(QPainter &p, QPointF from, QPointF to, double width, const QColor &c)
{
    const QLineF line(from, to);
    const double head = width * 2.2;
    QLineF shaft = line;
    shaft.setLength(line.length() - head * 0.8);
    p.setPen(QPen(c, width, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(shaft);
    QLineF n = line.normalVector();
    n.setLength(head * 0.75);
    QLineF back(to, from);
    back.setLength(head);
    const QPointF base = back.p2();
    const QPointF off = n.p2() - n.p1();
    QPainterPath tri;
    tri.moveTo(to);
    tri.lineTo(base + off);
    tri.lineTo(base - off);
    tri.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawPath(tri);
}

void drawServer(QPainter &p, const QRectF &r, const QColor &c)
{
    const double h = r.height() / 3.2;
    for (int i = 0; i < 3; ++i) {
        QRectF unit(r.left(), r.top() + i * (h * 1.1), r.width(), h);
        QLinearGradient g(unit.topLeft(), unit.bottomLeft());
        g.setColorAt(0, c.lighter(140));
        g.setColorAt(1, c.darker(110));
        p.setPen(QPen(c.darker(170), qMax(1.0, r.width() / 30)));
        p.setBrush(g);
        p.drawRoundedRect(unit, h * 0.2, h * 0.2);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(80, 220, 90));
        p.drawEllipse(QPointF(unit.right() - h * 0.5, unit.center().y()), h * 0.15, h * 0.15);
    }
}

} // namespace

QIcon app(bool active)
{
    return render([active](QPainter &p, double s) {
        const QColor base = active ? QColor(30, 120, 220) : QColor(140, 140, 140);
        drawGlobe(p, QRectF(s * 0.06, s * 0.06, s * 0.88, s * 0.88), base);
        drawArrow(p, QPointF(s * 0.22, s * 0.72), QPointF(s * 0.80, s * 0.28), s * 0.11, active ? QColor(255, 170, 30) : QColor(230, 230, 230));
    });
}

QIcon proxyServers()
{
    return render([](QPainter &p, double s) {
        drawServer(p, QRectF(s * 0.12, s * 0.08, s * 0.76, s * 0.84), QColor(110, 130, 160));
    });
}

QIcon rules()
{
    return render([](QPainter &p, double s) {
        QRectF page(s * 0.14, s * 0.06, s * 0.62, s * 0.86);
        p.setPen(QPen(QColor(120, 120, 120), qMax(1.0, s / 32)));
        p.setBrush(QColor(250, 250, 245));
        p.drawRoundedRect(page, s * 0.05, s * 0.05);
        for (int i = 0; i < 4; ++i) {
            const double y = page.top() + page.height() * (0.2 + i * 0.2);
            p.setPen(Qt::NoPen);
            p.setBrush(i == 1 ? QColor(220, 60, 50) : QColor(60, 170, 70));
            p.drawEllipse(QPointF(page.left() + page.width() * 0.18, y), s * 0.045, s * 0.045);
            p.setPen(QPen(QColor(100, 100, 110), qMax(1.0, s / 22)));
            p.drawLine(QPointF(page.left() + page.width() * 0.33, y), QPointF(page.right() - page.width() * 0.12, y));
        }
        drawArrow(p, QPointF(s * 0.70, s * 0.88), QPointF(s * 0.94, s * 0.56), s * 0.09, QColor(30, 120, 220));
    });
}

QIcon nameResolution()
{
    return render([](QPainter &p, double s) {
        drawGlobe(p, QRectF(s * 0.06, s * 0.06, s * 0.70, s * 0.70), QColor(40, 160, 90));
        QFont f;
        f.setBold(true);
        f.setPixelSize(int(s * 0.34));
        p.setFont(f);
        QRectF tag(s * 0.36, s * 0.58, s * 0.62, s * 0.38);
        p.setPen(QPen(QColor(60, 60, 60), qMax(1.0, s / 40)));
        p.setBrush(QColor(255, 236, 140));
        p.drawRoundedRect(tag, s * 0.06, s * 0.06);
        p.setPen(QColor(60, 60, 60));
        p.drawText(tag, Qt::AlignCenter, "IP");
    });
}

QIcon checker()
{
    return render([](QPainter &p, double s) {
        drawServer(p, QRectF(s * 0.06, s * 0.06, s * 0.56, s * 0.64), QColor(110, 130, 160));
        p.setPen(QPen(QColor(50, 50, 60), s * 0.09, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(s * 0.72, s * 0.72), QPointF(s * 0.92, s * 0.92));
        p.setPen(QPen(QColor(50, 50, 60), s * 0.06));
        p.setBrush(QColor(200, 230, 255, 200));
        p.drawEllipse(QPointF(s * 0.6, s * 0.6), s * 0.2, s * 0.2);
    });
}

QIcon disconnect()
{
    return render([](QPainter &p, double s) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(210, 50, 45));
        p.drawEllipse(QRectF(s * 0.1, s * 0.1, s * 0.8, s * 0.8));
        p.setPen(QPen(Qt::white, s * 0.12, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(s * 0.34, s * 0.34), QPointF(s * 0.66, s * 0.66));
        p.drawLine(QPointF(s * 0.66, s * 0.34), QPointF(s * 0.34, s * 0.66));
    });
}

QIcon endProcess()
{
    return render([](QPainter &p, double s) {
        QPolygonF oct;
        for (int i = 0; i < 8; ++i) {
            const double a = (22.5 + i * 45) * M_PI / 180;
            oct << QPointF(s * 0.5 + std::cos(a) * s * 0.42, s * 0.5 + std::sin(a) * s * 0.42);
        }
        p.setPen(QPen(QColor(120, 20, 20), qMax(1.0, s / 32)));
        p.setBrush(QColor(200, 40, 40));
        p.drawPolygon(oct);
        p.setPen(QPen(Qt::white, s * 0.1));
        p.drawLine(QPointF(s * 0.3, s * 0.5), QPointF(s * 0.7, s * 0.5));
    });
}

QIcon chain()
{
    return render([](QPainter &p, double s) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(90, 100, 120), s * 0.1));
        p.drawRoundedRect(QRectF(s * 0.08, s * 0.32, s * 0.48, s * 0.36), s * 0.18, s * 0.18);
        p.setPen(QPen(QColor(30, 120, 220), s * 0.1));
        p.drawRoundedRect(QRectF(s * 0.44, s * 0.32, s * 0.48, s * 0.36), s * 0.18, s * 0.18);
    });
}

QIcon proxy()
{
    return render([](QPainter &p, double s) {
        drawServer(p, QRectF(s * 0.14, s * 0.1, s * 0.72, s * 0.8), QColor(30, 120, 220));
    });
}

QIcon direct()
{
    return render([](QPainter &p, double s) {
        drawArrow(p, QPointF(s * 0.1, s * 0.5), QPointF(s * 0.92, s * 0.5), s * 0.16, QColor(60, 170, 70));
    });
}

QIcon block()
{
    return render([](QPainter &p, double s) {
        p.setPen(QPen(QColor(210, 50, 45), s * 0.12));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(s * 0.14, s * 0.14, s * 0.72, s * 0.72));
        p.drawLine(QPointF(s * 0.26, s * 0.74), QPointF(s * 0.74, s * 0.26));
    });
}

QIcon profile()
{
    return render([](QPainter &p, double s) {
        QRectF page(s * 0.18, s * 0.08, s * 0.64, s * 0.84);
        p.setPen(QPen(QColor(120, 120, 120), qMax(1.0, s / 32)));
        p.setBrush(QColor(255, 250, 225));
        p.drawRoundedRect(page, s * 0.05, s * 0.05);
        p.setPen(QPen(QColor(140, 140, 150), qMax(1.0, s / 24)));
        for (int i = 0; i < 4; ++i) {
            const double y = page.top() + page.height() * (0.25 + i * 0.17);
            p.drawLine(QPointF(page.left() + page.width() * 0.15, y), QPointF(page.right() - page.width() * 0.15, y));
        }
    });
}

QIcon arrowUp()
{
    return render([](QPainter &p, double s) {
        QPolygonF tri{QPointF(s * 0.5, s * 0.2), QPointF(s * 0.85, s * 0.75), QPointF(s * 0.15, s * 0.75)};
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(70, 90, 120));
        p.drawPolygon(tri);
    });
}

QIcon arrowDown()
{
    return render([](QPainter &p, double s) {
        QPolygonF tri{QPointF(s * 0.5, s * 0.8), QPointF(s * 0.85, s * 0.25), QPointF(s * 0.15, s * 0.25)};
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(70, 90, 120));
        p.drawPolygon(tri);
    });
}

} // namespace icons
