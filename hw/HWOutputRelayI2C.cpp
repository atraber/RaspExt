
#include "hw/HWOutputRelayI2C.h"
#include "hw/I2CThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

HWOutputRelayI2C::HWOutputRelayI2C()
{
    m_slaveAddress = -1;
    m_channel = -1;
    m_i2cThread = NULL;
}

HWOutput* HWOutputRelayI2C::load(QDomElement *root)
{
    HWOutputRelayI2C* hw = new HWOutputRelayI2C();
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

        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_slaveAddress > 127 || hw->m_slaveAddress < 0 || hw->m_channel > 15)
    {
        pi_warn("Invalid i2c parameters");
        return NULL;
    }

    return hw;
}

QDomElement HWOutputRelayI2C::save(QDomElement* root, QDomDocument* document)
{
    QDomElement output = HWOutputRelay::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("I2C");
    type.appendChild(typeText);

    output.appendChild(type);

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

void HWOutputRelayI2C::setI2C(I2CThread* i2cThread)
{
    unsigned char buf[2];

    if( !m_i2cThread->setSlaveAddress(m_slaveAddress) )
    {
        I2C_warn("Failed to talk to slave");
        return;
    }

    // first select register for PWM brightness control
    buf[0] = 0x02 + m_channel;

    // now set brightness (fully on or off for relay, otherwise this is the same as for the LED part)
    buf[1] = m_value ? 0xFF : 0x000;

    if( !m_i2cThread->write(buf, 2) )
    {
        I2C_warn("Could not write to bus");
        return;
    }
}

void HWOutputRelayI2C::setupI2C(I2CThread* i2cThread)
{
    unsigned char buf[2];

    if( !m_i2cThread->setSlaveAddress(m_slaveAddress) )
    {
        I2C_warn("Failed to talk to slave");
        return;
    }

    // activate oscillator and deactivate all call address

    // first select mode register
    buf[0] = 0x00;
    buf[1] = 0x00;
    if( !m_i2cThread->write(buf, 2) )
    {
        I2C_warn("Could not write to bus");
        return;
    }

    // set LED output state to control individual brightness

    // first select register to set output state of LED
    buf[0] = 0x14 + m_channel / 4;

    // now set state to "LED driver x is individual brightness can be controlled through its PWMx register"
    // see page 17 of datasheet
    buf[1] = 0xAA;

    if( !m_i2cThread->write(buf, 2) )
    {
        I2C_warn("Could not write to bus");
        return;
    }
}

void HWOutputRelayI2C::init(ConfigManager *config)
{
    m_i2cThread = config->getI2CThread();

    // setup I2C device
    m_i2cThread->addOutput( std::bind(&HWOutputRelayI2C::setupI2C, this, std::placeholders::_1) );
}

void HWOutputRelayI2C::deinit(ConfigManager* config)
{
    m_i2cThread = NULL;
}

void HWOutputRelayI2C::outputChanged()
{
    HWOutputRelay::outputChanged();

    m_i2cThread->addOutput( std::bind(&HWOutputRelayI2C::setI2C, this, std::placeholders::_1) );
}
