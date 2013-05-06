
#include "hw/HWOutputDCMotorI2C.h"
#include "hw/I2CThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

HWOutputDCMotorI2C::HWOutputDCMotorI2C()
{
    m_slaveAddress = -1;
    m_i2cThread = NULL;
}

HWOutput* HWOutputDCMotorI2C::load(QDomElement *root)
{
    HWOutputDCMotorI2C* hw = new HWOutputDCMotorI2C();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if( elem.tagName().toLower().compare("slaveaddress") == 0 )
        {
            hw->m_slaveAddress = elem.text().toInt();
        }
        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_slaveAddress > 127 || hw->m_slaveAddress < 0)
    {
        LOG_WARN(Logger::I2C, "Invalid i2c parameters");
        return NULL;
    }

    return hw;
}

QDomElement HWOutputDCMotorI2C::save(QDomElement* root, QDomDocument* document)
{
    QDomElement output = HWOutputDCMotor::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("I2C");
    type.appendChild(typeText);

    output.appendChild(type);

    QDomElement slaveAddr = document->createElement("SlaveAddress");
    QDomText slaveAddrText = document->createTextNode(QString::number( m_slaveAddress ));
    slaveAddr.appendChild(slaveAddrText);

    output.appendChild(slaveAddr);

    return output;
}

void HWOutputDCMotorI2C::setI2C(I2CThread* i2cThread)
{
    unsigned char buf[2];

    if( !m_i2cThread->setSlaveAddress(m_slaveAddress) )
    {
        LOG_WARN(Logger::I2C, "Failed to talk to slave");

        this->handleError(true);

        return;
    }
    // first select register 0x00 on drv8830
    buf[0] = 0x00;

    // now set output state
    buf[1] = m_state | ((m_speed * 63 / 100) << 2);

    if( !m_i2cThread->write(buf, 2) )
    {
        LOG_WARN(Logger::I2C, "Could not write to bus");

        this->handleError(true);

        return;
    }
}

void HWOutputDCMotorI2C::init(ConfigManager *config)
{
    m_i2cThread = config->getI2CThread();
}

void HWOutputDCMotorI2C::deinit(ConfigManager *config)
{
    m_i2cThread = NULL;
}

void HWOutputDCMotorI2C::outputChanged()
{
    HWOutputDCMotor::outputChanged();

    m_i2cThread->addOutput( std::bind(&HWOutputDCMotorI2C::setI2C, this, std::placeholders::_1) );
}
