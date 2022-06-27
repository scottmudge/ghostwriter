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

#include "defaultactions.h"
#include "actions.h"

namespace ghostwriter
{

static inline QString boolToYesNo(bool value)
{
    if (value) {
        return "yes";
    }

    return "no";
}

static inline void printAction(const QString &id, const QAction *const action)
{
    qInfo() << "id:" << id;

    QList<QKeySequence> shortcuts = action->shortcuts();

    if (!shortcuts.isEmpty()) {
        QString shortcutsString = "";
        for (QKeySequence shortcut : shortcuts) {
            shortcutsString += shortcut.toString();
        }

        qInfo() << "shortcuts:" << shortcutsString;
    }

    qInfo() << "checkable:" << boolToYesNo(action->isCheckable());

    if (action->isCheckable()) {
        qInfo() << "checked:" << boolToYesNo(action->isChecked());
    }

    if (!action->whatsThis().isNull()
            && !action->whatsThis().isEmpty()) {
        qInfo() << "description:" << action->whatsThis();
    }
}

const QString Actions::NewFile = "ghostwriter.file.new";
const QString Actions::OpenFile = "ghostwriter.file.open";
const QString Actions::OpenRecent0 = "ghostwriter.file.openrecent.0";
const QString Actions::OpenRecent1 = "ghostwriter.file.openrecent.1";
const QString Actions::OpenRecent2 = "ghostwriter.file.openrecent.2";
const QString Actions::OpenRecent3 = "ghostwriter.file.openrecent.3";
const QString Actions::OpenRecent4 = "ghostwriter.file.openrecent.4";
const QString Actions::OpenRecent5 = "ghostwriter.file.openrecent.5";
const QString Actions::OpenRecent6 = "ghostwriter.file.openrecent.6";
const QString Actions::OpenRecent7 = "ghostwriter.file.openrecent.7";
const QString Actions::OpenRecent8 = "ghostwriter.file.openrecent.8";
const QString Actions::OpenRecent9 = "ghostwriter.file.openrecent.9";
const QString Actions::ReopenLast = "ghostwriter.file.reopenlast";
const QString Actions::ClearHistory = "ghostwriter.file.clearhistory";
const QString Actions::Save = "ghostwriter.file.save";
const QString Actions::SaveAs = "ghostwriter.file.saveas";
const QString Actions::RenameFile = "ghostwriter.file.rename";
const QString Actions::ReloadFile = "ghostwriter.file.reloadfile";
const QString Actions::ExportFile = "ghostwriter.file.export";
const QString Actions::Quit = "ghostwriter.file.quit";
const QString Actions::Undo = "ghostwriter.edit.undo";
const QString Actions::Redo = "ghostwriter.edit.redo";
const QString Actions::Cut = "ghostwriter.edit.cut";
const QString Actions::Copy = "ghostwriter.edit.copy";
const QString Actions::Paste = "ghostwriter.edit.paste";
const QString Actions::CopyHtml = "ghostwriter.edit.copyhtml";
const QString Actions::InsertImage = "ghostwriter.edit.insertimage";
const QString Actions::Find = "ghostwriter.edit.find";
const QString Actions::Replace = "ghostwriter.edit.replace";
const QString Actions::FindNext = "ghostwriter.edit.findnext";
const QString Actions::FindPrevious = "ghostwriter.edit.findprevious";
const QString Actions::SelectAll = "ghostwriter.edit.selectall";
const QString Actions::ToggleHemingwayMode = "ghostwriter.edit.togglehemingwaymode";
const QString Actions::SpellCheck = "ghostwriter.edit.spellcheck";
const QString Actions::Bold = "ghostwriter.format.bold";
const QString Actions::Italic = "ghostwriter.format.italic";
const QString Actions::Strikethrough = "ghostwriter.format.strikethrough";
const QString Actions::HtmlComment = "ghostwriter.format.htmlcomment";
const QString Actions::Indent = "ghostwriter.format.indent";
const QString Actions::Unindent = "ghostwriter.format.unindent";
const QString Actions::BlockQuote = "ghostwriter.format.blockquote";
const QString Actions::StripBlockQuote = "ghostwriter.format.stripblockquote";
const QString Actions::BulletListAsterisk = "ghostwriter.format.bulletlistasterisk";
const QString Actions::BulletListMinus = "ghostwriter.format.bulletlistminus";
const QString Actions::BulletListPlus = "ghostwriter.format.bulletlistplus";
const QString Actions::NumberedListPeriod = "ghostwriter.format.numberedlistperiod";
const QString Actions::NumberedListParenthesis = "ghostwriter.format.numberedlistparenthesis";
const QString Actions::TaskList = "ghostwriter.format.tasklist";
const QString Actions::ToggleTaskComplete = "ghostwriter.format.toggletaskcomplete";
const QString Actions::ToggleFullScreen = "ghostwriter.view.togglefullscreen";
const QString Actions::ToggleHtmlPreview = "ghostwriter.view.togglehtmlpreview";
const QString Actions::ToggleDistractionFreeMode = "ghostwriter.view.toggledistractionfreemode";
const QString Actions::ToggleSidebar = "ghostwriter.view.togglesidebar";
const QString Actions::ToggleDarkMode = "ghostwriter.view.toggledarkmode";
const QString Actions::ShowOutline = "ghostwriter.view.showoutline";
const QString Actions::ShowSessionStatistics = "ghostwriter.view.showsessionstatistics";
const QString Actions::ShowDocumentStatistics = "ghostwriter.view.showdocumentstatistics";
const QString Actions::ShowCheatSheet = "ghostwriter.view.showcheatsheet";
const QString Actions::ZoomIn = "ghostwriter.view.zoomin";
const QString Actions::ZoomOut = "ghostwriter.view.zoomout";
const QString Actions::ShowThemes = "ghostwriter.settings.showthemes";
const QString Actions::ShowFonts = "ghostwriter.settings.showfonts";
const QString Actions::ShowAppLanguages = "ghostwriter.settings.showapplanguages";
const QString Actions::ShowPreviewOptions = "ghostwriter.settings.showpreviewoptions";
const QString Actions::ShowPreferences = "ghostwriter.settings.showpreferences";
const QString Actions::About = "ghostwriter.help.about";
const QString Actions::AboutQt = "ghostwriter.help.aboutqt";
const QString Actions::Documentation = "ghostwriter.help.documentation";
const QString Actions::Wiki = "ghostwriter.help.wiki";

Actions *Actions::m_instance;

Actions * Actions::instance()
{
    if (nullptr == m_instance) {
        m_instance = new Actions();
    }

    return m_instance;
}

Actions::~Actions()
{
    ;
}

QAction *Actions::addAction(const QString &id,
    const QString &text,
    const QKeySequence &shortcut,
    const QString &description,
    bool checkable,
    bool checked
)
{
    if (m_actions.contains(id)) {
        qWarning() << "Cannot add duplicate action ID:" << id;
        return nullptr;
    }

    QAction *action = new QAction(text);

    action->setObjectName(id);

    if (!description.isNull()) {
        action->setWhatsThis(description);
        action->setToolTip(description);
    }

    action->setCheckable(checkable);
    action->setChecked(checked);
    action->setShortcut(shortcut);
    action->setShortcutContext(Qt::WindowShortcut);
    m_actions.insert(id, action);

    // If debugging, add debugging prints for when actions are
    // triggered/toggled.
    connect(action,
        &QAction::triggered,
        [action]() {
            qDebug() << action->objectName() << "triggered";
        }
    );

    connect(action,
        &QAction::toggled,
        [action](bool checked) {
            qDebug() << action->objectName() << "toggled, checked =" << checked;
        }
    );

    return action;
}

QAction * Actions::action(const QString &id) const
{
    if (!m_actions.contains(id)) {
        qWarning() << "Action does not exist for ID:" << id;
        return nullptr;
    }

    return m_actions.value(id);
}

void Actions::printActions() const
{
    QMapIterator<QString, QAction*> i(m_actions);

    while (i.hasNext()) {
        i.next();
        printAction(i.key(), i.value());

        if (i.hasNext()) {
            qInfo() << "-----";
        }
    }
}

Actions::Actions() : QObject()
{
    addAction(NewFile, tr("&New"), QKeySequence::New);
    addAction(OpenFile, tr("&Open"), QKeySequence::Open);
    addAction(OpenRecent0, "Open recent file #1");
    addAction(OpenRecent1, "Open recent file #2");
    addAction(OpenRecent2, "Open recent file #3");
    addAction(OpenRecent3, "Open recent file #4");
    addAction(OpenRecent4, "Open recent file #5");
    addAction(OpenRecent5, "Open recent file #6");
    addAction(OpenRecent6, "Open recent file #7");
    addAction(OpenRecent7, "Open recent file #8");
    addAction(OpenRecent8, "Open recent file #9");
    addAction(OpenRecent9, "Open recent file #10");
    addAction(ReopenLast, tr("Reopen Closed File"), tr("SHIFT+CTRL+T"));
    addAction(ClearHistory, tr("Clear Menu"));
    addAction(Save, tr("&Save"), QKeySequence::Save);
    addAction(SaveAs, tr("Save &As..."), QKeySequence::SaveAs);
    addAction(RenameFile, tr("R&ename..."));
    addAction(ReloadFile, tr("Re&load from Disk..."));
    addAction(ExportFile, tr("&Export"), tr("CTRL+E"));
    addAction(Quit, tr("&Quit"),
        QKeySequence::Quit)->setMenuRole(QAction::QuitRole);
    addAction(Undo, tr("&Undo"), QKeySequence::Undo);
    addAction(Redo, tr("&Redo"), QKeySequence::Redo);
    addAction(Cut, tr("Cu&t"), QKeySequence::Cut);
    addAction(Copy, tr("&Copy"), QKeySequence::Copy);
    addAction(Paste, tr("&Paste"), QKeySequence::Paste);
    addAction(CopyHtml, tr("Copy &HTML"), tr("SHIFT+CTRL+C"));
    addAction(InsertImage, tr("&Insert Image..."));
    addAction(Find, tr("&Find"), QKeySequence::Find);
    addAction(Replace, tr("Rep&lace"), QKeySequence::Replace);
    addAction(FindNext, tr("Find &Next"), QKeySequence::FindNext);
    addAction(FindPrevious, tr("Find &Previous"), QKeySequence::FindPrevious);
    addAction(SelectAll, tr("Select &All"), QKeySequence::SelectAll);
    addAction(ToggleHemingwayMode, tr("Hemingway Mode"), QKeySequence(),
        tr("Toggle Hemingway mode to enable/disable the backspace and delete keys."),
        true, false);
    addAction(SpellCheck, tr("&Spell check"));
    addAction(Bold, tr("&Bold"), QKeySequence::Bold);
    addAction(Italic, tr("&Italic"), QKeySequence::Italic);
    addAction(Strikethrough, tr("Stri&kethrough"), tr("Ctrl+K"));
    addAction(HtmlComment, tr("&HTML Comment"), tr("Ctrl+/"));
    addAction(Indent, tr("I&ndent"), tr("Tab"));
    addAction(Unindent, tr("&Unindent"), tr("Shift+Tab"));
    addAction(BlockQuote, tr("Block &Quote"), tr("Ctrl+."));
    addAction(StripBlockQuote, tr("&Strip Block Quote"), tr("Ctrl+,"));
    addAction(BulletListAsterisk, tr("&* Bullet List"), tr("Ctrl+8"));
    addAction(BulletListMinus, tr("&- Bullet List"), tr("Ctrl+Shift+-"));
    addAction(BulletListPlus, tr("&+ Bullet List"), tr("Ctrl+Shift+="));
    addAction(NumberedListPeriod, tr("1&. Numbered List"), tr("Ctrl+1"));
    addAction(NumberedListParenthesis, tr("1&) Numbered List"), tr("Ctrl+0"));
    addAction(TaskList, tr("&Task List"), tr("Ctrl+T"));
    addAction(ToggleTaskComplete, tr("Toggle Task(s) &Complete"), tr("Ctrl+D"));
    addAction(ToggleFullScreen, tr("&Full Screen"), QKeySequence::FullScreen,
        tr("Toggle full screen mode"), true, false);
    addAction(ToggleHtmlPreview, tr("&Preview in HTML"), tr("CTRL+P"),
        tr("Toggle Live HTML Preview"), true, false);
    addAction(ToggleDistractionFreeMode, tr("Distraction-Free Mode"),
        QKeySequence(), tr("Toggle distraction free mode"), true, false);
    addAction(ToggleSidebar, tr("Show Side&bar"), tr("CTRL+SPACE"),
        tr("Toggle sidebar"), true, false);
    addAction(ToggleDarkMode, tr("Dark Mode"), QKeySequence(),
        tr("Toggle dark mode"), true, false);
    addAction(ShowOutline, tr("&Outline"), tr("CTRL+J"));
    addAction(ShowSessionStatistics, tr("&Session Statistics"));
    addAction(ShowDocumentStatistics, tr("&Document Statistics"));
    addAction(ShowCheatSheet, tr("&Cheat Sheet"));
    addAction(ZoomIn, tr("Increase Font Size"), QKeySequence::ZoomIn);
    addAction(ZoomOut, tr("Decrease Font Size"), QKeySequence::ZoomOut);
    addAction(ShowThemes, tr("Themes..."));
    addAction(ShowFonts, tr("Font..."));
    addAction(ShowAppLanguages, tr("Application Language..."));
    addAction(ShowPreviewOptions, tr("Preview Options..."));
    addAction(ShowPreferences,
        tr("Preferences..."))->setMenuRole(QAction::PreferencesRole);
    addAction(About,
        tr("&About"))->setMenuRole(QAction::AboutRole);
    addAction(AboutQt, tr("About &Qt"))->setMenuRole(QAction::AboutQtRole);
    addAction(Documentation, tr("Quick &Reference Guide"),
        QKeySequence::HelpContents);
    addAction(Wiki, tr("Wiki"));
}
} // namespace ghostwriter