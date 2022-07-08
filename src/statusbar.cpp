#include <QGridLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QGraphicsColorizeEffect>
#include <QToolButton>
#include <QIcon>
#include <QLabel>

#include "statusbar.h"
#include "statisticsindicator.h"

namespace ghostwriter
{

class StatusBarPrivate
{
public:
    StatusBarPrivate() { }
    ~StatusBarPrivate() { }

    AppSettings *appSettings;
    FindReplace *findReplace;
    StatisticsIndicator *statisticsIndicator;
    QLabel *statusIndicator;
    TimeLabel *timeIndicator;
    QAction *htmlPreviewMenuAction;
    QAction *fullScreenMenuAction;
    QPushButton *fullScreenButton;
    QAction *showSidebarAction;
    QList<QGraphicsColorizeEffect *> buttonColorizers;
    QList<QWidget *> statusBarWidgets;
};

StatusBar::StatusBar(FindReplace *findReplace,
                     StatisticsIndicator *statisticsIndicator,
                     QWidget *parent)
    : QStatusBar(parent), d(new StatusBarPrivate())
{

    d->appSettings = AppSettings::instance();
    d->findReplace = findReplace;
    d->statisticsIndicator = statisticsIndicator;

    QGridLayout *statusBarLayout = new QGridLayout();
    statusBarLayout->setSpacing(0);
    statusBarLayout->setContentsMargins(0, 0, 0, 0);

    statusBarLayout->addWidget(d->findReplace, 0, 0, 1, 3);

    // Divide the status bar into thirds for placing widgets.
    QFrame *leftWidget = new QFrame(this);
    leftWidget->setObjectName("leftStatusBarWidget");
    leftWidget->setStyleSheet("#leftStatusBarWidget { border: 0; margin: 0; padding: 0 }");
    QFrame *midWidget = new QFrame(this);
    midWidget->setObjectName("midStatusBarWidget");
    midWidget->setStyleSheet("#midStatusBarWidget { border: 0; margin: 0; padding: 0 }");
    QFrame *rightWidget = new QFrame(this);
    rightWidget->setObjectName("rightStatusBarWidget");
    rightWidget->setStyleSheet("#rightStatusBarWidget { border: 0; margin: 0; padding: 0 }");

    QHBoxLayout *leftLayout = new QHBoxLayout(leftWidget);
    leftWidget->setLayout(leftLayout);
    leftLayout->setContentsMargins(0,0,0,0);
    QHBoxLayout *midLayout = new QHBoxLayout(midWidget);
    midWidget->setLayout(midLayout);
    midLayout->setContentsMargins(0,0,0,0);
    QHBoxLayout *rightLayout = new QHBoxLayout(rightWidget);
    rightWidget->setLayout(rightLayout);
    rightLayout->setContentsMargins(0,0,0,0);

    // Add left-most widgets to status bar.
    QToolButton *button = new QToolButton();
    QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect();
    button->setIcon(QIcon(":/resources/images/fontawesome/terminal-solid.svg"));
    button->setObjectName("showSidebarButton");
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolTip(tr("Toggle sidebar"));
    button->setCheckable(false);
    button->setChecked(false);
    button->setGraphicsEffect(effect);

    statusBarButtons.append(toggleSidebarButton);

    leftLayout->addWidget(toggleSidebarButton, 0, Qt::AlignLeft);
    statusBarWidgets.append(toggleSidebarButton);

    this->connect(toggleSidebarButton,
        &QPushButton::toggled,
        [this](bool checked) {
            emit toggleSidebar(checked);
        }
    );

    timeIndicator = new TimeLabel(this);
    leftLayout->addWidget(timeIndicator, 0, Qt::AlignLeft);
    leftWidget->setContentsMargins(0, 0, 0, 0);
    statusBarWidgets.append(timeIndicator);

    QWidget *window = this->parentWidget();

    if (nullptr == window) {
        window = this;
    }

    if (!window->isFullScreen() || appSettings->displayTimeInFullScreenEnabled()) {
        timeIndicator->hide();
    }

    statusBarLayout->addWidget(leftWidget, 1, 0, 1, 1, Qt::AlignLeft);

    // Add middle widgets to status bar.
    statusIndicator = new QLabel();
    midLayout->addWidget(statusIndicator, 0, Qt::AlignCenter);
    statusIndicator->hide();

    if ((appSettings->favoriteStatistic() >= 0)
            && (appSettings->favoriteStatistic() < statisticsIndicator->count())) {
        statisticsIndicator->setCurrentIndex(appSettings->favoriteStatistic());
    }
    else {
        statisticsIndicator->setCurrentIndex(0);
    }

    this->connect(statisticsIndicator,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        appSettings,
        &AppSettings::setFavoriteStatistic);

    midLayout->addWidget(statisticsIndicator, 0, Qt::AlignCenter);
    midWidget->setContentsMargins(0, 0, 0, 0);
    statusBarLayout->addWidget(midWidget, 1, 1, 1, 1, Qt::AlignCenter);
    statusBarWidgets.append(statisticsIndicator);

    // Add right-most widgets to status bar.
    QToolButton *button = new QToolButton();
    QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect();
    button->setIcon(QIcon(":/resources/images/fontawesome/moon-solid.svg"));
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolTip(tr("Toggle dark mode"));
    button->setCheckable(true);
    button->setChecked(appSettings->darkModeEnabled());
    button->setGraphicsEffect(effect);
    buttonColorizers.append(effect);

    this->connect
    (
        button,
        &QPushButton::toggled,
        [this](bool enabled) {
            appSettings->setDarkModeEnabled(enabled);
            applyTheme();
        }
    );

    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);

    button = new QPushButton();
    button->setIcon(QIcon(":/resources/images/fontawesome/code-solid.svg"));
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolTip(tr("Toggle Live HTML Preview"));
    button->setCheckable(true);
    button->setChecked(appSettings->htmlPreviewVisible());
    connect(button, SIGNAL(toggled(bool)), this, SLOT(toggleHtmlPreview(bool)));
    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);
    buttonColorizers.append(button);
    QGraphicsColorizeEffect *e = new QGraphicsColorizeEffect(button);

    this->connect
    (
        this->htmlPreviewMenuAction,
        &QAction::toggled,
        [button](bool checked) {
            button->blockSignals(true);
            button->setChecked(checked);
            button->blockSignals(false);
        }
    );

    button = new QPushButton();
    button->setIcon(QIcon(":/resources/images/fontawesome/delete-left-solid.svg"));
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolTip(tr("Toggle Hemingway mode"));
    button->setCheckable(true);
    connect(button, SIGNAL(toggled(bool)), this, SLOT(toggleHemingwayMode(bool)));
    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);
    buttonColorizers.append(button);

    button = new QPushButton();
    button->setIcon(QIcon(":/resources/images/fontawesome/headphones-simple-solid.svg"));
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolTip(tr("Toggle distraction free mode"));
    button->setCheckable(true);
    connect(button, SIGNAL(toggled(bool)), this, SLOT(toggleFocusMode(bool)));
    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);
    statusBarButtons.append(button);

    button = new QPushButton(QChar(fa::expand));
    button->setIcon(QIcon(":/resources/images/fontawesome/expand-solid.svg"));
    button->setFocusPolicy(Qt::NoFocus);
    button->setObjectName("fullscreenButton");
    button->setToolTip(tr("Toggle full screen mode"));
    button->setCheckable(true);
    button->setChecked(this->isFullScreen());
    connect(button, SIGNAL(toggled(bool)), this, SLOT(toggleFullScreen(bool)));
    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);
    statusBarButtons.append(button);
    this->fullScreenButton = button;

    rightWidget->setContentsMargins(0, 0, 0, 0);
    statusBarLayout->addWidget(rightWidget, 1, 2, 1, 1, Qt::AlignRight);

    QWidget *container = new QWidget(this);
    container->setObjectName("statusBarWidgetContainer");
    container->setLayout(statusBarLayout);
    container->setContentsMargins(0, 0, 2, 0);
    container->setStyleSheet("#statusBarWidgetContainer { border: 0; margin: 0; padding: 0 }");

    this->addWidget(container, 1);
    this->setSizeGripEnabled(false);
}

StatusBar::~StatusBar()
{
    ;
}

void StatusBar::setTheme(const Theme &theme)
{

}

} // namespace ghostwriter
