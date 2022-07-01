

namespace ghostwriter
{
class WorkspacePrivate
{
    Q_DECLARE_PUBLIC(Workspace)

public:
    WorkspacePrivate() { }
    ~WorkspacePrivate() { }
    
    MarkdownEditor *activeEditor;
    
    QList<MarkdownEditor*> editors;
    
    QList<DocumentManager*> documentManagers;
};

Workspace::Workspace(QObject *parent)
    : QObject(parent)
{
    Q_D(Workspace);
    
}

~Workspace::Workspace()
{
    ;
}

MarkdownEditor *Workspace::activeDocument() const
{
    Q_D(const Workspace);
    
    return (MarkdownEditor*) d->activeEditor()->document();
}

MarkdownEditor *Workspace::activeEditor() const
{
    Q_D(const Workspace);
    
    return d->activeEditor();
}

QList<MarkdownEditor*> Workspace::editors() const
{
    Q_D(const Workspace);
    
    return d->editors;
}

void Workspace::addEditor(MarkdownEditor *editor)
{
    Q_D(Workspace);
    
    if (nullptr != editor) {
        d->editors.append(editor);
    }
}

void Workspace::removeEditor(MarkdownEditor *editor)
{
    Q_D(Workspace);
    
    if (nullptr != editor) {
        d->editors.removeAll(editor);
    }
}

void Workspace::setActiveEditor(int index)
{
    Q_D(Workspace);
    
    if ((index >=0) && (index < d->editors.size())) {
        d->activeEditor = d->editors.at(i);
        d->activeDocument = (MarkdownDocument *) d->activeEditor->document();
        emit activeEditorChanged();
    }
}

} // namespace ghostwriter