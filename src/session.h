/***********************************************************************
 *
 * Copyright (C) 2014-2022 wereturtle
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

#ifndef SESSION_H
#define SESSION_H

#include <QObject>
#include <QScopedPointer>

#include "markdowndocument.h"
#include "markdowneditor.h"

namespace ghostwriter
{
/**
 * Manages the life-cycle of a document, facilitating user interaction for
 * opening, closing, saving, etc.
 */
class SessionPrivate;
class Session : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Session)

public:
    /**
     * Constructor.
     */
    Session(QWidget *parent = 0);

    /**
     * Destructor.
     */
    virtual ~Session();
    
    OutlineWidget *
    SessionStatistics *currentSessionStatistics();
    DocumentStatistics *currentDocumentStatistics();

protected slots:
    /**
     * Prompts the user for a file path, and loads the document with the
     * file contents at the selected path.
     */
    void open(const QString &filePath = QString(), bool draft = false);

    /**
     * Reopens the last closed file, if any is available in the session
     * history.
     */
    void reopenLastClosedFile();

    /**
     * Reloads current document from disk contents.  This method does nothing if
     * the document is new and is not associated with a file on disk.
     * Note that if the document is modified, this method will discard
     * changes before reloading.  It is left to the caller to check for
     * modification and save any changes before calling this method.
     */
    void reload();

    /**
     * Renames current file represented by this document to the given file path.
     * This method does nothing if the document is new and is not
     * associated with a file on disk.
     */
    void rename();

    /**
     * Saves document contents to disk.  This method will call saveAs() if the
     * document is new (no file on disk) or a draft (untitled).
     */
    bool save();

    /**
     * Prompts the user for a file path location, and saves the document
     * contents to the selected file path. This method is
     * also called if the document is new and is now going to be saved to
     * a file path for the first time.  Future save operations to the same
     * file path can be achieved by calling save() instead.
     */
    bool saveAs();

    /**
     * Closes the current file, clearing the editor of text and leaving
     * only new (empty) document in its place if no other files are opened.
     */
    bool close();

    /**
     * Exports the current file, prompting the user for the desired
     * export format.
     */
    void exportFile();

private:
    QScopedPointer<SessionPrivate> d_ptr;
};
}

#endif // SESSION_H

