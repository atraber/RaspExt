
#include "ui/ActionDialog.h"
#include "ui_ActionDialog.h"

#include "script/ActionOutput.h"
#include "script/ActionOutputLED.h"
#include "script/ActionOutputDCMotor.h"
#include "script/ActionOutputRelay.h"
#include "script/ActionOutputGPO.h"
#include "script/ActionOutputStepper.h"
#include "script/ActionOutputStepperSoftStop.h"
#include "script/ActionOutputStepperRunVelocity.h"
#include "script/ActionOutputStepperPositioning.h"
#include "script/ActionOutputStepperSetParam.h"
#include "script/ActionVariable.h"
#include "script/ActionSleep.h"
#include "script/ActionCallRule.h"
#include "script/ActionMusic.h"
#include "hw/HWInput.h"
#include "util/Debug.h"

#include <QFileDialog>

ActionDialog::ActionDialog(QWidget *parent, Script *script) :
    QDialog(parent),
    ui(new Ui::ActionDialog)
{
    pi_assert(script != NULL);

    ui->setupUi(this);

    m_script = script;
    m_action = NULL;
    m_baseWidget = NULL;

    // connect all signals - slots
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(okPressed()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(ui->comboAction, SIGNAL(currentIndexChanged(int)), this, SLOT(actionChanged(int)));

    // default values
    this->actionChanged( ui->comboAction->currentIndex() );
}

void ActionDialog::actionChanged(int index)
{
    if(m_baseWidget != NULL)
    {
        ui->frameGrid->removeWidget(m_baseWidget);
        delete m_baseWidget;
        m_baseWidget = NULL;
    }

    Action::Type type = (Action::Type)index;

    switch(type)
    {
    case Action::Output:
        m_baseWidget = new ActionOutputWidget(this, m_script);
        break;

    case Action::Var:
        m_baseWidget = new ActionVariableWidget(this, m_script);
        break;

    case Action::Sleep:
        m_baseWidget = new ActionSleepWidget(this, m_script);
        break;

    case Action::CallRule:
        m_baseWidget = new ActionCallRuleWidget(this, m_script);
        break;
    case Action::Music:
        m_baseWidget = new ActionMusicWidget(this, m_script);
        break;
    }

    if(m_baseWidget != NULL)
    {
        ui->frameGrid->addWidget(m_baseWidget, 1, 0, 1, 2);
    }
}

void ActionDialog::okPressed()
{
    pi_assert(m_action == NULL);
    pi_assert(m_baseWidget != NULL);

    m_action = ((IActionWidget*)m_baseWidget)->assemble();

    if(m_action == NULL)
        pi_warn("Assembling Action failed");

    this->done(Accepted);
}

void ActionDialog::edit(Action *action)
{
    ui->comboAction->setCurrentIndex(action->getType());
    if(m_baseWidget != NULL)
    {
        m_baseWidget->edit(action);
    }
}

ActionDialog::~ActionDialog()
{
    delete ui;
}



ActionOutputWidget::ActionOutputWidget(QWidget* parent, Script *script) : IActionWidget(parent)
{
    m_baseWidget = NULL;
    m_script = script;

    m_comboOutput = new QComboBox(this);
    m_label = new QLabel("Select input", this);
    m_labelType = new QLabel("");

    // fill combo box with all outputs
    std::list<HWOutput*> listOutputs = script->getOutputList();
    for(std::list<HWOutput*>::iterator it = listOutputs.begin(); it != listOutputs.end(); it++)
    {
        m_comboOutput->addItem((*it)->getName().c_str(), qVariantFromValue((void*)*it));
    }

    m_layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    m_layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    m_layout->addWidget(m_labelType, 0, 0);
    m_layout->addWidget(m_comboOutput, 0, 1);
    m_layout->addWidget(m_labelType, 1, 0, 1, 2);

    this->setLayout(m_layout);

    // connect all signals - slots
    connect(m_comboOutput, SIGNAL(currentIndexChanged(int)), this, SLOT(outputChanged(int)));

    // default values
    this->outputChanged( m_comboOutput->currentIndex() );
}

void ActionOutputWidget::outputChanged(int index)
{
    // if the combobox is empty, then there is nothing to do here
    if(m_comboOutput->count() == 0)
        return;

    QVariant variant = m_comboOutput->itemData( index );
    HWOutput* hw = (HWOutput*)variant.value<void*>();

    HWOutput::HWOutputType type = hw->getType();

    QString outputtype = "Selected output type is ";
    outputtype.append( HWOutput::HWOutputTypeToString( type ).c_str() );

    m_labelType->setText( outputtype );

    // delete old baseWidget if it exists
    if(m_baseWidget != NULL)
    {
        m_layout->removeWidget(m_baseWidget);
        delete m_baseWidget;
        m_baseWidget = NULL;
    }

    switch(type)
    {
    case HWOutput::Relay:
        m_baseWidget = new ActionOutputRelayWidget(this, m_script);
        break;
    case HWOutput::GPO:
        m_baseWidget = new ActionOutputGPOWidget(this, m_script);
        break;
    case HWOutput::DCMotor:
        m_baseWidget = new ActionOutputDCMotorWidget(this, m_script);
        break;
    case HWOutput::LED:
        m_baseWidget = new ActionOutputLEDWidget(this, m_script);
        break;
    case HWOutput::Stepper:
        m_baseWidget = new ActionOutputStepperWidget(this, m_script);
        break;

    default:
        pi_warn("Unknown output type");
        break;
    }

    // add new baseWidget, this should never be NULL. If it is, somewhere something went wrong
    if(m_baseWidget != NULL)
    {
        m_layout->addWidget(m_baseWidget, 2, 0, 1, 2);
    }
}

void ActionOutputWidget::edit(Action* act)
{
    ActionOutput* action = (ActionOutput*)act;

    QString str = QString::fromStdString( action->getHWName() );

    for(int i = 0; i < m_comboOutput->count(); i++)
    {
        if( m_comboOutput->itemText(i).compare(str, Qt::CaseInsensitive) == 0 )
        {
            m_comboOutput->setCurrentIndex(i);
            break;
        }
    }

    if( m_baseWidget != NULL)
    {
        m_baseWidget->edit(action);
    }
}

Action* ActionOutputWidget::assemble()
{
    pi_assert(m_baseWidget != NULL);

    ActionOutput* action = (ActionOutput*) ((IActionWidget*)m_baseWidget)->assemble();

    action->setHWName( m_comboOutput->currentText().toStdString() );

    return action;
}


ActionOutputRelayWidget::ActionOutputRelayWidget(QWidget* parent, Script *script) : IActionWidget(parent)
{
    // create comboBox and add items
    m_combo = new QComboBox(this);
    m_combo->addItem("Off");
    m_combo->addItem("On");
    m_combo->addItem("Toggle");


    m_label = new QLabel("Set relay to", this);

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(m_label, 0, 0);
    layout->addWidget(m_combo, 0, 1);

    this->setLayout(layout);
}

Action* ActionOutputRelayWidget::assemble()
{
    ActionOutputRelay* action = new ActionOutputRelay();

    action->setState( (ActionOutputRelay::State)m_combo->currentIndex() );

    return action;
}

void ActionOutputRelayWidget::edit(Action* act)
{
    ActionOutputRelay* action = (ActionOutputRelay*)act;
    m_combo->setCurrentIndex( action->getState());
}



ActionOutputGPOWidget::ActionOutputGPOWidget(QWidget* parent, Script *script) : IActionWidget(parent)
{
    // create comboBox and add items
    m_combo = new QComboBox(this);
    m_combo->addItem("Low");
    m_combo->addItem("High");
    m_combo->addItem("Toggle");

    m_label = new QLabel("Set GPO to", this);

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(m_label, 0, 0);
    layout->addWidget(m_combo, 0, 1);

    this->setLayout(layout);
}

Action* ActionOutputGPOWidget::assemble()
{
    ActionOutputGPO* action = new ActionOutputGPO();

    action->setState( (ActionOutputGPO::State)m_combo->currentIndex() );

    return action;
}

void ActionOutputGPOWidget::edit(Action* act)
{
    ActionOutputGPO* action = (ActionOutputGPO*)act;
    m_combo->setCurrentIndex( action->getState());
}


ActionOutputLEDWidget::ActionOutputLEDWidget(QWidget* parent, Script *script) : IActionWidget(parent)
{
    m_label = new QLabel("Set brightness of LED to ", this);
    m_spinBox = new QSpinBox(this);
    m_spinBox->setMinimum(0);
    m_spinBox->setMaximum(100);

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(m_label, 0, 0);
    layout->addWidget(m_spinBox, 0, 1);

    this->setLayout(layout);
}

Action* ActionOutputLEDWidget::assemble()
{
    ActionOutputLED* action = new ActionOutputLED();

    action->setValue( m_spinBox->value() );

    return action;
}

void ActionOutputLEDWidget::edit(Action* act)
{
    ActionOutputLED* action = (ActionOutputLED*)act;

    m_spinBox->setValue( action->getValue() );
}

ActionOutputDCMotorWidget::ActionOutputDCMotorWidget(QWidget* parent, Script* script) : IActionWidget(parent)
{
    m_labelState = new QLabel("Set motor to", this);
    // create comboBox and add items
    m_comboState = new QComboBox(this);
    m_comboState->addItem("Standby");
    m_comboState->addItem("Reverse");
    m_comboState->addItem("Forward");
    m_comboState->addItem("Brake");

    m_labelSpeed = new QLabel("Set speed to", this);

    // create comboBox to select a fader
    m_comboSpeed = new QComboBox(this);
    m_comboSpeed->addItem("value");
    std::list<HWInput*> listInputs = script->getInputList();
    for(std::list<HWInput*>::iterator it = listInputs.begin(); it != listInputs.end(); it++)
    {
        if( (*it)->getType() == HWInput::Fader )
            m_comboSpeed->addItem((*it)->getName().c_str());
    }

    m_spinSpeed = new QSpinBox(this);
    m_spinSpeed->setMaximum(100);
    m_spinSpeed->setMinimum(0);

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(m_labelState, 0, 0);
    layout->addWidget(m_comboState, 0, 1, 1, 2);
    layout->addWidget(m_labelSpeed, 1, 0);
    layout->addWidget(m_comboSpeed, 1, 1);
    layout->addWidget(m_spinSpeed, 1, 2);

    this->setLayout(layout);

    // connect all signals-slots
    connect(m_comboSpeed, SIGNAL(currentIndexChanged(int)), this, SLOT(speedComboChanged(int)));
}

void ActionOutputDCMotorWidget::speedComboChanged(int index)
{
    if( index != 0 )
        m_spinSpeed->hide();
    else
        m_spinSpeed->show();
}

Action* ActionOutputDCMotorWidget::assemble()
{
    ActionOutputDCMotor* action = new ActionOutputDCMotor();

    action->setState( (HWOutputDCMotor::MotorState)m_comboState->currentIndex() );
    action->setSpeed( m_spinSpeed->value() );

    if( m_comboSpeed->currentIndex() != 0 )
    {
        action->setInputName( m_comboSpeed->currentText().toStdString() );
    }

    return action;
}

void ActionOutputDCMotorWidget::edit(Action* act)
{
    ActionOutputDCMotor* action = (ActionOutputDCMotor*)act;

    m_comboState->setCurrentIndex( action->getState() );
    m_spinSpeed->setValue( action->getSpeed() );


    // only set this if it is used
    if( !action->getInputName().empty() )
    {
        QString str = QString::fromStdString( action->getInputName() );

        for(int i = 0; i < m_comboSpeed->count(); i++)
        {
            if( m_comboSpeed->itemText(i).compare(str, Qt::CaseInsensitive) == 0 )
            {
                m_comboSpeed->setCurrentIndex(i);
                break;
            }
        }
    }
}

ActionOutputStepperWidget::ActionOutputStepperWidget(QWidget* parent, Script* script) : IActionWidget(parent)
{
    m_baseWidget = NULL;

    m_label = new QLabel("Select type", this);

    // create comboBox for all possible types
    m_combo = new QComboBox(this);
    m_combo->addItem("Soft stop");
    m_combo->addItem("RunVelocity");
    m_combo->addItem("Positioning");
    m_combo->addItem("Set parameters");

    m_layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    m_layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    m_layout->addWidget(m_label, 0, 0);
    m_layout->addWidget(m_combo, 0, 1);

    this->setLayout(m_layout);

    // connect all signals-slots
    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(typeComboChanged(int)));

    // default values
    this->typeComboChanged( m_combo->currentIndex() );
}

void ActionOutputStepperWidget::typeComboChanged(int index)
{
    if(m_baseWidget != NULL)
    {
        m_layout->removeWidget(m_baseWidget);
        delete m_baseWidget;
        m_baseWidget = NULL;
    }

    switch(index)
    {
    case ActionOutputStepper::SoftStop:
        m_baseWidget = new ActionOutputStepperSoftStopWidget(this, NULL); // we do not need Script in this class, so we just take NULL
        break;
    case ActionOutputStepper::RunVelocity:
        m_baseWidget = new ActionOutputStepperRunVelocityWidget(this, NULL); // we do not need Script in this class, so we just take NULL
        break;
    case ActionOutputStepper::Positioning:
        m_baseWidget = new ActionOutputStepperPositioningWidget(this, NULL); // we do not need Script in this class, so we just take NULL
        break;
    case ActionOutputStepper::SetParam:
        m_baseWidget = new ActionOutputStepperSetParamWidget(this, NULL); // we do not need Script in this class, so we just take NULL
        break;
    }

    // add new baseWidget, this should never be NULL. If it is, somewhere something went wrong
    if(m_baseWidget != NULL)
    {
        m_layout->addWidget(m_baseWidget, 2, 0, 1, 2);
    }
}

Action* ActionOutputStepperWidget::assemble()
{
    if( m_baseWidget != NULL)
        return m_baseWidget->assemble();

    return NULL;
}

void ActionOutputStepperWidget::edit(Action* act)
{
    ActionOutputStepper* action = (ActionOutputStepper*)act;

    m_combo->setCurrentIndex( action->getStepperType() );

    if( m_baseWidget != NULL)
    {
        m_baseWidget->edit(action);
    }
}

ActionOutputStepperSoftStopWidget::ActionOutputStepperSoftStopWidget(QWidget* parent, Script* script) : IActionWidget(parent)
{
    m_label = new QLabel("Do a soft stop", this);

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(m_label, 0, 0);

    this->setLayout(layout);
}

Action* ActionOutputStepperSoftStopWidget::assemble()
{
    return new ActionOutputStepperSoftStop();
}

void ActionOutputStepperSoftStopWidget::edit(Action* act)
{
    // nothing to do
}

ActionOutputStepperRunVelocityWidget::ActionOutputStepperRunVelocityWidget(QWidget* parent, Script* script) : IActionWidget(parent)
{
    m_label = new QLabel("Send command RunVelocity to the stepper", this);

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(m_label, 0, 0);

    this->setLayout(layout);
}

Action* ActionOutputStepperRunVelocityWidget::assemble()
{
    return new ActionOutputStepperRunVelocity();
}

void ActionOutputStepperRunVelocityWidget::edit(Action* act)
{
    // nothing to do
}

ActionOutputStepperPositioningWidget::ActionOutputStepperPositioningWidget(QWidget* parent, Script* script) : IActionWidget(parent)
{
    QLabel* labelType = new QLabel("Select positioning type", this);
    m_comboType = new QComboBox(this);
    m_comboType->addItem("Set Position");
    m_comboType->addItem("Set Dual Position");
    m_comboType->addItem("Reset Position");

    m_label = new QLabel("Set target position", this);
    m_spinBox = new QSpinBox(this);
    m_spinBox->setMinimum( SHRT_MIN );
    m_spinBox->setMaximum( SHRT_MAX );

    m_label2 = new QLabel("Set target position 2", this);
    m_spinBox2 = new QSpinBox(this);
    m_spinBox2->setMinimum( SHRT_MIN );
    m_spinBox2->setMaximum( SHRT_MAX );

    m_labelVmin = new QLabel("Set Vmin", this);
    m_spinVmin = new QSpinBox(this);
    m_spinVmin->setMinimum( 0 );
    m_spinVmin->setMaximum( 15 );

    m_labelVmax = new QLabel("Set Vmax", this);
    m_spinVmax = new QSpinBox(this);
    m_spinVmax->setMinimum( 0 );
    m_spinVmax->setMaximum( 15 );

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(labelType, 0, 0);
    layout->addWidget(m_comboType, 0, 1);
    layout->addWidget(m_label, 1, 0);
    layout->addWidget(m_spinBox, 1, 1);
    layout->addWidget(m_label2, 2, 0);
    layout->addWidget(m_spinBox2, 2, 1);

    layout->addWidget(m_labelVmin, 3, 0);
    layout->addWidget(m_spinVmin, 3, 1);
    layout->addWidget(m_labelVmax, 4, 0);
    layout->addWidget(m_spinVmax, 4, 1);

    this->setLayout(layout);

    // connect all signals - slots
    connect(m_comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(comboTypeChanged(int)));

    // set default values
    this->comboTypeChanged( m_comboType->currentIndex() );
}

void ActionOutputStepperPositioningWidget::comboTypeChanged(int index)
{
    if(index == ActionOutputStepperPositioning::SetPosition)
    {
        m_label->setText("Set target position");

        m_label->show();
        m_spinBox->show();
        m_label2->hide();
        m_spinBox2->hide();
        m_labelVmin->hide();
        m_spinVmin->hide();
        m_labelVmax->hide();
        m_spinVmax->hide();
    }
    else if(index == ActionOutputStepperPositioning::SetDualPosition)
    {
        m_label->setText("Set target position 1");

        m_label->show();
        m_spinBox->show();
        m_label2->show();
        m_spinBox2->show();
        m_labelVmin->show();
        m_spinVmin->show();
        m_labelVmax->show();
        m_spinVmax->show();
    }
    else // index == ActionOutputStepperPositioning::ResetPosition
    {
        m_label->hide();
        m_spinBox->hide();
        m_label2->hide();
        m_spinBox2->hide();
        m_labelVmin->hide();
        m_spinVmin->hide();
        m_labelVmax->hide();
        m_spinVmax->hide();
    }
}

Action* ActionOutputStepperPositioningWidget::assemble()
{
    ActionOutputStepperPositioning* action =  new ActionOutputStepperPositioning();

    action->setPositioningType( (ActionOutputStepperPositioning::PositioningType) m_comboType->currentIndex() );
    action->setDualPosition( m_spinBox->value(), m_spinBox2->value(), m_spinVmin->value(), m_spinVmax->value());
    action->setPosition( m_spinBox->value() );

    return action;
}

void ActionOutputStepperPositioningWidget::edit(Action* act)
{
    ActionOutputStepperPositioning* action = (ActionOutputStepperPositioning*)act;

    m_comboType->setCurrentIndex( action->getPositioningType() );
    m_spinBox->setValue( action->getPosition() );
    m_spinBox2->setValue( action->getDualPosition2() );
    m_spinVmin->setValue( action->getDualVmin() );
    m_spinVmax->setValue( action->getDualVmax() );
}

ActionOutputStepperSetParamWidget::ActionOutputStepperSetParamWidget(QWidget* parent, Script* script) : IActionWidget(parent)
{
    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    unsigned int i = 0;

    QLabel* m_labelIrun = new QLabel("Irun", this);
    m_comboIrun = new QComboBox(this);
    m_comboIrun->addItem("keep");
    m_comboIrun->addItem("set");
    m_spinIrun = new QSpinBox(this);
    m_spinIrun->setMinimum(0);
    m_spinIrun->setMaximum(0xF);

    layout->addWidget(m_labelIrun, i, 0);
    layout->addWidget(m_comboIrun, i, 1);
    layout->addWidget(m_spinIrun, i, 2);
    i++;


    QLabel* m_labelIhold = new QLabel("Ihold", this);
    m_comboIhold = new QComboBox(this);
    m_comboIhold->addItem("keep");
    m_comboIhold->addItem("set");
    m_spinIhold = new QSpinBox(this);
    m_spinIhold->setMinimum(0);
    m_spinIhold->setMaximum(0xF);

    layout->addWidget(m_labelIhold, i, 0);
    layout->addWidget(m_comboIhold, i, 1);
    layout->addWidget(m_spinIhold, i, 2);
    i++;


    QLabel* m_labelVmax = new QLabel("Vmax", this);
    m_comboVmax = new QComboBox(this);
    m_comboVmax->addItem("keep");
    m_comboVmax->addItem("set");
    m_spinVmax = new QSpinBox(this);
    m_spinVmax->setMinimum(0);
    m_spinVmax->setMaximum(0xF);

    layout->addWidget(m_labelVmax, i, 0);
    layout->addWidget(m_comboVmax, i, 1);
    layout->addWidget(m_spinVmax, i, 2);
    i++;


    QLabel* m_labelVmin = new QLabel("Vmin", this);
    m_comboVmin = new QComboBox(this);
    m_comboVmin->addItem("keep");
    m_comboVmin->addItem("set");
    m_spinVmin = new QSpinBox(this);
    m_spinVmin->setMinimum(0);
    m_spinVmin->setMaximum(0xF);

    layout->addWidget(m_labelVmin, i, 0);
    layout->addWidget(m_comboVmin, i, 1);
    layout->addWidget(m_spinVmin, i, 2);
    i++;


    QLabel* m_labelMinSamples = new QLabel("Min Samples", this);
    m_comboMinSamples = new QComboBox(this);
    m_comboMinSamples->addItem("keep");
    m_comboMinSamples->addItem("set");
    m_spinMinSamples = new QSpinBox(this);
    m_spinMinSamples->setMinimum(0);
    m_spinMinSamples->setMaximum(0x7);

    layout->addWidget(m_labelMinSamples, i, 0);
    layout->addWidget(m_comboMinSamples, i, 1);
    layout->addWidget(m_spinMinSamples, i, 2);
    i++;


    QLabel* m_labelShaft = new QLabel("Shaft", this);
    m_comboShaft = new QComboBox(this);
    m_comboShaft->addItem("keep");
    m_comboShaft->addItem("set");
    m_spinShaft = new QSpinBox(this);
    m_spinShaft->setMinimum(0);
    m_spinShaft->setMaximum(1);

    layout->addWidget(m_labelShaft, i, 0);
    layout->addWidget(m_comboShaft, i, 1);
    layout->addWidget(m_spinShaft, i, 2);
    i++;


    QLabel* m_labelAcc = new QLabel("Acceleration", this);
    m_comboAcc = new QComboBox(this);
    m_comboAcc->addItem("keep");
    m_comboAcc->addItem("set");
    m_spinAcc = new QSpinBox(this);
    m_spinAcc->setMinimum(0);
    m_spinAcc->setMaximum(0xF);

    layout->addWidget(m_labelAcc, i, 0);
    layout->addWidget(m_comboAcc, i, 1);
    layout->addWidget(m_spinAcc, i, 2);
    i++;


    QLabel* m_labelAbsThr = new QLabel("Absolut Threshold", this);
    m_comboAbsThr = new QComboBox(this);
    m_comboAbsThr->addItem("keep");
    m_comboAbsThr->addItem("set");
    m_spinAbsThr = new QSpinBox(this);
    m_spinAbsThr->setMinimum(0);
    m_spinAbsThr->setMaximum(0xF);

    layout->addWidget(m_labelAbsThr, i, 0);
    layout->addWidget(m_comboAbsThr, i, 1);
    layout->addWidget(m_spinAbsThr, i, 2);
    i++;


    QLabel* m_labelDelThr = new QLabel("Delta Threshold", this);
    m_comboDelThr = new QComboBox(this);
    m_comboDelThr->addItem("keep");
    m_comboDelThr->addItem("set");
    m_spinDelThr = new QSpinBox(this);
    m_spinDelThr->setMinimum(0);
    m_spinDelThr->setMaximum(0xF);

    layout->addWidget(m_labelDelThr, i, 0);
    layout->addWidget(m_comboDelThr, i, 1);
    layout->addWidget(m_spinDelThr, i, 2);
    i++;

    QLabel* m_labelFS2StallEn = new QLabel("FS2 Stall Enabled", this);
    m_comboFS2StallEn = new QComboBox(this);
    m_comboFS2StallEn->addItem("keep");
    m_comboFS2StallEn->addItem("set");
    m_spinFS2StallEn = new QSpinBox(this);
    m_spinFS2StallEn->setMinimum(0);
    m_spinFS2StallEn->setMaximum(0x7);

    layout->addWidget(m_labelFS2StallEn, i, 0);
    layout->addWidget(m_comboFS2StallEn, i, 1);
    layout->addWidget(m_spinFS2StallEn, i, 2);
    i++;

    QLabel* m_labelAccShape = new QLabel("Acceleration Shape", this);
    m_comboAccShape = new QComboBox(this);
    m_comboAccShape->addItem("keep");
    m_comboAccShape->addItem("set");
    m_spinAccShape = new QSpinBox(this);
    m_spinAccShape->setMinimum(0);
    m_spinAccShape->setMaximum(1);

    layout->addWidget(m_labelAccShape, i, 0);
    layout->addWidget(m_comboAccShape, i, 1);
    layout->addWidget(m_spinAccShape, i, 2);
    i++;

    QLabel* m_labelStepMode= new QLabel("Step Mode", this);
    m_comboStepMode = new QComboBox(this);
    m_comboStepMode->addItem("keep");
    m_comboStepMode->addItem("set");
    m_spinStepMode = new QSpinBox(this);
    m_spinStepMode->setMinimum(0);
    m_spinStepMode->setMaximum(3);

    layout->addWidget(m_labelStepMode, i, 0);
    layout->addWidget(m_comboStepMode, i, 1);
    layout->addWidget(m_spinStepMode, i, 2);
    i++;

    QLabel* m_labelDC100StEn= new QLabel("DC100StEn", this);
    m_comboDC100StEn = new QComboBox(this);
    m_comboDC100StEn->addItem("keep");
    m_comboDC100StEn->addItem("set");
    m_spinDC100StEn = new QSpinBox(this);
    m_spinDC100StEn->setMinimum(0);
    m_spinDC100StEn->setMaximum(1);

    layout->addWidget(m_labelDC100StEn, i, 0);
    layout->addWidget(m_comboDC100StEn, i, 1);
    layout->addWidget(m_spinDC100StEn, i, 2);
    i++;


    QLabel* m_labelPWMJEn= new QLabel("PWM Jitter Enable", this);
    m_comboPWMJEn = new QComboBox(this);
    m_comboPWMJEn->addItem("keep");
    m_comboPWMJEn->addItem("set");
    m_spinPWMJEn = new QSpinBox(this);
    m_spinPWMJEn->setMinimum(0);
    m_spinPWMJEn->setMaximum(1);

    layout->addWidget(m_labelPWMJEn, i, 0);
    layout->addWidget(m_comboPWMJEn, i, 1);
    layout->addWidget(m_spinPWMJEn, i, 2);
    i++;


    QLabel* m_labelPWMfreq= new QLabel("PWM Frequency", this);
    QLabel* m_labelPWMfreq2 = new QLabel("set");
    m_spinPWMfreq = new QSpinBox(this);
    m_spinPWMfreq->setMinimum(0);
    m_spinPWMfreq->setMaximum(1);

    layout->addWidget(m_labelPWMfreq, i, 0);
    layout->addWidget(m_labelPWMfreq2, i, 1);
    layout->addWidget(m_spinPWMfreq, i, 2);
    i++;


    QLabel* m_labelSecPos = new QLabel("Secure Position", this);
    m_comboSecPos = new QComboBox(this);
    m_comboSecPos->addItem("keep");
    m_comboSecPos->addItem("set");
    m_spinSecPos = new QSpinBox(this);
    m_spinSecPos->setMinimum(0);
    m_spinSecPos->setMaximum(0x03FF);

    layout->addWidget(m_labelSecPos, i, 0);
    layout->addWidget(m_comboSecPos, i, 1);
    layout->addWidget(m_spinSecPos, i, 2);
    i++;

    this->setLayout(layout);
}

Action* ActionOutputStepperSetParamWidget::assemble()
{
    ActionOutputStepperSetParam* action = new ActionOutputStepperSetParam();

    HWOutputStepper::Param param;

    param.irunSet = m_comboIrun->currentIndex();
    param.irun = m_spinIrun->value();

    param.iholdSet = m_comboIhold->currentIndex();
    param.ihold = m_spinIhold->value();

    param.vmaxSet = m_comboVmax->currentIndex();
    param.vmax = m_spinVmax->value();

    param.vminSet = m_comboVmin->currentIndex();
    param.vmin = m_spinVmin->value();

    param.accShapeSet = m_comboAccShape->currentIndex();
    param.accShape = m_spinAccShape->value();

    param.stepModeSet = m_comboStepMode->currentIndex();
    param.stepMode = m_spinStepMode->value();

    param.shaftSet = m_comboShaft->currentIndex();
    param.shaft = m_spinShaft->value();

    param.accSet = m_comboAcc->currentIndex();
    param.acc = m_spinAcc->value();

    param.absoluteThresholdSet = m_comboAbsThr->currentIndex();
    param.absoluteThreshold = m_spinAbsThr->value();

    param.deltaThresholdSet = m_comboDelThr->currentIndex();
    param.deltaThreshold = m_spinDelThr->value();

    param.securePositionSet = m_comboSecPos->currentIndex();
    param.securePosition = m_spinSecPos->value();

    param.fs2StallEnabledSet = m_comboFS2StallEn->currentIndex();
    param.fs2StallEnabled = m_spinFS2StallEn->value();

    param.minSamplesSet = m_comboMinSamples->currentIndex();
    param.minSamples = m_spinMinSamples->value();

    param.dc100StallEnableSet = m_comboDC100StEn->currentIndex();
    param.dc100StallEnable = m_spinDC100StEn->value();

    param.PWMJitterEnableSet = m_comboPWMJEn->currentIndex();
    param.PWMJitterEnable = m_spinPWMJEn->value();

    param.PWMfreqSet = true; // TODO: Make this adjustable
    param.PWMfreq = m_spinPWMfreq->value();

    action->setParam(param);

    return action;
}

void ActionOutputStepperSetParamWidget::edit(Action* act)
{
    ActionOutputStepperSetParam* action = (ActionOutputStepperSetParam*)act;
    HWOutputStepper::Param param = action->getParam();

    m_comboIrun->setCurrentIndex( param.irunSet );
    m_spinIrun->setValue( param.irun );

    m_comboIhold->setCurrentIndex( param.iholdSet );
    m_spinIhold->setValue( param.ihold );

    m_comboVmax->setCurrentIndex( param.vmaxSet );
    m_spinVmax->setValue( param.vmax );

    m_comboVmin->setCurrentIndex( param.vminSet );
    m_spinVmin->setValue( param.vmin );

    m_comboAccShape->setCurrentIndex( param.accShapeSet );
    m_spinAccShape->setValue( param.accShape );

    m_comboStepMode->setCurrentIndex( param.stepModeSet );
    m_spinStepMode->setValue( param.stepMode );

    m_comboShaft->setCurrentIndex( param.shaftSet );
    m_spinShaft->setValue( param.shaft );

    m_comboAcc->setCurrentIndex( param.accSet );
    m_spinAcc->setValue( param.acc );

    m_comboAbsThr->setCurrentIndex( param.absoluteThresholdSet );
    m_spinAbsThr->setValue( param.absoluteThreshold );

    m_comboDelThr->setCurrentIndex( param.deltaThresholdSet );
    m_spinDelThr->setValue( param.deltaThreshold );

    m_comboSecPos->setCurrentIndex( param.securePositionSet );
    m_spinSecPos->setValue( param.securePosition );

    m_comboFS2StallEn->setCurrentIndex( param.fs2StallEnabledSet );
    m_spinFS2StallEn->setValue( param.fs2StallEnabled );

    m_comboMinSamples->setCurrentIndex( param.minSamplesSet );
    m_spinMinSamples->setValue( param.minSamples );

    m_comboDC100StEn->setCurrentIndex( param.dc100StallEnableSet );
    m_spinDC100StEn->setValue( param.dc100StallEnable );

    m_comboPWMJEn->setCurrentIndex( param.PWMJitterEnableSet );
    m_spinPWMJEn->setValue( param.PWMJitterEnable );

    m_spinPWMfreq->setValue( param.PWMfreq );
}


ActionVariableWidget::ActionVariableWidget(QWidget* parent, Script *script) : IActionWidget(parent)
{
    m_combo = new QComboBox(this);
    m_label = new QLabel("Select variable", this);

    // fill combo box with all variables
    std::list<Variable*> listVariable = script->getVariableList();
    for(std::list<Variable*>::iterator it = listVariable.begin(); it != listVariable.end(); it++)
    {
        m_combo->addItem((*it)->getName().c_str());
    }

    m_comboOperator = new QComboBox(this);
    m_comboOperator->addItem("=");
    m_comboOperator->addItem("+=");
    m_comboOperator->addItem("-=");

    m_labelOperator = new QLabel("Set variable", this);

    m_spinOperand = new QSpinBox(this);
    m_spinOperand->setMinimum(-100);
    m_spinOperand->setMaximum(100);


    m_layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    m_layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    m_layout->addWidget(m_label, 0, 0);
    m_layout->addWidget(m_combo, 0, 1, 1, 2);
    m_layout->addWidget(m_labelOperator, 1, 0);
    m_layout->addWidget(m_comboOperator, 1, 1);
    m_layout->addWidget(m_spinOperand, 1, 2);

    this->setLayout(m_layout);
}

void ActionVariableWidget::edit(Action* act)
{
    ActionVariable* action = (ActionVariable*)act;

    QString str = QString::fromStdString( action->getVarName() );

    for(int i = 0; i < m_combo->count(); i++)
    {
        if( m_combo->itemText(i).compare(str, Qt::CaseInsensitive) == 0 )
        {
            m_combo->setCurrentIndex(i);
            break;
        }
    }

    m_comboOperator->setCurrentIndex( action->getOperator() );
    m_spinOperand->setValue( action->getOperand() );
}

Action* ActionVariableWidget::assemble()
{
    ActionVariable* action = new ActionVariable();

    action->setVarName( m_combo->currentText().toStdString() );
    action->setOperator( (ActionVariable::Operator)m_comboOperator->currentIndex() );
    action->setOperand( m_spinOperand->value() );

    return action;
}

ActionSleepWidget::ActionSleepWidget(QWidget* parent, Script *script) : IActionWidget(parent)
{
    m_label = new QLabel("Sleep for", this);

    m_spinWaitMs = new QSpinBox(this);
    m_spinWaitMs->setMinimum(0);
    m_spinWaitMs->setMaximum(30 * 1000);

    m_labelMs = new QLabel("ms", this);


    m_layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    m_layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    m_layout->addWidget(m_label, 0, 0);
    m_layout->addWidget(m_spinWaitMs, 0, 1);
    m_layout->addWidget(m_labelMs, 0, 2);

    this->setLayout(m_layout);
}

void ActionSleepWidget::edit(Action* act)
{
    ActionSleep* action = (ActionSleep*)act;

    m_spinWaitMs->setValue( action->getWaitMs() );
}

Action* ActionSleepWidget::assemble()
{
    ActionSleep* action = new ActionSleep();

    action->setWaitMs( m_spinWaitMs->value() );

    return action;
}

ActionCallRuleWidget::ActionCallRuleWidget(QWidget* parent, Script *script) : IActionWidget(parent)
{
    m_label = new QLabel("Select rule", this);

    m_comboRule = new QComboBox(this);

    // fill combo box with all rule names
    std::vector<Rule*> list = script->getRuleList();
    for(std::vector<Rule*>::iterator it = list.begin(); it != list.end(); it++)
    {
        m_comboRule->addItem((*it)->getName().c_str());
    }


    m_layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    m_layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    m_layout->addWidget(m_label, 0, 0);
    m_layout->addWidget(m_comboRule, 0, 1);

    this->setLayout(m_layout);
}

void ActionCallRuleWidget::edit(Action* act)
{
    ActionCallRule* action = (ActionCallRule*)act;

    QString str = QString::fromStdString( action->getRuleName() );

    for(int i = 0; i < m_comboRule->count(); i++)
    {
        if( m_comboRule->itemText(i).compare(str, Qt::CaseInsensitive) == 0 )
        {
            m_comboRule->setCurrentIndex(i);
            break;
        }
    }
}

Action* ActionCallRuleWidget::assemble()
{
    ActionCallRule* action = new ActionCallRule();

    action->setRuleName( m_comboRule->currentText().toStdString() );

    return action;
}


ActionMusicWidget::ActionMusicWidget(QWidget* parent, Script *script) : IActionWidget(parent)
{
    QLabel* label = new QLabel("Select type", this);

    m_combo = new QComboBox(this);
    m_combo->addItem( "Play music" );
    m_combo->addItem( "Stop music" );

    m_labelMusic = new QLabel(this);
    m_buttonSelect = new QPushButton("Select", this);

    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // add our widgets to the layout
    layout->addWidget(label, 0, 0);
    layout->addWidget(m_combo, 0, 1);
    layout->addWidget(m_labelMusic, 1, 0, 1, 2);
    layout->addWidget(m_buttonSelect, 2, 1);

    this->setLayout(layout);

    // connect all signals-slots
    connect(m_buttonSelect, SIGNAL(clicked()), this, SLOT(selectMusic()));
    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(comboChanged(int)));
}

void ActionMusicWidget::selectMusic()
{
    QFileDialog dialog;

    dialog.setNameFilter("*.mp3 *.wav *.ogg *.flac");

    if( dialog.exec() == QDialog::Accepted )
        m_labelMusic->setText( dialog.selectedFiles().front() );
}

void ActionMusicWidget::comboChanged(int index)
{
    if(index == ActionMusic::Play)
    {
        m_labelMusic->show();
        m_buttonSelect->show();
    }
    else
    {
        m_labelMusic->hide();
        m_buttonSelect->hide();
    }
}

void ActionMusicWidget::edit(Action* act)
{
    ActionMusic* action = (ActionMusic*)act;

    m_combo->setCurrentIndex( action->getMusicAction() );
    m_labelMusic->setText( QString::fromStdString( action->getFilename() ) );
}

Action* ActionMusicWidget::assemble()
{
    ActionMusic* action = new ActionMusic();

    action->setMusicAction( (ActionMusic::MusicAction)m_combo->currentIndex() );
    action->setFilename( m_labelMusic->text().toStdString() );

    return action;
}
