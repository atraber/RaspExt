
#include "script/ActionVariable.h"
#include "ConfigManager.h"

#include "util/Debug.h"

Action* ActionVariable::load(QDomElement* root)
{
    ActionVariable* action = new ActionVariable();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("name") == 0)
        {
            action->setVarName( elem.text().toStdString() );
        }
        else if(elem.tagName().toLower().compare("operand") == 0)
        {
            action->setOperand( elem.text().toInt() );
        }
        else if(elem.tagName().toLower().compare("operator") == 0)
        {
            Operator op = StringToOperator(elem.text().toStdString() );
            action->setOperator( op );
        }

        elem = elem.nextSiblingElement();
    }

    if(action->getVarName().empty())
    {
        delete action;
        return NULL;
    }

    return action;
}

QDomElement ActionVariable::save(QDomElement* root, QDomDocument* document)
{
    QDomElement action = Action::save(root, document);

    QDomElement type = document->createElement("type");
    QDomText typeText = document->createTextNode("variable");
    type.appendChild(typeText);

    action.appendChild(type);

    QDomElement name = document->createElement("name");
    QDomText nameText = document->createTextNode( QString::fromStdString( m_varName ) );
    name.appendChild(nameText);

    action.appendChild(name);

    QDomElement operand = document->createElement("operand");
    QDomText operandText = document->createTextNode( QString::number( m_operand ) );
    operand.appendChild(operandText);

    action.appendChild(operand);

    QDomElement op = document->createElement("operator");
    QDomText opText = document->createTextNode( QString::fromStdString( this->OperatorToString( m_operator ) ) );
    op.appendChild(opText);

    action.appendChild(op);

    return action;
}

void ActionVariable::getRequiredList(std::list<Rule::RequiredInput>* listInput,
                                     std::list<Rule::RequiredOutput>* listOutput,
                                     std::list<Rule::RequiredVariable>* listVariable) const
{
    if(listVariable != NULL)
    {
        Rule::RequiredVariable req;
        req.name = m_varName;
        req.exists = false;

        listVariable->push_back(req);
    }
}

void ActionVariable::init(ConfigManager *config)
{
    m_var = config->getVariableByName(m_varName);
}

void ActionVariable::deinit()
{
    m_var = NULL;
}

std::string ActionVariable::getDescription() const
{
    std::string str = "Set ";
    str.append( m_varName );
    switch(m_operator)
    {
    case Equal:
        str.append(" = ");
        break;
    case Plus:
        str.append(" += ");
        break;
    case Minus:
        str.append(" -= ");
        break;
    }

    str.append( std::to_string( m_operand ) );

    return str;
}

bool ActionVariable::execute(unsigned int)
{
    if( m_var == NULL)
        return true;

    switch(m_operator)
    {
    case Equal:
        m_var->setValue( m_operand );
        break;
    case Plus:
        m_var->setValue( m_var->getValue() + m_operand );
        break;
    case Minus:
        m_var->setValue( m_var->getValue() - m_operand );
        break;
    }

    return true;
}


std::string ActionVariable::OperatorToString(Operator op)
{
    switch(op)
    {
    case Equal:
        return "Equal";
    case Plus:
        return "Plus";
    case Minus:
        return "Minus";
    default:
        LOG_WARN(Logger::Script, "Received invalid operator");
        return "";
    }
}

ActionVariable::Operator ActionVariable::StringToOperator(std::string str)
{
    const char* cstr = str.c_str();

    if( strcasecmp(cstr, "equal") == 0)
        return Equal;
    else if( strcasecmp(cstr, "plus") == 0)
        return Plus;
    else if( strcasecmp(cstr, "minus") == 0)
        return Minus;
    else
        return EINVALID;
}
