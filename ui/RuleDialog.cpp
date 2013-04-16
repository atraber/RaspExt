
#include "ui/RuleDialog.h"
#include "ui/ConditionDialog.h"
#include "ui/ActionDialog.h"
#include "ui_RuleDialog.h"

#include <QMessageBox>

RuleDialog::RuleDialog(QWidget *parent, Rule* rule, Script *script) :
    QDialog(parent),
    ui(new Ui::RuleDialog),
    m_actionModel(this, rule),
    m_conditionModel(this, rule)
{
    ui->setupUi(this);
    m_rule = rule;
    m_script = script;

    ui->editName->setText( QString::fromStdString( m_rule->getName() ) );
    ui->comboType->setCurrentIndex( m_rule->getType() );
    ui->checkConcurrent->setChecked( !m_rule->getNoConcurrent() );

    ui->listActions->setModel(&m_actionModel);

    ui->listConditions->setModel(&m_conditionModel);

    // connect all signals-slots
    connect(ui->buttonAddCondition, SIGNAL(clicked()), this, SLOT(addCondition()));
    connect(ui->buttonEditCondition, SIGNAL(clicked()), this, SLOT(editCondition()));
    connect(ui->buttonDelCondition, SIGNAL(clicked()), this, SLOT(deleteCondition()));

    connect(ui->buttonAddAction, SIGNAL(clicked()), this, SLOT(addAction()));
    connect(ui->buttonEditAction, SIGNAL(clicked()), this, SLOT(editAction()));
    connect(ui->buttonDelAction, SIGNAL(clicked()), this, SLOT(deleteAction()));
    connect(ui->buttonUp, SIGNAL(clicked()), this, SLOT(actionUp()));
    connect(ui->buttonDown, SIGNAL(clicked()), this, SLOT(actionDown()));

    connect(ui->buttonClose, SIGNAL(clicked()), this, SLOT(closePressed()));
}

RuleDialog::~RuleDialog()
{
    delete this->ui;
}

void RuleDialog::addAction()
{
    ActionDialog* dialog = new ActionDialog(this, m_script);

    if( dialog->exec() == Accepted )
    {
        Action* action = dialog->getAction();

        // only try to add if action is not NULL
        if(action != NULL)
        {
            m_actionModel.add(action);
        }
    }

    delete dialog;
}


void RuleDialog::editAction()
{
    QModelIndexList indices = ui->listActions->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        ActionDialog* dialog = new ActionDialog(this, m_script);
        dialog->edit( m_actionModel.get( indices.front().row() ) );

        if( dialog->exec() == Accepted )
        {
            Action* action = dialog->getAction();

            // only try to add if action is not NULL
            if(action != NULL)
            {
                m_actionModel.removeRow( indices.front().row() );
                m_actionModel.add(action);
            }
        }
    }
}

void RuleDialog::deleteAction()
{
    QModelIndexList indices = ui->listActions->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        m_actionModel.removeRow(indices.front().row());
    }
}

void RuleDialog::actionUp()
{
    QModelIndexList indices = ui->listActions->selectionModel()->selection().indexes();

    if(indices.size() != 0 && indices.front().row() != 0)
    {
        m_actionModel.swap( indices.front().row(), indices.front().row() - 1 );
    }
}

void RuleDialog::actionDown()
{
    QModelIndexList indices = ui->listActions->selectionModel()->selection().indexes();

    if(indices.size() != 0 && indices.front().row() != (m_actionModel.rowCount() - 1))
    {
        m_actionModel.swap( indices.front().row() + 1, indices.front().row() );
    }
}

void RuleDialog::addCondition()
{
    ConditionDialog* dialog = new ConditionDialog(this, m_script);

    if( dialog->exec() == Accepted )
    {
        Condition* condition = dialog->getCondition();

        // only try to add if condition is not NULL
        if(condition != NULL)
        {
            m_conditionModel.add(condition);
        }
    }

    delete dialog;
}

void RuleDialog::editCondition()
{
    QModelIndexList indices = ui->listConditions->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        ConditionDialog* dialog = new ConditionDialog(this, m_script);
        dialog->edit( m_conditionModel.get( indices.front().row() ) );

        if( dialog->exec() == Accepted )
        {
            Condition* condition = dialog->getCondition();

            // only try to add if condition is not NULL
            if(condition != NULL)
            {
                m_conditionModel.removeRow( indices.front().row() );
                m_conditionModel.add(condition);
            }
        }

        delete dialog;
    }
}

void RuleDialog::deleteCondition()
{
    QModelIndexList indices = ui->listConditions->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        m_conditionModel.removeRow(indices.front().row());
    }
}


void RuleDialog::closePressed()
{
    if( ui->editName->text().size() == 0 )
    {
        // name is empty, we cannot save a script with an empty name
        QMessageBox(QMessageBox::Warning,
                    "Empty Name",
                    "The name of a rule cannot be empty! Please fill in a name",
                    QMessageBox::Ok,
                    this).exec();
        return;
    }

    // check if a rule with this name already exists and is not identical to this one (if the name did not change)
    Rule* rule = m_script->getRuleByName( ui->editName->text().toStdString() );
    if( rule != NULL && rule != m_rule)
    {
        QMessageBox(QMessageBox::Warning,
                    "Warning",
                    "A rule with this name already exists!\nPlease choose a different name",
                    QMessageBox::Ok,
                    this).exec();
        return;
    }

    m_rule->setName( ui->editName->text().toStdString() );
    m_rule->setType( (Rule::Type)ui->comboType->currentIndex() );
    m_rule->setNoConcurrent( !ui->checkConcurrent->isChecked() );

    this->done(Accepted);
}
