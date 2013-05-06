
#include "hw/PCF8575I2C.h"
#include "hw/HWInputButtonI2C.h"
#include "hw/HWOutputGPOI2C.h"
#include "hw/I2CThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

PCF8575I2C::PCF8575I2C(int slaveAddress)
{
    m_slaveAddress = slaveAddress;
    m_portMask = 0;
    m_i2cThread = NULL;
}

void PCF8575I2C::addInput(HWInputButtonI2C *hw, unsigned int port)
{
    // TODO: check if already registered
    InputElement el;
    el.hw = hw;
    el.port = port;

    m_listInput.push_back( el );


    // Set port to 1, as only then if can detect inputs
    m_portMask = m_portMask | ( 1 << port );

    // we have to update I2C to set the new port mask
    this->updateI2C();
}

void PCF8575I2C::removeInput(HWInputButtonI2C *hw)
{
    for(std::list<InputElement>::iterator it = m_listInput.begin(); it != m_listInput.end(); it++)
    {
        if(it->hw == hw)
        {
            // reset port mask, setting the bit to zero which this input corresponds to
            m_portMask = m_portMask & ~(1 << it->port);

            m_listInput.erase(it);
            break;
        }
    }
}

void PCF8575I2C::addOutput(HWOutputGPOI2C *hw, unsigned int port)
{
    OutputElement el;
    el.hw = hw;
    el.port = port;

    m_listOutput.push_back(el);

    hw->registerOutputListener(this);
}

void PCF8575I2C::removeOutput(HWOutputGPOI2C *hw)
{
    for(std::list<OutputElement>::iterator it = m_listOutput.begin(); it != m_listOutput.end(); it++)
    {
        if(it->hw == hw)
        {
            hw->unregisterOutputListener(this);

            m_listOutput.erase(it);
            break;
        }
    }
}

void PCF8575I2C::setI2C(I2CThread *i2cThread)
{
    unsigned char buf[2];

    if( !m_i2cThread->setSlaveAddress(m_slaveAddress) )
    {
        LOG_WARN(Logger::I2C, "Failed to talk to slave");
        this->handleErrorOutput(true);
        return;
    }

    // directly write two bytes to the bus, these are the states of all inputs and outputs
    *((unsigned short*)buf) = m_portMask;

    if( !m_i2cThread->write(buf, 2) )
    {
        LOG_WARN(Logger::I2C, "Could not write to bus");
        this->handleErrorOutput(true);
        return;
    }
    this->handleErrorOutput(false);
}


void PCF8575I2C::poll(I2CThread* i2cThread)
{
    unsigned char buf[2];

    if( !m_i2cThread->setSlaveAddress(m_slaveAddress) )
    {
        LOG_WARN(Logger::I2C, "Failed to talk to slave");
        this->handleErrorInput(true);
        return;
    }

    // read back value
    if( !m_i2cThread->read(buf, 2) )
    {
        LOG_WARN(Logger::I2C, "Could not read from bus");
        this->handleErrorInput(true);
        return;
    }

    unsigned short portState = *((unsigned short*)buf);

    for(std::list<InputElement>::iterator it = m_listInput.begin(); it != m_listInput.end(); it++)
    {
        // call every HW Element to check, if its input has changed
        it->hw->onInputPolled( (portState & (1 << it->port)) == 0 );
        it->hw->handleError(false);
        // TODO: think about optimization here
    }
}

void PCF8575I2C::updateI2C()
{
    pi_assert(m_i2cThread != NULL);

    m_i2cThread->addOutput( std::bind(&PCF8575I2C::setI2C, this, std::placeholders::_1) );
}

void PCF8575I2C::init(I2CThread* thread)
{
    m_i2cThread = thread;

    m_i2cThread->addInput(this, 50);
}

void PCF8575I2C::deinit()
{
    m_i2cThread->removeInput(this);
    m_i2cThread = NULL;
}

void PCF8575I2C::onOutputChanged(HWOutput *hw)
{
    HWOutputGPOI2C* hw_i2c = (HWOutputGPOI2C*)hw;

    unsigned int port =  hw_i2c->getPort();
    m_portMask = (m_portMask & ~(1 << port)) | (hw_i2c->getValue() ? 1 << port : 0);

    // after we have set the new port mask, we have to update the device as well
    this->updateI2C();
}

void PCF8575I2C::handleErrorInput(bool errorOccurred, bool catastrophic)
{
    for(std::list<InputElement>::iterator it = m_listInput.begin(); it != m_listInput.end(); it++)
    {
        it->hw->handleError(errorOccurred, catastrophic);
    }
}

void PCF8575I2C::handleErrorOutput(bool errorOccurred, bool catastrophic)
{
    for(std::list<OutputElement>::iterator it = m_listOutput.begin(); it != m_listOutput.end(); it++)
    {
        it->hw->handleError(errorOccurred, catastrophic);
    }
}
