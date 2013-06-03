
#include "script/Script.h"
#include "util/Debug.h"
#include "ConfigManager.h"

#include <QDomDocument>
#include <QFile>

Script::Script()
{
    m_config = NULL;
}

Script::~Script()
{
    for(std::vector<Rule*>::iterator ruleIt = m_listRules.begin(); ruleIt != m_listRules.end(); ruleIt++)
    {
        delete (*ruleIt);
    }

    this->clearVariables();
}

Script* Script::load(std::string name)
{
    std::string filename = "scripts/";
    filename.append(name);
    filename.append(".xml");

    QFile file(filename.c_str());
    if(!file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(Logger::Script, "Could not open file %s", filename.c_str());
        return NULL;
    }

    QDomDocument document;
    document.setContent(&file);

    QDomElement docElem = document.documentElement();

    // check if this is a valid script file
    if(docElem.tagName().toLower().compare("script") != 0)
    {
        LOG_ERROR(Logger::Script, "Invalid script file %s: tag \"script\" is missing", filename.c_str());
        return NULL;
    }

    Script* script = new Script();

    script->m_name = name;

    QDomElement elem = docElem.firstChildElement();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("rule") == 0)
        {
            Rule* rule = Rule::load(&elem);
            if(rule != NULL)
            {
                script->addRule(rule);
            }
        }
        else if(elem.tagName().toLower().compare("variable") == 0)
        {
            Variable* variable = Variable::load(&elem);
            if(variable != NULL)
            {
                script->addVariable(variable);
            }
        }
        else if(elem.tagName().toLower().compare("description") == 0)
        {
            script->m_desc = elem.text().toStdString();
        }

        elem = elem.nextSiblingElement();
    }

    file.close();

    return script;
}

bool Script::save()
{
    std::string filename = "scripts/";
    filename.append(m_name);
    filename.append(".xml");

    QFile file(filename.c_str());
    if(!file.open(QIODevice::WriteOnly))
    {
        LOG_WARN(Logger::Script, "Could not open file");
        return false;
    }

    QDomDocument document;

    QDomElement script = document.createElement("script");

    document.appendChild(script);


    // save every rule
    for(std::vector<Rule*>::iterator ruleIt = m_listRules.begin(); ruleIt != m_listRules.end(); ruleIt++)
    {
        (*ruleIt)->save(&script, &document);
    }

    // save every variable
    for(std::list<Variable*>::iterator it = m_listVars.begin(); it != m_listVars.end(); it++)
    {
        (*it)->save(&script, &document);
    }

    // save description
    QDomElement desc = document.createElement("description");
    QDomText descText = document.createTextNode(QString::fromStdString(this->m_desc));
    desc.appendChild(descText);

    script.appendChild(desc);


    file.write(document.toByteArray(4));

    file.close();

    return true;
}

int RequiredInputCmp(const Rule::RequiredInput& lhs, const Rule::RequiredInput& rhs)
{
    return lhs.name.compare(rhs.name);
}

bool RequiredInputEq(const Rule::RequiredInput& lhs, const Rule::RequiredInput& rhs)
{
    return lhs.name.compare(rhs.name) == 0 && lhs.type == rhs.type;
}

int RequiredOutputCmp(const Rule::RequiredOutput& lhs, const Rule::RequiredOutput& rhs)
{
    return lhs.name.compare(rhs.name);
}

bool RequiredOutputEq(const Rule::RequiredOutput& lhs, const Rule::RequiredOutput& rhs)
{
    return lhs.name.compare(rhs.name) == 0 && lhs.type == rhs.type;
}

int RequiredVariableCmp(const Rule::RequiredVariable& lhs, const Rule::RequiredVariable& rhs)
{
    return lhs.name.compare(rhs.name);
}

bool RequiredVariableEq(const Rule::RequiredVariable& lhs, const Rule::RequiredVariable& rhs)
{
    return lhs.name.compare(rhs.name) == 0;
}

/**
 * @brief Script::getRequiredList This method fills the given lists with all inputs, outputs and variables which are required by this script.
 * This method also removes duplicates which are introduced as an input/output/variable can be used several times.
 * For variables it checks, if they exist and sets the correspoding field to true.
 * @param listInput
 * @param listOutput
 * @param listVariable
 */
void Script::getRequiredList(std::list<Rule::RequiredInput>* listInput,
                             std::list<Rule::RequiredOutput>* listOutput,
                             std::list<Rule::RequiredVariable>* listVariable)
{
    for(std::vector<Rule*>::iterator ruleIt = m_listRules.begin(); ruleIt != m_listRules.end(); ruleIt++)
    {
        (*ruleIt)->getRequiredList(listInput, listOutput, listVariable);
    }

    // we have the lists, now we have to remove all duplicates in the list
    if(listInput != NULL)
    {
        listInput->sort( RequiredInputCmp );
        listInput->unique( RequiredInputEq );
    }

    if(listOutput != NULL)
    {
        listOutput->sort( RequiredOutputCmp );
        listOutput->unique( RequiredOutputEq );
    }

    if(listVariable != NULL)
    {
        listVariable->sort( RequiredVariableCmp );
        listVariable->unique( RequiredVariableEq );

        // Check if variables exist and set exist to true if they do
        for(std::list<Rule::RequiredVariable>::iterator it = listVariable->begin(); it != listVariable->end(); it++)
        {
            for(std::list<Variable*>::iterator varIt = m_listVars.begin(); varIt != m_listVars.end(); varIt++)
            {
                if( (*it).name.compare( (*varIt)->getName() ) == 0 )
                {
                    it->exists = true;
                    break;
                }
            }
        }
    }
}

void Script::addRule(Rule* rule)
{
    m_listRules.push_back(rule);
}

void Script::addVariable(Variable *var)
{
    m_listVars.push_back(var);
}

/**
 * @brief Script::clearVariables This method deletes all variables in this script
 */
void Script::clearVariables()
{
    for(std::list<Variable*>::iterator varIt = m_listVars.begin(); varIt != m_listVars.end(); varIt++)
    {
        delete (*varIt);
    }

    m_listVars.clear();
}

bool cmpCallable(const Rule* lhs, const Rule* rhs)
{
    if( lhs->getType() == Rule::MustBeCalled  && rhs->getType() != Rule::MustBeCalled)
        return true;
    else
        return false;
}

void Script::init(ConfigManager *config)
{
    // sort rules such that callable rules are in the list BEFORE the not callable rules,
    // this ensures that if a not callable rule calls a callable rule
    // the conditions of the callable rule are already true
    std::sort(m_listRules.begin(), m_listRules.end(), cmpCallable);

    // Actions and therefore Outputs have to be initialized before the conditions, because the inputs can fire as soon as they are initialized
    for(std::vector<Rule*>::iterator ruleIt = m_listRules.begin(); ruleIt != m_listRules.end(); ruleIt++)
    {
        (*ruleIt)->initActions(config);
    }

    for(std::vector<Rule*>::iterator ruleIt = m_listRules.begin(); ruleIt != m_listRules.end(); ruleIt++)
    {
        (*ruleIt)->initConditions(config);
    }
}

void Script::deinit()
{
    for(std::vector<Rule*>::iterator ruleIt = m_listRules.begin(); ruleIt != m_listRules.end(); ruleIt++)
    {
        (*ruleIt)->deinit();
    }
}

void Script::setConfig(ConfigManager *config)
{
    m_config = config;
}


std::list<Variable*> Script::getVariableList() const
{
    return m_listVars;
}

std::list<HWInput*> Script::getInputList() const
{
    if(m_config != NULL)
        return m_config->getInputList();
    else
    {
        LOG_WARN(Logger::Script, "Config not set, returning empty list");
        return std::list<HWInput*>();
    }
}

std::list<HWOutput*> Script::getOutputList() const
{
    if(m_config != NULL)
        return m_config->getOutputList();
    else
    {
        LOG_WARN(Logger::Script, "Config not set, returning empty list");
        return std::list<HWOutput*>();
    }
}

std::vector<Rule*> Script::getRuleList() const
{
    return m_listRules;
}

Rule* Script::getRuleByName(std::string name) const
{
    const char* cstr = name.c_str();
    for(unsigned int i = 0; i < m_listRules.size(); i++)
    {
        if(strcasecmp( m_listRules[i]->getName().c_str(), cstr) == 0)
            return m_listRules[i];
    }

    return NULL;
}
