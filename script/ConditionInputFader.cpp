

#include "script/ConditionInputFader.h"
#include "hw/HWInput.h"
#include "hw/HWInputFader.h"

#include "util/Debug.h"

Condition* ConditionInputFader::load(QDomElement* root)
{
    QDomElement elem = root->firstChildElement();

    ConditionInputFader* condition = new ConditionInputFader();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("trigger") == 0)
        {
            condition->setTrigger( StringToTrigger( elem.text().toStdString() ) );
        }
        else if(elem.tagName().toLower().compare("triggervalue") == 0)
        {
            condition->m_triggerValue = elem.text().toInt();
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

QDomElement ConditionInputFader::save(QDomElement* root, QDomDocument* document)
{
    QDomElement condition = ConditionInput::save(root, document);

    QDomElement subtype = document->createElement("subtype");
    QDomText subtypeText = document->createTextNode("Fader");
    subtype.appendChild(subtypeText);

    condition.appendChild(subtype);

    QDomElement trigger = document->createElement("trigger");
    QDomText triggerText = document->createTextNode("");

    triggerText.setData( QString::fromStdString( TriggerToString(m_trigger) ) );
    trigger.appendChild(triggerText);

    condition.appendChild(trigger);

    QDomElement triggerValue = document->createElement("TriggerValue");
    QDomText triggerValueText = document->createTextNode( QString::number(m_triggerValue) );
    triggerValue.appendChild(triggerValueText);

    condition.appendChild(triggerValue);

    return condition;
}

void ConditionInputFader::getRequiredList(std::list<Rule::RequiredInput>* listInput,
                                     std::list<Rule::RequiredOutput>* listOutput,
                                     std::list<Rule::RequiredVariable>* listVariable) const
{
    if(listInput != NULL)
    {
        Rule::RequiredInput req;
        req.name = m_HWName;
        req.type = HWInput::Fader;
        req.exists = false;

        listInput->push_back(req);
    }
}

void ConditionInputFader::onInputChanged(HWInput* hw)
{
    pi_assert(m_hw == hw && m_hw->getType() == HWInput::Fader);

    bool isFulfilled = false;
    HWInputFader* fader = (HWInputFader*) hw;

    if(m_trigger == GreaterThan)
    {
        if( fader->getValue() > m_triggerValue )
            isFulfilled = true;
    }
    else if(m_trigger == LessThan)
    {
        if( fader->getValue() < m_triggerValue )
            isFulfilled = true;
    }
    else if(m_trigger == Equal)
    {
        if( fader->getValue() == m_triggerValue )
            isFulfilled = true;
    }
    else // m_trigger == Changed
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

std::string ConditionInputFader::getDescription() const
{
    std::string str = std::string("If ").append(m_HWName);

    switch(m_trigger)
    {
    case GreaterThan:
        str.append(" is greater than ");
        break;
    case LessThan:
        str.append(" is less than ");
        break;
    case Equal:
        str.append(" is equal to ");
        break;
    case Changed:
        str.append(" has changed");
        break;
    }

    if( m_trigger != Changed)
    {
        str.append( std::to_string(m_triggerValue) );
    }

    return str;
}

std::string ConditionInputFader::TriggerToString(Trigger trigger)
{
    switch(trigger)
    {
    case GreaterThan:
        return "GreaterThan";
        break;

    case LessThan:
        return "LessThan";
        break;

    case Equal:
        return "equal";
        break;
    case Changed:
        return "changed";
        break;
    }

    LOG_WARN(Logger::Script, "Received invalid trigger");
    return "";
}

ConditionInputFader::Trigger ConditionInputFader::StringToTrigger(std::string str)
{
    const char* cstr = str.c_str();

    if( strcasecmp(cstr, "greaterthan") == 0)
        return GreaterThan;
    else if( strcasecmp(cstr, "lessthan") == 0)
        return LessThan;
    else if( strcasecmp(cstr, "equal") == 0)
        return Equal;
    else if( strcasecmp(cstr, "changed") == 0)
        return Changed;
    else
        return EINVALID;
}
