
#include "hw/HWOutputDCMotor.h"
#include "hw/HWOutputDCMotorI2C.h"
#include "hw/HWOutputDCMotorBt.h"

#include "util/Debug.h"

#include <QDomElement>

HWOutputDCMotor::HWOutputDCMotor()
{
    m_type = DCMotor;

    m_state = Standby;
    m_speed = 0;
}

HWOutput* HWOutputDCMotor::load(QDomElement* root)
{
    HWOutputDCMotor* hw = NULL;
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("hwtype") == 0)
        {
            if(elem.text().toLower().compare("i2c") == 0)
            {
                hw = (HWOutputDCMotor*)HWOutputDCMotorI2C::load(root);
            }
            else if(elem.text().toLower().compare("bt") == 0)
            {
                hw = (HWOutputDCMotor*)HWOutputDCMotorBt::load(root);
            }
        }
        elem = elem.nextSiblingElement();
    }

    if(hw == NULL)
        hw =  new HWOutputDCMotor();

    return hw;
}

QDomElement HWOutputDCMotor::save(QDomElement *root, QDomDocument *document)
{
    QDomElement output = HWOutput::save(root, document);

    QDomElement type = document->createElement("type");
    QDomText typeText = document->createTextNode("DCMotor");
    type.appendChild(typeText);

    output.appendChild(type);

    return output;
}

void HWOutputDCMotor::setOverrideMotorState(MotorState state)
{
    if(!m_bOverride)
        return;

    // States have to differ, otherwise every action would be senseless
    if(m_state != state)
    {
        m_state = state;
        this->outputChanged();
    }
}

void HWOutputDCMotor::setOverrideSpeed(unsigned int speed)
{
    if(!m_bOverride)
        return;

    // Speeds have to differ, otherwise every action would be senseless
    if(m_speed != speed)
    {
        m_speed = speed;
        this->outputChanged();
    }
}

void HWOutputDCMotor::setOverrideSpeedAndState(unsigned int speed, MotorState state)
{
    if(!m_bOverride)
        return;

    // if either speed or state has changed, we have to do something
    if(m_speed != speed || m_state != state)
    {
        m_speed = speed;
        m_state = state;
        this->outputChanged();
    }
}

void HWOutputDCMotor::setSpeedAndState(unsigned int speed, MotorState state)
{
    // if override is set, ignore this settings
    if(m_bOverride)
        return;

    // if either speed or state has changed, we have to do something
    if(m_speed != speed || m_state != state)
    {
        m_speed = speed;
        m_state = state;
        this->outputChanged();
    }
}

std::string HWOutputDCMotor::MotorStateToString(MotorState type)
{
    switch(type)
    {
    case Standby:
        return "Standby";
        break;
    case Forward:
        return "Forward";
        break;
    case Reverse:
        return "Reverse";
        break;
    case Brake:
        return "Brake";
        break;
    default:
        LOG_WARN(Logger::Misc, "Received invalid type");
        return "";
    }
}

HWOutputDCMotor::MotorState HWOutputDCMotor::StringToMotorState(std::string str)
{
    const char* cstr = str.c_str();
    if( strcasecmp(cstr, "Standby") == 0)
        return Standby;
    else if( strcasecmp(cstr, "Forward") == 0)
        return Forward;
    else if( strcasecmp(cstr, "Reverse") == 0)
        return Reverse;
    else if( strcasecmp(cstr, "Brake") == 0)
        return Brake;
    else
        return EINVALID;
}
