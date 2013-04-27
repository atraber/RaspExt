
#include "hw/HWOutputGPOI2C.h"
#include "hw/I2CThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

HWOutputGPOI2C::HWOutputGPOI2C()
{
    m_slaveAddress = -1;
    m_port = -1;
    m_i2cThread = NULL;
}

HWOutput* HWOutputGPOI2C::load(QDomElement *root)
{
    HWOutputGPOI2C* hw = new HWOutputGPOI2C();
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

QDomElement HWOutputGPOI2C::save(QDomElement* root, QDomDocument* document)
{
    QDomElement output = HWOutputGPO::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("I2C");
    type.appendChild(typeText);

    output.appendChild(type);

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

void HWOutputGPOI2C::init(ConfigManager *config)
{
    m_i2cThread = config->getI2CThread();
    m_i2cThread->addOutputPCF8575(this, m_slaveAddress, m_port);

}

void HWOutputGPOI2C::deinit(ConfigManager* config)
{
    m_i2cThread->removeOutputPCF8575(this, m_slaveAddress);
    m_i2cThread = NULL;
}

void HWOutputGPOI2C::handleError(bool errorOccurred, bool catastrophic)
{
    HWOutput::handleError(errorOccurred, catastrophic);
}
