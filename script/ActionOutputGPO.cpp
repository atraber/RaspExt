
#include "script/ActionOutputGPO.h"
#include "hw/HWOutputGPO.h"
#include "util/Debug.h"

ActionOutputGPO::ActionOutputGPO()
{
    m_state = Low;
}

Action* ActionOutputGPO::load(QDomElement* root)
{
    QDomElement elem = root->firstChildElement();

    ActionOutputGPO* action = new ActionOutputGPO();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("state") == 0)
        {
            if(elem.text().toLower().compare("high") == 0)
                action->setState(High);
            else if(elem.text().toLower().compare("low") == 0)
                action->setState(Low);
            else if(elem.text().toLower().compare("toggle") == 0)
                action->setState(Toggle);
        }

        elem = elem.nextSiblingElement();
    }

    return action;
}

QDomElement ActionOutputGPO::save(QDomElement* root, QDomDocument* document)
{
    QDomElement action = ActionOutput::save(root, document);

    QDomElement subtype = document->createElement("subtype");
    QDomText subtypeText = document->createTextNode("GPO");
    subtype.appendChild(subtypeText);

    action.appendChild(subtype);

    QDomElement state = document->createElement("state");
    QDomText stateText = document->createTextNode("");

    switch(m_state)
    {
    case Low:
        stateText.setData("Low");
        break;
    case High:
        stateText.setData("High");
        break;
    case Toggle:
        stateText.setData("Toggle");
        break;
    }

    state.appendChild(stateText);

    action.appendChild(state);

    return action;
}


void ActionOutputGPO::getRequiredList(std::list<Rule::RequiredInput>* listInput,
                                     std::list<Rule::RequiredOutput>* listOutput,
                                     std::list<Rule::RequiredVariable>* listVariable) const
{
    if(listOutput != NULL)
    {
        Rule::RequiredOutput req;
        req.name = m_HWName;
        req.type = HWOutput::GPO;
        req.exists = false;

        listOutput->push_back(req);
    }
}


bool ActionOutputGPO::execute(unsigned int)
{
    if(m_hw == NULL)
    {
        LOG_WARN(Logger::Script, "HW does not exist, cannot execute action");
        return true; // still return true as false would mean that all following actions should not be executed
    }

    bool outputState = false;
    switch(m_state)
    {
    case High:
        outputState = true;
        break;
    case Low:
        outputState = false;
        break;
    case Toggle:
        outputState = !((HWOutputGPO*)m_hw)->getValue();
        break;
    }
    ((HWOutputGPO*)m_hw)->setValue(outputState);

    return true;
}

std::string ActionOutputGPO::getDescription() const
{
    std::string str = std::string("Set ").append(m_HWName).append(" to ");
    switch(m_state)
    {
    case High:
        str.append("High");
        break;
    case Low:
        str.append("Low");
        break;
    case Toggle:
        str.append("Toggle");
        break;
    }

    return str;
}
