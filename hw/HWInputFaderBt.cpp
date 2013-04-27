
#include "hw/HWInputFaderBt.h"
#include "ConfigManager.h"
#include "util/Debug.h"

#include <QDomElement>

HWInputFaderBt::HWInputFaderBt()
{
    m_slaveAddress = -1;
    m_channel = -1;
    m_btThread = NULL;
}

HWInput* HWInputFaderBt::load(QDomElement *root)
{
    HWInputFaderBt* hw = new HWInputFaderBt();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if( elem.tagName().toLower().compare("slaveaddress") == 0 )
        {
            hw->setSlaveAddress( elem.text().toInt() );
        }
        else if( elem.tagName().toLower().compare("channel") == 0 )
        {
            hw->setChannel( elem.text().toInt() );
        }
        else if( elem.tagName().toLower().compare("btboard") == 0 )
        {
            hw->setBTName( elem.text().toStdString() );
        }

        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_slaveAddress > 127 || hw->m_slaveAddress < 0 ||
            hw->m_channel < 0 || hw->m_channel > 7 || hw->m_btName.empty())
    {
        pi_warn("Invalid parameters");
        delete hw;
        return NULL;
    }

    return hw;
}

QDomElement HWInputFaderBt::save(QDomElement* root, QDomDocument* document)
{
    QDomElement input = HWInputFader::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("Bt");
    type.appendChild(typeText);

    input.appendChild(type);

    QDomElement slaveAddr = document->createElement("SlaveAddress");
    QDomText slaveAddrText = document->createTextNode(QString::number( m_slaveAddress )); // todo: maybe save in hex?
    slaveAddr.appendChild(slaveAddrText);

    input.appendChild(slaveAddr);

    QDomElement channel = document->createElement("Channel");
    QDomText channelText = document->createTextNode(QString::number( m_channel ));
    channel.appendChild(channelText);

    input.appendChild(channel);

    QDomElement btBoard = document->createElement("BtBoard");
    QDomText btBoardText = document->createTextNode( QString::fromStdString( m_btName ) );
    btBoard.appendChild(btBoardText);

    input.appendChild(btBoard);

    return input;
}

bool HWInputFaderBt::init(ConfigManager* config)
{
    m_btThread = config->getBTThreadByName(m_btName);

    // if we cannot find the bluetooth board, it does not exist and we should fail
    if(m_btThread == NULL)
    {
        pi_warn("Bluetooth board does not exist");

        return false;
    }

    m_btThread->addInput(this, 50); // TODO: DEBUG set this to 50

    return true;
}

void HWInputFaderBt::deinit(ConfigManager* config)
{
    m_btThread = NULL;
}

void HWInputFaderBt::poll(BTThread* thread)
{
    // If override is active, our polled value is of no interest anyway, so we do not even poll
    if(this->getOverride())
        return;

    BTI2CPacket packet;

    // write to ads7830 which channel it should sample
    packet.slaveAddress = m_slaveAddress;
    packet.read = 1;
    packet.request = 1;

    packet.commandLength = 1;
    packet.commandBuffer = (char*)malloc(packet.commandLength);
    packet.commandBuffer[0] = ((m_channel & 6) >> 1 | (m_channel & 1) << 2)<< 4; // bit 0 => bit 2, bit 1,2 => 0,1
    packet.commandBuffer[0] = packet.commandBuffer[0] | 1 << 7 | 1 << 3 | 1 << 2; // internal reference and ad convert on

    packet.readLength = 1;
    packet.callbackFunc = std::bind(&HWInputFaderBt::onRead, this, std::placeholders::_1, std::placeholders::_2);

    m_btThread->sendI2CPackets(&packet, 1);
}

void HWInputFaderBt::onRead(BTThread* thread, BTI2CPacket* packet)
{
    // first check if this is a valid packet
    if(packet->readLength != 1 || packet->error != 0 || !packet->read)
    {
        this->handleError(true);
        return;
    }
    this->handleError(false);

    // convert ad value to percent
    unsigned int value = ((unsigned char)packet->readBuffer[0]) * 100 / 255;

    this->setValue(value);
}
