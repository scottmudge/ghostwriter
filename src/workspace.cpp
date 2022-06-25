

namespace ghostwriter
{
class WorkspacePrivate
{
    Q_DECLARE_PUBLIC(DocumentManager)

public:
    static const QString FILE_CHOOSER_FILTER;

    WorkspacePrivate
    (
        DocumentManager *q_ptr
    ) : q_ptr(q_ptr)
    {
        ;
    }

    ~WorkspacePrivate()
    {

    }
    
    Theme& theme;

    /*
    * This flag is used to prevent notifying the user that the document
    * was modified when the user is the one who modified it by saving.
    * See code for onFileChangedExternally() for details.
    */
    bool saveInProgress;

    /*
    * Boolean flag used to track if the prompt for the file having been
    * externally modified is already displayed and should not be displayed
    * again.
    */
    bool documentModifiedNotifVisible;

    /*
    * Begins asynchronous save operation.  Called by save() and saveAs().
    */
    void saveFile();

    /*
    * Handles the event where a file as been modified externally on disk.
    */
    void onFileChangedExternally(const QString &path);

    /*
    * Loads the document with the file contents at the given path.
    */
    bool loadFile(const QString &filePath);

    /*
    * Sets the file path for the document, such that the file will be
    * monitored for external changes made to it, and the display name
    * for the document updated.
    */
    void setFilePath(const QString &filePath);

    /*
    * Checks if changes need to be saved before an operation
    * can continue.  The user will be prompted to save if
    * necessary, and this method will return true if the
    * operation can proceed.
    */
    bool checkSaveChanges();

    /*
    * Checks if file on the disk is read only.  This method will return
    * true if save operation can proceed.
    */
    bool checkPermissionsBeforeSave();

    /*
    * Creates a draft (named "untitled-##.md") document in the configured
    * draft location.
    */
    void createDraft();
    
    /**
     * Prompts the user for a file path, and loads the document with the
     * file contents at the selected path.
     */
    void open();

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
     * Saves the active editor's document contents to disk.  This method will
     * call saveActiveDocumentAs() if the document is new (no file on disk) or
     * a draft (untitled).
     */
    bool saveActiveDocument();

    /**
     * Prompts the user for a file path location, and saves the active editor's
     * document contents to the selected file path. This method is also called
     * if the document is new and is now going to be saved to a file path for
     * the first time.  Future save operations to the same file path can be
     * achieved by calling saveActiveDocument() instead.
     */
    bool saveActiveDocumentAs();

    /**
     * Prompts the user for a file path location, and saves the active editor's
     * document contents to the selected file path. This method is also called
     * if the document is new and is now going to be saved to a file path for
     * the first time.  Future save operations to the same file path can be
     * achieved by calling saveActiveDocument() instead.
     */
    bool saveAll();

    /**
     * Closes the active editor's document, clearing the editor of text and
     * leaving only a new (empty) document in its place if no other documents
     * are opened.
     */
    bool closeActiveEditor();

    /**
     * Closes the active editor's document, clearing the editor of text and
     * leaving only a new (empty) document in its place if no other documents
     * are opened.
     */
    bool closeAll();

    /**
     * Exports the active editor's document, prompting the user for the desired
     * export format.
     */
    void exportActiveDocument();
};