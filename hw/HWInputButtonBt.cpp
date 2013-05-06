
#include "hw/HWInputButtonBt.h"
#include "ConfigManager.h"
#include "hw/BTThread.h"
#include "util/Debug.h"

#include <QDomElement>

HWInputButtonBt::HWInputButtonBt()
{
    this->m_slaveAddress = -1;
    this->m_port = -1;
}

HWInput* HWInputButtonBt::load(QDomElement *root)
{
    HWInputButtonBt* hw = new HWInputButtonBt();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if( elem.tagName().toLower().compare("slaveaddress") == 0 )
        {
            hw->m_slaveAddress = elem.text().toInt();
        }
        else if( elem.tagName().toLower().compare("port") == 0 )
        {
            hw->m_port = elem.text().toInt();
        }
        else if( elem.tagName().toLower().compare("btboard") == 0 )
        {
            hw->m_btName = elem.text().toStdString();
        }
        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_slaveAddress > 127 || hw->m_slaveAddress < 0 || hw->m_port > 15 || hw->m_btName.empty())
    {
        LOG_WARN(Logger::BT, "Invalid parameters");
        return NULL;
    }

    return hw;
}

QDomElement HWInputButtonBt::save(QDomElement* root, QDomDocument* document)
{
    QDomElement input = HWInputButton::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("Bt");
    type.appendChild(typeText);

    input.appendChild(type);

    QDomElement btBoard = document->createElement("BtBoard");
    QDomText btBoardText = document->createTextNode( QString::fromStdString( m_btName ) );
    btBoard.appendChild(btBoardText);

    input.appendChild(btBoard);

    QDomElement slaveAddr = document->createElement("SlaveAddress");
    QDomText slaveAddrText = document->createTextNode(QString::number( m_slaveAddress ));
    slaveAddr.appendChild(slaveAddrText);

    input.appendChild(slaveAddr);

    QDomElement channel = document->createElement("Port");
    QDomText channelText = document->createTextNode(QString::number( m_port ));
    channel.appendChild(channelText);

    input.appendChild(channel);

    return input;
}

bool HWInputButtonBt::init(ConfigManager* config)
{
    BTThread* btThread = config->getBTThreadByName(m_btName);

    // if we cannot find the bluetooth board, it does not exist and we should fail
    if(btThread == NULL)
    {
        LOG_WARN(Logger::BT, "Bluetooth board does not exist");

        return false;
    }

    btThread->addInputPCF8575(this, m_slaveAddress, m_port);

    return true;
}

void HWInputButtonBt::deinit(ConfigManager* config)
{
    BTThread* btThread = config->getBTThreadByName(m_btName);

    if(btThread != NULL)
        btThread->removeInputPCF8575(this, m_slaveAddress);
}

void HWInputButtonBt::onInputPolled(bool state)
{
    // if we are on override mode, we do not care about the inputs
    if(this->m_bOverride)
        return;

    // only to something, if the input has changed
    if( m_value != state )
    {
        m_value = state;
        this->inputChanged();
    }
}
