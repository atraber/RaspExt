
#include "script/Rule.h"
#include "script/Action.h"
#include "script/Condition.h"
#include "util/Debug.h"

Rule::Rule()
{
    m_type = Normal;
    m_noConcurrent = false;
    m_ruleRunning = false;
}

Rule::~Rule()
{
    for(std::vector<Action*>::iterator it = m_listActions.begin(); it != m_listActions.end(); it++)
    {
        delete (*it);
    }

    for(std::vector<Condition*>::iterator it = m_listConditions.begin(); it != m_listConditions.end(); it++)
    {
        delete (*it);
    }
}

Rule* Rule::load(QDomElement* root)
{
    Rule* rule = new Rule();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("action") == 0)
        {
            Action* action = Action::load(&elem);
            if(action != NULL)
                rule->addAction(action);
        }
        else if(elem.tagName().toLower().compare("condition") == 0)
        {
            Condition* condition = Condition::load(&elem);
            if(condition != NULL)
                rule->addCondition(condition);
        }
        else if(elem.tagName().toLower().compare("name") == 0)
        {
            rule->setName( elem.text().toStdString() );
        }
        else if(elem.tagName().toLower().compare("type") == 0)
        {
            rule->setType( StringToType( elem.text().toStdString() ) );
        }
        else if(elem.tagName().toLower().compare("noconcurrent") == 0)
        {
            rule->setNoConcurrent( elem.text().compare("true", Qt::CaseInsensitive) == 0 );
        }

        elem = elem.nextSiblingElement();
    }

    return rule;
}

void Rule::save(QDomElement* root, QDomDocument* document)
{
    QDomElement rule = document->createElement("rule");

    for(std::vector<Condition*>::iterator it = m_listConditions.begin(); it != m_listConditions.end(); it++)
    {
        (*it)->save(&rule, document);
    }

    for(std::vector<Action*>::iterator it = m_listActions.begin(); it != m_listActions.end(); it++)
    {
        (*it)->save(&rule, document);
    }

    // save name
    QDomElement name = document->createElement("name");
    QDomText nameText = document->createTextNode(QString::fromStdString(this->m_name));
    name.appendChild(nameText);

    rule.appendChild(name);

    // save type
    QDomElement type = document->createElement("type");
    QDomText typeText = document->createTextNode( QString::fromStdString( TypeToString(m_type) ) );
    type.appendChild(typeText);

    rule.appendChild(type);

    // save no concurrent
    QDomElement noconcurrent = document->createElement("NoConcurrent");
    QDomText noconcurrentText = document->createTextNode( m_noConcurrent ? "true" : "false" );
    noconcurrent.appendChild(noconcurrentText);

    rule.appendChild(noconcurrent);

    root->appendChild(rule);
}

void Rule::addCondition(Condition *cond, int index)
{
    cond->setRule(this);

    if(index == -1)
        m_listConditions.push_back(cond);
    else
        m_listConditions.insert(m_listConditions.begin() + index, cond);
}

void Rule::editCondition(Condition *oldCond, Condition *newCond)
{
    newCond->setRule(this);

    for(std::vector<Condition*>::iterator it = m_listConditions.begin(); it != m_listConditions.end(); it++)
    {
        if(*it == oldCond)
        {
            *it = newCond;
            break;
        }
    }
}

void Rule::removeCondition(Condition* cond)
{
    for(std::vector<Condition*>::iterator it = m_listConditions.begin(); it != m_listConditions.end(); it++)
    {
        if(*it == cond)
        {
            m_listConditions.erase(it);
            break;
        }
    }
}

void Rule::addAction(Action *action, int index)
{
    action->setRule(this);

    if(index == -1)
        m_listActions.push_back(action);
    else
        m_listActions.insert(m_listActions.begin() + index, action);
}

void Rule::editAction(Action *oldAction, Action *newAction)
{
    newAction->setRule(this);

    for(std::vector<Action*>::iterator it = m_listActions.begin(); it != m_listActions.end(); it++)
    {
        if(*it == oldAction)
        {
            *it = newAction;
            break;
        }
    }
}

/**
 * @brief Rule::getRequiredList This function is used to get a list of all required inputs, outputs and variables.
 * If a pointer to a list is NULL, this list will be ignored.
 * @param listInput
 * @param listOutput
 * @param listVariable
 */
void Rule::getRequiredList(std::list<RequiredInput>* listInput, std::list<RequiredOutput>* listOutput, std::list<RequiredVariable>* listVariable)
{
    for(std::vector<Condition*>::iterator it = m_listConditions.begin(); it != m_listConditions.end(); it++)
    {
        (*it)->getRequiredList(listInput, listOutput, listVariable);
    }

    for(std::vector<Action*>::iterator it = m_listActions.begin(); it != m_listActions.end(); it++)
    {
        (*it)->getRequiredList(listInput, listOutput, listVariable);
    }
}

void Rule::conditionChanged(Condition *cond)
{
    // only if type is normal, we react on changed conditions directly
    if(m_type != Normal)
        return;

    if(this->conditionsTrue())
    {
        if(m_noConcurrent)
        {
            m_mutexConcurrent.lock();
            if( !m_ruleRunning )
            {
                m_ruleRunning = true;
                m_mutexConcurrent.unlock();
            }
            else
            {
                m_mutexConcurrent.unlock();
                return; // abort execution as the previous execution of this rule has not yet finished
            }
        }

        this->executeActions();
    }
}

bool Rule::conditionsTrue()
{
    bool ret = true;
    for(std::vector<Condition*>::iterator it = m_listConditions.begin(); it != m_listConditions.end(); it++)
    {
        // maybe this can be optimized?
        ret = ret && (*it)->isFulfilled();
    }

    return ret;
}

void Rule::init(ConfigManager *config)
{
    // Actions and therefore Outputs have to be initialized before the conditions, because the inputs can fire as soon as they are initialized
    for(std::vector<Action*>::iterator it = m_listActions.begin(); it != m_listActions.end(); it++)
    {
        (*it)->init(config);
    }

    for(std::vector<Condition*>::iterator it = m_listConditions.begin(); it != m_listConditions.end(); it++)
    {
        (*it)->init(config);
    }
}

void Rule::deinit()
{
    // Conditions should be deinitialized before Actions, as they are the conditions for the actions (actually obvious)
    for(std::vector<Condition*>::iterator it = m_listConditions.begin(); it != m_listConditions.end(); it++)
    {
        (*it)->deinit();
    }

    for(std::vector<Action*>::iterator it = m_listActions.begin(); it != m_listActions.end(); it++)
    {
        (*it)->deinit();
    }
}

void Rule::executeActions(unsigned int start)
{
    pi_assert(start <= m_listActions.size());

    // start executing at start-element
    for(; start < m_listActions.size(); start++)
    {
        if( !(m_listActions.at(start)->execute(start) ) )
            break; // Action said we should stop executing other actions
    }

    // as soon as every action in this rule was executed once,
    // the rule has stopped running and a new rule can start (for non-concurrent rules)
    if(m_noConcurrent && start == m_listActions.size())
    {
        m_mutexConcurrent.lock();
        m_ruleRunning = false;
        m_mutexConcurrent.unlock();
    }
}

/**     call is used by ActionCallRule to call another rule.
 *      The conditions in this rule must be true, otherwise it is not going to be executed
 */
void Rule::call()
{
    pi_assert(m_type == MustBeCalled);

    if(this->conditionsTrue())
        this->executeActions();
}

Rule::Type Rule::StringToType(std::string str)
{
    const char* cstr = str.c_str();
    if( strcasecmp(cstr, "normal") == 0)
        return Normal;
    else if( strcasecmp(cstr, "mustbecalled") == 0)
        return MustBeCalled;
    else
        return EINVALID;
}

std::string Rule::TypeToString(Type type)
{
    switch(type)
    {
    case Normal:
        return "Normal";
        break;
    case MustBeCalled:
        return "MustBeCalled";
        break;
    default:
        pi_warn("Invalid type");
        return "";
        break;
    }
}
