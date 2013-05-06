
#include "hw/BTThread.h"
#include "util/Config.h"
#include "util/Debug.h"

#include <QDomDocument>

#include "hw/BTClassicThread.h"
#include "hw/BLEThread.h"

BTThread::BTThread()
{
    m_bStop = false;
    m_thread = 0;
    m_btaddr = "11:22:33:44:55:66";
}

BTThread::~BTThread()
{
}

/**
 * @brief BTThread::load loads the necessary information from an XML file.
 * The param root must be a XML node under which the information for this thread are stored.
 * If this method does not find all necessary information and thus cannot be loaded it returns NULL
 * @param root
 * @return returns NULL if an error occurred or an instance of BTClassicThread
 */
BTThread* BTThread::load(QDomElement* root)
{
    std::string name;
    std::string btaddr;
    bool is_lowEnergy = false;
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("name") == 0)
        {
            name = elem.text().toStdString();
        }
        else if(elem.tagName().toLower().compare("btaddress") == 0)
        {
            btaddr = elem.text().toStdString();
        }
        else if(elem.tagName().toLower().compare("lowenergy") == 0)
        {
            is_lowEnergy = true;
        }

        elem = elem.nextSiblingElement();
    }

    BTThread* btthread;
    if(is_lowEnergy)
        btthread = BLEThread::load(root);
    else
        btthread = BTClassicThread::load(root);

    btthread->setName(name);
    btthread->setBTAddr(btaddr);

    if(btthread->getName().empty() || btthread->getBTAddr().empty())
    {
        LOG_WARN(Logger::BT, "Could not load bluetooth, name and/or address for this module were empty");

        delete btthread;
        return NULL;
    }

    return btthread;
}

/**
 * @brief BTClassicThread::save saves this instance of BTClassicThread to an XML file.
 * The param root must contain an XML node under wich the informations should be stored.
 * @param root
 * @param document
 * @return
 */
QDomElement BTThread::save(QDomElement* root, QDomDocument* document)
{
    QDomElement output = document->createElement("bluetooth");

    QDomElement name = document->createElement("name");
    QDomText nameText = document->createTextNode( QString::fromStdString( this->getName() ) );
    name.appendChild(nameText);

    output.appendChild(name);

    QDomElement btaddr = document->createElement("BTAddress");
    QDomText btaddrText = document->createTextNode( QString::fromStdString( this->getBTAddr() ) );
    btaddr.appendChild(btaddrText);

    output.appendChild(btaddr);

    root->appendChild(output);

    return output;
}

/**
* @brief BTThread::setBTAddr sets the bluetooth address of this BTThread.
* The format of addr must be 11:22:33:44:55:66, otherwise it is rejected
* @param addr
*/
void BTThread::setBTAddr(std::string addr)
{
    // convert to upper case
    std::transform(addr.begin(), addr.end(),addr.begin(), ::toupper);

    // check if the address is valid
    bool valid = true;
    if(addr.size() != 17)
        valid = false;

    for(unsigned int i = 0; i < addr.size(); i++)
    {
        if( (i + 1) % 3 == 0 )
        {
            if(addr.at(i) != ':')
                valid = false;
        }
        else
        {
            if(addr.at(i) > 'F' && addr.at(i) < '0')
                valid = false;
        }
    }

    if(!valid)
    {
        LOG_WARN(Logger::BT, "Bluetooth address is not valid");
        return;
    }

    m_btaddr = addr;
}
