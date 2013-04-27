#ifndef HWOUTPUTGPOI2C_H
#define HWOUTPUTGPOI2C_H

#include "hw/HWOutputGPO.h"

class I2CThread;

/**
 * @brief The HWOutputGPOI2C class
 * This implementation is specific for the PCF8575. It uses the PCF8575I2C class to communicate over i2c with its chip.
 */
class HWOutputGPOI2C : public HWOutputGPO
{
public:
    HWOutputGPOI2C();

    HWType getHWType() const { return I2C;}

    virtual void init(ConfigManager* config);
    virtual void deinit(ConfigManager* config);

    static HWOutput* load(QDomElement* root);
    virtual QDomElement save(QDomElement* root, QDomDocument* document);

    virtual void handleError(bool errorOccurred, bool catastrophic = false);

    unsigned int getPort() const { return m_port;}
    void setPort(unsigned int port) { m_port = port;}

    int getSlaveAddress() const { return m_slaveAddress;}
    void setSlaveAddress(int slaveAddress) { m_slaveAddress = slaveAddress;}
private:
    int m_slaveAddress;
    unsigned int m_port;
    I2CThread* m_i2cThread;
};

#endif // HWOUTPUTGPOI2C_H
