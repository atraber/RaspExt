
#include "hw/HWOutputLEDI2C.h"
#include "hw/I2CThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

HWOutputLEDI2C::HWOutputLEDI2C()
{
    m_slaveAddress = -1;
    m_channel = -1;
    m_i2cThread = NULL;
}

HWOutput* HWOutputLEDI2C::load(QDomElement *root)
{
    HWOutputLEDI2C* hw = new HWOutputLEDI2C();
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

QDomElement HWOutputLEDI2C::save(QDomElement* root, QDomDocument* document)
{
    QDomElement output = HWOutputLED::save(root, document);

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

void HWOutputLEDI2C::setI2C(I2CThread* i2cThread)
{
    unsigned char buf[2];

    if( !m_i2cThread->setSlaveAddress(m_slaveAddress) )
    {
        I2C_warn("Failed to talk to slave");

        this->handleError(true);

        return;
    }

    // first select register for PWM brightness control
    buf[0] = 0x02 + m_channel;

    // now set brightness
    buf[1] = m_value * 255 / 100;

    if( !m_i2cThread->write(buf, 2) )
    {
        I2C_warn("Could not write to bus");

        this->handleError(true);

        return;
    }
    this->handleError(false);
}

void HWOutputLEDI2C::setupI2C(I2CThread *i2cThread)
{
    unsigned char buf[2];

    if( !m_i2cThread->setSlaveAddress(m_slaveAddress))
    {
        I2C_warn("Failed to talk to slave");

        this->handleError(true, true);

        return;
    }

    // activate oscillator and deactivate all call address

    // first select mode register
    buf[0] = 0x00;
    buf[1] = 0x00;

    if( !m_i2cThread->write(buf, 2) )
    {
        I2C_warn("Could not write to bus");

        this->handleError(true, true);

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

        this->handleError(true, true);

        return;
    }
    this->handleError(false);
}

void HWOutputLEDI2C::init(ConfigManager *config)
{
    m_i2cThread = config->getI2CThread();

    // setup I2C device
    m_i2cThread->addOutput(std::bind(&HWOutputLEDI2C::setupI2C, this, std::placeholders::_1));
}

void HWOutputLEDI2C::deinit(ConfigManager* config)
{
    m_i2cThread = NULL;
}

void HWOutputLEDI2C::outputChanged()
{
    HWOutputLED::outputChanged();

    m_i2cThread->addOutput( std::bind(&HWOutputLEDI2C::setI2C, this, std::placeholders::_1) );
}
