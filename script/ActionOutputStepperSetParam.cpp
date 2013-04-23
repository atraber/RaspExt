
#include "script/ActionOutputStepperSetParam.h"
#include "util/Debug.h"

ActionOutputStepperSetParam::ActionOutputStepperSetParam()
{
}

Action* ActionOutputStepperSetParam::load(QDomElement* root)
{
    QDomElement elem = root->firstChildElement();
    ActionOutputStepperSetParam* action = new ActionOutputStepperSetParam();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("irun") == 0)
        {
            action->m_param.irunSet = true;
            action->m_param.irun = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("ihold") == 0)
        {
            action->m_param.iholdSet = true;
            action->m_param.ihold = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("vmax") == 0)
        {
            action->m_param.vmaxSet = true;
            action->m_param.vmax = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("vmin") == 0)
        {
            action->m_param.vminSet = true;
            action->m_param.vmin = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("accShape") == 0)
        {
            action->m_param.accShapeSet = true;
            action->m_param.accShape = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("stepMode") == 0)
        {
            action->m_param.stepModeSet = true;
            action->m_param.stepMode = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("shaft") == 0)
        {
            action->m_param.shaftSet = true;
            action->m_param.shaft = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("acc") == 0)
        {
            action->m_param.accSet = true;
            action->m_param.acc = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("absolutThreshold") == 0)
        {
            action->m_param.absoluteThresholdSet = true;
            action->m_param.absoluteThreshold = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("deltaThreshold") == 0)
        {
            action->m_param.deltaThresholdSet = true;
            action->m_param.deltaThreshold = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("securePosition") == 0)
        {
            action->m_param.securePositionSet = true;
            action->m_param.securePosition = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("fs2StallEnabled") == 0)
        {
            action->m_param.fs2StallEnabledSet = true;
            action->m_param.fs2StallEnabled = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("minSamples") == 0)
        {
            action->m_param.minSamplesSet = true;
            action->m_param.minSamples = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("dc100StallEnable") == 0)
        {
            action->m_param.dc100StallEnableSet = true;
            action->m_param.dc100StallEnable = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("PWMJitterEnable") == 0)
        {
            action->m_param.PWMJitterEnableSet = true;
            action->m_param.PWMJitterEnable = elem.text().toShort();
        }
        else if(elem.tagName().toLower().compare("PWMfreq") == 0)
        {
            action->m_param.PWMfreqSet = true;
            action->m_param.PWMfreq = elem.text().toShort();
        }

        elem = elem.nextSiblingElement();
    }

    return action;
}

void createXMLTextNode(QDomDocument* document, QDomElement* action, std::string tag, std::string value)
{
    QDomElement element = document->createElement( QString::fromStdString( tag ) );
    QDomText text = document->createTextNode( QString::fromStdString( value ) );
    element.appendChild(text);

    action->appendChild(element);
}

QDomElement ActionOutputStepperSetParam::save(QDomElement* root, QDomDocument* document)
{
    QDomElement action = ActionOutputStepper::save(root, document);

    QDomElement steppertype = document->createElement("StepperType");
    QDomText steppertypeText = document->createTextNode("SetParam");
    steppertype.appendChild(steppertypeText);

    action.appendChild(steppertype);

    // saving every value is a real pain... I hope I do never have to change this
    // we only save a value if it is enabled, this saves us saving the enabled variable too

    if(m_param.irunSet)
    {
        createXMLTextNode(document, &action, "irun", std::to_string(m_param.irun));
    }

    if(m_param.iholdSet)
    {
        createXMLTextNode(document, &action, "ihold", std::to_string(m_param.ihold));
    }

    if(m_param.vmaxSet)
    {
        createXMLTextNode(document, &action, "vmax", std::to_string(m_param.vmax));
    }

    if(m_param.vminSet)
    {
        createXMLTextNode(document, &action, "vmin", std::to_string(m_param.vmin));
    }

    if(m_param.accShapeSet)
    {
        createXMLTextNode(document, &action, "accShape", std::to_string(m_param.accShape));
    }

    if(m_param.stepModeSet)
    {
        createXMLTextNode(document, &action, "stepMode", std::to_string(m_param.stepMode));
    }

    if(m_param.shaftSet)
    {
        createXMLTextNode(document, &action, "shaft", std::to_string(m_param.shaft));
    }

    if(m_param.accSet)
    {
        createXMLTextNode(document, &action, "acc", std::to_string(m_param.acc));
    }

    if(m_param.absoluteThresholdSet)
    {
        createXMLTextNode(document, &action, "absolutThreshold", std::to_string(m_param.absoluteThreshold));
    }

    if(m_param.deltaThresholdSet)
    {
        createXMLTextNode(document, &action, "deltaThreshold", std::to_string(m_param.deltaThreshold));
    }

    if(m_param.securePositionSet)
    {
        createXMLTextNode(document, &action, "securePosition", std::to_string(m_param.securePosition));
    }

    if(m_param.fs2StallEnabledSet)
    {
        createXMLTextNode(document, &action, "fs2StallEnabled", std::to_string(m_param.fs2StallEnabled));
    }

    if(m_param.minSamplesSet)
    {
        createXMLTextNode(document, &action, "minSamples", std::to_string(m_param.minSamples));
    }

    if(m_param.dc100StallEnableSet)
    {
        createXMLTextNode(document, &action, "dc100StallEnable", std::to_string(m_param.dc100StallEnable));
    }

    if(m_param.PWMJitterEnableSet)
    {
        createXMLTextNode(document, &action, "PWMJitterEnable", std::to_string(m_param.PWMJitterEnable));
    }

    if(m_param.PWMfreqSet)
    {
        createXMLTextNode(document, &action, "PWMfreq", std::to_string(m_param.PWMfreq));
    }

    return action;
}

bool ActionOutputStepperSetParam::execute(unsigned int)
{
    if(m_hw == NULL)
    {
        pi_warn("HW does not exist, cannot execute action");
        return true; // still return true as false would mean that all following actions should not be executed
    }

    HWOutputStepper* hw = (HWOutputStepper*)m_hw;

    hw->setParam(m_param);

    return true;
}

std::string ActionOutputStepperSetParam::getDescription() const
{
    std::string str = std::string("Set parameters on ").append(m_HWName);

    return str;
}
