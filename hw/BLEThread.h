#ifndef BLETHREAD_H
#define BLETHREAD_H

#include "hw/BTThread.h"

#include <glib.h>
#include <gio/gio.h>

#include "hw/ble/attrib/gattrib.h"
static void helper_events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data);

class BLEThread : public BTThread
{
public:
    BLEThread();
    ~BLEThread();

    void start();
    void kill();

    static BTThread* load(QDomElement* root);
    QDomElement save(QDomElement* root, QDomDocument* document);

    void addGPInput(HWInputButtonBtGPIO* hw);
    void removeGPInput(HWInputButtonBtGPIO* hw);

    void setName(std::string name) { m_name = name;}
    std::string getName() const { return m_name;}
    void setBTAddr(std::string addr);
    std::string getBTAddr() const { return m_btaddr;}


    // the following functions are not implemented
    void addInput(BTI2CPolling* hw, unsigned int freq);
    void removeInput(BTI2CPolling* hw);

    void addOutput(std::function<void (BTThread*)> func);

    void addInputPCF8575(HWInput* hw, int slaveAddress, unsigned int port);
    void removeInputPCF8575(HWInput* hw, int slaveAddress);
    void addOutputPCF8575(HWOutput* hw, int slaveAddress, unsigned int port);
    void removeOutputPCF8575(HWOutput* hw, int slaveAddress);

    // ATTENTION: USE ONLY IN BTTHREAD!!!!
    void sendI2CPackets(BTI2CPacket* packets, unsigned int num);


    // TODO: should be private, does this make sense?
    void setState(unsigned char state);

    void bleConnect();
    void bleConnectCb(GIOChannel* io, GError* err);
    void bleChannelWatcher(GIOChannel *chan, GIOCondition cond);
private:
    static void* run_internal(void* arg);
    void run();

    struct GPInput
    {
        HWInputButtonBtGPIO* hw;
        unsigned int pinGroup;
        unsigned int pin;
    };

    std::list<GPInput> m_listGPInput;

    GMainLoop* m_loop;
    GIOChannel* m_iochan;

    char* m_opt_dst;
    char* m_opt_dst_type;
    char* m_opt_sec_level;
    GAttrib* m_attrib;

    friend void helper_events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data);
};

#endif // BLETHREAD_H
