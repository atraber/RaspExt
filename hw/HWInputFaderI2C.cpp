
#include "hw/HWInputFaderI2C.h"
#include "hw/I2CThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

#include <QDomElement>

HWInputFaderI2C::HWInputFaderI2C()
{
    this->m_slaveAddress = -1;
    this->m_channel = -1;
}

HWInput* HWInputFaderI2C::load(QDomElement *root)
{
    HWInputFaderI2C* hw = new HWInputFaderI2C();
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
        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_slaveAddress > 127 || hw->m_slaveAddress < 0 ||
            hw->m_channel < 0 || hw->m_channel > 7)
    {
        pi_warn("Invalid i2c parameters");
        delete hw;
        return NULL;
    }

    return hw;
}

QDomElement HWInputFaderI2C::save(QDomElement* root, QDomDocument* document)
{
    QDomElement input = HWInputFader::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("I2C");
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

    return input;
}

bool HWInputFaderI2C::init(ConfigManager* config)
{
    I2CThread* i2c = config->getI2CThread();
    i2c->addInput(this, 50);

    return true;
}

void HWInputFaderI2C::deinit(ConfigManager* config)
{
    I2CThread* i2c = config->getI2CThread();
    i2c->removeInput(this);
}

void HWInputFaderI2C::poll(I2CThread* i2cThread)
{
    // If override is active, our polled value is of no interest anyway, so we do not even poll
    if(this->getOverride())
        return;

    unsigned char buf[1];

    if( !i2cThread->setSlaveAddress(m_slaveAddress) )
    {
        I2C_warn("Failed to talk to slave");

        this->handleError(true);

        return;
    }

    // first write to ads7830 which channel it should sample

    buf[0] = ((m_channel & 6) >> 1 | (m_channel & 1) << 2)<< 4; // bit 0 => bit 2, bit 1,2 => 0,1
    buf[0] = buf[0] | 1 << 7 | 1 << 3 | 1 << 2; // internal reference and ad convert on

    if( !i2cThread->write(buf, 1) )
    {
        I2C_warn("Could not write to bus");

        this->handleError(true);

        return;
    }

    // read back value
    if( !i2cThread->read(buf, 1) )
    {
        I2C_warn("Could not read from bus");

        this->handleError(true);
        return;
    }
    this->handleError(false);

    // convert ad value to percent
    unsigned int value = buf[0] * 100 / 255;

    this->setValue(value);
}
