#include <QChar>
#include <QMenuBar>
#include <QScopedPointer>
#include <QString>
#include <QWidget>

namespace ghostwriter
{
class MenuBarPrivate;
class MenuBar : public QMenuBar
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(MenuBar)

public:
    MenuBar(QWidget *parent = nullptr);
    ~MenuBar();
    
signals:
    void createNewDocument();
    void openDocument();
    void openRecentDocument();
    void reopenLastClosedDocument();
    void clearRecentDocumentHistory();
    void saveActiveDocument();
    void saveActiveDocumentAs();
    void renameActiveDocument();
    void reloadActiveDocument();
    void exportActiveDocument();
    void quitApplication();
    void closeWorkspace();
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void copyHtml();
    void insertImage();
    void showFindView();
    void showReplaceView();
    void findNext();
    void findPrevious();
    void runSpellCheck();
    void selectAll();
    void bold();
    void italic();
    void strikethrough();
    void htmlComment();
    void indent();
    void unindent();
    void blockQuote();
    void stripBlockQuote();
    void bulletListWithAsteriskMarker();
    void bulletListWithMinusMarker();
    void bulletListWithPlusMarker();
    void numberedListWithPeriodMarker();
    void numberedListWithParenthesisMarker();
    void taskList();
    void toggleTasksComplete();
    void toggleFullScreen(bool enabled);
    void toggleHtmlPreview(bool enabled);
    void toggleSidebar(bool enabled);
    void showOutline(bool enabled);
    void showSessionStatistics(bool enabled);
    void showDocumentStatistics(bool enabled);
    void showCheatSheet(bool enabled);
    void increaseFontSize();
    void decreaseFontSize();
    void showThemes();
    void showFontOptions();
    void showApplicationLanguageOptions();
    void showPreviewOptions();
    void showPreferences();
    void showAbout();
    void showAboutQt();
    void showQuickReferenceGuide();
    void showWikiPage();

public slots:
    void addRecentDocument(const QString &path);
    void clearRecentDocuments();
    void fullscreenToggled(bool enabled);
    void htmlPreviewToggled(bool enabled);
    void sidebarToggled(bool enabled);
    void setAutoHideEnabled(bool enabled);

private:
    QScopedPointer<MenuBarPrivate> d_ptr;
};
} // namespace ghostwriter
