
#include "hw/HWOutputDCMotorBt.h"
#include "hw/BTThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

HWOutputDCMotorBt::HWOutputDCMotorBt()
{
    m_slaveAddress = -1;
    m_btThread = NULL;
}

HWOutput* HWOutputDCMotorBt::load(QDomElement *root)
{
    HWOutputDCMotorBt* hw = new HWOutputDCMotorBt();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if( elem.tagName().toLower().compare("slaveaddress") == 0 )
        {
            hw->m_slaveAddress = elem.text().toInt();
        }
        else if( elem.tagName().toLower().compare("btboard") == 0 )
        {
            hw->m_btName = elem.text().toStdString();
        }

        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_slaveAddress > 127 || hw->m_slaveAddress < 0 || hw->m_btName.empty())
    {
        LOG_WARN(Logger::BT, "Invalid i2c parameters");
        return NULL;
    }

    return hw;
}

QDomElement HWOutputDCMotorBt::save(QDomElement* root, QDomDocument* document)
{
    QDomElement output = HWOutputDCMotor::save(root, document);

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

    return output;
}

void HWOutputDCMotorBt::setI2CBt(BTThread* btThread)
{
    // allocate our buffer
    BTI2CPacket packet;
    packet.commandLength = 2;
    packet.commandBuffer = (char*) malloc(packet.commandLength);
    packet.commandBuffer[0] = 0x00;

    // now set output state
    packet.commandBuffer[1] = m_state | ((m_speed * 63 / 100) << 2);

    packet.read = 0;
    packet.slaveAddress = m_slaveAddress;
    packet.request = 1;
    packet.error = 0;

    btThread->sendI2CPackets(&packet, 1);
}

void HWOutputDCMotorBt::init(ConfigManager *config)
{
    m_btThread = config->getBTThreadByName(m_btName);

    if(m_btThread == NULL)
    {
        LOG_WARN(Logger::BT, "Bluetooth board does not exist");
    }
}

void HWOutputDCMotorBt::deinit(ConfigManager* config)
{
    m_btThread = NULL;
}

void HWOutputDCMotorBt::outputChanged()
{
    HWOutputDCMotor::outputChanged();

    if(m_btThread != NULL)
        m_btThread->addOutput( std::bind(&HWOutputDCMotorBt::setI2CBt, this, std::placeholders::_1) );
}
