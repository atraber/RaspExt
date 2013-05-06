
#include "hw/Config.h"
#include "hw/HWInput.h"
#include "hw/HWOutput.h"
#include "hw/BTThread.h"
#include "util/Debug.h"

#include <QDomDocument>
#include <QFile>

Config::~Config()
{
    this->clear();
}

/**
 * @brief Config::load loads the configuration given by name
 * @param name
 * @return
 */
bool Config::load(std::string name)
{
    std::string filename = "config/";
    filename.append(name);
    filename.append(".xml");

    QFile file(filename.c_str());
    if(!file.open(QIODevice::ReadOnly))
    {
        LOG_WARN(Logger::Misc, "Could not open file");
        return false;
    }

    QDomDocument document;
    document.setContent(&file);

    QDomElement docElem = document.documentElement();

    // check if this is a valid configuration file
    if(docElem.tagName().toLower().compare("config") != 0)
    {
        LOG_WARN(Logger::Misc, "Invalid configuration file: tag \"config\" is missing");
        return false;
    }

    // now set the filename
    m_name = name;

    // load xml data
    QDomElement elem = docElem.firstChildElement();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("input") == 0)
        {
            HWInput* hw = HWInput::load(&elem);
            if(hw != NULL)
                m_listInput.push_back(hw);
        }
        else if(elem.tagName().toLower().compare("output") == 0)
        {
            HWOutput* hw = HWOutput::load(&elem);
            if(hw != NULL)
                m_listOutput.push_back(hw);
        }
        else if(elem.tagName().toLower().compare("bluetooth") == 0)
        {
            BTThread* bt = BTThread::load(&elem);
            if(bt != NULL)
                m_listBTThread.push_back(bt);
        }

        elem = elem.nextSiblingElement();
    }

    file.close();

    return true;
}

/**
 * @brief Config::save saves the configuration
 * @return
 */
bool Config::save()
{

    std::string filename = "config/";
    filename.append(m_name);
    filename.append(".xml");

    QFile file(filename.c_str());
    if(!file.open(QIODevice::WriteOnly))
    {
        LOG_WARN(Logger::Misc, "Could not open file");
        return false;
    }

    QDomDocument document;

    QDomElement config = document.createElement("config");

    document.appendChild(config);


    for(std::list<HWInput*>::iterator it = m_listInput.begin(); it != m_listInput.end(); it++)
    {
        (*it)->save(&config, &document);
    }

    for(std::list<HWOutput*>::iterator it = m_listOutput.begin(); it != m_listOutput.end(); it++)
    {
        (*it)->save(&config, &document);
    }

    // save BT Parameters
    for(std::list<BTThread*>::iterator it = m_listBTThread.begin(); it != m_listBTThread.end(); it++)
    {
        (*it)->save(&config, &document);
    }


    file.write(document.toByteArray(4));

    file.close();

    return true;
}

/**
 * @brief Config::clear clears the whole list of inputs, outputs, variables and BTThreads
 */
void Config::clear()
{
    for(std::list<HWInput*>::iterator it = m_listInput.begin(); it != m_listInput.end(); it++)
    {
        // delete Pointer, we must clean the list afterwards immediatly!
        delete (*it);
    }
    m_listInput.clear();

    for(std::list<HWOutput*>::iterator it = m_listOutput.begin(); it != m_listOutput.end(); it++)
    {
        // delete Pointer, we must clean the list afterwards immediatly!
        delete (*it);
    }
    m_listOutput.clear();

    for(std::list<BTThread*>::iterator it = m_listBTThread.begin(); it != m_listBTThread.end(); it++)
    {
        // delete Pointer, we must clean the list afterwards immediatly!
        delete (*it);
    }
    m_listBTThread.clear();
}
