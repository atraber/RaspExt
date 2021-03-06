#ifndef HWOUTPUTRELAYBT_H
#define HWOUTPUTRELAYBT_H

#include "hw/HWOutputRelay.h"

class BTThread;

/** HWOutputRelayBt is essentially the same as HWOutputLEDBt, as both use the same chip.
 * However, with the led implementation one can control the individual brightness of each led,
 * whereas with this implementation one can only control if it is fully on or fully off (relay mode).
 * This implementation is specific for TLC59116, it will not work for other led drivers!
 */
class HWOutputRelayBt : public HWOutputRelay
{
public:
    HWOutputRelayBt();

    HWType getHWType() const { return BtI2C;}

    virtual void init(ConfigManager* config);
    virtual void deinit(ConfigManager* config);

    static HWOutput* load(QDomElement* root);
    virtual QDomElement save(QDomElement* root, QDomDocument* document);

    int getChannel() const { return m_channel;}
    void setChannel(unsigned int channel) { m_channel = channel;}

    int getSlaveAddress() const { return m_slaveAddress;}
    void setSlaveAddress(int slaveAddress) { m_slaveAddress = slaveAddress;}

    std::string getBTName() const { return m_btName;}
    void setBTName(std::string name) { m_btName = name;}
private:
    void outputChanged();

    void setI2CBt(BTThread* btThread);
    void setupI2CBt(BTThread* btThread);

    int m_slaveAddress;
    unsigned int m_channel;
    std::string m_btName;
    BTThread* m_btThread;
};

#endif // HWOUTPUTRELAYBT_H
