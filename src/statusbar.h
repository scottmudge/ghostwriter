#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QPushButton>
#include <QList>
#include <QWidget>
#include <QGraphicsColorizeEffect>
#include <QScopedPointer>

#include "findreplace.h"
#include "appsettings.h"
#include "timelabel.h"
#include "statisticsindicator.h"
#include "theme.h"

namespace ghostwriter
{
class StatusBarPrivate;
class StatusBar : public QStatusBar
{
    Q_OBJECT

public:
    StatusBar(FindReplace *findReplace,
              StatisticsIndicator *statisticsIndicator,
              QWidget *parent = nullptr);
    ~StatusBar();

signals:
    void toggleSidebar(bool checked);

public slots:
    void setTheme(const Theme &theme);

private:
    QScopedPointer<StatusBarPrivate> d;

};

} // namespace ghostwriter

#endif // STATUSBAR_H
