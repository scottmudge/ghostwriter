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

#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QDebug>
#include <QString>

#include "../../src/actions.h"
#include "triggerhandler.h"
#include "togglehandler.h"

using namespace ghostwriter;

/**
 * Unit test for the Actions class.
 */
class ActionsTest: public QObject
{
    Q_OBJECT

private slots:
    void initialization();
    void addAction();
    void addActionError();
    void retrieveAction();
    void retrieveActionError();
    void registerTriggerLambdaHandler();
    void registerToggleLambdaHandler();
    void registerLambdaHandlerError();
    void registerTriggerMethodHandler();
    void registerToggleMethodHandler();
    void registerTriggerMethodHandlerError();
    void registerToggleMethodHandlerError();
    void toolButton();
    void prettyPrint();
};

/***
 * OBJECTIVE:
 *      Verify singleton initialization (nominal case).
 *
 * INPUTS:
 *      None
 *
 * EXPECTED RESULTS:
 *      - appActions global is not null
 *      - Actions::instance() returns non-null value
 *      - appActions and Actions::instance() pointer values match
 */
void ActionsTest::initialization()
{
    // Verify action manager's global variable and singleton were initialized.
    QVERIFY(nullptr != appActions);
    QVERIFY(nullptr != Actions::instance());
    QCOMPARE(appActions, Actions::instance());
}

/***
 * OBJECTIVE:
 *      Successfully add new actions in various combinations (nominal case).
 *
 * INPUTS:
 *      - Action with no shortcut, not checkable
 *      - Action with no shortcut, checkable and checked
 *      - Action with shortcut, no description, not checkable
 *
 * EXPECTED RESULTS:
 *      In each call to addAction(), the method returns a valid QAction whose
 *      values match the inputs provided.
 */
void ActionsTest::addAction()
{
    //**************************************************************************
    // Action with no shortcut, not checkable
    //**************************************************************************

    QAction *action = appActions->addAction(
        "test.id.1",
        "&Test",
        QKeySequence(),
        "Description"
    );

    // Verify action was created successfully.
    QVERIFY(nullptr != action);

    // Verify action values.
    QCOMPARE(action->objectName(), "test.id.1");
    QCOMPARE(action->text(), "&Test");
    QCOMPARE(action->whatsThis(), "Description");
    QCOMPARE(action->toolTip(), "Description");
    QCOMPARE(action->shortcut(), QKeySequence());
    QCOMPARE(action->isCheckable(), false);
    QCOMPARE(action->isChecked(), false);
    QCOMPARE(action->shortcutContext(), Qt::WindowShortcut);

    //**************************************************************************
    // Action with no shortcut, checkable and checked
    //**************************************************************************

    action = appActions->addAction(
        "test.id.2",
        "&Test",
        QKeySequence(),
        "Description",
        true,
        true
    );

    // Verify action was created successfully.
    QVERIFY(nullptr != action);

    // Verify action values.
    QCOMPARE(action->objectName(), "test.id.2");
    QCOMPARE(action->text(), "&Test");
    QCOMPARE(action->whatsThis(), "Description");
    QCOMPARE(action->toolTip(), "Description");
    QCOMPARE(action->shortcut(), QKeySequence());
    QCOMPARE(action->isCheckable(), true);
    QCOMPARE(action->isChecked(), true);
    QCOMPARE(action->shortcutContext(), Qt::WindowShortcut);

    //**************************************************************************
    // Action with shortcut, no description, not checkable
    //**************************************************************************

    action = appActions->addAction(
        "test.id.3",
        "&Test",
        QString("CTRL+{")
    );

    // Verify action was created successfully.
    QVERIFY(nullptr != action);

    // Verify action values.
    QCOMPARE(action->objectName(), "test.id.3");
    QCOMPARE(action->text(), "&Test");
    QCOMPARE(action->whatsThis(), QString());
    QCOMPARE(action->toolTip(), "Test");
    QCOMPARE(action->shortcut(), QKeySequence("CTRL+{"));
    QCOMPARE(action->isCheckable(), false);
    QCOMPARE(action->isChecked(), false);
    QCOMPARE(action->shortcutContext(), Qt::WindowShortcut);
}

/***
 * OBJECTIVE:
 *      Attempt to add a new action whose ID is already registered
 *      (robustness case).
 *
 * INPUTS:
 *      Action ID that already exists
 *
 * EXPECTED RESULTS:
 *      addAction() returns null.
 */
void ActionsTest::addActionError()
{
    QAction *action = appActions->addAction(
        "test.id.1",
        "&Test",
        QKeySequence(),
        "Description",
        true,
        true
    );

    // Verify action not created.
    QCOMPARE(action, nullptr);
}

/***
 * OBJECTIVE:
 *      Attempt to retrieve an action whose ID is already registered
 *      (robustness case).
 *
 * INPUTS:
 *      - Default application action ID
 *      - Newly added action ID from prior test case
 *
 * EXPECTED RESULTS:
 *      In each call to action(), the method returns a valid QAction with
 *      fields matching the values from the prior calls to addAction().
 */
void ActionsTest::retrieveAction()
{
    //**************************************************************************
    // Default action ID.
    //**************************************************************************

    QAction *action = appActions->action(Actions::NewFile);

    // Verify action returned.
    QVERIFY(nullptr != action);

    // Verify action values.
    QCOMPARE(action->objectName(), Actions::NewFile);
    QCOMPARE(action->text(), "&New");
    QCOMPARE(action->whatsThis(), QString());
    QCOMPARE(action->toolTip(), "New");
    QCOMPARE(action->shortcut(), QKeySequence::New);
    QCOMPARE(action->isCheckable(), false);
    QCOMPARE(action->isChecked(), false);
    QCOMPARE(action->shortcutContext(), Qt::WindowShortcut);

    //**************************************************************************
    // New action ID from previous test.
    //**************************************************************************

    action = appActions->action("test.id.1");

    // Verify action returned.
    QVERIFY(nullptr != action);

    // Verify action values.
    QCOMPARE(action->objectName(), "test.id.1");
    QCOMPARE(action->text(), "&Test");
    QCOMPARE(action->whatsThis(), "Description");
    QCOMPARE(action->toolTip(), "Description");
    QCOMPARE(action->shortcut(), QKeySequence());
    QCOMPARE(action->isCheckable(), false);
    QCOMPARE(action->isChecked(), false);
    QCOMPARE(action->shortcutContext(), Qt::WindowShortcut);
}

/***
 * OBJECTIVE:
 *      Attempt to retrieve an action whose ID does not exist (robustness case).
 *
 * INPUTS:
 *      Action ID that does not exist
 *
 * EXPECTED RESULTS:
 *      action() returns null.
 */
void ActionsTest::retrieveActionError()
{
    QAction *action = appActions->action("does.not.exist");

    // Verify action not found.
    QCOMPARE(action, nullptr);
}

/***
 * OBJECTIVE:
 *      Register a lambda function handler for a triggered() signal from an
 *      existing action (nominal case).
 *
 * INPUTS:
 *      - Action ID that exists and is not checkable
 *      - Lambda function that takes no parameters
 *      - Action is triggered
 *
 * EXPECTED RESULTS:
 *      - registerHandler() returns true (successful)
 *      - Lambda function is called
 */
void ActionsTest::registerTriggerLambdaHandler()
{
    bool lambdaFunctionCalled = false;

    // Register the action handler.
    QAction *action =
        appActions->registerHandler("test.id.1",
            [&lambdaFunctionCalled]() {
                lambdaFunctionCalled = true;
            }
        );

    // Verify handler registered successfully.
    QVERIFY(nullptr != action);
    QCOMPARE(action, appActions->action("test.id.1"));

    // Trigger the action.
    action->trigger();
    qApp->processEvents();

    // Verify lambda function was called.
    QVERIFY(lambdaFunctionCalled);
}

/***
 * OBJECTIVE:
 *      Register a lambda function handler for a toggled() signal from an
 *      existing action (nominal case).
 *
 * INPUTS:
 *      - Action ID that exists and is checkable
 *      - Lambda function that takes a boolean parameter
 *      - Action is initially checked
 *      - Action is toggled
 *
 * EXPECTED RESULTS:
 *      - registerHandler() returns true (successful)
 *      - Lambda function is called
 *      - Parameter passed to lambda function is false (unchecked)
 */
void ActionsTest::registerToggleLambdaHandler()
{
    bool lambdaFunctionCalled = false;
    bool actionChecked = true;

    // Register the action handler.
    QAction *action =
        appActions->registerHandler(
            "test.id.2",
            [&lambdaFunctionCalled, &actionChecked](bool checked) {
                lambdaFunctionCalled = true;
                actionChecked = checked;
            }
        );

    // Verify handler registered successfully.
    QVERIFY(nullptr != action);
    QCOMPARE(action, appActions->action("test.id.2"));

    // Toggle the action.
    action->toggle();
    qApp->processEvents();

    // Verify lambda function was called.
    QVERIFY(lambdaFunctionCalled);

    // Verify action was toggled from checked to unchecked,
    // and that the lambda function captured that.
    QCOMPARE(actionChecked, false);
}

/***
 * OBJECTIVE:
 *      Verify registration for a lambda function handler fails when the
 *      provided action ID does not exist (robustness case).
 *
 * INPUTS:
 *      - Action ID that does not exist
 *      - A lambda function
 *
 * EXPECTED RESULTS:
 *      - registerHandler() returns false (unsuccessful)
 */
void ActionsTest::registerLambdaHandlerError()
{
    // Register the action handler.
    QAction *action =
        appActions->registerHandler(
            "does.not.exist",
            []() { }
        );

    // Verify handler was not registered.
    QCOMPARE(action, nullptr);
}

/***
 * OBJECTIVE:
 *      Attempt to register an object method handler for the triggered() signal
 *      with an action ID that exists and whose action is NOT checkable
 *      (nominal case).
 *
 * INPUTS:
 *      - Action ID that exists and is NOT checkable
 *      - An object pointer (QObject base)
 *      - A pointer to a method within the object having no parameters
 *
 * EXPECTED RESULTS:
 *      - registerHandler() returns true (successful)
 *      - Handler method is called
 */
void ActionsTest::registerTriggerMethodHandler()
{
    TriggerHandler handler;

    // Register the action handler.
    QAction *action =
        appActions->registerHandler("test.id.3",
            &handler,
            &TriggerHandler::handleAction
        );

    // Verify handler registered successfully.
    QVERIFY(nullptr != action);
    QCOMPARE(action, appActions->action("test.id.3"));

    // Trigger the action.
    action->trigger();
    qApp->processEvents();

    // Verify handler method was called.
    QVERIFY(handler.called);
}

/***
 * OBJECTIVE:
 *      Attempt to register an object method handler for the toggled() signal
 *      with an action ID that exists and whose action is checkable
 *      (nominal case).
 *
 * INPUTS:
 *      - Action ID that exists and is checkable and initially unchecked
 *      - An object pointer (QObject base)
 *      - A pointer to a method within the object having a boolean parameter
 *
 * EXPECTED RESULTS:
 *      - registerHandler() returns QAction (successful)
 *      - Handler method is called
 *      - Parameter passed to handler method is true (checked)
 */
void ActionsTest::registerToggleMethodHandler()
{
    ToggleHandler handler;

    // Register the action handler.
    QAction *action =
        appActions->registerHandler("test.id.2",
            &handler,
            &ToggleHandler::handleAction
        );

    // Verify handler registered successfully.
    QVERIFY(nullptr != action);
    QCOMPARE(action, appActions->action("test.id.2"));

    // Toggle the action.
    action->toggle();
    qApp->processEvents();

    // Verify handler method was called.
    QVERIFY(handler.called);

    // Verify handler received checked=true.
    QVERIFY(handler.checked);
}

/***
 * OBJECTIVE:
 *      Attempt to register an object method handler for the triggered() signal
 *      with an action ID that does not exist (robustness case).
 *
 * INPUTS:
 *      - Action ID that does not exist
 *      - An object pointer (QObject base)
 *      - A pointer to a method within the object that takes no parameters
 *
 * EXPECTED RESULTS:
 *      registerHandler() returns null (unsuccessful).
 */
void ActionsTest::registerTriggerMethodHandlerError()
{
    TriggerHandler handler;

    // Register the action handler.
    QAction *action =
        appActions->registerHandler(
            "does.not.exist",
            &handler,
            &TriggerHandler::handleAction
        );

    // Verify handler was not registered.
    QCOMPARE(action, nullptr);
}

/***
 * OBJECTIVE:
 *      Attempt to register an object method handler for the toggled() signal
 *      with an action ID that does not exist (robustness case).
 *
 * INPUTS:
 *      - Action ID that does not exist
 *      - An object pointer (QObject base)
 *      - A pointer to a method within the object that takes a boolean parameter
 *
 * EXPECTED RESULTS:
 *      registerHandler() returns false (unsuccessful).
 */
void ActionsTest::registerToggleMethodHandlerError()
{
    ToggleHandler handler;

    // Register the action handler.
    QAction *action =
        appActions->registerHandler(
            "does.not.exist",
            &handler,
            &ToggleHandler::handleAction
        );

    // Verify handler was not registered.
    QCOMPARE(action, nullptr);
}

/***
 * OBJECTIVE:
 *      Assign an action to a QToolButton and verify that toggling either
 *      will keep the other's "checked" in sync.
 *
 * INPUTS:
 *      - A new action ID that is added to the Actions and set as the default
 *        action for the QToolButton
 *      - A handler registered for the action's toggle() signal
 *        (to track number of toggle() signals sent)
 *      - A button click to toggle its checked state
 *      - An action checked state toggle
 *
 * EXPECTED RESULTS:
 *      - When the button is clicked, the action reflects the button's checked
 *        state.
 *      - When the action's checked state is changed, the button reflects the
 *        action's checked state.
 */
void ActionsTest::toolButton()
{
    int toggleCount = 0;
    QToolButton *button = new QToolButton();
    QAction *action = appActions->addAction(
        "test.id.4",
        "Test Button",
        QKeySequence(),
        QString("Test description"),
        true,
        false
    );

    appActions->registerHandler("test.id.4",
        [&toggleCount](bool checked) {
            Q_UNUSED(checked)
            toggleCount++;
        }
    );

    button->setDefaultAction(action);

    // Verify button state matches initial action state.
    QCOMPARE(button->isCheckable(), action->isCheckable());
    QCOMPARE(button->isChecked(), action->isChecked());

    // Toggle the button.
    button->click();
    qApp->processEvents();

    // Verify both the action and the button were toggled to checked.
    QCOMPARE(action->isChecked(), true);
    QCOMPARE(button->isChecked(), true);

    // Verify the handler for the toggle() signal was called only once.
    QCOMPARE(toggleCount, 1);

    // Toggle the action.
    toggleCount = 0;
    action->setChecked(false);
    qApp->processEvents();

    // Verify both the action and the button were toggled to unchecked.
    QCOMPARE(action->isChecked(), false);
    QCOMPARE(button->isChecked(), false);

    // Verify the handler for the toggle() signal was called only once.
    QCOMPARE(toggleCount, 1);
}

/***
 * OBJECTIVE:
 *      Test printing of all actions (nominal case).
 *      Note: This test requires visual verificaiton.
 *
 * INPUTS:
 *      None
 *
 * EXPECTED RESULTS:
 *      Visual verification that actions were printed.
 */
void ActionsTest::prettyPrint()
{
    appActions->printActions();
}

QTEST_MAIN(ActionsTest)
#include "actionstest.moc"