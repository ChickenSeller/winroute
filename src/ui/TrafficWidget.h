#pragma once

#include <QColor>
#include <QLabel>
#include <QWidget>
#include <array>
#include <deque>

// "Traffic" tab: rate graph over the last minutes. Four lines: this machine's
// incoming and outgoing traffic, and, while the proxy gateway is on, the
// traffic it forwards for other devices (down = to them, up = from them).
// Clicking a line's caption above the graph shows or hides that line.
class TrafficGraph : public QWidget {
    Q_OBJECT
public:
    enum Series { In, Out, GatewayDown, GatewayUp, SeriesCount };
    using Sample = std::array<double, SeriesCount>;   // bytes/sec

    explicit TrafficGraph(QWidget *parent = nullptr);
    void addSample(const Sample &s);
    void setSeriesVisible(Series s, bool visible);   // the caption's toggle
    void setGatewayShown(bool shown);               // the gateway lines exist only while it runs
    void clear();
    static QColor color(Series s);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    bool drawn(int s) const;

    std::deque<Sample> m_samples;
    std::array<bool, SeriesCount> m_visible{true, true, true, true};
    bool m_gatewayShown = false;
};

// A line's caption: coloured text that toggles the line when clicked, greyed
// and struck through while the line is hidden.
class SeriesLabel : public QLabel {
    Q_OBJECT
public:
    SeriesLabel(const QColor &color, QWidget *parent = nullptr);
    // What the line shows, for the tooltip.
    void setMeaning(const QString &meaning);
    void setOn(bool on);
    bool isOn() const { return m_on; }

signals:
    void toggled(bool on);

protected:
    void mousePressEvent(QMouseEvent *e) override;

private:
    void restyle();

    QColor m_color;
    bool m_on = true;
};

// The graph with short captions above it ("Down: 1.2 KB/sec | 3.4 MB/sec");
// what each part means is in the tooltips.
class TrafficWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrafficWidget(QWidget *parent = nullptr);
    // One second of traffic. gatewayOn: whether the proxy gateway is running.
    void addSample(double inRate, double outRate, bool gatewayOn = false, double gwDown = 0, double gwUp = 0);
    void clear();

private:
    void updateTotal();

    TrafficGraph *m_graph;
    std::array<SeriesLabel *, TrafficGraph::SeriesCount> m_labels{};
    QLabel *m_totalLabel;
    std::array<double, TrafficGraph::SeriesCount> m_max{};
    std::array<double, TrafficGraph::SeriesCount> m_total{};
    bool m_gatewayOn = false;
};
