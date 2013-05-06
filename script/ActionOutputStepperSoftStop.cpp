
#include "script/ActionOutputStepperSoftStop.h"
#include "hw/HWOutputStepper.h"
#include "util/Debug.h"

Action* ActionOutputStepperSoftStop::load(QDomElement* root)
{
    return new ActionOutputStepperSoftStop();
}

QDomElement ActionOutputStepperSoftStop::save(QDomElement* root, QDomDocument* document)
{
    QDomElement action = ActionOutputStepper::save(root, document);

    QDomElement steppertype = document->createElement("StepperType");
    QDomText steppertypeText = document->createTextNode("SoftStop");
    steppertype.appendChild(steppertypeText);

    action.appendChild(steppertype);

    return action;
}

bool ActionOutputStepperSoftStop::execute(unsigned int)
{
    if(m_hw == NULL)
    {
        LOG_WARN(Logger::Script, "HW does not exist, cannot execute action");
        return true; // still return true as false would mean that all following actions should not be executed
    }

    ((HWOutputStepper*)m_hw)->softStop();

    return true;
}

std::string ActionOutputStepperSoftStop::getDescription() const
{
    std::string str = std::string("Send command soft stop to ").append(m_HWName);

    return str;
}
