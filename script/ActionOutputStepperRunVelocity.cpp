
#include "script/ActionOutputStepperRunVelocity.h"
#include "hw/HWOutputStepper.h"
#include "util/Debug.h"

Action* ActionOutputStepperRunVelocity::load(QDomElement* root)
{
    return new ActionOutputStepperRunVelocity();
}

QDomElement ActionOutputStepperRunVelocity::save(QDomElement* root, QDomDocument* document)
{
    QDomElement action = ActionOutputStepper::save(root, document);

    QDomElement steppertype = document->createElement("StepperType");
    QDomText steppertypeText = document->createTextNode("RunVelocity");
    steppertype.appendChild(steppertypeText);

    action.appendChild(steppertype);

    return action;
}

bool ActionOutputStepperRunVelocity::execute(unsigned int)
{
    if(m_hw == NULL)
    {
        LOG_WARN(Logger::Script, "HW does not exist, cannot execute action");
        return true; // still return true as false would mean that all following actions should not be executed
    }

    ((HWOutputStepper*)m_hw)->runVelocity();

    return true;
}

std::string ActionOutputStepperRunVelocity::getDescription() const
{
    std::string str = std::string("Send command RunVelocity to ").append(m_HWName);

    return str;
}
