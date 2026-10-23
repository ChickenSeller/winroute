#pragma once

#include <QWidget>
#include <deque>

class QComboBox;
class QLabel;

// "Traffic" tab: incoming/outgoing rate graph over the last minutes.
class TrafficGraph : public QWidget {
    Q_OBJECT
public:
    enum Mode { Both, IncomingOnly, OutgoingOnly };

    explicit TrafficGraph(QWidget *parent = nullptr);
    void addSample(double inRate, double outRate);
    void setMode(Mode m);
    void clear();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    std::deque<std::pair<double, double>> m_samples;   // (in, out) bytes/sec
    Mode m_mode = Both;
};

class TrafficWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrafficWidget(QWidget *parent = nullptr);
    void addSample(double inRate, double outRate);
    void clear();

private:
    TrafficGraph *m_graph;
    QComboBox *m_mode;
    QLabel *m_inLabel;
    QLabel *m_outLabel;
    QLabel *m_totalLabel;
    double m_inMax = 0, m_outMax = 0;
    double m_inTotal = 0, m_outTotal = 0;
};
