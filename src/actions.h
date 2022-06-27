/***********************************************************************
 *
 * Copyright (C) 2022 wereturtle
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

#include <QAction>
#include <QDebug>
#include <QMap>
#include <QObject>
#include <QString>

namespace ghostwriter
{
/***
 * Convenience alias.  Note:  Cannot use a proper global variable since
 * Qt keyboard shortcuts are not yet initialized until the Qt app is
 * initialized.  Attempts to use QKeySequence until then will cause a
 * segmentation fault.
 */
class Actions;
#define appActions Actions::instance()

/***
 * Manages the application's actions with their shortcuts.  Default actions
 * for the application are registered with this class on initialization.
 * See defaultactions.h for the complete list of default application action IDs.
 * Note that each QAction's object name is set to its action ID.
 */
class Actions : public QObject
{
    Q_OBJECT

public:
    /***
     * Returns the single instance of this class.
     */
    static Actions *instance();

    /***
     * Destructor.
     */
    ~Actions();

    /***
     * Adds a new QAction with the given ID, display text, shortcut, etc.
     * The description (if supplied) will be applied to the tooltip and the
     * "what's this" tip.  If the action can be toggled, set checkable to true
     * and set checked to the initial value the action should have.
     * Note that all actions have their context set to Qt::WindowShortcut.
     */
    QAction *addAction(const QString &id,
        const QString &text,
        const QKeySequence &shortcut = QKeySequence(),
        const QString &description = QString(),
        bool checkable = false,
        bool checked = false
    );

    /***
     * Returns the QAction for the given action ID.
     */
    QAction *action(const QString &id) const;

    /***
     * Connects the given function, functor, or lambda function to the
     * triggered() or toggled() (if checkable) signal of the action with the
     * the given ID. Returns the action if successful, or else null.
     */
    template<class Handler>
    QAction *registerHandler(const QString &id, Handler handler);

    /***
     * Connects the given QObject and method to the triggered() signal of the
     * action with the the given ID. Returns the action if successful, or else
     * null.
     */
    template<class Receiver>
    QAction *registerHandler(const QString &id,
        Receiver *receiver,
        void(Receiver::*method)());

    /***
     * Connects the given QObject and method to the toggled() signal of the
     * action with the the given ID.  Note that the method must take a boolean
     * as a parameter. Returns the action if successful, or else null.
     */
    template<class Receiver>
    QAction *registerHandler(const QString &id,
        Receiver *receiver,
        void(Receiver::*method)(bool checked));

    void addActionShortcutsToWidget(QWidget *widget);

    void invoke(const QString &id);

    /***
     * Convenience method that prints a descriptive list of actions for
     * debugging/documentation purposes.
     */
    void printActions() const;

    /***
     * Default action IDs.
     */
    static const QString NewFile;
    static const QString OpenFile;
    static const QString OpenRecent0;
    static const QString OpenRecent1;
    static const QString OpenRecent2;
    static const QString OpenRecent3;
    static const QString OpenRecent4;
    static const QString OpenRecent5;
    static const QString OpenRecent6;
    static const QString OpenRecent7;
    static const QString OpenRecent8;
    static const QString OpenRecent9;
    static const QString ReopenLast;
    static const QString ClearHistory;
    static const QString Save;
    static const QString SaveAs;
    static const QString RenameFile;
    static const QString ReloadFile;
    static const QString ExportFile;
    static const QString Quit;
    static const QString Undo;
    static const QString Redo;
    static const QString Cut;
    static const QString Copy;
    static const QString Paste;
    static const QString CopyHtml;
    static const QString InsertImage;
    static const QString Find;
    static const QString Replace;
    static const QString FindNext;
    static const QString FindPrevious;
    static const QString SelectAll;
    static const QString ToggleHemingwayMode;
    static const QString SpellCheck;
    static const QString Bold;
    static const QString Italic;
    static const QString Strikethrough;
    static const QString HtmlComment;
    static const QString Indent;
    static const QString Unindent;
    static const QString BlockQuote;
    static const QString StripBlockQuote;
    static const QString BulletListAsterisk;
    static const QString BulletListMinus;
    static const QString BulletListPlus;
    static const QString NumberedListPeriod;
    static const QString NumberedListParenthesis;
    static const QString TaskList;
    static const QString ToggleTaskComplete;
    static const QString ToggleFullScreen;
    static const QString ToggleHtmlPreview;
    static const QString ToggleDistractionFreeMode;
    static const QString ToggleSidebar;
    static const QString ToggleDarkMode;
    static const QString ShowOutline;
    static const QString ShowSessionStatistics;
    static const QString ShowDocumentStatistics;
    static const QString ShowCheatSheet;
    static const QString ZoomIn;
    static const QString ZoomOut;
    static const QString ShowThemes;
    static const QString ShowFonts;
    static const QString ShowAppLanguages;
    static const QString ShowPreviewOptions;
    static const QString ShowPreferences;
    static const QString About;
    static const QString AboutQt;
    static const QString Documentation;
    static const QString Wiki;

private:
    /**
     * Constructor.
     */
    Actions();

    // Instance pointer.
    static Actions *m_instance;

    // Map of action IDs to actions.
    QMap<QString, QAction*> m_actions;
};

/// TEMPLATE METHOD DEFINITIONS

template<class Handler>
QAction *Actions::registerHandler(const QString &id, Handler handler)
{
    if (!m_actions.contains(id)) {
        qWarning() << "Unknown action ID:" << id;
        return nullptr;
    }

    QAction *a = m_actions.value(id);

    if (a->isCheckable()) {
        a->connect(a, &QAction::toggled, handler);
    } else {
        a->connect(a, &QAction::triggered, handler);
    }

    return a;
}

template<class Receiver>
QAction *Actions::registerHandler(const QString &id,
    Receiver* receiver,
    void(Receiver::*method)())
{
    if (!m_actions.contains(id)) {
        qWarning() << "Unknown action ID:" << id;
        return nullptr;
    }

    QAction *a = m_actions.value(id);
    a->connect(a, &QAction::triggered, receiver, method);
    return a;
}

template<class Receiver>
QAction *Actions::registerHandler(const QString &id,
    Receiver* receiver,
    void(Receiver::*method)(bool checked))
{
    if (!m_actions.contains(id)) {
        qWarning() << "Unknown action ID:" << id;
        return nullptr;
    }

    QAction *a = m_actions.value(id);
    a->connect(a, &QAction::toggled, receiver, method);
    return a;
}

} // namespace ghostwriter