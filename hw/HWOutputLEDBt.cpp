
#include "hw/HWOutputLEDBt.h"
#include "hw/BTThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

HWOutputLEDBt::HWOutputLEDBt()
{
    m_slaveAddress = -1;
    m_channel = -1;
    m_btThread = NULL;
}

HWOutput* HWOutputLEDBt::load(QDomElement *root)
{
    HWOutputLEDBt* hw = new HWOutputLEDBt();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if( elem.tagName().toLower().compare("slaveaddress") == 0 )
        {
            hw->m_slaveAddress = elem.text().toInt();
        }
        else if( elem.tagName().toLower().compare("channel") == 0 )
        {
            hw->m_channel = elem.text().toInt();
        }
        else if( elem.tagName().toLower().compare("btboard") == 0 )
        {
            hw->m_btName = elem.text().toStdString();
        }

        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_slaveAddress > 127 || hw->m_slaveAddress < 0 || hw->m_channel > 15 || hw->m_btName.empty())
    {
        LOG_WARN(Logger::BT, "Invalid parameters");
        return NULL;
    }

    return hw;
}

QDomElement HWOutputLEDBt::save(QDomElement* root, QDomDocument* document)
{
    QDomElement output = HWOutputLED::save(root, document);

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

    QDomElement channel = document->createElement("Channel");
    QDomText channelText = document->createTextNode(QString::number( m_channel ));
    channel.appendChild(channelText);

    output.appendChild(channel);

    return output;
}

void HWOutputLEDBt::setI2CBt(BTThread* btThread)
{
    // allocate our buffer
    BTI2CPacket packet;
    packet.commandLength = 2;
    packet.commandBuffer = (char*) malloc(packet.commandLength);

    // first select register for PWM brightness control
    packet.commandBuffer[0] = 0x02 + m_channel;

    // now set brightness
    packet.commandBuffer[1] = m_value * 255 / 100;

    packet.read = 0;
    packet.slaveAddress = m_slaveAddress;
    packet.request = 1;
    packet.error = 0;

    btThread->sendI2CPackets(&packet, 1);
}

void HWOutputLEDBt::setupI2CBt(BTThread* btThread)
{
    // allocate our buffer
    BTI2CPacket packets[2];

    // activate oscillator and deactivate all call address
    packets[0].read = 0;
    packets[0].slaveAddress = m_slaveAddress;
    packets[0].request = 1;
    packets[0].error = 0;

    packets[0].commandLength = 2;
    packets[0].commandBuffer = (char*) malloc(packets[0].commandLength);
    packets[0].commandBuffer[0] = 0x00;
    packets[0].commandBuffer[1] = 0x00;

    // set LED output state to control individual brightness
    packets[1].read = 0;
    packets[1].slaveAddress = m_slaveAddress;
    packets[1].request = 1;
    packets[1].error = 0;

    packets[1].commandLength = 2;
    packets[1].commandBuffer = (char*) malloc(packets[1].commandLength);

    // first select register to set output state of LED
    packets[1].commandBuffer[0] = 0x14 + m_channel / 4;
    // now set state to "LED driver x is individual brightness can be controlled through its PWMx register"
    // see page 17 of datasheet
    packets[1].commandBuffer[1] = 0xAA;


    // now send it
    btThread->sendI2CPackets(packets, 2);
}

void HWOutputLEDBt::init(ConfigManager *config)
{
    m_btThread = config->getBTThreadByName(m_btName);

    // if we cannot find the bluetooth board, it does not exist and we should fail
    if(m_btThread == NULL)
    {
        LOG_WARN(Logger::BT, "Bluetooth board does not exist");

        return;
    }

    // setup I2C device
    m_btThread->addOutput(std::bind(&HWOutputLEDBt::setupI2CBt, this, std::placeholders::_1));
}

void HWOutputLEDBt::deinit(ConfigManager* config)
{
    m_btThread = NULL;
}

void HWOutputLEDBt::outputChanged()
{
    HWOutputLED::outputChanged();

    if(m_btThread != NULL)
        m_btThread->addOutput(std::bind(&HWOutputLEDBt::setI2CBt, this, std::placeholders::_1));
}
