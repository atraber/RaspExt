
#include "script/ActionOutputDCMotor.h"
#include "hw/HWInputFader.h"
#include "ConfigManager.h"
#include "util/Debug.h"

ActionOutputDCMotor::ActionOutputDCMotor()
{
    m_state = HWOutputDCMotor::Standby;
    m_speed = 0;

    m_hwInput = NULL;
}

Action* ActionOutputDCMotor::load(QDomElement* root)
{
    QDomElement elem = root->firstChildElement();

    ActionOutputDCMotor* action = new ActionOutputDCMotor();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("state") == 0)
        {
            action->setState( HWOutputDCMotor::StringToMotorState( elem.text().toStdString() ) );
        }
        else if(elem.tagName().toLower().compare("speed") == 0)
        {
            action->setSpeed( elem.text().toInt() );
        }
        else if(elem.tagName().toLower().compare("inputname") == 0)
        {
            action->setInputName( elem.text().toStdString() );
        }

        elem = elem.nextSiblingElement();
    }

    return action;
}

QDomElement ActionOutputDCMotor::save(QDomElement* root, QDomDocument* document)
{
    QDomElement action = ActionOutput::save(root, document);

    QDomElement subtype = document->createElement("subtype");
    QDomText subtypeText = document->createTextNode("DCMotor");
    subtype.appendChild(subtypeText);

    action.appendChild(subtype);

    if(m_inputName.empty())
    {
        // only save speed if input is empty, otherwise the value of input is used anyways, so we dont care anymore about speed

        QDomElement speed = document->createElement("speed");
        QDomText speedText = document->createTextNode( QString::fromStdString( std::to_string(m_speed) ) );
        speed.appendChild(speedText);

        action.appendChild(speed);
    }
    else
    {
        QDomElement inputname = document->createElement("InputName");
        QDomText inputnameText = document->createTextNode( QString::fromStdString( m_inputName ) );
        inputname.appendChild(inputnameText);

        action.appendChild(inputname);
    }

    QDomElement state = document->createElement("state");
    QDomText stateText = document->createTextNode( QString::fromStdString( HWOutputDCMotor::MotorStateToString(m_state) ) );

    state.appendChild(stateText);

    action.appendChild(state);

    return action;
}


void ActionOutputDCMotor::getRequiredList(std::list<Rule::RequiredInput>* listInput,
                                     std::list<Rule::RequiredOutput>* listOutput,
                                     std::list<Rule::RequiredVariable>* listVariable) const
{
    if(listOutput != NULL)
    {
        Rule::RequiredOutput req;
        req.name = m_HWName;
        req.type = HWOutput::DCMotor;
        req.exists = false;

        listOutput->push_back(req);
    }

    if(listInput != NULL && !m_inputName.empty())
    {
        Rule::RequiredInput req;
        req.name = m_inputName;
        req.type = HWInput::Fader;
        req.exists = false;

        listInput->push_back(req);
    }
}

void ActionOutputDCMotor::init(ConfigManager *config)
{
    ActionOutput::init(config);

    if(!m_inputName.empty())
    {
        m_hwInput = config->getInputByName(m_inputName);

        pi_assert(m_hwInput != NULL && m_hwInput->getType() == HWInput::Fader);
    }
}

void ActionOutputDCMotor::deinit()
{
    m_hwInput = NULL;
}

bool ActionOutputDCMotor::execute(unsigned int)
{
    if(m_hw == NULL)
    {
        LOG_WARN(Logger::Script, "HW does not exist, cannot execute action");
        return true; // still return true as false would mean that all following actions should not be executed
    }

    HWOutputDCMotor* hw = (HWOutputDCMotor*)m_hw;

    if( m_inputName.empty() )
    {
        hw->setSpeedAndState(m_speed, m_state);
    }
    else
    {
        if(m_hwInput == NULL)
        {
            LOG_WARN(Logger::Script, "Input HW does not exist, cannot execute action");
            return true; // still return true as false would mean that all following actions should not be executed
        }

        unsigned int speed = ((HWInputFader*)m_hwInput)->getValue();

        hw->setSpeedAndState(speed, m_state);
    }

    return true;
}

std::string ActionOutputDCMotor::getDescription() const
{
    std::string str = std::string("Set ").append(m_HWName).append(" to ");
    str.append( HWOutputDCMotor::MotorStateToString(m_state) );
    str.append(" with speed set to ");

    if( m_inputName.empty() )
        str.append( std::to_string(m_speed) );
    else
    {
        str.append("value of ");
        str.append( m_inputName );
    }

    return str;
}
