
#include "hw/BLEThread.h"
#include "hw/HWInputButtonBtGPIO.h"
#include "util/Config.h"
#include "util/Debug.h"

#include <QDomDocument>

#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <bluetooth/uuid.h>


#include "hw/ble/attrib/gatt_helper.h"
#include "hw/ble/attrib/gattrib.h"
#include "hw/ble/attrib/att.h"
#include "hw/ble/attrib/gatt.h"


/*******************************************************
 * BLEThread implementation
 *******************************************************/

BLEThread::BLEThread()
{
    m_loop = NULL;
}

BLEThread::~BLEThread()
{
    if(m_thread != 0)
        this->kill();
}

/**
 * @brief BTClassicThread::start starts this bluetooth thread.
 * The thread is automatically killed as soon as it is deleted, or it can be killed manually by BTClassicThread::kill
 */
void
BLEThread::start()
{
    pi_assert(m_thread == 0);

    // create and run thread
    pthread_create(&m_thread, NULL, BLEThread::run_internal, (void*)this);
}

/**
 * @brief BTClassicThread::kill kills this thread.
 */
void
BLEThread::kill()
{
    g_main_loop_quit(m_loop);

    pthread_join(m_thread, NULL);
    m_thread = 0;
}

void*
BLEThread::run_internal(void* arg)
{
    BLEThread* thread = (BLEThread*)arg;
    thread->run();

    return NULL;
}

BTThread*
BLEThread::load(QDomElement *root)
{
    return new BLEThread();
}

QDomElement
BLEThread::save(QDomElement *root, QDomDocument *document)
{
    QDomElement output = BTThread::save(root, document);

    QDomElement lowEnergy = document->createElement("LowEnergy");

    output.appendChild(lowEnergy);

    return output;
}

void
BLEThread::addGPInput(HWInputButtonBtGPIO* hw)
{
    GPInput gp;
    gp.hw = hw;
    gp.pin = hw->getPin();
    gp.pinGroup = hw->getPinGroup();

    // we cannot handle other pins then those in pingroup 2 and only pins from 0 to 3
    if(gp.pinGroup != 2 || gp.pin > 3)
        return;

    m_listGPInput.push_back(gp);
}

void
BLEThread::removeGPInput(HWInputButtonBtGPIO* hw)
{
    for(std::list<GPInput>::iterator it = m_listGPInput.begin(); it != m_listGPInput.end(); it++)
    {
        if( (*it).hw == hw)
        {
            m_listGPInput.erase(it);
            break;
        }
    }
}

void
BLEThread::setState(unsigned char state)
{
    for(std::list<GPInput>::iterator it = m_listGPInput.begin(); it != m_listGPInput.end(); it++)
    {
        if( (*it).pinGroup == 2 )
        {
            (*it).hw->setValue( (state & (1 << (*it).pin)) != 0);
        }
    }
}

static gboolean helper_channel_watcher(GIOChannel *chan, GIOCondition cond,
                gpointer user_data)
{
    BLEThread* thread = (BLEThread*)user_data;

    thread->bleChannelWatcher(chan, cond);

    return FALSE;
}

void BLEThread::bleChannelWatcher(GIOChannel *chan, GIOCondition cond)
{

    LOG_DEBUG(Logger::BT, "Channel_watcher called");

    switch(cond)
    {
    case G_IO_IN:
    case G_IO_OUT:
    case G_IO_PRI:
        break;

    case G_IO_ERR:
        LOG_DEBUG(Logger::BT, "Condition is G_IO_ERR");
        break;

    case G_IO_HUP:
        LOG_DEBUG(Logger::BT, "Condition is G_IO_HUP");
        break;

    case G_IO_NVAL:
        LOG_DEBUG(Logger::BT, "Condition is G_IO_NVAL");
        break;

    default:
        LOG_DEBUG(Logger::BT, "Crap");
        break;
    }

    if(cond == G_IO_HUP)
    {
        // we have lost connection and have to reconnect to the BT module
        g_io_channel_shutdown(m_iochan, FALSE, NULL);
        g_io_channel_unref(m_iochan);
        m_iochan = NULL;

        this->bleConnect();
    }
}

static void helper_events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
    BLEThread* thread = (BLEThread*)user_data;
    GAttrib *attrib = (GAttrib*)thread->m_attrib;
    uint8_t *opdu;
    uint16_t handle, i, olen = 0;
    size_t plen;

    handle = att_get_u16(&pdu[1]);

    // TODO: Forward new state to BLEThread
    switch (pdu[0])
    {
    case ATT_OP_HANDLE_NOTIFY:
        LOG_DEBUG(Logger::BT, "Notification handle = 0x%04x value: ", handle);
        break;
    case ATT_OP_HANDLE_IND:
        LOG_DEBUG(Logger::BT, "Indication   handle = 0x%04x value: ", handle);
        break;
    default:
        LOG_DEBUG(Logger::BT, "Invalid opcode");
        return;
    }

    for (i = 3; i < len; i++)
        LOG_DEBUG(Logger::BT, "%02x ", pdu[i]);

    if(len >= 3)
        thread->setState(pdu[3]);

    if (pdu[0] == ATT_OP_HANDLE_NOTIFY)
        return;

    opdu = g_attrib_get_buffer(attrib, &plen);
    olen = enc_confirmation(opdu, plen);

    if(olen > 0)
        g_attrib_send(attrib, 0, opdu, olen, NULL, NULL, NULL);
}

static void helper_char_read_cb(guint8 status, const guint8 *pdu, guint16 plen,
                            gpointer user_data)
{
    BLEThread* thread = (BLEThread*)user_data;
    uint8_t value[plen];
    ssize_t vlen;
    int i;

    if (status != 0)
    {
        LOG_WARN(Logger::BT, "Characteristic value/descriptor read failed: %s",
                            att_ecode2str(status));

        return;
    }

    vlen = dec_read_resp(pdu, plen, value, sizeof(value));

    if (vlen < 0)
    {
        LOG_WARN(Logger::BT, "Protocol error");
        return;
    }

    if(vlen != 1)
    {
        LOG_WARN(Logger::BT, "Invalid length of response");
        return;
    }

    thread->setState(value[0]);
}


static void helper_connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
    BLEThread* thread = (BLEThread*)user_data;

    thread->bleConnectCb(io, err);
}

void BLEThread::bleConnectCb(GIOChannel* io, GError* err)
{
    GAttrib *attrib;

    if(err)
    {
        // Connecting has failed
        LOG_WARN(Logger::BT, "%s", err->message);
        g_main_loop_quit(m_loop);

        return;
    }

    LOG_DEBUG(Logger::BT, "Connected successfully");

    attrib = g_attrib_new(m_iochan);

    // we have successfully connected and are now ready to receive events

    g_attrib_register(attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
                        helper_events_handler, this, NULL);
    g_attrib_register(attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
                        helper_events_handler, this, NULL);

    // additionally update the key state
    int opt_handle = 0x0025;
    gatt_read_char(attrib, opt_handle, helper_char_read_cb, this);
}

// helper function
static gboolean helper_ble_connect(gpointer user_data)
{
    BLEThread* thread = (BLEThread*)user_data;

    thread->bleConnect();

    return FALSE;
}

void BLEThread::bleConnect()
{
    GError *gerr = NULL;
    // connect to gatt, this functions returns NULL on errors
    m_iochan = gatt_connect(NULL, // OPT_SRC
                            m_opt_dst,
                            m_opt_dst_type,
                            m_opt_sec_level,
                            0, // PSM
                            0, // MTU
                            helper_connect_cb,
                            this,
                            &gerr);

    if(m_iochan == NULL)
    {
        // could not connet
        g_printerr("Could not connect: %s", gerr->message);
        g_clear_error(&gerr);

        // schedule a retry
        g_timeout_add(500, helper_ble_connect, this);
    }
    else
    {
        LOG_DEBUG(Logger::BT, "Connecting...");
        g_io_add_watch(m_iochan, G_IO_HUP, helper_channel_watcher, this);
    }
}

void
BLEThread::run()
{
    m_opt_dst = g_strdup(m_btaddr.c_str());

    m_opt_dst_type = g_strdup("public");
    m_opt_sec_level = g_strdup("low");

    // Connect now
    this->bleConnect();


    m_loop = g_main_loop_new(NULL, FALSE);

    // run main loop, this method does not return until finished
    g_main_loop_run(m_loop);

    // main loop has ended, we can now kill everything
    g_main_loop_unref(m_loop);

    // cleanup
    g_free(m_opt_dst);
    g_free(m_opt_dst_type);
    g_free(m_opt_sec_level);
}

/*******************************************************
 * Virtual methods, not yet implemented
 *******************************************************/

void
BLEThread::addInput(BTI2CPolling* hw, unsigned int freq)
{
    LOG_WARN(Logger::BT, "Not yet implemented");
}

void
BLEThread::removeInput(BTI2CPolling* hw)
{
    LOG_WARN(Logger::BT, "Not yet implemented");
}

void
BLEThread::addOutput(std::function<void (BTThread*)> func)
{
    LOG_WARN(Logger::BT, "Not yet implemented");
}

void
BLEThread::addInputPCF8575(HWInput* hw, int slaveAddress, unsigned int port)
{
    LOG_WARN(Logger::BT, "Not yet implemented");
}

void
BLEThread::removeInputPCF8575(HWInput* hw, int slaveAddress)
{
    LOG_WARN(Logger::BT, "Not yet implemented");
}

void
BLEThread::addOutputPCF8575(HWOutput* hw, int slaveAddress, unsigned int port)
{
    LOG_WARN(Logger::BT, "Not yet implemented");
}

void
BLEThread::removeOutputPCF8575(HWOutput* hw, int slaveAddress)
{
    LOG_WARN(Logger::BT, "Not yet implemented");
}

// ATTENTION: USE ONLY IN BTTHREAD!!!!
void
BLEThread::sendI2CPackets(BTI2CPacket* packets, unsigned int num)
{
    LOG_WARN(Logger::BT, "Not yet implemented");
}
