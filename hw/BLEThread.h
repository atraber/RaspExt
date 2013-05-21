#ifndef BLETHREAD_H
#define BLETHREAD_H

#include "hw/BTThread.h"

#include <glib.h>
#include <gio/gio.h>

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


    // TODO: should be private
    void setState(unsigned char state);
private:
    static void* run_internal(void* arg);
    void run();

    bool bluezInit();
    bool bluezCreateDevice();
    bool bluezFindDevice();
    void bluezRemoveDevice();
    bool bluezDiscoverCharacteristics();
    bool bluezCreateWatcher();
    void bluezCleanup();
    void bluezDestroyWatcher();

    bool bluezRegisterWatcher();
    bool bluezRegisterDisconnectHandler();
    bool bluezUnregisterWatcher();
    void bluezUnregisterDisconnectHandler();
    bool bluezCheckConnection();
    void bluezDisconnectHandler(bool connected);

    struct GPInput
    {
        HWInputButtonBtGPIO* hw;
        unsigned int pinGroup;
        unsigned int pin;
    };

    std::list<GPInput> m_listGPInput;

    GMainLoop* m_loop;
    GDBusConnection* m_connection;
    guint m_busOwnerID;
    guint m_disconnectSignalID;
    gchar* m_servicePath;
    gchar* m_watcherPath;
    gchar* m_devicePath;
    gchar* m_adapterPath;

    friend gboolean check_connection_helper(gpointer data);
    friend void
    handle_disconnect_helper(GDBusConnection       *connection,
                            const gchar           *sender,
                            const gchar           *object_path,
                            const gchar           *interface_name,
                            const gchar           *signal_name,
                            GVariant              *parameters,
                            gpointer               user_data);
};

#endif // BLETHREAD_H
