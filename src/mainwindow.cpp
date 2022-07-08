/***********************************************************************
 *
 * Copyright (C) 2014-2022 wereturtle
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include <QApplication>
#include <QClipboard>
#include <QCommonStyle>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDialog>
#include <QGridLayout>
#include <QIcon>
#include <QIODevice>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QScrollBar>
#include <QSettings>
#include <QSizePolicy>
#include <QStatusBar>
#include <QToolButton>

#include "3rdparty/QtAwesome/QtAwesome.h"

#include "actions.h"
#include "documenthistory.h"
#include "exporter.h"
#include "exporterfactory.h"
#include "findreplace.h"
#include "localedialog.h"
#include "mainwindow.h"
#include "messageboxhelper.h"
#include "preferencesdialog.h"
#include "previewoptionsdialog.h"
#include "sandboxedwebpage.h"
#include "simplefontdialog.h"
#include "spelling/dictionarymanager.h"
#include "spelling/spellcheckdecorator.h"
#include "stylesheetbuilder.h"
#include "themeselectiondialog.h"

namespace ghostwriter
{
enum SidebarTabIndex {
    FirstSidebarTab,
    OutlineSidebarTab = FirstSidebarTab,
    SessionStatsSidebarTab,
    DocumentStatsSidebarTab,
    CheatSheetSidebarTab,
    LastSidebarTab = CheatSheetSidebarTab
};

#define GW_MAIN_WINDOW_GEOMETRY_KEY "Window/mainWindowGeometry"
#define GW_MAIN_WINDOW_STATE_KEY "Window/mainWindowState"
#define GW_SPLITTER_GEOMETRY_KEY "Window/splitterGeometry"

MainWindow::MainWindow(const QString &filePath, QWidget *parent)
    : QMainWindow(parent)
{
    QString fileToOpen;
    this->focusModeEnabled = false;
    this->awesome = new QtAwesome(qApp);
    this->awesome->initFontAwesome();
    setWindowIcon(QIcon(":/resources/images/ghostwriter.svg"));
    this->setObjectName("mainWindow");
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    appSettings = AppSettings::instance();

    QString themeName = appSettings->themeName();

    QString err;
    theme = ThemeRepository::instance()->loadTheme(themeName, err);

    MarkdownDocument *document = new MarkdownDocument();

    editor = new MarkdownEditor(document, theme.lightColorScheme(), this);
    editor->setMinimumWidth(0.1 * qApp->primaryScreen()->size().width());
    editor->setFont(appSettings->editorFont().family(), appSettings->editorFont().pointSize());
    editor->setUseUnderlineForEmphasis(appSettings->useUnderlineForEmphasis());
    editor->setEnableLargeHeadingSizes(appSettings->largeHeadingSizesEnabled());
    editor->setAutoMatchEnabled(appSettings->autoMatchEnabled());
    editor->setBulletPointCyclingEnabled(appSettings->bulletPointCyclingEnabled());
    editor->setPlainText("");
    editor->setEditorWidth((EditorWidth) appSettings->editorWidth());
    editor->setEditorCorners((InterfaceStyle) appSettings->interfaceStyle());
    editor->setItalicizeBlockquotes(appSettings->italicizeBlockquotes());
    editor->setTabulationWidth(appSettings->tabWidth());
    editor->setInsertSpacesForTabs(appSettings->insertSpacesForTabsEnabled());

    connect(editor,
        &MarkdownEditor::fontSizeChanged,
        this,
        &MainWindow::onFontSizeChanged
    );
    this->setFocusProxy(editor);

    // We need to set an empty style for the editor's scrollbar in order for the
    // scrollbar CSS stylesheet to take full effect.  Otherwise, the scrollbar's
    // background color will have the Windows 98 checkered look rather than
    // being a solid or transparent color.
    //
    editor->verticalScrollBar()->setStyle(new QCommonStyle());
    editor->horizontalScrollBar()->setStyle(new QCommonStyle());

    spelling = new SpellCheckDecorator(editor);
    spelling->setLiveSpellCheckEnabled(appSettings->liveSpellCheckEnabled());

    buildSidebar();

    documentManager = new DocumentManager(editor, this);
    documentManager->setAutoSaveEnabled(appSettings->autoSaveEnabled());
    documentManager->setFileBackupEnabled(appSettings->backupFileEnabled());
    documentManager->setDraftLocation(appSettings->draftLocation());
    documentManager->setFileHistoryEnabled(appSettings->fileHistoryEnabled());
    setWindowTitle(documentManager->document()->displayName() + "[*] - " + qAppName());
    connect(documentManager, SIGNAL(documentDisplayNameChanged(QString)), this, SLOT(changeDocumentDisplayName(QString)));
    connect(documentManager, SIGNAL(documentModifiedChanged(bool)), this, SLOT(setWindowModified(bool)));
    connect(documentManager, SIGNAL(operationStarted(QString)), this, SLOT(onOperationStarted(QString)));
    connect(documentManager, SIGNAL(operationUpdate(QString)), this, SLOT(onOperationStarted(QString)));
    connect(documentManager, SIGNAL(operationFinished()), this, SLOT(onOperationFinished()));
    connect(documentManager, SIGNAL(documentClosed()), this, SLOT(refreshRecentFiles()));

    editor->setAutoMatchEnabled('\"', appSettings->autoMatchCharEnabled('\"'));
    editor->setAutoMatchEnabled('\'', appSettings->autoMatchCharEnabled('\''));
    editor->setAutoMatchEnabled('(', appSettings->autoMatchCharEnabled('('));
    editor->setAutoMatchEnabled('[', appSettings->autoMatchCharEnabled('['));
    editor->setAutoMatchEnabled('{', appSettings->autoMatchCharEnabled('{'));
    editor->setAutoMatchEnabled('*', appSettings->autoMatchCharEnabled('*'));
    editor->setAutoMatchEnabled('_', appSettings->autoMatchCharEnabled('_'));
    editor->setAutoMatchEnabled('`', appSettings->autoMatchCharEnabled('`'));
    editor->setAutoMatchEnabled('<', appSettings->autoMatchCharEnabled('<'));

    QWidget *editorPane = new QWidget(this);
    editorPane->setObjectName("editorLayoutArea");
    editorPane->setLayout(editor->preferredLayout());

    QStringList recentFiles;

    if (appSettings->fileHistoryEnabled()) {
        DocumentHistory history;
        recentFiles = history.recentFiles(MAX_RECENT_FILES + 2);
    }

    if (!filePath.isNull() && !filePath.isEmpty()) {
        fileToOpen = filePath;
        recentFiles.removeAll(QFileInfo(filePath).absoluteFilePath());
    }

    if (fileToOpen.isNull()
            && appSettings->fileHistoryEnabled()
            && appSettings->restoreSessionEnabled()) {
        QString lastFile;

        if (!recentFiles.isEmpty()) {
            lastFile = recentFiles.first();
        }

        if (QFileInfo::exists(lastFile)) {
            fileToOpen = lastFile;
            recentFiles.removeAll(lastFile);
        }
    }

    recentFilesActions.append(appActions->action(Actions::OpenRecent0));
    recentFilesActions.append(appActions->action(Actions::OpenRecent1));
    recentFilesActions.append(appActions->action(Actions::OpenRecent2));
    recentFilesActions.append(appActions->action(Actions::OpenRecent3));
    recentFilesActions.append(appActions->action(Actions::OpenRecent4));
    recentFilesActions.append(appActions->action(Actions::OpenRecent5));
    recentFilesActions.append(appActions->action(Actions::OpenRecent6));
    recentFilesActions.append(appActions->action(Actions::OpenRecent7));
    recentFilesActions.append(appActions->action(Actions::OpenRecent8));
    recentFilesActions.append(appActions->action(Actions::OpenRecent9));

    for (int i = 0; i < recentFilesActions.size(); i++) {
        if (i < recentFiles.size()) {
            recentFilesActions.at(i)->setText(recentFiles.at(i));

            // Use the action's data for access to the actual file path, since
            // KDE Plasma will add a keyboard accelerator to the action's text
            // by inserting an ampersand (&) into it.
            //
            recentFilesActions.at(i)->setData(recentFiles.at(i));

            recentFilesActions.at(i)->setVisible(true);
        } else {
            recentFilesActions.at(i)->setVisible(false);
        }
    }

    // Set dimensions for the main window.  This is best done before
    // building the status bar, so that we can determine whether the full
    // screen button should be checked.
    //
    QSettings windowSettings;

    if (windowSettings.contains(GW_MAIN_WINDOW_GEOMETRY_KEY)) {
        restoreGeometry(windowSettings.value(GW_MAIN_WINDOW_GEOMETRY_KEY).toByteArray());
        restoreState(windowSettings.value(GW_MAIN_WINDOW_STATE_KEY).toByteArray());
    } else {
        this->adjustSize();
    }

    connect(appSettings, SIGNAL(autoSaveChanged(bool)), documentManager, SLOT(setAutoSaveEnabled(bool)));
    connect(appSettings, SIGNAL(backupFileChanged(bool)), documentManager, SLOT(setFileBackupEnabled(bool)));
    connect(appSettings, SIGNAL(tabWidthChanged(int)), editor, SLOT(setTabulationWidth(int)));
    connect(appSettings, SIGNAL(insertSpacesForTabsChanged(bool)), editor, SLOT(setInsertSpacesForTabs(bool)));
    connect(appSettings, SIGNAL(useUnderlineForEmphasisChanged(bool)), editor, SLOT(setUseUnderlineForEmphasis(bool)));
    connect(appSettings, SIGNAL(italicizeBlockquotesChanged(bool)), editor, SLOT(setItalicizeBlockquotes(bool)));
    connect(appSettings, SIGNAL(largeHeadingSizesChanged(bool)), editor, SLOT(setEnableLargeHeadingSizes(bool)));
    connect(appSettings, SIGNAL(autoMatchChanged(bool)), editor, SLOT(setAutoMatchEnabled(bool)));
    connect(appSettings, SIGNAL(autoMatchCharChanged(QChar, bool)), editor, SLOT(setAutoMatchEnabled(QChar, bool)));
    connect(appSettings, SIGNAL(bulletPointCyclingChanged(bool)), editor, SLOT(setBulletPointCyclingEnabled(bool)));
    connect(appSettings, SIGNAL(autoMatchChanged(bool)), editor, SLOT(setAutoMatchEnabled(bool)));
    connect(appSettings, SIGNAL(focusModeChanged(FocusMode)), this, SLOT(changeFocusMode(FocusMode)));
    connect(appSettings, SIGNAL(hideMenuBarInFullScreenChanged(bool)), this, SLOT(toggleHideMenuBarInFullScreen(bool)));
    connect(appSettings, SIGNAL(fileHistoryChanged(bool)), this, SLOT(toggleFileHistoryEnabled(bool)));
    connect(appSettings, SIGNAL(displayTimeInFullScreenChanged(bool)), this, SLOT(toggleDisplayTimeInFullScreen(bool)));
    connect(appSettings, SIGNAL(dictionaryLanguageChanged(QString)), spelling, SLOT(setDictionary(QString)));
    connect(appSettings, SIGNAL(liveSpellCheckChanged(bool)), spelling, SLOT(setSpellCheckEnabled(bool)));
    connect(appSettings, SIGNAL(editorWidthChanged(EditorWidth)), this, SLOT(changeEditorWidth(EditorWidth)));
    connect(appSettings, SIGNAL(interfaceStyleChanged(InterfaceStyle)), this, SLOT(changeInterfaceStyle(InterfaceStyle)));
    connect(appSettings, SIGNAL(previewTextFontChanged(QFont)), this, SLOT(applyTheme()));
    connect(appSettings, SIGNAL(previewCodeFontChanged(QFont)), this, SLOT(applyTheme()));

    if (this->isFullScreen() && appSettings->hideMenuBarInFullScreenEnabled()) {
        this->menuBar()->hide();
    }

    // Default language for dictionary is set from AppSettings initialization.
    QString language = appSettings->dictionaryLanguage();

    // If we have an available dictionary, then set up spell checking.
    if (!language.isNull() && !language.isEmpty()) {
        spelling->setDictionary(language);
        spelling->setLiveSpellCheckEnabled(appSettings->liveSpellCheckEnabled());
    } else {
        spelling->setLiveSpellCheckEnabled(false);
    }

    this->connect
    (
        documentManager,
        &DocumentManager::documentLoaded,
        [this]() {
            this->sessionStats->startNewSession(this->documentStats->wordCount());
            refreshRecentFiles();
        }
    );

    this->connect
    (
        documentManager,
        &DocumentManager::documentClosed,
        [this]() {
            this->sessionStats->startNewSession(0);
        }
    );

    // Note that the parent widget for this new window must be NULL, so that
    // it will hide beneath other windows when it is deactivated.
    //
    htmlPreview = new HtmlPreview
    (
        documentManager->document(),
        appSettings->currentHtmlExporter(),
        this
    );

    connect(editor, SIGNAL(textChanged()), htmlPreview, SLOT(updatePreview()));
    connect(outlineWidget, SIGNAL(headingNumberNavigated(int)), htmlPreview, SLOT(navigateToHeading(int)));
    connect(appSettings, SIGNAL(currentHtmlExporterChanged(Exporter *)), htmlPreview, SLOT(setHtmlExporter(Exporter *)));

    htmlPreview->setMinimumWidth(0.1 * qApp->primaryScreen()->size().width());
    htmlPreview->setObjectName("htmlpreview");
    htmlPreview->setVisible(appSettings->htmlPreviewVisible());

    this->findReplace = new FindReplace(this->editor, this);
    statusBarWidgets.append(this->findReplace);
    this->findReplace->setVisible(false);

    registerActionHandlers();
    buildMenuBar();
    buildStatusBar();

    sidebar->setMinimumWidth(0.1 * qApp->primaryScreen()->size().width());

    splitter = new QSplitter(this);
    splitter->addWidget(sidebar);
    splitter->addWidget(editorPane);
    splitter->addWidget(htmlPreview);
    splitter->setChildrenCollapsible(false);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 1);

    // Set default sizes for splitter.
    QList<int> sizes;
    int sidebarWidth = this->width() * 0.2;
    int otherWidth = (this->width() - sidebarWidth) / 2;
    sizes.append(sidebarWidth);
    sizes.append(otherWidth);
    sizes.append(otherWidth);

    splitter->setSizes(sizes);

    // If previous splitter geometry was stored, load it.
    if (windowSettings.contains(GW_SPLITTER_GEOMETRY_KEY)) {
        this->splitter->restoreState(windowSettings.value(GW_SPLITTER_GEOMETRY_KEY).toByteArray());
    }

    this->connect(splitter,
        &QSplitter::splitterMoved,
        [this](int pos, int index) {
            Q_UNUSED(pos)
            Q_UNUSED(index)
            adjustEditor();
        });

    this->setCentralWidget(splitter);

    qApp->installEventFilter(this);

    toggleHideMenuBarInFullScreen(appSettings->hideMenuBarInFullScreenEnabled());
    menuBarMenuActivated = false;

    // Need this call for GTK / Gnome 42 segmentation fault workaround.
    qApp->processEvents();

    show();

    // Apply the theme only after show() is called on all the widgets,
    // since the Outline scrollbars can end up transparent in Windows if
    // the theme is applied before show().  We cannot call show() and
    // then apply the theme in the constructor due to a bug with
    // Wayland + GTK that causes a segmentation fault.
    //
    applyTheme();
    adjustEditor();

    // Show the theme right away before loading any files.
    qApp->processEvents();

    // Load file from command line or last session.
    if (!fileToOpen.isNull() && !fileToOpen.isEmpty()) {
        documentManager->open(fileToOpen);
    }

    spelling->startLiveSpellCheck();
}

MainWindow::~MainWindow()
{
    ;
}

QSize MainWindow::sizeHint() const
{
    return QSize(800, 500);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    int width = event->size().width();

    if (width < (0.5 * qApp->primaryScreen()->size().width())) {
        this->sidebar->setVisible(false);
        this->sidebar->setAutoHideEnabled(true);
        this->sidebarHiddenForResize = true;
    }
    else {
        this->sidebarHiddenForResize = false;

        if (!this->focusModeEnabled && this->appSettings->sidebarVisible()) {
            this->sidebar->setAutoHideEnabled(false);
            this->sidebar->setVisible(true);
        }
        else {
            this->sidebar->setAutoHideEnabled(true);
            this->sidebar->setVisible(false);
        }
    }

    adjustEditor();
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();

    switch (key) {
    case Qt::Key_Escape:
        if (this->isFullScreen()) {
            appActions->invoke(Actions::ToggleFullScreen);
        }
        break;
    case Qt::Key_Alt:
        if (this->isFullScreen() && appSettings->hideMenuBarInFullScreenEnabled()) {
            if (!this->menuBar()->isVisible()) {
                this->menuBar()->show();
            } else {
                this->menuBar()->hide();
            }
        }
        break;
    case Qt::Key_Tab:
        if (findReplace->isVisible() && findReplace->hasFocus()) {
            findReplace->keyPressEvent(e);
            return;
        }
        else if (!this->editor->hasFocus()) {
            QMainWindow::keyPressEvent(e);
        }
        break;
    default:
        break;
    }

    QMainWindow::keyPressEvent(e);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (this->isFullScreen() && appSettings->hideMenuBarInFullScreenEnabled()) {
        if ((this->menuBar() == obj) 
                && (QEvent::Leave == event->type()) 
                && !menuBarMenuActivated) {
            this->menuBar()->hide();
        } else if (QEvent::MouseMove == event->type()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if ((mouseEvent->globalY()) <= 0 && !this->menuBar()->isVisible()) {
                this->menuBar()->show();
            }
        } else if ((this == obj) 
                && (((QEvent::Leave == event->type()) && !menuBarMenuActivated) 
                    || (QEvent::WindowDeactivate == event->type()))) {
            this->menuBar()->hide();
        }
    }

    return false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (documentManager->close()) {
        this->quitApplication();
    } else {
        event->ignore();
    }
}

void MainWindow::quitApplication()
{
    if (documentManager->close()) {
        appSettings->store();

        QSettings windowSettings;

        windowSettings.setValue(GW_MAIN_WINDOW_GEOMETRY_KEY, saveGeometry());
        windowSettings.setValue(GW_MAIN_WINDOW_STATE_KEY, saveState());
        windowSettings.setValue(GW_SPLITTER_GEOMETRY_KEY, splitter->saveState());
        windowSettings.sync();

        DictionaryManager::instance()->addProviders();
        DictionaryManager::instance()->setDefaultLanguage(language);

        this->editor->document()->disconnect();
        this->editor->disconnect();
        this->htmlPreview->disconnect();
        StyleSheetBuilder::clearCache();

        qApp->quit();
    }
}

void MainWindow::changeTheme()
{
    ThemeSelectionDialog *themeDialog = new ThemeSelectionDialog(theme.name(), appSettings->darkModeEnabled(), this);
    themeDialog->setAttribute(Qt::WA_DeleteOnClose);

    this->connect
    (
        themeDialog,
        &ThemeSelectionDialog::finished,
        [this, themeDialog](int result) {
            Q_UNUSED(result)
            this->theme = themeDialog->theme();
            applyTheme();
        }
    );

    themeDialog->open();
}

void MainWindow::openPreferencesDialog()
{
    PreferencesDialog *preferencesDialog = new PreferencesDialog(this);
    preferencesDialog->show();
}

void MainWindow::toggleHtmlPreview(bool checked)
{
    htmlPreview->setVisible(checked);
    htmlPreview->updatePreview();

    appSettings->setHtmlPreviewVisible(checked);
    this->update();
    adjustEditor();
}

void MainWindow::toggleHemingwayMode(bool checked)
{
    if (checked) {
        editor->setHemingWayModeEnabled(true);
    } else {
        editor->setHemingWayModeEnabled(false);
    }
}

void MainWindow::toggleFocusMode(bool checked)
{
    this->focusModeEnabled = checked;

    if (checked) {
        editor->setFocusMode(appSettings->focusMode());
        sidebar->setVisible(false);
        sidebar->setAutoHideEnabled(true);
    } else {
        editor->setFocusMode(FocusModeDisabled);

        if (!this->sidebarHiddenForResize && this->appSettings->sidebarVisible()) {
            sidebar->setAutoHideEnabled(false);
            sidebar->setVisible(true);
        }
    }
}

void MainWindow::toggleFullScreen(bool checked)
{
    static bool lastStateWasMaximized = false;

    if (this->isFullScreen() || !checked) {
        if (appSettings->displayTimeInFullScreenEnabled()) {
            timeIndicator->hide();
        }

        // If the window had been maximized prior to entering
        // full screen mode, then put the window back to
        // to maximized.  Don't call showNormal(), as that
        // doesn't restore the window to maximized.
        //
        if (lastStateWasMaximized) {
            showMaximized();
        }
        // Put the window back to normal (not maximized).
        else {
            showNormal();
        }

        if (appSettings->hideMenuBarInFullScreenEnabled()) {
            this->menuBar()->show();
        }
    } else {
        if (appSettings->displayTimeInFullScreenEnabled()) {
            timeIndicator->show();
        }

        if (this->isMaximized()) {
            lastStateWasMaximized = true;
        } else {
            lastStateWasMaximized = false;
        }

        showFullScreen();

        if (appSettings->hideMenuBarInFullScreenEnabled()) {
            this->menuBar()->hide();
        }
    }
}

void MainWindow::toggleHideMenuBarInFullScreen(bool checked)
{
    if (this->isFullScreen()) {
        if (checked) {
            this->menuBar()->hide();
        } else {
            this->menuBar()->show();
        }
    }
}

void MainWindow::toggleFileHistoryEnabled(bool checked)
{
    if (!checked) {
        this->clearRecentFileHistory();
    }

    documentManager->setFileHistoryEnabled(checked);
}

void MainWindow::toggleDisplayTimeInFullScreen(bool checked)
{
    if (this->isFullScreen()) {
        if (checked) {
            this->timeIndicator->show();
        } else {
            this->timeIndicator->hide();
        }
    }
}

void MainWindow::changeEditorWidth(EditorWidth editorWidth)
{
    editor->setEditorWidth(editorWidth);
    adjustEditor();
}

void MainWindow::changeInterfaceStyle(InterfaceStyle style)
{
    Q_UNUSED(style);

    applyTheme();
}

void MainWindow::insertImage()
{
    QString startingDirectory = QString();
    MarkdownDocument *document = documentManager->document();

    if (!document->isNew()) {
        startingDirectory = QFileInfo(document->filePath()).dir().path();
    }

    QString imagePath =
        QFileDialog::getOpenFileName
        (
            this,
            tr("Insert Image"),
            startingDirectory,
            QString("%1 (*.jpg *.jpeg *.gif *.png *.bmp);; %2")
            .arg(tr("Images"))
            .arg(tr("All Files"))
        );

    if (!imagePath.isNull() && !imagePath.isEmpty()) {
        QFileInfo imgInfo(imagePath);
        bool isRelativePath = false;

        if (imgInfo.exists()) {
            if (!document->isNew()) {
                QFileInfo docInfo(document->filePath());

                if (docInfo.exists()) {
                    imagePath = docInfo.dir().relativeFilePath(imagePath);
                    isRelativePath = true;
                }
            }
        }

        if (!isRelativePath) {
            imagePath = QString("file://") + imagePath;
        }

        QTextCursor cursor = editor->textCursor();
        cursor.insertText(QString("![](%1)").arg(imagePath));
    }
}

void MainWindow::showQuickReferenceGuide()
{
    QDesktopServices::openUrl(QUrl("https://wereturtle.github.io/ghostwriter/quickrefguide.html"));
}

void MainWindow::showWikiPage()
{
    QDesktopServices::openUrl(QUrl("https://github.com/wereturtle/ghostwriter/wiki"));
}

void MainWindow::showAbout()
{
    QString aboutText =
        QString("<p><b>") +  qAppName() + QString(" ")
        + qApp->applicationVersion() + QString("</b></p>")
        + tr("<p>Copyright &copy; 2014-2022 wereturtle</b>"
             "<p>You may use and redistribute this software under the terms of the "
             "<a href=\"http://www.gnu.org/licenses/gpl.html\">"
             "GNU General Public License Version 3</a>.</p>"
             "<p>Visit the official website at "
             "<a href=\"http://github.com/wereturtle/ghostwriter\">"
             "http://github.com/wereturtle/ghostwriter</a>.</p>"
             "<p>Special thanks and credit for reused code goes to</p>"
             "<p><a href=\"mailto:graeme@gottcode.org\">Graeme Gott</a>, "
             "author of "
             "<a href=\"http://gottcode.org/focuswriter/\">FocusWriter</a><br/>"
             "Dmitry Shachnev, author of "
             "<a href=\"http://sourceforge.net/p/retext/home/ReText/\">Retext</a><br/>"
             "<a href=\"mailto:gabriel@teuton.org\">Gabriel M. Beddingfield</a>, "
             "author of <a href=\"http://www.teuton.org/~gabriel/stretchplayer/\">"
             "StretchPlayer</a><br/>"
             "<p>I am also deeply indebted to "
             "<a href=\"mailto:w.vollprecht@gmail.com\">Wolf Vollprecht</a>, "
             "the author of "
             "<a href=\"http://uberwriter.wolfvollprecht.de/\">UberWriter</a>, "
             "for the inspiration he provided in creating such a beautiful "
             "Markdown editing tool.</p>");

    QMessageBox::about(this, tr("About %1").arg(qAppName()), aboutText);
}

void MainWindow::changeFocusMode(FocusMode focusMode)
{
    if (FocusModeDisabled != editor->focusMode()) {
        editor->setFocusMode(focusMode);
    }
}

void MainWindow::refreshRecentFiles()
{
    if (appSettings->fileHistoryEnabled()) {
        DocumentHistory history;
        QStringList recentFiles = history.recentFiles(MAX_RECENT_FILES + 1);
        MarkdownDocument *document = documentManager->document();

        if (!document->isNew()) {
            QString sanitizedPath =
                QFileInfo(document->filePath()).absoluteFilePath();
            recentFiles.removeAll(sanitizedPath);
        }

        for (int i = 0;
                (i < recentFilesActions.size()) && (i < recentFiles.size());
                i++) {
            QAction *action = recentFilesActions.at(i);

            if (nullptr != action) {
                action->setText(recentFiles.at(i));
                action->setData(recentFiles.at(i));
                action->setVisible(true);
            }
        }

        for (int i = recentFiles.size(); i < recentFilesActions.size(); i++) {
            QAction *action = recentFilesActions.at(i);

            if (nullptr != action) {
                action->setVisible(false);
            }
        }
    }
}

void MainWindow::clearRecentFileHistory()
{
    DocumentHistory history;
    history.clear();

    for (QAction *action : recentFilesActions) {
        if (nullptr != action) {
            action->setVisible(false);
        }
    }
}

void MainWindow::changeDocumentDisplayName(const QString &displayName)
{
    setWindowTitle(displayName + QString("[*] - ") + qAppName());

    if (documentManager->document()->isModified()) {
        setWindowModified(!appSettings->autoSaveEnabled());
    } else {
        setWindowModified(false);
    }
}

void MainWindow::onOperationStarted(const QString &description)
{
    if (!description.isNull()) {
        statusIndicator->setText(description);
    }

    statisticsIndicator->hide();
    statusIndicator->show();
    this->update();
    qApp->processEvents();
}

void MainWindow::onOperationFinished()
{
    statusIndicator->setText(QString());
    statisticsIndicator->show();
    statusIndicator->hide();
    this->update();
    qApp->processEvents();
}

void MainWindow::changeFont()
{
    bool success;

    QFont font =
        SimpleFontDialog::font(&success, editor->font(), this);

    if (success) {
        editor->setFont(font.family(), font.pointSize());
        appSettings->setEditorFont(font);
    }
}

void MainWindow::onFontSizeChanged(int size)
{
    QFont font = editor->font();
    font.setPointSize(size);
    appSettings->setEditorFont(font);
}

void MainWindow::onSetLocale()
{
    bool ok;

    QString locale =
        LocaleDialog::locale
        (
            &ok,
            appSettings->locale(),
            appSettings->translationsPath()
        );

    if (ok && (locale != appSettings->locale())) {
        appSettings->setLocale(locale);

        QMessageBox::information
        (
            this,
            QApplication::applicationName(),
            tr("Please restart the application for changes to take effect.")
        );
    }
}

void MainWindow::copyHtml()
{
    Exporter *htmlExporter = appSettings->currentHtmlExporter();

    if (nullptr != htmlExporter) {
        QTextCursor c = editor->textCursor();
        QString markdownText;
        QString html;

        if (c.hasSelection()) {
            // Get only selected text from the document.
            markdownText = c.selection().toPlainText();
        } else {
            // Get all text from the document.
            markdownText = editor->toPlainText();
        }

        // Convert Markdown to HTML.
        htmlExporter->exportToHtml(markdownText, html);

        // Insert HTML into clipboard.
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(html);
    }
}

void MainWindow::showPreviewOptions()
{
    static PreviewOptionsDialog *dialog = new PreviewOptionsDialog(this);
    
    dialog->setModal(false);
    dialog->show();
}

void MainWindow::onAboutToHideMenuBarMenu()
{
    menuBarMenuActivated = false;

    if (!this->menuBar()->underMouse()
            && this->isFullScreen()
            && appSettings->hideMenuBarInFullScreenEnabled()
            && this->menuBar()->isVisible()) {
        this->menuBar()->hide();
    }
}

void MainWindow::onAboutToShowMenuBarMenu()
{
    menuBarMenuActivated = true;

    if (this->isFullScreen()
            && appSettings->hideMenuBarInFullScreenEnabled()
            && !this->menuBar()->isVisible()) {
        this->menuBar()->show();
    }
}

void MainWindow::onSidebarVisibilityChanged(bool visible)
{
    if (!visible) {
        editor->setFocus();
    }

    this->adjustEditor();
}

void MainWindow::toggleSidebarVisible(bool visible)
{
    this->appSettings->setSidebarVisible(visible);

    if (!this->sidebarHiddenForResize
            && !this->focusModeEnabled
            && this->appSettings->sidebarVisible()) {
        sidebar->setAutoHideEnabled(false);
    }
    else {
        sidebar->setAutoHideEnabled(true);
    }

    this->sidebar->setVisible(visible);
    this->sidebar->setFocus();
    adjustEditor();
}

QAction* MainWindow::createWindowAction
(
    const QString &text,
    QObject *receiver,
    const char *member,
    const QKeySequence &shortcut
)
{
    QAction* action = new QAction(text, this);
    action->setShortcut(shortcut);
    action->setShortcutContext(Qt::WindowShortcut);
    
    connect(action, SIGNAL(triggered(bool)), receiver, member);
    this->addAction(action);

    return action;
}

QAction* MainWindow::createWidgetAction
(
    const QString &text,
    QWidget *receiver,
    const char *member,
    const QKeySequence &shortcut
)
{
    QAction* action = new QAction(text, receiver);
    action->setShortcut(shortcut);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    
    connect(action, SIGNAL(triggered(bool)), receiver, member);
    receiver->addAction(action);

    return action;
}

void MainWindow::buildMenuBar()
{
    QMenu *fileMenu = this->menuBar()->addMenu(tr("&File"));

    fileMenu->addAction(appActions->registerHandler(Actions::NewFile,
        [this]() {
            documentManager->close();
        }
    ));
    fileMenu->addAction(appActions->registerHandler(Actions::OpenFile,
        [this]() {
            documentManager->open();
        }
    ));

    QMenu *recentFilesMenu = new QMenu(tr("Open &Recent..."));
    recentFilesMenu->addAction(appActions->action(Actions::ReopenLast));
    recentFilesMenu->addSeparator();

    for (QAction *action : recentFilesActions) {
        recentFilesMenu->addAction(action);
    }

    recentFilesMenu->addSeparator();
    recentFilesMenu->addAction(appActions->action(Actions::ClearHistory));

    fileMenu->addMenu(recentFilesMenu);

    fileMenu->addSeparator();
    fileMenu->addAction(appActions->action(Actions::Save));
    fileMenu->addAction(appActions->action(Actions::SaveAs));
    fileMenu->addAction(appActions->action(Actions::RenameFile));
    fileMenu->addAction(appActions->action(Actions::ReloadFile));
    fileMenu->addSeparator();
    fileMenu->addAction(appActions->action(Actions::ExportFile));
    fileMenu->addSeparator();
    fileMenu->addAction(appActions->action(Actions::Quit));

    QMenu *editMenu = this->menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(appActions->action(Actions::Undo));
    editMenu->addAction(appActions->action(Actions::Redo));
    editMenu->addSeparator();
    editMenu->addAction(appActions->action(Actions::Cut));
    editMenu->addAction(appActions->action(Actions::Copy));
    editMenu->addAction(appActions->action(Actions::Paste));
    editMenu->addAction(appActions->action(Actions::CopyHtml));
    editMenu->addSeparator();
    editMenu->addAction(appActions->action(Actions::InsertImage));
    editMenu->addSeparator();

    editMenu->addAction(appActions->action(Actions::Find));
    editMenu->addAction(appActions->action(Actions::Replace));
    editMenu->addAction(appActions->action(Actions::FindNext));
    editMenu->addAction(appActions->action(Actions::FindPrevious));
    editMenu->addSeparator();
    editMenu->addAction(appActions->action(Actions::SpellCheck));

    QMenu *formatMenu = this->menuBar()->addMenu(tr("For&mat"));
    formatMenu->addAction(appActions->action(Actions::Bold));
    formatMenu->addAction(appActions->action(Actions::Italic));
    formatMenu->addAction(appActions->action(Actions::Strikethrough));
    formatMenu->addAction(appActions->action(Actions::HtmlComment));
    formatMenu->addSeparator();

    formatMenu->addAction(appActions->action(Actions::Indent));
    formatMenu->addAction(appActions->action(Actions::Unindent));
    formatMenu->addSeparator();
    formatMenu->addAction(appActions->action(Actions::BlockQuote));
    formatMenu->addAction(appActions->action(Actions::StripBlockQuote));
    formatMenu->addSeparator();
    formatMenu->addAction(appActions->action(Actions::BulletListAsterisk));
    formatMenu->addAction(appActions->action(Actions::BulletListMinus));
    formatMenu->addAction(appActions->action(Actions::BulletListPlus));
    formatMenu->addSeparator();
    formatMenu->addAction(appActions->action(Actions::NumberedListPeriod));
    formatMenu->addAction(appActions->action(Actions::NumberedListParenthesis));
    formatMenu->addSeparator();
    formatMenu->addAction(appActions->action(Actions::TaskList));
    formatMenu->addAction(appActions->action(Actions::ToggleTaskComplete));

    QMenu *viewMenu = this->menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(appActions->action(Actions::ToggleFullScreen));
    viewMenu->addAction(appActions->action(Actions::ToggleHtmlPreview));
    viewMenu->addAction(appActions->action(Actions::ToggleSidebar));
    viewMenu->addAction(appActions->action(Actions::ToggleDarkMode));
    viewMenu->addAction(appActions->action(Actions::ToggleHemingwayMode));
    viewMenu->addAction(appActions->action(Actions::ToggleDistractionFreeMode));
    viewMenu->addAction(appActions->action(Actions::ShowOutline));
    viewMenu->addAction(appActions->action(Actions::ShowSessionStatistics));
    viewMenu->addAction(appActions->action(Actions::ShowDocumentStatistics));
    viewMenu->addAction(appActions->action(Actions::ShowCheatSheet));
    
    viewMenu->addSeparator();
    viewMenu->addAction(appActions->action(Actions::ZoomIn));
    viewMenu->addAction(appActions->action(Actions::ZoomOut));

    QMenu *settingsMenu = this->menuBar()->addMenu(tr("&Settings"));
    settingsMenu->addAction(appActions->action(Actions::ShowThemes));
    settingsMenu->addAction(appActions->action(Actions::ShowFonts));
    settingsMenu->addAction(appActions->action(Actions::ShowAppLanguages));
    settingsMenu->addAction(appActions->action(Actions::ShowPreviewOptions));
    settingsMenu->addAction(appActions->action(Actions::ShowPreferences));

    QMenu *helpMenu = this->menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(appActions->action(Actions::About));
    helpMenu->addAction(appActions->action(Actions::AboutQt));
    helpMenu->addAction(appActions->action(Actions::Documentation));
    helpMenu->addAction(appActions->action(Actions::Wiki));

    connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowMenuBarMenu()));
    connect(fileMenu, SIGNAL(aboutToHide()), this, SLOT(onAboutToHideMenuBarMenu()));
    connect(editMenu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowMenuBarMenu()));
    connect(editMenu, SIGNAL(aboutToHide()), this, SLOT(onAboutToHideMenuBarMenu()));
    connect(formatMenu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowMenuBarMenu()));
    connect(formatMenu, SIGNAL(aboutToHide()), this, SLOT(onAboutToHideMenuBarMenu()));
    connect(viewMenu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowMenuBarMenu()));
    connect(viewMenu, SIGNAL(aboutToHide()), this, SLOT(onAboutToHideMenuBarMenu()));
    connect(settingsMenu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowMenuBarMenu()));
    connect(settingsMenu, SIGNAL(aboutToHide()), this, SLOT(onAboutToHideMenuBarMenu()));
    connect(helpMenu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowMenuBarMenu()));
    connect(helpMenu, SIGNAL(aboutToHide()), this, SLOT(onAboutToHideMenuBarMenu()));
}

void MainWindow::buildStatusBar()
{
    QGridLayout *statusBarLayout = new QGridLayout();
    statusBarLayout->setSpacing(0);
    statusBarLayout->setContentsMargins(0, 0, 0, 0);

    statusBarLayout->addWidget(this->findReplace, 0, 0, 1, 3);

    // Divide the status bar into thirds for placing widgets.
    QFrame *leftWidget = new QFrame(this->statusBar());
    leftWidget->setObjectName("leftStatusBarWidget");
    leftWidget->setStyleSheet("#leftStatusBarWidget { border: 0; margin: 0; padding: 0 }");
    QFrame *midWidget = new QFrame(this->statusBar());
    midWidget->setObjectName("midStatusBarWidget");
    midWidget->setStyleSheet("#midStatusBarWidget { border: 0; margin: 0; padding: 0 }");
    QFrame *rightWidget = new QFrame(this->statusBar());
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
    QFont buttonFont(this->awesome->font(style::stfas, 16));

    QToolButton *button = new QToolButton(this);
    button->setDefaultAction(appActions->action(Actions::ToggleSidebar));
    button->setText(QChar(fa::chevronright));
    button->setObjectName("showSidebarButton");
    button->setFont(buttonFont);
    button->setFocusPolicy(Qt::NoFocus);

    leftLayout->addWidget(button, 0, Qt::AlignLeft);
    statusBarWidgets.append(button);

    if (appSettings->sidebarVisible()) {
        button->setText(QChar(fa::chevronleft));
    }

    // Change the icon whenever the sidebar button's action is toggled.
    appActions->registerHandler(Actions::ToggleSidebar,
        [button](bool checked) {
            if (checked) {
                button->setText(QChar(fa::chevronleft));
            } else {
                button->setText(QChar(fa::chevronright));
            }
        }
    );
    
    timeIndicator = new TimeLabel(this);
    leftLayout->addWidget(timeIndicator, 0, Qt::AlignLeft);
    leftWidget->setContentsMargins(0, 0, 0, 0);
    statusBarWidgets.append(timeIndicator);

    if (!this->isFullScreen() || appSettings->displayTimeInFullScreenEnabled()) {
        timeIndicator->hide();
    }

    statusBarLayout->addWidget(leftWidget, 1, 0, 1, 1, Qt::AlignLeft);

    // Add middle widgets to status bar.
    statusIndicator = new QLabel();
    midLayout->addWidget(statusIndicator, 0, Qt::AlignCenter);
    statusIndicator->hide();

    statisticsIndicator = new StatisticsIndicator(this->documentStats, this->sessionStats, this);

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
    button = new QToolButton();
    button->setDefaultAction(appActions->action(Actions::ToggleDarkMode));
    button->setText(QChar(fa::moon));
    button->setFont(buttonFont);
    button->setFocusPolicy(Qt::NoFocus);
    appActions->registerHandler(Actions::ToggleDarkMode,
        [button](bool checked) {
            Q_UNUSED(checked)
            button->setText(QChar(fa::moon));
        }
    );

    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);

    button = new QToolButton();
    button->setDefaultAction(appActions->action(Actions::ToggleHtmlPreview));
    button->setText(QChar(fa::code));
    button->setFont(buttonFont);
    button->setFocusPolicy(Qt::NoFocus);
    appActions->registerHandler(Actions::ToggleHtmlPreview,
        [button](bool checked) {
            Q_UNUSED(checked)
            button->setText(QChar(fa::code));
        }
    );

    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);

    button = new QToolButton();
    button->setDefaultAction(appActions->action(Actions::ToggleHemingwayMode));
    button->setText(QChar(fa::backspace));
    button->setFont(buttonFont);
    button->setFocusPolicy(Qt::NoFocus);
    appActions->registerHandler(Actions::ToggleHemingwayMode,
        [button](bool checked) {
            Q_UNUSED(checked)
            button->setText(QChar(fa::backspace));
        }
    );

    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);

    button = new QToolButton();
    button->setDefaultAction(appActions->action(Actions::ToggleDistractionFreeMode));
    button->setText(QChar(fa::headphonesalt));
    button->setFont(buttonFont);
    button->setFocusPolicy(Qt::NoFocus);
    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);
    appActions->registerHandler(Actions::ToggleDistractionFreeMode,
        [button](bool checked) {
            Q_UNUSED(checked)
            button->setText(QChar(fa::headphonesalt));
        }
    );

    button = new QToolButton();
    button->setDefaultAction(appActions->action(Actions::ToggleFullScreen));
    button->setText(QChar(fa::expand));
    button->setFont(buttonFont);
    button->setFocusPolicy(Qt::NoFocus);
    rightLayout->addWidget(button, 0, Qt::AlignRight);
    statusBarWidgets.append(button);
    appActions->registerHandler(Actions::ToggleFullScreen,
        [button](bool checked) {
            Q_UNUSED(checked)
            button->setText(QChar(fa::expand));
        }
    );

    rightWidget->setContentsMargins(0, 0, 0, 0);
    statusBarLayout->addWidget(rightWidget, 1, 2, 1, 1, Qt::AlignRight);
    
    QWidget *container = new QWidget(this);
    container->setObjectName("statusBarWidgetContainer");
    container->setLayout(statusBarLayout);
    container->setContentsMargins(0, 0, 2, 0);
    container->setStyleSheet("#statusBarWidgetContainer { border: 0; margin: 0; padding: 0 }");

    this->statusBar()->addWidget(container, 1);
    this->statusBar()->setSizeGripEnabled(false);
}

void MainWindow::registerActionHandlers()
{
    // Initialize checked/unchecked states of any checkable actions.
    // Note: Need to do this before registering handlers to prevent the handlers
    // from being invoked.
    appActions->action(Actions::ToggleFullScreen)->setChecked(this->isFullScreen());
    appActions->action(Actions::ToggleHtmlPreview)->setChecked(appSettings->htmlPreviewVisible());
    appActions->action(Actions::ToggleSidebar)->setChecked(appSettings->sidebarVisible());
    appActions->action(Actions::ToggleDarkMode)->setChecked(appSettings->darkModeEnabled());
    appActions->action(Actions::ToggleHemingwayMode)->setChecked(false);
    appActions->action(Actions::ToggleDistractionFreeMode)->setChecked(false);

    appActions->registerHandler(Actions::NewFile,
        [this]() {
            documentManager->close();
        }
    );
    appActions->registerHandler(Actions::OpenFile,
        [this]() {
            documentManager->open();
        }
    );

    appActions->registerHandler(Actions::ReopenLast, documentManager, &DocumentManager::reopenLastClosedFile);

    for (QAction *action : recentFilesActions) {
        appActions->registerHandler(
            action->objectName(),
            [this, action]() {
                // Use the action's data for access to the actual file path, since
                // KDE Plasma will add a keyboard accelerator to the action's text
                // by inserting an ampersand (&) into it.
                //
                if (nullptr != action) {
                    documentManager->open(action->data().toString());
                }

                refreshRecentFiles();
            }
        );
    }

    appActions->registerHandler(Actions::Save,
        [this]() {
            documentManager->save();
        }
    );
    appActions->registerHandler(Actions::SaveAs,
        [this]() {
            documentManager->saveAs();
        }
    );
    appActions->registerHandler(Actions::RenameFile, documentManager, &DocumentManager::rename);
    appActions->registerHandler(Actions::ReloadFile, documentManager, &DocumentManager::reload);
    appActions->registerHandler(Actions::ExportFile, documentManager, &DocumentManager::exportFile);
    appActions->registerHandler(Actions::Quit, this, &MainWindow::quitApplication);
    appActions->registerHandler(Actions::Undo, (QPlainTextEdit*)editor, &MarkdownEditor::undo);
    appActions->registerHandler(Actions::Redo, (QPlainTextEdit*)editor, &MarkdownEditor::redo);
    appActions->registerHandler(Actions::Cut, (QPlainTextEdit*)editor, &MarkdownEditor::cut);
    appActions->registerHandler(Actions::Copy, (QPlainTextEdit*)editor, &MarkdownEditor::copy);
    appActions->registerHandler(Actions::Paste, (QPlainTextEdit*)editor, &MarkdownEditor::paste);
    appActions->registerHandler(Actions::CopyHtml, this, &MainWindow::copyHtml);
    appActions->registerHandler(Actions::InsertImage, this, &MainWindow::insertImage);
    appActions->registerHandler(Actions::Find, findReplace, &FindReplace::showFindView);
    appActions->registerHandler(Actions::Replace, findReplace, &FindReplace::showReplaceView);
    appActions->registerHandler(Actions::FindNext, findReplace, &FindReplace::findNext);
    appActions->registerHandler(Actions::FindPrevious, findReplace, &FindReplace::findPrevious);
    appActions->registerHandler(Actions::SpellCheck, spelling, &SpellCheckDecorator::runSpellCheck);
    appActions->registerHandler(Actions::Bold, editor, &MarkdownEditor::bold);
    appActions->registerHandler(Actions::Italic, editor, &MarkdownEditor::italic);
    appActions->registerHandler(Actions::Strikethrough, editor, &MarkdownEditor::strikethrough);
    appActions->registerHandler(Actions::HtmlComment, editor, &MarkdownEditor::insertComment);
    appActions->registerHandler(Actions::Indent, editor, &MarkdownEditor::indentText);
    appActions->registerHandler(Actions::Unindent, editor, &MarkdownEditor::unindentText);
    appActions->registerHandler(Actions::BlockQuote, editor, &MarkdownEditor::createBlockquote);
    appActions->registerHandler(Actions::StripBlockQuote, editor, &MarkdownEditor::removeBlockquote);
    appActions->registerHandler(Actions::BulletListAsterisk, editor, &MarkdownEditor::createBulletListWithAsteriskMarker);
    appActions->registerHandler(Actions::BulletListMinus, editor, &MarkdownEditor::createBulletListWithMinusMarker);
    appActions->registerHandler(Actions::BulletListPlus, editor, &MarkdownEditor::createBulletListWithPlusMarker);
    appActions->registerHandler(Actions::NumberedListPeriod, editor, &MarkdownEditor::createNumberedListWithPeriodMarker);
    appActions->registerHandler(Actions::NumberedListParenthesis, editor, &MarkdownEditor::createNumberedListWithParenthesisMarker);
    appActions->registerHandler(Actions::TaskList, editor, &MarkdownEditor::createTaskList);
    appActions->registerHandler(Actions::ToggleTaskComplete, editor, &MarkdownEditor::toggleTaskComplete);
    appActions->registerHandler(Actions::ToggleFullScreen, this, &MainWindow::toggleFullScreen);
    appActions->registerHandler(Actions::ToggleHtmlPreview, this, &MainWindow::toggleHtmlPreview);
    appActions->registerHandler(Actions::ToggleSidebar, this, &MainWindow::toggleSidebarVisible);
    appActions->registerHandler(Actions::ToggleHemingwayMode, this, &MainWindow::toggleHemingwayMode);
    appActions->registerHandler(Actions::ToggleDistractionFreeMode, this, &MainWindow::toggleFocusMode);

    appActions->registerHandler(
        Actions::ToggleDarkMode,
        [this](bool checked) {
            appSettings->setDarkModeEnabled(checked);
            applyTheme();
        }
    );

    appActions->registerHandler(
        Actions::ShowOutline,
        [this]() {
            sidebar->setVisible(true);
            sidebar->setCurrentTabIndex(OutlineSidebarTab);
        }
    );

    appActions->registerHandler(
        Actions::ShowSessionStatistics,
        [this]() {
            sidebar->setVisible(true);
            sidebar->setCurrentTabIndex(SessionStatsSidebarTab);
        }
    );

    appActions->registerHandler(
        Actions::ShowDocumentStatistics,
        [this]() {
            sidebar->setVisible(true);
            sidebar->setCurrentTabIndex(DocumentStatsSidebarTab);
        }
    );

    appActions->registerHandler(
        Actions::ShowCheatSheet,
        [this]() {
            sidebar->setVisible(true);
            sidebar->setCurrentTabIndex(CheatSheetSidebarTab);
        }
    );

    appActions->registerHandler(Actions::ZoomIn, editor, &MarkdownEditor::increaseFontSize);
    appActions->registerHandler(Actions::ZoomOut, editor, &MarkdownEditor::decreaseFontSize);
    appActions->registerHandler(Actions::ShowThemes, this, &MainWindow::changeTheme);
    appActions->registerHandler(Actions::ShowFonts, this, &MainWindow::changeFont);
    appActions->registerHandler(Actions::ShowAppLanguages, this, &MainWindow::onSetLocale);
    appActions->registerHandler(Actions::ShowPreviewOptions, this, &MainWindow::showPreviewOptions);
    appActions->registerHandler(Actions::ShowPreferences, this, &MainWindow::openPreferencesDialog);
    appActions->registerHandler(Actions::About, this, &MainWindow::showAbout);
    appActions->registerHandler(Actions::AboutQt, []() { qApp->aboutQt(); } );
    appActions->registerHandler(Actions::Documentation, this, &MainWindow::showQuickReferenceGuide);
    appActions->registerHandler(Actions::Wiki, this, &MainWindow::showWikiPage);

    // Add all actions with shortcuts to this window to ensure that the
    // shortcuts activate their respective actions when the menu bar is hidden.
    appActions->addActionShortcutsToWidget(this);
}

void MainWindow::buildSidebar()
{
    this->showSidebarAction = new QAction(tr("Show Sidebar"), this);
    showSidebarAction->setCheckable(true);
    showSidebarAction->setChecked(appSettings->sidebarVisible());
    showSidebarAction->setShortcut(QKeySequence("CTRL+SPACE"));
    showSidebarAction->setShortcutContext(Qt::WindowShortcut);
    
    connect(this->showSidebarAction,
        &QAction::toggled,
        [this](bool visible) {
            this->toggleSidebarVisible(visible);
        });

    cheatSheetWidget = new QListWidget(this);

    cheatSheetWidget->setSelectionMode(QAbstractItemView::NoSelection);
    cheatSheetWidget->setAlternatingRowColors(false);

    cheatSheetWidget->addItem(tr("# Heading 1"));
    cheatSheetWidget->addItem(tr("## Heading 2"));
    cheatSheetWidget->addItem(tr("### Heading 3"));
    cheatSheetWidget->addItem(tr("#### Heading 4"));
    cheatSheetWidget->addItem(tr("##### Heading 5"));
    cheatSheetWidget->addItem(tr("###### Heading 6"));
    cheatSheetWidget->addItem(tr("*Emphasis* _Emphasis_"));
    cheatSheetWidget->addItem(tr("**Strong** __Strong__"));
    cheatSheetWidget->addItem(tr("1. Numbered List"));
    cheatSheetWidget->addItem(tr("* Bullet List"));
    cheatSheetWidget->addItem(tr("+ Bullet List"));
    cheatSheetWidget->addItem(tr("- Bullet List"));
    cheatSheetWidget->addItem(tr("> Block Quote"));
    cheatSheetWidget->addItem(tr("`Code Span`"));
    cheatSheetWidget->addItem(tr("``` Code Block"));
    cheatSheetWidget->addItem(tr("[Link](http://url.com \"Title\")"));
    cheatSheetWidget->addItem(tr("[Reference Link][ID]"));
    cheatSheetWidget->addItem(tr("[ID]: http://url.com \"Reference Definition\""));
    cheatSheetWidget->addItem(tr("![Image](./image.jpg \"Title\")"));
    cheatSheetWidget->addItem(tr("--- *** ___ Horizontal Rule"));

    documentStatsWidget = new DocumentStatisticsWidget();
    documentStatsWidget->setSelectionMode(QAbstractItemView::NoSelection);
    documentStatsWidget->setAlternatingRowColors(false);

    sessionStatsWidget = new SessionStatisticsWidget();
    sessionStatsWidget->setSelectionMode(QAbstractItemView::NoSelection);
    sessionStatsWidget->setAlternatingRowColors(false);

    outlineWidget = new OutlineWidget(editor, this);
    outlineWidget->setAlternatingRowColors(false);

    documentStats = new DocumentStatistics((MarkdownDocument *) editor->document(), this);
    connect(documentStats, &DocumentStatistics::wordCountChanged,
            documentStatsWidget, &DocumentStatisticsWidget::setWordCount);
    connect(documentStats, &DocumentStatistics::characterCountChanged,
            documentStatsWidget, &DocumentStatisticsWidget::setCharacterCount);
    connect(documentStats, &DocumentStatistics::sentenceCountChanged,
            documentStatsWidget, &DocumentStatisticsWidget::setSentenceCount);
    connect(documentStats, &DocumentStatistics::paragraphCountChanged,
            documentStatsWidget, &DocumentStatisticsWidget::setParagraphCount);
    connect(documentStats, &DocumentStatistics::pageCountChanged,
            documentStatsWidget, &DocumentStatisticsWidget::setPageCount);
    connect(documentStats, &DocumentStatistics::complexWordsChanged,
            documentStatsWidget, &DocumentStatisticsWidget::setComplexWords);
    connect(documentStats, &DocumentStatistics::readingTimeChanged,
            documentStatsWidget, &DocumentStatisticsWidget::setReadingTime);
    connect(documentStats, &DocumentStatistics::lixReadingEaseChanged,
            documentStatsWidget, &DocumentStatisticsWidget::setLixReadingEase);
    connect(documentStats, &DocumentStatistics::readabilityIndexChanged,
            documentStatsWidget, &DocumentStatisticsWidget::setReadabilityIndex);
    connect(editor, SIGNAL(textSelected(QString, int, int)), documentStats, SLOT(onTextSelected(QString, int, int)));
    connect(editor, SIGNAL(textDeselected()), documentStats, SLOT(onTextDeselected()));

    sessionStats = new SessionStatistics(this);
    connect(documentStats, SIGNAL(totalWordCountChanged(int)), sessionStats, SLOT(onDocumentWordCountChanged(int)));
    connect(sessionStats, SIGNAL(wordCountChanged(int)), sessionStatsWidget, SLOT(setWordCount(int)));
    connect(sessionStats, SIGNAL(pageCountChanged(int)), sessionStatsWidget, SLOT(setPageCount(int)));
    connect(sessionStats, SIGNAL(wordsPerMinuteChanged(int)), sessionStatsWidget, SLOT(setWordsPerMinute(int)));
    connect(sessionStats, SIGNAL(writingTimeChanged(int)), sessionStatsWidget, SLOT(setWritingTime(int)));
    connect(sessionStats, SIGNAL(idleTimePercentageChanged(int)), sessionStatsWidget, SLOT(setIdleTime(int)));
    connect(editor, SIGNAL(typingPaused()), sessionStats, SLOT(onTypingPaused()));
    connect(editor, SIGNAL(typingResumed()), sessionStats, SLOT(onTypingResumed()));

    sidebar = new Sidebar(this);
    sidebar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sidebar->setMinimumWidth(0.1 * QGuiApplication::primaryScreen()->availableSize().width());
    sidebar->setMaximumWidth(0.5 * QGuiApplication::primaryScreen()->availableSize().width());

    QPushButton *tabButton = new QPushButton();
    tabButton->setFont(this->awesome->font(style::stfas, 16));
    tabButton->setText(QChar(fa::hashtag));
    tabButton->setToolTip(tr("Outline"));
    sidebar->addTab(tabButton, outlineWidget);

    tabButton = new QPushButton();
    tabButton->setFont(this->awesome->font(style::stfas, 16));
    tabButton->setText(QChar(fa::tachometeralt));
    tabButton->setToolTip(tr("Session Statistics"));
    sidebar->addTab(tabButton, sessionStatsWidget);

    tabButton = new QPushButton();
    tabButton->setFont(this->awesome->font(style::stfas, 16));
    tabButton->setText(QChar(fa::chartbar));
    tabButton->setToolTip(tr("Document Statistics"));
    sidebar->addTab(tabButton, documentStatsWidget);

    tabButton = new QPushButton();
    tabButton->setFont(this->awesome->font(style::stfab, 16));
    tabButton->setText(QChar(fa::markdown));
    tabButton->setToolTip(tr("Cheat Sheet"));
    sidebar->addTab(tabButton, cheatSheetWidget);

    // We need to set an empty style for the scrollbar in order for the
    // scrollbar CSS stylesheet to take full effect.  Otherwise, the scrollbar's
    // background color will have the Windows 98 checkered look rather than
    // being a solid or transparent color.
    //
    outlineWidget->verticalScrollBar()->setStyle(new QCommonStyle());
    outlineWidget->horizontalScrollBar()->setStyle(new QCommonStyle());
    documentStatsWidget->verticalScrollBar()->setStyle(new QCommonStyle());
    documentStatsWidget->horizontalScrollBar()->setStyle(new QCommonStyle());
    sessionStatsWidget->verticalScrollBar()->setStyle(new QCommonStyle());
    sessionStatsWidget->horizontalScrollBar()->setStyle(new QCommonStyle());
    cheatSheetWidget->verticalScrollBar()->setStyle(new QCommonStyle());
    cheatSheetWidget->horizontalScrollBar()->setStyle(new QCommonStyle());

    int tabIndex = QSettings().value("sidebarCurrentTab", (int)FirstSidebarTab).toInt();

    if (tabIndex < 0 || tabIndex >= sidebar->tabCount()) {
        tabIndex = (int) FirstSidebarTab;
    }

    sidebar->setCurrentTabIndex(tabIndex);

    QPushButton *button = new QPushButton(QChar(fa::cog));
    button->setFont(this->awesome->font(style::stfas, 16));
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolTip(tr("Settings"));
    button->setCheckable(false);
    sidebar->addButton(button);
    this->connect
    (
        button,
        &QPushButton::clicked,
        [this, button]() {
            QMenu *popupMenu = new QMenu(button);
            popupMenu->addAction(tr("Themes..."), this, SLOT(changeTheme()));
            popupMenu->addAction(tr("Font..."), this, SLOT(changeFont()));
            popupMenu->addAction(tr("Application Language..."), this, SLOT(onSetLocale()));
            popupMenu->addAction(tr("Preview Options..."), this, SLOT(showPreviewOptions()));
            popupMenu->addAction(tr("Preferences..."), this, SLOT(openPreferencesDialog()))->setMenuRole(QAction::PreferencesRole);
            popupMenu->popup(button->mapToGlobal(QPoint(button->width() / 2, -(button->height() / 2) - 10)));
        }
    );

    this->connect(this->sidebar,
        &Sidebar::visibilityChanged,
        this,
        &MainWindow::onSidebarVisibilityChanged);
    
    if (!this->sidebarHiddenForResize
            && !this->focusModeEnabled
            && appSettings->sidebarVisible()) {
        this->sidebar->setAutoHideEnabled(false);
        this->sidebar->setVisible(true);
    }
    else {
        this->sidebar->setAutoHideEnabled(true);
        this->sidebar->setVisible(false);
    }
}

void MainWindow::adjustEditor()
{
    // Make sure editor size is updated.
    qApp->processEvents();

    int width = this->width();
    int sidebarWidth = 0;

    // Make sure live preview does not crowd out editor.
    // It should not take up more than 50% of the window space
    // left after the sidebar is accounted for.
    //
    if (this->sidebar->isVisible()) {
        sidebarWidth = this->sidebar->width();
    }

    this->htmlPreview->setMaximumWidth((width - sidebarWidth) / 2);

    // Resize the editor's margins.
    this->editor->setupPaperMargins();

    // Scroll to cursor position.
    this->editor->centerCursor();
}

void MainWindow::applyTheme()
{
    if (!theme.name().isNull() && !theme.name().isEmpty()) {
        appSettings->setThemeName(theme.name());
    }

    ColorScheme colorScheme = theme.lightColorScheme();

    if (appSettings->darkModeEnabled()) {
        colorScheme = theme.darkColorScheme();
    }

    StyleSheetBuilder styler(colorScheme,
        (InterfaceStyleRounded == appSettings->interfaceStyle()),
        appSettings->previewTextFont(),
        appSettings->previewCodeFont());

    editor->setColorScheme(colorScheme);
    editor->setStyleSheet(styler.editorStyleSheet());
    spelling->setErrorColor(colorScheme.error);

    // Do not call this->setStyleSheet().  Calling it more than once in a run
    // (i.e., when changing a theme) causes a crash in Qt 5.11.  Instead,
    // change the main window's style sheet via qApp.
    //
    qApp->setStyleSheet(styler.layoutStyleSheet());

    this->splitter->setStyleSheet(styler.splitterStyleSheet());
    this->statusBar()->setStyleSheet(styler.statusBarStyleSheet());

    foreach (QWidget *w, statusBarWidgets) {
        w->setStyleSheet(styler.statusBarWidgetsStyleSheet());
    }

    findReplace->setStyleSheet(styler.findReplaceStyleSheet());
    sidebar->setStyleSheet(styler.sidebarStyleSheet());

    // Clear style sheet cache by setting to empty string before
    // setting the new style sheet.
    //
    outlineWidget->setStyleSheet("");
    outlineWidget->setStyleSheet(styler.sidebarWidgetStyleSheet());
    cheatSheetWidget->setStyleSheet("");
    cheatSheetWidget->setStyleSheet(styler.sidebarWidgetStyleSheet());
    documentStatsWidget->setStyleSheet("");
    documentStatsWidget->setStyleSheet(styler.sidebarWidgetStyleSheet());
    sessionStatsWidget->setStyleSheet("");
    sessionStatsWidget->setStyleSheet(styler.sidebarWidgetStyleSheet());

    htmlPreview->setStyleSheet(styler.htmlPreviewCss());

    adjustEditor();
}

} // namespace ghostwriter
