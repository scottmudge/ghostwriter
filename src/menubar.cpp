#include <QAction>
#include <QMenu>
#include <QString>
#include <QApplication>
#include <QQueue>


#include "menubar.h"

#define MAX_RECENT_FILES 10;

namespace ghostwriter
{

class MenuBuilder
{
public:
    MenuBuilder(QMenu* menu, QWidget *parent) 
        : menu(menu), parent(parent) { }
    ~MenuBuilder() { }
    
    QAction *add(const QString &text,
        void(MenuBar::*signal)(),
        const QKeySequence& shortcut = QKeySequence()
    );

private:
    QMenu* menu;
    
    // Note:  Parent needs to be a QMainWindow
    //        (i.e., the parent of the MenuBar).
    QWidget* parent;
};

class MenuBarPrivate
{
    Q_DECLARE_PUBLIC(MenuBar)
    
public:
    MenuBarPrivate(MenuBar *q_ptr) 
        : q_ptr(q_ptr),
          autoHideEnabled(false),
          menuActivated(false) { }
    
    ~MenuBarPrivate() { }
    
    MenuBar *q_ptr;
    bool autoHideEnabled;
    bool menuActivated;
    QAction *recentFilesActions[MAX_RECENT_FILES];
    QList<QString> recentFiles;
    
    QMenu * addMenu() const;
    
    void buildFileMenu();
    void buildEditMenu();
    void buildFormatMenu();
    void buildViewMenu();
    void buildSettingsMenu();
    void buildHelpMenu();
    QMenu * buildRecentFilesSubmenu();
};
    
void MenuBar::MenuBar(QWidget *parent) :
    QMenuBar(parent), d_ptr(new MenuBarPrivate(this))
{
    Q_D(MenuBar);
    
    buildFileMenu();
    buildEditMenu();
    buildFormatMenu();
    buildViewMenu();
    buildSettingsMenu();
    buildHelpMenu();
}

void MenuBar::addRecentDocument(const QString &path)
{
    Q_D(MenuBar);
    
    // Make sure new recent file is unique.
    d->recentFiles.removeAll(path);
    
    // Insert new recent file at the front of the list
    // (since it is most recent).
    d->recentFiles.prepend(path);
    
    // Remove any excess number of recent files from the list.
    while (d->recentFiles.size() > MAX_RECENT_FILES) {
        d->recentFiles.removeLast();
    }
    
    // Cleanup the recent files actions list to reflect the actual recent
    // files list.
    for (int i = 0; i < MAX_RECENT_FILES; i++) {
        if (i < recentFiles.size()) {
            d->recentFilesActions[i]->setText(recentFiles.at(i));

            // Use the action's data for access to the actual file path, since
            // KDE Plasma will add a keyboard accelerator to the action's text
            // by inserting an ampersand (&) into it.
            d->recentFilesActions[i]->setData(recentFiles.at(i));
            
            // Show the action in the menu.
            d->recentFilesActions[i]->setVisible(true);
        }
        else {
            // No more recent files in the list!  Set all other actions to
            // be invisible.
            d->recentFilesActions[i]->setVisible(false);
        }
    }
}

void MenuBar::fullscreenToggled(bool enabled)
{
    Q_D(MenuBar);
    
    d->fullScreenAction->blockSignals(true);
    d->fullScreenAction->setChecked(enabled);
    d->fullScreenAction->blockSignals(false);
}

void MenuBar::htmlPreviewToggled(bool enabled)
{
    Q_D(MenuBar);
    
    d->previewAction->blockSignals(true);
    d->previewAction->setChecked(enabled);
    d->previewAction->blockSignals(false);
}

void MenuBar::sidebarToggled(bool enabled)
{
    Q_D(MenuBar);
    
    d->toggleSidebarAction->blockSignals(true);
    d->toggleSidebarAction->setChecked(enabled);
    d->toggleSidebarAction->blockSignals(false);
}

void MenuBar::setAutoHideEnabled(bool enabled)
{
    Q_D(MenuBar);
    
    d->autoHideEnabled = enabled;
    
    this->setVisible(enabled);
}

void MenuBar::clearRecentDocuments()
{
    Q_D(MenuBar);
    
    // Clear the recent file list.
    d->recentFiles.clear();
    
    // Hide all recent file actions in the menu.
    for (int i = 0; i < MAX_RECENT_FILES; i++) {
        d->recentFilesActions[i]->setVisible(false);
    }
}

QAction * MenuBuilder::add(
    const QString &text,
    void(MenuBar::*signal)(),
    const QKeySequence& shortcut
)
{
    QAction *action = new QAction(text, this->parent);
    
    action->setShortcut(shortcut);
    action->setShortcutContext(Qt::WindowShortcut);
    action->connect(action, &QAction::triggered, q, signal);
    this->menu->addAction(action);
    return action;
}

QMenu * MenuBarPrivate::addMenu(const QString &text)
{
    Q_Q(MenuBar);
    
    QMenu *menu = q->addMenu(text);
    
    q->connect(
        menu,
        &QMenu::aboutToShow
        [this, q]() {
            this->menuActivated = true;

            if (this->autoHideEnabled && !q->isVisible()) {
                q->show();
            }
        });
    
    q->connect(
        menu,
        &QMenu::aboutToHide
        [this, q]() {
            this->menuActivated = false;

            if (!q->underMouse()
                    && this->autoHideEnabled
                    && q->isVisible()) {
                q->hide();
            }
        });
    
    return menu;
}

void MenuBarPrivate::buildFileMenu()
{
    Q_Q(MenuBar);
    
    QMenu *menu = this->addMenu(tr("&File"));
    MenuBuilder builder(menu, q->parent());
    
    builder.add(tr("&New"), &MenuBar::createNewDocument, QKeySequence::New);
    builder.add(tr("&Open"), &MenuBar::openDocument, QKeySequence::Open);
    
    menu->addMenu(buildRecentFilesSubmenu());

    menu->addSeparator();
    builder.add(tr("&Save"), &MenuBar::saveActiveDocument, QKeySequence::Save);
    builder.add(tr("Save &As..."), &MenuBar::saveActiveDocumentAs, QKeySequence::SaveAs);
    builder.add(tr("R&ename..."), &MenuBar::renameActiveDocument);
    builder.add(tr("Re&load from Disk..."), &MenuBar::reloadActiveDocument);
    menu->addSeparator();
    builder.add(tr("&Export"), &MenuBar::exportActiveDocument, "CTRL+E");
    menu->addSeparator();
    
    QAction *quitAction =
        builder.add(tr("&Quit"), &MenuBar::quitApplication, QKeySequence::Quit);
    quitAction->setMenuRole(QAction::QuitRole);
}

void MenuBarPrivate::buildEditMenu()
{
    Q_Q(MenuBar);
    
    QMenu *menu = this->addMenu(tr("&Edit"));
    MenuBuilder builder(menu, q->parent());
    
    builder.add(tr("&Undo"), &MenuBar::undo, QKeySequence::Undo);
    builder.add(tr("&Redo"), &MenuBar::redo, QKeySequence::Redo);
    menu->addSeparator();
    builder.add(tr("Cu&t"), &MenuBar::cut, QKeySequence::Cut);
    builder.add(tr("&Copy"), &MenuBar::copy, QKeySequence::Copy);
    builder.add(tr("&Paste"), &MenuBar::paste, QKeySequence::Paste);
    builder.add(tr("Copy &HTML"), &MenuBar::copyHtml, "SHIFT+CTRL+C");
    menu->addSeparator();
    builder.add(tr("&Insert Image..."), &MenuBar::insertImage());
    menu->addSeparator();

    builder.add(tr("&Find"), &MenuBar::showFindView, QKeySequence::Find);
    builder.add(tr("Rep&lace"), &MenuBar::showReplaceView, QKeySequence::Replace);
    builder.add(tr("Find &Next"), &MenuBar::findNext, QKeySequence::FindNext);
    builder.add(tr("Find &Previous"), &MenuBar::findPrevious, QKeySequence::FindPrevious);
    menu->addSeparator();
    builder.add(tr("Select &All"), &MenuBar::selectAll, QKeySequence::SelectAll);
    menu->addSeparator();
    builder.add(tr("&Spell check"), &MenuBar::runSpellChecker);
}

void MenuBarPrivate::buildFormatMenu()
{
    Q_Q(MenuBar);
    
    QMenu *menu = this->addMenu(tr("For&mat"));
    MenuBuilder builder(menu, q->parent());
    
    builder.add(tr("&Bold"), &MenuBar::bold, QKeySequence::Bold);
    builder.add(tr("&Italic"), &MenuBar::italic, QKeySequence::Italic);
    builder.add(tr("Stri&kethrough"), &MenuBar::strikethrough, "Ctrl+K");
    builder.add(tr("&HTML Comment"), &MenuBar::insertComment, "Ctrl+/");
    menu->addSeparator();

    builder.add(tr("I&ndent"), &MenuBar::indentText, "Tab");
    builder.add(tr("&Unindent"), &MenuBar::unindentText, "Shift+Tab");
    menu->addSeparator();
    builder.add(tr("Block &Quote"), &MenuBar::createBlockquote, "Ctrl+.");
    builder.add(tr("&Strip Block Quote"), &MenuBar::removeBlockquote, "Ctrl+,");
    menu->addSeparator();
    builder.add(tr("&* Bullet List"), &MenuBar::bulletListWithAsteriskMarker, "Ctrl+8");
    builder.add(tr("&- Bullet List"), &MenuBar::bulletListWithMinusMarker, "Ctrl+Shift+-");
    builder.add(tr("&+ Bullet List"), &MenuBar::bulletListWithPlusMarker, "Ctrl+Shift+=");
    menu->addSeparator();
    builder.add(tr("1&. Numbered List"), &MenuBar::numberedListWithPeriodMarker, "Ctrl+1");
    builder.add(tr("1&) Numbered List"), &MenuBar::numberedListWithParenthesisMarker, "Ctrl+0");
    menu->addSeparator();
    builder.add(tr("&Task List"), &MenuBar::taskList, "Ctrl+T");
    builder.add(tr("Toggle Task(s) &Complete"), &MenuBar::toggleTaskComplete, "Ctrl+D");
}

void MenuBarPrivate::buildViewMenu()
{
    Q_Q(MenuBar);
    
    QMenu *menu = this->addMenu(tr("&View"));
    MenuBuilder builder(menu, q->parent());
    
    this->fullScreenAction = new QAction(tr("&Full Screen"), menu);
    this->fullScreenAction->setCheckable(true);
    this->fullScreenAction->setChecked(this->isFullScreen());
    this->fullScreenAction->setShortcut(QKeySequence::FullScreen);
    this->fullScreenAction->setShortcutContext(Qt::WindowShortcut);
    q->connect(this->fullScreenAction, &QAction::toggled, &MenuBar::toggleFullScreen);
    menu->addAction(this->fullScreenAction);
    
    this->previewAction = new QAction(tr("&Preview in HTML"), menu);
    this->previewAction->setCheckable(true);
    this->previewAction->setChecked(false);
    this->previewAction->setShortcut("CTRL+P");
    this->previewAction->setShortcutContext(Qt::WindowShortcut);
    q->connect(this->previewAction, &QAction::toggled, &MenuBar::toggleHtmlPreview);
    menu->addAction(this->previewAction);
    
    this->toggleSidebarAction = new QAction(tr("Show Side&bar"), menu);
    this->toggleSidebarAction->setCheckable(true);
    this->toggleSidebarAction->setChecked(false);
    this->toggleSidebarAction->setShortcut("CTRL+SPACE");
    this->toggleSidebarAction->setShortcutContext(Qt::WindowShortcut);
    q->connect(this->toggleSidebarAction, &QAction::toggled, &MenuBar::toggleSidebar);
    menu->addAction(this->toggleSidebarAction);

    builder.add(tr("&Outline"), &MenuBar::showOutline, "CTRL+J");
    builder.add(tr("&Session Statistics"), &MenuBar::showSessionStatistics);
    builder.add(tr("&Document Statistics"), &MenuBar::showDocumentStatistics);
    builder.add(tr("&Cheat Sheet"), &MenuBar::showCheatSheet);
    
    menu->addSeparator();
    builder.add(tr("Increase Font Size"), &MenuBar::increaseFontSize, QKeySequence::ZoomIn);
    builder.add(tr("Decrease Font Size"), &MenuBar::decreaseFontSize, QKeySequence::ZoomOut);
}

void MenuBarPrivate::buildSettingsMenu()
{
    Q_Q(MenuBar);
    
    QMenu *menu = this->addMenu(tr("&Settings"));
    MenuBuilder builder(menu, q->parent());
    
    
    builder.add(tr("Themes..."), &MenuBar::changeTheme);
    builder.add(tr("Font..."), &MenuBar::changeFont);
    builder.add(tr("Application Language..."), &MenuBar::onSetLocale);
    builder.add(tr("Preview Options..."), &MenuBar::showPreviewOptions);
    QAction *preferencesAction = 
        builder.add(tr("Preferences..."), &MenuBar::openPreferencesDialog);
    preferencesAction->setMenuRole(QAction::PreferencesRole);
}

void MenuBarPrivate::buildHelpMenu()
{
    Q_Q(MenuBar);
    
    QMenu *menu = this->addMenu(tr("&Help"));
    MenuBuilder builder(menu, q->parent());
    
    QAction *helpAction = builder.add(tr("&About"), &MenuBar::showAbout);
    helpAction->setMenuRole(QAction::AboutRole);
    helpAction = builder.add(tr("About &Qt"), qApp, &QApplication::aboutQt);
    helpAction->setMenuRole(QAction::AboutQtRole);
    builder.add(tr("Quick &Reference Guide"), &MenuBar::showQuickReferenceGuide);
    builder.add(tr("Wiki"), &MenuBar::showWikiPage);
}

void MenuBarPrivate::buildRecentFilesSubmenu()
{
    Q_Q(MenuBar);
    
    QMenu *menu = new QMenu(tr("Open &Recent..."));
    
    MenuBuilder b(menu, q->parent());
    
    builder.add(tr("Reopen Closed File"),
        &MenuBar::reopenLastClosedDocument,
        "SHIFT+CTRL+T");
    
    menu->addSeparator();
    
    // Create actions for each recent file slot.  Set to invisible since no
    // recent files have been added yet.
    for (int i = 0; i < MAX_RECENT_FILES; i++) {
        this->recentFilesActions[i] = new QAction(menu);

        this->connect(
            this->recentFilesActions[i],
            &QAction::triggered,
            [this, q, i]() {

                if (nullptr != this->recentFilesActions[i]) {
                    // Use the action's data for access to the actual file path,
                    // since KDE Plasma will add a keyboard accelerator to the
                    // action's text by inserting an ampersand (&) into it.
                    //
                    emit q->openDocument(this->recentFilesActions[i]->data().toString());
                }
            }
        );
        
        this->setVisible(false);
        menu->addAction(this->recentFilesActions[i]);
    }
    
    menu->addSeparator();
    builder.add(tr("Clear Menu"), &MenuBar::clearRecentDocumentHistory);
    
    return menu;
}

} // namespace ghostwriter