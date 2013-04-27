
#include "hw/HWInputButtonI2C.h"
#include "ConfigManager.h"
#include "hw/I2CThread.h"
#include "util/Debug.h"

#include <QDomElement>

HWInputButtonI2C::HWInputButtonI2C()
{
    this->m_slaveAddress = -1;
    this->m_port = -1;
}

HWInput* HWInputButtonI2C::load(QDomElement *root)
{
    HWInputButtonI2C* hw = new HWInputButtonI2C();
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
        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_slaveAddress > 127 || hw->m_slaveAddress < 0 || hw->m_port > 15)
    {
        pi_warn("Invalid i2c parameters");
        return NULL;
    }

    return hw;
}

QDomElement HWInputButtonI2C::save(QDomElement* root, QDomDocument* document)
{
    QDomElement input = HWInputButton::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("I2C");
    type.appendChild(typeText);

    input.appendChild(type);

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

bool HWInputButtonI2C::init(ConfigManager* config)
{
    I2CThread* i2c = config->getI2CThread();
    i2c->addInputPCF8575(this, m_slaveAddress, m_port);

    return true;
}

void HWInputButtonI2C::deinit(ConfigManager* config)
{
    I2CThread* i2c = config->getI2CThread();
    i2c->removeInputPCF8575(this, m_slaveAddress);
}

void HWInputButtonI2C::onInputPolled(bool state)
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

void HWInputButtonI2C::handleError(bool errorOccurred, bool catastrophic)
{
    HWInput::handleError(errorOccurred, catastrophic);
}
