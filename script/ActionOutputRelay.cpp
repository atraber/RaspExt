
#include "script/ActionOutputRelay.h"
#include "hw/HWOutputRelay.h"
#include "util/Debug.h"

ActionOutputRelay::ActionOutputRelay()
{
    m_state = Off;
}

Action* ActionOutputRelay::load(QDomElement* root)
{
    QDomElement elem = root->firstChildElement();

    ActionOutputRelay* action = new ActionOutputRelay();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("state") == 0)
        {
            if(elem.text().toLower().compare("on") == 0)
                action->setState(On);
            else if(elem.text().toLower().compare("off") == 0)
                action->setState(Off);
            else if(elem.text().toLower().compare("toggle") == 0)
                action->setState(Toggle);
        }

        elem = elem.nextSiblingElement();
    }

    return action;
}

QDomElement ActionOutputRelay::save(QDomElement* root, QDomDocument* document)
{
    QDomElement action = ActionOutput::save(root, document);

    QDomElement subtype = document->createElement("subtype");
    QDomText subtypeText = document->createTextNode("Relay");
    subtype.appendChild(subtypeText);

    action.appendChild(subtype);

    QDomElement state = document->createElement("state");
    QDomText stateText = document->createTextNode("");

    switch(m_state)
    {
    case Off:
        stateText.setData("Off");
        break;
    case On:
        stateText.setData("On");
        break;
    case Toggle:
        stateText.setData("Toggle");
        break;
    }

    state.appendChild(stateText);

    action.appendChild(state);

    return action;
}


void ActionOutputRelay::getRequiredList(std::list<Rule::RequiredInput>* listInput,
                                     std::list<Rule::RequiredOutput>* listOutput,
                                     std::list<Rule::RequiredVariable>* listVariable) const
{
    if(listOutput != NULL)
    {
        Rule::RequiredOutput req;
        req.name = m_HWName;
        req.type = HWOutput::Relay;
        req.exists = false;

        listOutput->push_back(req);
    }
}


bool ActionOutputRelay::execute(unsigned int)
{
    if(m_hw == NULL)
    {
        LOG_WARN(Logger::Script, "HW does not exist, cannot execute action");
        return true; // still return true as false would mean that all following actions should not be executed
    }

    bool outputState = false;
    switch(m_state)
    {
    case On:
        outputState = true;
        break;
    case Off:
        outputState = false;
        break;
    case Toggle:
        outputState = !((HWOutputRelay*)m_hw)->getValue();
        break;
    }
    ((HWOutputRelay*)m_hw)->setValue(outputState);

    return true;
}

std::string ActionOutputRelay::getDescription() const
{
    std::string str = std::string("Set ").append(m_HWName).append(" to ");
    switch(m_state)
    {
    case On:
        str.append("On");
        break;
    case Off:
        str.append("Off");
        break;
    case Toggle:
        str.append("Toggle");
        break;
    }

    return str;
}
