#include "TrafficWidget.h"
#include "core/Utils.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSettings>
#include <cmath>

static constexpr size_t kMaxSamples = 300;   // 5 minutes at 1 sample/sec

QColor TrafficGraph::color(Series s)
{
    switch (s) {
    case In: return QColor(40, 160, 60);
    case Out: return QColor(210, 60, 50);
    case GatewayDown: return QColor(40, 110, 210);
    case GatewayUp: return QColor(225, 135, 20);
    default: return Qt::gray;
    }
}

TrafficGraph::TrafficGraph(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TrafficGraph::addSample(const Sample &s)
{
    m_samples.push_back(s);
    while (m_samples.size() > kMaxSamples)
        m_samples.pop_front();
    update();
}

void TrafficGraph::setSeriesVisible(Series s, bool visible)
{
    m_visible[s] = visible;
    update();
}

void TrafficGraph::setGatewayShown(bool shown)
{
    if (m_gatewayShown == shown)
        return;
    m_gatewayShown = shown;
    update();
}

void TrafficGraph::clear()
{
    m_samples.clear();
    update();
}

bool TrafficGraph::drawn(int s) const
{
    if (!m_visible[s])
        return false;
    return (s != GatewayDown && s != GatewayUp) || m_gatewayShown;
}

// Rounds up to 1/2/5 * 10^n so the scale labels look tidy.
static double niceCeil(double v)
{
    if (v <= 1024)
        return 1024;
    const double e = std::pow(10.0, std::floor(std::log10(v)));
    for (double m : {1.0, 2.0, 5.0, 10.0})
        if (v <= m * e)
            return m * e;
    return 10 * e;
}

void TrafficGraph::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRect area = rect().adjusted(64, 8, -8, -20);
    p.fillRect(rect(), palette().base());
    p.fillRect(area, QColor(250, 251, 253));

    // The scale follows the lines that are drawn.
    double maxV = 0;
    for (const Sample &s : m_samples)
        for (int i = 0; i < SeriesCount; ++i)
            if (drawn(i))
                maxV = qMax(maxV, s[i]);
    maxV = niceCeil(maxV * 1.1);

    // grid + scale: solid labelled lines, with dotted, paler lines marking the
    // finer subdivisions between them. Drawn without antialiasing so the dots
    // stay crisp.
    const QPen majorPen(QColor(223, 226, 234), 1, Qt::SolidLine);
    const QPen minorPen(QColor(228, 232, 239), 1, Qt::DotLine);
    p.setRenderHint(QPainter::Antialiasing, false);
    const int gridRows = 4;
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() * 0.85);
    p.setFont(f);
    // horizontal: a dotted line halfway between each labelled value line
    for (int i = 0; i <= gridRows * 2; ++i) {
        const int y = area.top() + area.height() * i / (gridRows * 2);
        const bool labelled = (i % 2 == 0);
        p.setPen(labelled ? majorPen : minorPen);
        p.drawLine(area.left(), y, area.right(), y);
        if (labelled) {
            p.setPen(palette().color(QPalette::Text));
            p.drawText(QRect(0, y - 8, area.left() - 6, 16), Qt::AlignRight | Qt::AlignVCenter,
                       util::formatRate(maxV * (gridRows - i / 2) / gridRows));
        }
    }
    // vertical: a dotted line every 15 s, a solid labelled one each minute
    for (int s = 15; s < int(kMaxSamples); s += 15) {
        const int x = area.right() - int(area.width() * double(s) / (kMaxSamples - 1));
        const bool minute = (s % 60 == 0);
        p.setPen(minute ? majorPen : minorPen);
        p.drawLine(x, area.top(), x, area.bottom());
        if (minute) {
            p.setPen(palette().color(QPalette::Text));
            p.drawText(QRect(x - 30, area.bottom() + 2, 60, 16), Qt::AlignCenter, tr("-%1 min").arg(s / 60));
        }
    }
    p.setPen(QColor(190, 195, 205));
    p.drawRect(area);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (m_samples.size() < 2)
        return;
    auto drawSeries = [&](int series, int fillAlpha) {
        QPainterPath path;
        const size_t n = m_samples.size();
        for (size_t i = 0; i < n; ++i) {
            const double v = m_samples[i][series];
            const double x = area.right() - area.width() * double(n - 1 - i) / (kMaxSamples - 1);
            const double y = area.bottom() - area.height() * (v / maxV);
            if (i == 0)
                path.moveTo(x, y);
            else
                path.lineTo(x, y);
        }
        const QColor c = color(Series(series));
        if (fillAlpha > 0) {
            QPainterPath fill = path;
            fill.lineTo(area.right(), area.bottom());
            fill.lineTo(area.right() - area.width() * double(n - 1) / (kMaxSamples - 1), area.bottom());
            fill.closeSubpath();
            QColor fc = c;
            fc.setAlpha(fillAlpha);
            p.fillPath(fill, fc);
        }
        p.setPen(QPen(c, 1.6));
        p.drawPath(path);
    };
    p.setClipRect(area);
    // The gateway lines are drawn with a fainter fill, so the four stay apart.
    for (int s : {GatewayDown, GatewayUp, In, Out})
        if (drawn(s))
            drawSeries(s, s == In || s == Out ? 50 : 25);
}

SeriesLabel::SeriesLabel(const QColor &color, QWidget *parent)
    : QLabel(parent), m_color(color)
{
    setCursor(Qt::PointingHandCursor);
    restyle();
}

void SeriesLabel::setMeaning(const QString &meaning)
{
    setToolTip(tr("%1\nCurrent rate | highest rate since cleared\nClick to show or hide this line").arg(meaning));
}

void SeriesLabel::setOn(bool on)
{
    m_on = on;
    restyle();
}

void SeriesLabel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QLabel::mousePressEvent(e);
        return;
    }
    setOn(!m_on);
    emit toggled(m_on);
}

void SeriesLabel::restyle()
{
    setStyleSheet(m_on ? QString("color: %1").arg(m_color.name())
                       : QStringLiteral("color: #a0a4ac; text-decoration: line-through"));
}

TrafficWidget::TrafficWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    // Each group's down and up captions are stacked in a column: this
    // computer's, then the proxy gateway's while it runs. Clear and the total
    // sit beside them, centred on the two rows.
    auto *top = new QGridLayout;
    top->setHorizontalSpacing(16);
    top->setVerticalSpacing(2);
    auto *clearBtn = new QPushButton(tr("Clear"));
    m_totalLabel = new QLabel;
    top->addWidget(clearBtn, 0, 0, 2, 1, Qt::AlignVCenter);
    m_graph = new TrafficGraph;

    // Which lines are hidden is remembered.
    const QStringList hidden = util::appSettings().value("traffic/hidden").toStringList();
    for (int s = 0; s < TrafficGraph::SeriesCount; ++s) {
        auto *label = new SeriesLabel(TrafficGraph::color(TrafficGraph::Series(s)));
        const bool on = !hidden.contains(QString::number(s));
        label->setOn(on);
        m_graph->setSeriesVisible(TrafficGraph::Series(s), on);
        connect(label, &SeriesLabel::toggled, this, [this, s](bool on) {
            m_graph->setSeriesVisible(TrafficGraph::Series(s), on);
            QStringList off;
            for (int i = 0; i < TrafficGraph::SeriesCount; ++i)
                if (!m_labels[i]->isOn())
                    off << QString::number(i);
            util::appSettings().setValue("traffic/hidden", off);
        });
        static const char *const meanings[] = {
            QT_TR_NOOP("This computer's incoming traffic"),
            QT_TR_NOOP("This computer's outgoing traffic"),
            QT_TR_NOOP("Proxy gateway: traffic to the devices using it"),
            QT_TR_NOOP("Proxy gateway: traffic from the devices using it"),
        };
        label->setMeaning(tr(meanings[s]));
        m_labels[s] = label;
        const bool gateway = s == TrafficGraph::GatewayDown || s == TrafficGraph::GatewayUp;
        top->addWidget(label, s % 2, gateway ? 2 : 1);
    }
    m_labels[TrafficGraph::GatewayDown]->setVisible(false);
    m_labels[TrafficGraph::GatewayUp]->setVisible(false);
    top->setColumnStretch(3, 1);
    top->addWidget(m_totalLabel, 0, 4, 2, 1, Qt::AlignVCenter);
    layout->addLayout(top);
    layout->addWidget(m_graph, 1);
    connect(clearBtn, &QPushButton::clicked, this, &TrafficWidget::clear);
    addSample(0, 0);
}

void TrafficWidget::addSample(double in, double out, bool gatewayOn, double gwDown, double gwUp)
{
    const TrafficGraph::Sample s{in, out, gwDown, gwUp};
    m_graph->addSample(s);
    for (int i = 0; i < TrafficGraph::SeriesCount; ++i) {
        m_max[i] = qMax(m_max[i], s[i]);
        m_total[i] += s[i];
    }
    if (gatewayOn != m_gatewayOn) {
        m_gatewayOn = gatewayOn;
        m_graph->setGatewayShown(gatewayOn);
        m_labels[TrafficGraph::GatewayDown]->setVisible(gatewayOn);
        m_labels[TrafficGraph::GatewayUp]->setVisible(gatewayOn);
    }
    m_labels[TrafficGraph::In]->setText(tr("Down: %1 | %2").arg(util::formatRate(in), util::formatRate(m_max[TrafficGraph::In])));
    m_labels[TrafficGraph::Out]->setText(tr("Up: %1 | %2").arg(util::formatRate(out), util::formatRate(m_max[TrafficGraph::Out])));
    m_labels[TrafficGraph::GatewayDown]->setText(
        tr("Gateway down: %1 | %2").arg(util::formatRate(gwDown), util::formatRate(m_max[TrafficGraph::GatewayDown])));
    m_labels[TrafficGraph::GatewayUp]->setText(
        tr("Gateway up: %1 | %2").arg(util::formatRate(gwUp), util::formatRate(m_max[TrafficGraph::GatewayUp])));
    updateTotal();
}

void TrafficWidget::updateTotal()
{
    const auto bytes = [this](int s) { return util::formatBytes(qint64(m_total[s])); };
    m_totalLabel->setText(tr("Total: %1 | %2").arg(bytes(TrafficGraph::In), bytes(TrafficGraph::Out)));
    QString tip = tr("Since cleared, this computer: %1 down | %2 up").arg(bytes(TrafficGraph::In), bytes(TrafficGraph::Out));
    if (m_gatewayOn || m_total[TrafficGraph::GatewayDown] > 0 || m_total[TrafficGraph::GatewayUp] > 0)
        tip += u'\n' + tr("Proxy gateway: %1 down | %2 up").arg(bytes(TrafficGraph::GatewayDown), bytes(TrafficGraph::GatewayUp));
    m_totalLabel->setToolTip(tip);
}

void TrafficWidget::clear()
{
    m_graph->clear();
    m_max = {};
    m_total = {};
    addSample(0, 0, m_gatewayOn);
}
