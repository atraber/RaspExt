
#include "script/ConditionInputButton.h"
#include "hw/HWInput.h"
#include "hw/HWInputButton.h"

#include "util/Debug.h"

Condition* ConditionInputButton::load(QDomElement* root)
{
    QDomElement elem = root->firstChildElement();

    ConditionInputButton* condition = new ConditionInputButton();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("trigger") == 0)
        {
            condition->setTrigger( StringToTrigger( elem.text().toStdString() ) );
        }
        else if(elem.tagName().toLower().compare("name") == 0)
            condition->setHWName(elem.text().toStdString());

        elem = elem.nextSiblingElement();
    }

    if(condition->getHWName().empty())
    {
        delete condition;
        return NULL;
    }

    return condition;
}

QDomElement ConditionInputButton::save(QDomElement* root, QDomDocument* document)
{
    QDomElement condition = ConditionInput::save(root, document);

    QDomElement subtype = document->createElement("subtype");
    QDomText subtypeText = document->createTextNode("Button");
    subtype.appendChild(subtypeText);

    condition.appendChild(subtype);

    QDomElement state = document->createElement("trigger");
    QDomText stateText = document->createTextNode( QString::fromStdString( TriggerToString(m_trigger) ) );

    state.appendChild(stateText);

    condition.appendChild(state);

    return condition;
}

void ConditionInputButton::getRequiredList(std::list<Rule::RequiredInput>* listInput,
                                     std::list<Rule::RequiredOutput>* listOutput,
                                     std::list<Rule::RequiredVariable>* listVariable) const
{
    if(listInput != NULL)
    {
        Rule::RequiredInput req;
        req.name = m_HWName;
        req.type = HWInput::Button;
        req.exists = false;

        listInput->push_back(req);
    }
}

void ConditionInputButton::onInputChanged(HWInput* hw)
{
    pi_assert(m_hw == hw && m_hw->getType() == HWInput::Button);

    bool isFulfilled = false;

    if(m_trigger == Pressed)
    {
        if(((HWInputButton*)hw)->getValue() == true)
            isFulfilled = true;
    }
    else if(m_trigger == Released)
    {
        if(((HWInputButton*)hw)->getValue() == false)
            isFulfilled = true;
    }
    else // m_trigger == Toggle
    {
        // we have to trick here, so that we do not have to change the part below
        // todo: find a better way to do this
        m_isFulfilled = false;
        isFulfilled = true; // we cannot compare with earlier values, so we have to believe that something has indeed changed
    }

    if(m_isFulfilled != isFulfilled)
    {
        m_isFulfilled = isFulfilled;

        // only call conditionChanged if there is the possibility that all conditons are true
        // if this conditon is not true there is no point in telling anyone
        // if the condition did not change, we MUST NOT tell anyone, because otherwise we could start a rule multiple times
        if(isFulfilled)
            this->conditionChanged();
    }
}

std::string ConditionInputButton::getDescription() const
{
    std::string str = std::string("If ").append(m_HWName).append(" is ");
    str.append( TriggerToString(this->m_trigger) );

    return str;
}

std::string ConditionInputButton::TriggerToString(Trigger trigger)
{
    switch(trigger)
    {
    case Pressed:
        return "Pressed";
        break;

    case Released:
        return "Released";
        break;

    case Changed:
        return "Changed";
        break;
    }

    LOG_WARN(Logger::Script, "Received invalid trigger");
    return "";
}

ConditionInputButton::Trigger ConditionInputButton::StringToTrigger(std::string str)
{
    const char* cstr = str.c_str();

    if( strcasecmp(cstr, "pressed") == 0)
        return Pressed;
    else if( strcasecmp(cstr, "released") == 0)
        return Released;
    else if( strcasecmp(cstr, "changed") == 0)
        return Changed;
    else
        return EINVALID;
}
