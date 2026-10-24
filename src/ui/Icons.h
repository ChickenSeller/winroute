#pragma once

#include <QIcon>

// Icons are drawn with QPainter so the application has no image assets to ship.
namespace icons {

QIcon app(bool active = true);
QIcon proxyServers();
QIcon rules();
QIcon nameResolution();
QIcon checker();
QIcon disconnect();
QIcon endProcess();
QIcon chain();
QIcon proxy();
QIcon direct();
QIcon block();
QIcon profile();
QIcon arrowUp();
QIcon arrowDown();

} // namespace icons
