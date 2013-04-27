#ifndef HWINPUTBUTTONI2C_H
#define HWINPUTBUTTONI2C_H

#include "hw/HWInputButton.h"

class HWInputButtonI2C : public HWInputButton
{
public:
    HWInputButtonI2C();

    HWType getHWType() const { return I2C;}

    bool init(ConfigManager* config);
    void deinit(ConfigManager* config);

    static HWInput* load(QDomElement* root);
    virtual QDomElement save(QDomElement* root, QDomDocument* document);

    void onInputPolled(bool state);
    virtual void handleError(bool errorOccurred, bool catastrophic = false);

    int getPort() const { return m_port;}
    void setPort(unsigned int port) { m_port = port;}

    int getSlaveAddress() const { return m_slaveAddress;}
    void setSlaveAddress(int slaveAddress) { m_slaveAddress = slaveAddress;}
private:
    int m_slaveAddress;
    unsigned int m_port;
};

#endif // HWINPUTBUTTONI2C_H
