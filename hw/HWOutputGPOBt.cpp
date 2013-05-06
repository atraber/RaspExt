
#include "hw/HWOutputGPOBt.h"
#include "hw/BTThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

HWOutputGPOBt::HWOutputGPOBt()
{
    m_slaveAddress = -1;
    m_port = -1;
}

HWOutput* HWOutputGPOBt::load(QDomElement *root)
{
    HWOutputGPOBt* hw = new HWOutputGPOBt();
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
        LOG_WARN(Logger::BT, "Invalid i2c parameters");
        return NULL;
    }

    return hw;
}

QDomElement HWOutputGPOBt::save(QDomElement* root, QDomDocument* document)
{
    QDomElement output = HWOutputGPO::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("Bt");
    type.appendChild(typeText);

    output.appendChild(type);

    QDomElement btBoard = document->createElement("BtBoard");
    QDomText btBoardText = document->createTextNode( QString::fromStdString( m_btName ) );
    btBoard.appendChild(btBoardText);

    output.appendChild(btBoard);

    QDomElement slaveAddr = document->createElement("SlaveAddress");
    QDomText slaveAddrText = document->createTextNode(QString::number( m_slaveAddress ));
    slaveAddr.appendChild(slaveAddrText);

    output.appendChild(slaveAddr);

    QDomElement port = document->createElement("Port");
    QDomText portText = document->createTextNode(QString::number( m_port ));
    port.appendChild(portText);

    output.appendChild(port);

    return output;
}

void HWOutputGPOBt::init(ConfigManager *config)
{
    BTThread* btThread = config->getBTThreadByName(m_btName);

    // if we cannot find the bluetooth board, it does not exist and we should fail
    if(btThread == NULL)
    {
        LOG_WARN(Logger::BT, "Bluetooth board does not exist");

        return;
    }

    btThread->addOutputPCF8575(this, m_slaveAddress, m_port);

}

void HWOutputGPOBt::deinit(ConfigManager* config)
{
    BTThread* btThread = config->getBTThreadByName(m_btName);

    if(btThread != NULL)
        btThread->removeOutputPCF8575(this, m_slaveAddress);
}
