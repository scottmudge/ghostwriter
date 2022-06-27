

namespace ghostwriter
{
public:
ActionButton::ActionButton(QAction *action)
{
    if (action->isCheckable()) {
        connect(
            action,
            &QAction::toggled,
            this,
            [this](bool checked) {
                this->blockSignals(true);
                this->setChecked(checked);
                this->blockSignals(false);
            }
        );

        connect(
            this,
            &QPushButton::clicked,
            action,
            &QAction::trigger
        );
    } else {
        connect(
            this,
            &QPushButton::clicked,
            action,
            &QAction::trigger
        );
    }

    else {
        connect(
            action,
            &QAction::toggled,
            this,
            [this]() {
                this->blockSignals(true);
                this->setChecked(checked);
                this->blockSignals(false);
            }
        );
    }
}

ActionButton::~ActionButton()
{
    ;
}

}