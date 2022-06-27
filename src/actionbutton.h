#include <QAction>
#include <QPushButton>

class ActionButton : public QPushButton
{
public:
    ActionButton(QAction *action);
    ~ActionButton();

private:
    QAction *m_action;
};