
#include "script/ActionOutputStepperPositioning.h"
#include "hw/HWOutputStepper.h"
#include "util/Debug.h"

ActionOutputStepperPositioning::ActionOutputStepperPositioning()
{
    m_posType = SetPosition;
    m_position = 0;
    m_position2 = 0;
    m_vmin = 0;
    m_vmax = 0;
}

Action* ActionOutputStepperPositioning::load(QDomElement* root)
{
    QDomElement elem = root->firstChildElement();
    ActionOutputStepperPositioning* action = new ActionOutputStepperPositioning();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("position") == 0)
        {
            action->setPosition( elem.text().toShort() );
        }
        else if(elem.tagName().toLower().compare("position2") == 0)
        {
            action->m_position2 = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("vmin") == 0)
        {
            action->m_vmin = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("vmax") == 0)
        {
            action->m_vmax = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("positioningtype") == 0)
        {
            if(elem.text().toLower().compare("setposition") == 0)
                action->setPositioningType(SetPosition);
            else if(elem.text().toLower().compare("setdualposition") == 0)
                action->setPositioningType(SetDualPosition);
            else if(elem.text().toLower().compare("resetposition") == 0)
                action->setPositioningType(ResetPosition);
        }

        elem = elem.nextSiblingElement();
    }

    return action;
}

QDomElement ActionOutputStepperPositioning::save(QDomElement* root, QDomDocument* document)
{
    QDomElement action = ActionOutputStepper::save(root, document);

    QDomElement steppertype = document->createElement("StepperType");
    QDomText steppertypeText = document->createTextNode("Positioning");
    steppertype.appendChild(steppertypeText);

    action.appendChild(steppertype);

    QString strPosType;
    switch(m_posType)
    {
    case SetPosition:
        strPosType = "SetPosition";
        break;
    case SetDualPosition:
        strPosType = "SetDualPosition";
        break;
    case ResetPosition:
        strPosType = "ResetPosition";
        break;
    }

    QDomElement posType = document->createElement("PositioningType");
    QDomText posTypeText = document->createTextNode( strPosType );
    posType.appendChild(posTypeText);

    action.appendChild(posType);

    if(m_posType == SetPosition || m_posType == SetDualPosition)
    {
        QDomElement position = document->createElement("Position");
        QDomText positionText = document->createTextNode( QString::number(m_position) );
        position.appendChild(positionText);

        action.appendChild(position);
    }

    if(m_posType == SetDualPosition)
    {
        QDomElement position2 = document->createElement("Position2");
        QDomText position2Text = document->createTextNode( QString::number(m_position2) );
        position2.appendChild(position2Text);

        action.appendChild(position2);

        QDomElement vmin = document->createElement("Vmin");
        QDomText vminText = document->createTextNode( QString::number(m_vmin) );
        vmin.appendChild(vminText);

        action.appendChild(vmin);

        QDomElement vmax = document->createElement("Vmax");
        QDomText vmaxText = document->createTextNode( QString::number(m_vmax) );
        vmax.appendChild(vmaxText);

        action.appendChild(vmax);
    }

    return action;
}

bool ActionOutputStepperPositioning::execute(unsigned int)
{
    if(m_hw == NULL)
    {
        pi_warn("HW does not exist, cannot execute action");
        return true; // still return true as false would mean that all following actions should not be executed
    }

    HWOutputStepper* hw = (HWOutputStepper*)m_hw;

    switch(m_posType)
    {
    case SetPosition:
        hw->setPosition(m_position);
        break;
    case SetDualPosition:
        hw->setDualPosition(m_position, m_position2, m_vmin, m_vmax);
        break;
    case ResetPosition:
        hw->resetPosition();
        break;
    }

    return true;
}

std::string ActionOutputStepperPositioning::getDescription() const
{
    std::string str;

    switch(m_posType)
    {
    case SetPosition:
        str = std::string("Set ").append(m_HWName);
        str.append(" to target position ");
        str.append( std::to_string(m_position) );
        break;
    case SetDualPosition:
        str = std::string("Set ").append(m_HWName);
        str.append(" to dual position ");
        str.append( std::to_string(m_position) );
        str.append(", ");
        str.append( std::to_string(m_position2) );
        break;
    case ResetPosition:
        str = std::string("Reset position of ").append(m_HWName);
        break;
    }

    return str;
}

void ActionOutputStepperPositioning::setDualPosition(short position1, short position2, unsigned char vmin, unsigned char vmax)
{
    pi_assert(vmin <= 15 && vmax <= 15);

    m_position = position1;
    m_position2 = position2;
    m_vmin = vmin;
    m_vmax = vmax;
}
