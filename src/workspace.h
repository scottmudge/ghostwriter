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

#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QObject>
#include <QScopedPointer>

#include "markdowndocument.h"
#include "markdowneditor.h"

namespace ghostwriter
{
/**
 * Contains the set of editors/documents being viewed/edited.
 */
class WorkspacePrivate;
class Workspace : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Workspace)

public:
    /**
     * Constructor.
     */
    Workspace(QWidget *parent = 0);

    /**
     * Destructor.
     */
    virtual ~Workspace();
    
    MarkdownDocument * activeDocument() const;
    MarkdownEditor * activeEditor() const;
    QList<MarkdownEditor*> editors() const;
    
    void addEditor(MarkdownEditor *editor);
    void removeEditor(MarkdownEditor *editor);
    
signals:
    void activeEditorChanged(int index);

public slots:
    void setActiveEditor(int index);

private:
    QScopedPointer<WorkspacePrivate> d_ptr;
};
}

#endif // WORKSPACE_H

