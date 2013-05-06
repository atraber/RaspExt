
#include "ui/ConditionDialog.h"
#include "ui_ConditionDialog.h"

#include "script/ConditionInputButton.h"
#include "script/ConditionInputFader.h"
#include "script/ConditionVariable.h"

#include "script/Script.h"

#include "hw/HWInput.h"
#include "hw/HWOutput.h"

#include "util/Debug.h"

ConditionDialog::ConditionDialog(QWidget *parent, Script *script) :
    QDialog(parent),
    ui(new Ui::ConditionDialog)
{
    pi_assert(script != NULL);

    ui->setupUi(this);

    m_script = script;
    m_condition = NULL;
    m_baseWidget = NULL;

    // connect all signals - slots
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(okPressed()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(ui->comboConditionType, SIGNAL(currentIndexChanged(int)), this, SLOT(conditionChanged(int)));

    // default values
    this->conditionChanged( ui->comboConditionType->currentIndex() );
}

void ConditionDialog::conditionChanged(int index)
{
    if(m_baseWidget != NULL)
    {
        ui->frameGrid->removeWidget(m_baseWidget);
        delete m_baseWidget;
        m_baseWidget = NULL;
    }

    Condition::Type type = (Condition::Type)index;

    switch(type)
    {
    case Condition::Input:
        m_baseWidget = new ConditionInputWidget(this, m_script);
        ui->frameGrid->addWidget(m_baseWidget, 1, 0, 1, 2);
    break;

    case Condition::Var:
        m_baseWidget = new ConditionVariableWidget(this, m_script);
        ui->frameGrid->addWidget(m_baseWidget, 1, 0, 1, 2);
        break;
    }
}

void ConditionDialog::okPressed()
{
    pi_assert(m_condition == NULL);
    pi_assert(m_baseWidget != NULL);

    m_condition = ((IConditionWidget*)m_baseWidget)->assemble();

    if(m_condition == NULL)
        LOG_WARN(Logger::UI, "Assembling Condition failed");

    this->done(Accepted);
}

void ConditionDialog::edit(Condition *condition)
{
    ui->comboConditionType->setCurrentIndex(condition->getType());
    if(m_baseWidget != NULL)
    {
        m_baseWidget->edit(condition);
    }
}

ConditionDialog::~ConditionDialog()
{
    delete ui;
}

ConditionInputWidget::ConditionInputWidget(QWidget* parent, Script *script) : IConditionWidget(parent)
{
    m_baseWidget = NULL;


    m_comboInput = new QComboBox(this);
    m_label = new QLabel("Select input", this);
    m_labelType = new QLabel("");

    // fill combo box with all inputs
    std::list<HWInput*> listInputs = script->getInputList();
    for(std::list<HWInput*>::iterator it = listInputs.begin(); it != listInputs.end(); it++)
    {
        m_comboInput->addItem((*it)->getName().c_str(), qVariantFromValue((void*)*it));
    }

    m_layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    m_layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    m_layout->addWidget(m_labelType, 0, 0);
    m_layout->addWidget(m_comboInput, 0, 1);
    m_layout->addWidget(m_labelType, 1, 0, 1, 2);

    this->setLayout(m_layout);

    // connect all signals - slots
    connect(m_comboInput, SIGNAL(currentIndexChanged(int)), this, SLOT(inputChanged(int)));

    // default values
    this->inputChanged( m_comboInput->currentIndex() );
}

void ConditionInputWidget::inputChanged(int index)
{
    QVariant variant = m_comboInput->itemData( index );
    HWInput* hw = (HWInput*)variant.value<void*>();

    HWInput::HWInputType type = hw->getType();

    QString inputtype = "Selected input type is ";
    inputtype.append( HWInput::HWInputTypeToString( type ).c_str() );

    m_labelType->setText( inputtype );

    // delete old baseWidget if it exists
    if(m_baseWidget != NULL)
    {
        m_layout->removeWidget(m_baseWidget);
        delete m_baseWidget;
        m_baseWidget = NULL;
    }

    switch(type)
    {
    case HWInput::Button:
        m_baseWidget = new ConditionInputButtonWidget(this);
        m_layout->addWidget(m_baseWidget, 2, 0, 1, 2);
        break;
    case HWInput::Fader:
        m_baseWidget = new ConditionInputFaderWidget(this);
        m_layout->addWidget(m_baseWidget, 2, 0, 1, 2);
        break;
    default:
        LOG_WARN(Logger::UI, "Unsupported input type");
        break;
    }
}

void ConditionInputWidget::edit(Condition* cond)
{
    ConditionInput* condition = (ConditionInput*)cond;

    QString str = QString::fromStdString( condition->getHWName() );

    for(unsigned int i = 0; i < m_comboInput->count(); i++)
    {
        if( m_comboInput->itemText(i).compare(str, Qt::CaseInsensitive) == 0 )
        {
            m_comboInput->setCurrentIndex(i);
            break;
        }
    }

    if( m_baseWidget != NULL)
    {
        m_baseWidget->edit(condition);
    }
}

Condition* ConditionInputWidget::assemble()
{
    pi_assert(m_baseWidget != NULL);

    ConditionInput* condition = (ConditionInput*) ((IConditionWidget*)m_baseWidget)->assemble();

    condition->setHWName( m_comboInput->currentText().toStdString() );

    return condition;
}


ConditionInputButtonWidget::ConditionInputButtonWidget(QWidget* parent) : IConditionWidget(parent)
{
    // create comboBox and add items
    m_combo = new QComboBox(this);
    m_combo->addItem("Pressed");
    m_combo->addItem("Released");
    m_combo->addItem("Changed");

    m_label = new QLabel("Select Trigger", this);

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(m_label, 0, 0);
    layout->addWidget(m_combo, 0, 1);

    this->setLayout(layout);
}

Condition* ConditionInputButtonWidget::assemble()
{
    ConditionInputButton* condition = new ConditionInputButton();

    condition->setTrigger( (ConditionInputButton::Trigger)m_combo->currentIndex() );

    return condition;
}

void ConditionInputButtonWidget::edit(Condition *cond)
{
    ConditionInputButton* condition = (ConditionInputButton*)cond;
    m_combo->setCurrentIndex(condition->getTrigger());
}

ConditionInputFaderWidget::ConditionInputFaderWidget(QWidget* parent) : IConditionWidget(parent)
{
    // create comboBox and add items
    m_combo = new QComboBox(this);
    m_combo->addItem("is greater than");
    m_combo->addItem("is less than");
    m_combo->addItem("is equal to");
    m_combo->addItem("has changed");

    m_label = new QLabel("If value", this);

    m_spinBox = new QSpinBox(this);
    m_spinBox->setMaximum(100);

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(m_label, 0, 0);
    layout->addWidget(m_combo, 0, 1);
    layout->addWidget(m_spinBox, 0, 2);

    this->setLayout(layout);

    // connect all signals - slots
    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(comboChanged(int)));
}

void ConditionInputFaderWidget::comboChanged(int index)
{
    if( m_combo->currentIndex() == ConditionInputFader::Changed )
        m_spinBox->hide();
    else
        m_spinBox->show();
}

Condition* ConditionInputFaderWidget::assemble()
{
    ConditionInputFader* condition = new ConditionInputFader();

    condition->setTrigger( (ConditionInputFader::Trigger)m_combo->currentIndex() );

    condition->setTriggerValue( m_spinBox->text().toInt() );

    return condition;
}

void ConditionInputFaderWidget::edit(Condition *cond)
{
    ConditionInputFader* condition = (ConditionInputFader*)cond;
    m_combo->setCurrentIndex(condition->getTrigger());
    m_spinBox->setValue( condition->getTriggerValue() );
}


ConditionVariableWidget::ConditionVariableWidget(QWidget* parent, Script *script) : IConditionWidget(parent)
{
    m_combo = new QComboBox(this);
    m_label = new QLabel("Select variable", this);

    // fill combo box with all variables
    std::list<Variable*> listVariable = script->getVariableList();
    for(std::list<Variable*>::iterator it = listVariable.begin(); it != listVariable.end(); it++)
    {
        m_combo->addItem((*it)->getName().c_str());
    }

    m_comboTrigger = new QComboBox(this);
    m_comboTrigger->addItem("equal to");
    m_comboTrigger->addItem("no longer equal to");
    m_comboTrigger->addItem("greater than to");
    m_comboTrigger->addItem("Less than to");

    m_labelTrigger = new QLabel("If value is ", this);

    m_spinValue = new QSpinBox(this);
    m_spinValue->setMaximum(100);


    m_layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    m_layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    m_layout->addWidget(m_label, 0, 0);
    m_layout->addWidget(m_combo, 0, 1, 1, 2);
    m_layout->addWidget(m_labelTrigger, 1, 0);
    m_layout->addWidget(m_comboTrigger, 1, 1);
    m_layout->addWidget(m_spinValue, 1, 2);

    this->setLayout(m_layout);
}

void ConditionVariableWidget::edit(Condition* cond)
{
    ConditionVariable* condition = (ConditionVariable*)cond;

    QString str = QString::fromStdString( condition->getVarName() );

    for(int i = 0; i < m_combo->count(); i++)
    {
        if( m_combo->itemText(i).compare(str, Qt::CaseInsensitive) == 0 )
        {
            m_combo->setCurrentIndex(i);
            break;
        }
    }

    m_comboTrigger->setCurrentIndex( condition->getTrigger() );
    m_spinValue->setValue( condition->getTriggerValue() );
}

Condition* ConditionVariableWidget::assemble()
{
    ConditionVariable* condition = new ConditionVariable();

    condition->setVarName( m_combo->currentText().toStdString() );
    condition->setTrigger( (ConditionVariable::Trigger)m_comboTrigger->currentIndex() );
    condition->setTriggerValue( m_spinValue->value() );

    return condition;
}
