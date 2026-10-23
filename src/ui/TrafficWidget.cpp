#include "TrafficWidget.h"
#include "core/Utils.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <cmath>

static constexpr size_t kMaxSamples = 300;   // 5 minutes at 1 sample/sec

static const QColor kInColor(40, 160, 60);
static const QColor kOutColor(210, 60, 50);

TrafficGraph::TrafficGraph(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TrafficGraph::addSample(double in, double out)
{
    m_samples.emplace_back(in, out);
    while (m_samples.size() > kMaxSamples)
        m_samples.pop_front();
    update();
}

void TrafficGraph::setMode(Mode m)
{
    m_mode = m;
    update();
}

void TrafficGraph::clear()
{
    m_samples.clear();
    update();
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

    double maxV = 0;
    for (const auto &[in, out] : m_samples) {
        if (m_mode != OutgoingOnly)
            maxV = qMax(maxV, in);
        if (m_mode != IncomingOnly)
            maxV = qMax(maxV, out);
    }
    maxV = niceCeil(maxV * 1.1);

    // grid + scale
    p.setPen(QColor(225, 228, 235));
    const int gridRows = 4;
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() * 0.85);
    p.setFont(f);
    for (int i = 0; i <= gridRows; ++i) {
        const int y = area.top() + area.height() * i / gridRows;
        p.setPen(QColor(225, 228, 235));
        p.drawLine(area.left(), y, area.right(), y);
        p.setPen(palette().color(QPalette::Text));
        p.drawText(QRect(0, y - 8, area.left() - 6, 16), Qt::AlignRight | Qt::AlignVCenter,
                   util::formatRate(maxV * (gridRows - i) / gridRows));
    }
    for (int s = 60; s < int(kMaxSamples); s += 60) {
        const int x = area.right() - int(area.width() * double(s) / (kMaxSamples - 1));
        p.setPen(QColor(225, 228, 235));
        p.drawLine(x, area.top(), x, area.bottom());
        p.setPen(palette().color(QPalette::Text));
        p.drawText(QRect(x - 30, area.bottom() + 2, 60, 16), Qt::AlignCenter, tr("-%1 min").arg(s / 60));
    }
    p.setPen(QColor(190, 195, 205));
    p.drawRect(area);

    if (m_samples.size() < 2)
        return;
    auto drawSeries = [&](bool incoming, const QColor &c) {
        QPainterPath path;
        const size_t n = m_samples.size();
        for (size_t i = 0; i < n; ++i) {
            const double v = incoming ? m_samples[i].first : m_samples[i].second;
            const double x = area.right() - area.width() * double(n - 1 - i) / (kMaxSamples - 1);
            const double y = area.bottom() - area.height() * (v / maxV);
            if (i == 0)
                path.moveTo(x, y);
            else
                path.lineTo(x, y);
        }
        QPainterPath fill = path;
        fill.lineTo(area.right(), area.bottom());
        fill.lineTo(area.right() - area.width() * double(n - 1) / (kMaxSamples - 1), area.bottom());
        fill.closeSubpath();
        QColor fc = c;
        fc.setAlpha(50);
        p.fillPath(fill, fc);
        p.setPen(QPen(c, 1.6));
        p.drawPath(path);
    };
    p.setClipRect(area);
    if (m_mode != OutgoingOnly)
        drawSeries(true, kInColor);
    if (m_mode != IncomingOnly)
        drawSeries(false, kOutColor);
}

TrafficWidget::TrafficWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    auto *top = new QHBoxLayout;
    m_mode = new QComboBox;
    m_mode->addItems({tr("Show Both"), tr("Incoming Only"), tr("Outgoing Only")});
    m_mode->setToolTip(tr("Select the kind of the traffic to display"));
    auto *clearBtn = new QPushButton(tr("Clear"));
    m_inLabel = new QLabel;
    m_outLabel = new QLabel;
    m_totalLabel = new QLabel;
    m_inLabel->setStyleSheet(QString("color: %1").arg(kInColor.name()));
    m_outLabel->setStyleSheet(QString("color: %1").arg(kOutColor.name()));
    top->addWidget(m_mode);
    top->addWidget(clearBtn);
    top->addSpacing(16);
    top->addWidget(m_inLabel);
    top->addSpacing(16);
    top->addWidget(m_outLabel);
    top->addStretch();
    top->addWidget(m_totalLabel);
    layout->addLayout(top);
    m_graph = new TrafficGraph;
    layout->addWidget(m_graph, 1);
    connect(m_mode, &QComboBox::currentIndexChanged, this, [this](int i) { m_graph->setMode(TrafficGraph::Mode(i)); });
    connect(clearBtn, &QPushButton::clicked, this, &TrafficWidget::clear);
    addSample(0, 0);
}

void TrafficWidget::addSample(double in, double out)
{
    m_graph->addSample(in, out);
    m_inMax = qMax(m_inMax, in);
    m_outMax = qMax(m_outMax, out);
    m_inTotal += in;
    m_outTotal += out;
    m_inLabel->setText(tr("Incoming: %1 (max %2)").arg(util::formatRate(in), util::formatRate(m_inMax)));
    m_outLabel->setText(tr("Outgoing: %1 (max %2)").arg(util::formatRate(out), util::formatRate(m_outMax)));
    m_totalLabel->setText(tr("Total: %1 received, %2 sent")
                              .arg(util::formatBytes(qint64(m_inTotal)), util::formatBytes(qint64(m_outTotal))));
}

void TrafficWidget::clear()
{
    m_graph->clear();
    m_inMax = m_outMax = 0;
    m_inTotal = m_outTotal = 0;
    addSample(0, 0);
}
