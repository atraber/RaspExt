
#include "hw/BTClassicThread.h"
#include "hw/HWInputButtonBtGPIO.h"
#include "hw/PCF8575Bt.h"
#include "util/Config.h"
#include "util/Debug.h"

#include <signal.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <poll.h>
#include <unistd.h>

#include <QDomDocument>

static void dummy_handler(int)
{
    // nothing here
}

BTClassicThread::BTClassicThread()
{
    m_socket = -1;
    m_seq = 0;

    // specify a dummy handler for SIGUSR1
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = dummy_handler;
    sigaction(SIGUSR1, &sa, NULL);
}

BTClassicThread::~BTClassicThread()
{
    if(m_thread != 0)
        this->kill();
}

/**
 * @brief BTClassicThread::start starts this bluetooth thread.
 * The thread is automatically killed as soon as it is deleted, or it can be killed manually by BTClassicThread::kill
 */
void BTClassicThread::start()
{
    pi_assert(m_thread == 0);

    m_bStop = false;

    // create and run thread
    pthread_create(&m_thread, NULL, BTClassicThread::run_internal, (void*)this);
}

/**
 * @brief BTClassicThread::kill kills this thread.
 */
void BTClassicThread::kill()
{
    // set stop to true, so the thread should exit soon
    m_mutex.lock();
    m_bStop = true;
    m_mutex.unlock();

    // deliver signal to thread to wake it up
    pthread_kill(m_thread, SIGUSR1);

    pthread_join(m_thread, NULL);
    m_thread = 0;
}

/**
 * @brief BTClassicThread::load loads the necessary information from an XML file.
 * The param root must be a XML node under which the information for this thread are stored.
 * If this method does not find all necessary information and thus cannot be loaded it returns NULL
 * @param root
 * @return returns NULL if an error occurred or an instance of BTClassicThread
 */
BTThread* BTClassicThread::load(QDomElement* root)
{
    BTClassicThread* btthread = new BTClassicThread();

    return btthread;
}

/**
 * @brief BTClassicThread::addInput adds an input to this thread which is polled with frequency freq.
 * @param hw
 * @param freq
 */
void BTClassicThread::addInput(BTI2CPolling *hw, unsigned int freq)
{
    InputElement element;
    element.freq = freq;
    clock_gettime(CLOCK_MONOTONIC, &element.time);
    element.hw = hw;

    m_mutex.lock();
    m_inputQueue.push(element);
    m_mutex.unlock();

    // deliver signal to thread to wake it up
    if(m_thread != 0)
        pthread_kill(m_thread, SIGUSR1);
}

/**
 * @brief BTClassicThread::removeInput removes an input from this thread which is then no longer polled.
 * For example if configuration has changed and the input is no longer needed.
 * @param hw
 */
void BTClassicThread::removeInput(BTI2CPolling *hw)
{
    // we only use this element to remove the corresponding element from the queue
    InputElement element;
    element.hw = hw;

    m_mutex.lock();
    m_inputQueue.remove(element);
    m_mutex.unlock();
}

void BTClassicThread::connectBt()
{
    // try to connect until it succeeds
    int status;
    do
    {
        // If we want to exit this thread
        if(m_bStop)
        {
            pthread_exit(0);
        }

        // open socket
        struct sockaddr_l2 addr;

        m_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

        if(m_socket == -1)
        {
            LOG_WARN(Logger::BT, "Could not open bluetooth socket");
            return;
        }

        // set the connection parameters (who to connect to)
        addr.l2_family = AF_BLUETOOTH;
        addr.l2_psm = htobs(0x1001);
        addr.l2_cid = 0;

        str2ba(m_btaddr.c_str(), &addr.l2_bdaddr);

        // connect to target
        // maybe wo should do this in the run loop, as we may loose our connection to the board or it is not yet available
        status = connect(m_socket, (struct sockaddr*)&addr, sizeof(addr));
        if(status == -1)
        {
            perror("Could not connect to bt-board, retrying");

            // wait for some time before trying to reconnect
            sleep(1);

            // close and release all resoureces associated with m_socket, so that it can be reused
            close(m_socket);
        }
    }
    while(status == -1);

    LOG_DEBUG(Logger::BT, "Connected to bluetooth board %s\n", m_name.c_str());

    // clean lists as the information in them is most likely invalid now
    m_listSeq.clear();

    // do a status update for pin group 2
    // TODO: check all registered pins for their pin groups and send a status update for each
    this->addOutput(std::bind(&BTClassicThread::sendGPUpdateRequest, this, 2, std::placeholders::_1));
}

void BTClassicThread::disconnectBt()
{
    close(m_socket);
}

void BTClassicThread::reconnectBt()
{
    LOG_DEBUG(Logger::BT, "Lost connection to bluetooth board %s\nTrying to reconnect\n", m_name.c_str());

    this->disconnectBt();
    this->connectBt();
}


void BTClassicThread::addInputPCF8575(HWInput* hw, int slaveAddress, unsigned int port)
{
    // first check if we already have an Object for this slave address
    for(std::list<PCF8575Bt*>::iterator it = m_listPCF8575.begin(); it != m_listPCF8575.end(); it++)
    {
        if( (*it)->getSlaveAddress() == slaveAddress )
        {
            // we found it
            (*it)->addInput((HWInputButtonBt*)hw, port);
            return;
        }
    }

    // we did not found an object for this slave address, so create a new one
    PCF8575Bt* pcf = new PCF8575Bt(slaveAddress);
    m_listPCF8575.push_back(pcf);

    pcf->init(this);

    pcf->addInput((HWInputButtonBt*)hw, port);
}

void BTClassicThread::removeInputPCF8575(HWInput* hw, int slaveAddress)
{
    // search for the corresponding pcf object
    for(std::list<PCF8575Bt*>::iterator it = m_listPCF8575.begin(); it != m_listPCF8575.end(); it++)
    {
        if( (*it)->getSlaveAddress() == slaveAddress )
        {
            // we found it
            (*it)->removeInput((HWInputButtonBt*)hw);

            // check if the pcf object is empty now
            if((*it)->empty())
            {
                (*it)->deinit();
                delete *it;

                m_listPCF8575.erase(it);
            }

            return;
        }
    }
}

void BTClassicThread::addOutputPCF8575(HWOutput* hw, int slaveAddress, unsigned int port)
{
    // first check if we already have an Object for this slave address
    for(std::list<PCF8575Bt*>::iterator it = m_listPCF8575.begin(); it != m_listPCF8575.end(); it++)
    {
        if( (*it)->getSlaveAddress() == slaveAddress )
        {
            // we found it
            (*it)->addOutput((HWOutputGPO*)hw, port);
            return;
        }
    }

    // we did not found an object for this slave address, so create a new one
    PCF8575Bt* pcf = new PCF8575Bt(slaveAddress);
    m_listPCF8575.push_back(pcf);

    pcf->init(this);

    pcf->addOutput((HWOutputGPO*)hw, port);
}

void BTClassicThread::removeOutputPCF8575(HWOutput* hw, int slaveAddress)
{
    // search for the corresponding pcf object
    for(std::list<PCF8575Bt*>::iterator it = m_listPCF8575.begin(); it != m_listPCF8575.end(); it++)
    {
        if( (*it)->getSlaveAddress() == slaveAddress )
        {
            // we found it
            (*it)->removeOutput((HWOutputGPO*)hw);

            // check if the pcf object is empty now
            if((*it)->empty())
            {
                (*it)->deinit();
                delete *it;

                m_listPCF8575.erase(it);
            }

            return;
        }
    }
}

void BTClassicThread::readBlocking()
{
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int readBytes = recv(m_socket, buffer, sizeof(buffer), 0);

    // TODO: check if this is enough error checking
    if(readBytes == 0)
    {
        // if read returns 0 this means end of file => we have lost connection and need to reconnect
        this->reconnectBt();
    }
    else if(readBytes == -1)
    {
        perror("Error occurred while doing read");
        if(errno == ENOTCONN)
            this->reconnectBt();
    }
    else
    {
        // we have received the packet, now we have to parse it and call the appropriate functions
        this->packetHandler(buffer, readBytes);
    }
}

void BTClassicThread::run()
{
#ifdef USE_BLUETOOTH
    // connect to bluetooth board
    this->connectBt();


    // now we are connected and can enter the run loop

    timespec currentTime;
    timespec waitTime;
    while(true)
    {
        // Priorities:
        // 1. Incoming data
        // 2. Outputs
        // 3. Input Pollings


        // TODO: maybe this should be moved further down?

        struct pollfd fd;
        fd.fd = m_socket;
        fd.events = POLLIN;
        fd.revents = 0;

        // we have to check first if data is available, otherwise read would block
        int dataReady = poll(&fd, 1, 0);

        if(dataReady == 1)
        {
            // there is data ready to be read, so read it!
            this->readBlocking();
        }


        m_mutex.lock();

        if(m_bStop)
        {
            // we want to exit the while loop
            m_mutex.unlock();
            break;
        }

        // if OutputQueue is not empty, then run the function in the element
        if( !m_outputQueue.empty() )
        {
            OutputElement element = m_outputQueue.front();
            m_outputQueue.pop();
            m_mutex.unlock();

            // run function
            element.func(this);
            continue;
        }

        // check if inputQueue is empty, if yes then sleep
        if( m_inputQueue.empty() )
        {
            m_mutex.unlock();

            // sleep for 100ms and wake up if we receive a signal
            timespec sleep;
            sleep.tv_sec = 0;
            sleep.tv_nsec = 100000000;

            // check if there is pending data to read, and if not wait for waitTime
            this->readWait(sleep);
            continue;
        }

        // get element from inputQueue
        InputElement element = m_inputQueue.top();

        m_mutex.unlock();

        // get current time
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        // if this is true we have to sleep first
        if( timespecGreaterThan(element.time, currentTime) )
        {
            // calculate time difference for sleep
            waitTime = timespecSub(element.time, currentTime);

            if(this->readWait(waitTime))
            {
                // there is something to do before the timeout has expired, go back to start
                continue;
            }
        }

        // now we should do something as the timer has expired
        element.hw->poll(this);


        // preperation for next poll
        pi_assert(element.freq > 0);
        element.time.tv_nsec = currentTime.tv_nsec + 1000000000 / element.freq;
        if( element.time.tv_nsec > 1000000000 )
        {
            // we have to increase seconds by one
            element.time.tv_sec = currentTime.tv_sec + 1;
            element.time.tv_nsec = element.time.tv_nsec % 1000000000;
        }
        else
        {
            // otherwise just set seconds
            element.time.tv_sec = currentTime.tv_sec;
        }


        m_mutex.lock();

        // modified element will replace original one
        // If the element is already removed from the list, the following call does nothing
        m_inputQueue.modify(element);

        m_mutex.unlock();
    }

    this->disconnectBt();
#endif
}

void* BTClassicThread::run_internal(void* arg)
{
    BTClassicThread* thread = (BTClassicThread*)arg;
    thread->run();

    return NULL;
}

/**
 * @brief BTClassicThread::waitForRead checks for pending data to read from the socket and if there is none waits for the amount of time specified by timeout.
 * @param timeout specifies the amount of time to wait
 * @return true if there is pending data to read or it was interrupted by a signal (which most of the time means data to write), false if timeout has expired
 */
bool BTClassicThread::readWait(timespec timeout)
{
    struct pollfd fd;
    fd.fd = m_socket;
    fd.events = POLLIN;
    fd.revents = 0;

    timespec local_timeout = timeout;
    int ret = ppoll(&fd, 1, &local_timeout, NULL);
    if(ret == -1)
    {
        // we have been interrupted by a signal
        return true;
    }
    else if(ret == 0)
    {
        // timeout has expired
        return false;
    }
    else
    {
        // there is data to read
        return true;
    }
}

void BTClassicThread::packetHandler(char* buffer, unsigned int length)
{
    // parse the packet
    while(length != 0)
    {
        unsigned char type = (buffer[0] & 0xE0) >> 5;
        unsigned char packetLength = (buffer[0] & 0x1F);

        pi_assert(packetLength <= length);

        unsigned char seq = buffer[1];
        unsigned char seqAck = buffer[2];

        if(type == BTPacketType::I2C) // I2C packet
        {
            BTI2CPacket packet;
            if( !packet.parse(buffer + 3, packetLength - 3) )
            {
                LOG_WARN(Logger::BT, "Parsing packet failed");
                return;
            }

            // now lets see if there is a callback function for this sequence number
            for(std::list<PacketSeq>::iterator it = m_listSeq.begin(); it != m_listSeq.end(); it++)
            {
                if(it->seq == seqAck)
                {
                    // we have found our callback function
                    packet.callbackFunc = it->callbackFunc;

                    m_listSeq.erase(it);

                    break;
                }
            }

            // if we have found a valid callback function, execute it
            if(packet.callbackFunc)
                packet.callbackFunc(this, &packet);
        }
        else if(type == BTPacketType::GPIO) // gpio packet
        {
            bool req = (buffer[3] & 0x80) != 0;
            bool err = (buffer[3] & 0x40) != 0;
            unsigned char pinGroup = buffer[3] & 0x1F;

            // error occurred, ignoring packet
            if(err)
                return;
            // TODO: the lines above have to be removed

            // now lets see if anything has changed, and if yes, inform the respective object
            for(std::list<GPInput>::iterator it = m_listGPInput.begin(); it != m_listGPInput.end(); it++)
            {
                if( (*it).pinGroup == pinGroup)
                {
                    (*it).hw->setValue( (buffer[4] & (1 << (*it).pin)) != 0);
                }
            }
        }

        buffer += packetLength;
        length -= packetLength;
    }
}

void BTClassicThread::sendGPUpdateRequest(unsigned int pinGroup, BTThread*)
{
    char buffer[5];
    buffer[0] = 1 << 5 | 5;
    buffer[1] = this->seqInc();
    buffer[2] = 0xFF;
    buffer[3] = 1 << 7 | pinGroup;
    buffer[4] = 0xFF;

    this->send(buffer, 5);
}

void BTClassicThread::addGPInput(HWInputButtonBtGPIO *hw)
{
    GPInput gp;
    gp.hw = hw;
    gp.pin = hw->getPin();
    gp.pinGroup = hw->getPinGroup();

    m_listGPInput.push_back(gp);

    // As we have a new general purpose input we should request a status update so that our inputs are consistent
    this->addOutput(std::bind(&BTClassicThread::sendGPUpdateRequest, this, gp.pinGroup, std::placeholders::_1));
}

void BTClassicThread::removeGPInput(HWInputButtonBtGPIO *hw)
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

/**
 * @brief BTClassicThread::addOutput adds an output to this thread.
 * The function specified by func will be executed as soon as it is on top of the queue.
 * So there might be a little delay between this function call and the execution of the function.
 * @param func
 */
void BTClassicThread::addOutput(std::function<void (BTThread*)> func)
{
    OutputElement el;
    el.func = func;

    m_mutex.lock();
    m_outputQueue.push(el);
    m_mutex.unlock();

    // deliver signal to thread to wake it up
    if(m_thread != 0)
        pthread_kill(m_thread, SIGUSR1);
}

/**
 * @brief BTClassicThread::sendI2CPackets actually sends the amount of packets given by num over Bluetooth.
 * Attention: This method can only be called by the Bluetooth thread. If it is called by any other thread undefined behaviour may result!
 * Each packet given by packets is sent in a seperate bluetooth packet,
 * altough it would be possible to send more than one packet in one bluetooth packet, we do not use it right now.
 * @param packets
 * @param num
 */
void BTClassicThread::sendI2CPackets(BTI2CPacket *packets, unsigned int num)
{
    // the following code sends multiple i2c packets in one big l2cap packet. Our bluetooth board currently does not support this

    //    unsigned int totalSize = 0;

    //    // first calculate the size of the whole packet
    //    for(unsigned int i = 0; i < num; i++)
    //    {
    //        totalSize = packets[i].size() + 3;
    //    }

    //    // now allocate a buffer of the required size
    //    char* buffer = (char*)malloc(totalSize * sizeof(char));

    //    char* runningBuffer = buffer;
    //    unsigned int runningSize = totalSize;
    //    for(unsigned int i = 0; i < num; i++)
    //    {
    //        unsigned short size = packets[i].size();
    //        unsigned char seq = this->seqInc();

    //        runningBuffer[0] = BTPacketType::I2C << 5 | (size + 3);
    //        runningBuffer[1] = seq;
    //        runningBuffer[2] = 0xFF;

    //        runningBuffer += 3;
    //        runningSize -= 3;

    //        packets[i].assemble(runningBuffer, runningSize);

    //        runningBuffer += size;
    //        runningSize -= size;

    //        // add the callback function (if any) to the list of callback functions for later matching of the response
    //        if(packets[i].callbackFunc)
    //        {
    //            PacketSeq seqCallback;
    //            seqCallback.seq = seq;
    //            seqCallback.callbackFunc = packets[i].callbackFunc;

    //            m_listSeq.push_back(seqCallback);
    //        }
    //    }

    //    pi_assert(runningSize == 0);

    //    // send the buffer
    //    this->send(buffer, totalSize);

    //    // we dont need it anymore, free it
    //    free(buffer);


    // the following code sends each i2c packet on a seperate l2cap packet
    for(unsigned int i = 0; i < num; i++)
    {
        unsigned int totalSize = packets[i].size() + 3;
        char* buffer = (char*)malloc(totalSize);

        unsigned short size = packets[i].size();
        unsigned char seq = this->seqInc();

        buffer[0] = BTPacketType::I2C << 5 | (size + 3);
        buffer[1] = seq;
        buffer[2] = 0xFF;

        packets[i].assemble(buffer + 3, totalSize - 3);


        PacketSeq seqCallback;
        seqCallback.seq = seq;

        // add the callback function (if any) to the list of callback functions for later matching of the response
        if(packets[i].callbackFunc)
            seqCallback.callbackFunc = packets[i].callbackFunc;

        m_listSeq.push_back(seqCallback);

        this->send(buffer, totalSize);

        // we dont need it anymore, free it
        free(buffer);
    }
}

/**
 * @brief BTClassicThread::send sends the packet given by buffer over bluetooth
 * @param buffer
 * @param length
 */
void BTClassicThread::send(char *buffer, unsigned int length)
{
    while(m_listSeq.size() > 5)
        readBlocking();

    int ret = write(m_socket, buffer, length);
    if(ret != length)
    {
        // TODO: check if we need to reconnect
        perror("Write to bluetooth socket has failed");

        if(errno == ENOTCONN)
        {
            this->reconnectBt();
        }
    }
}


BTI2CPacket::BTI2CPacket()
{
    this->slaveAddress = -1;
    this->read = 0;
    this->request = 1;
    this->error = 0;

    this->commandLength = 0;
    this->commandBuffer = NULL;

    this->readLength = 0;
    this->readBuffer = NULL;
}

BTI2CPacket::~BTI2CPacket()
{
    if(commandBuffer != NULL)
        free(commandBuffer);

    if(readBuffer != NULL)
        free(readBuffer);
}

/**
 * @brief BTI2CPacket::size
 * @return returns the size this BTI2CPacket would have if it were assembled
 */
unsigned int BTI2CPacket::size() const
{
    if(this->readBuffer != NULL)
        return 2 + this->readLength + this->commandLength;
    else
        return 2 + this->commandLength;
}

/**
 * @brief BTI2CPacket::assemble assumes that the param buf points to an empty buffer element where a new I2CPacket should be placed
 * @param buf
 * @param length
 */
void BTI2CPacket::assemble(char* buf, unsigned int length)
{
    pi_assert(length >= this->size());

    buf[0] = this->read << 7 | this->slaveAddress;

    if(this->read)
    {
        // the packet is a read
        buf[1] = this->request << 7 | this->error << 6 | this->readLength;

        if(this->commandLength != 0)
            memcpy(&buf[2], this->commandBuffer, this->commandLength);

        if(!this->request)
        {
            // the packet is a response, therefore we need the read buffer
            memcpy(&buf[2 + this->commandLength], this->readBuffer, this->readLength);
        }
    }
    else
    {
        // the packet is a write
        buf[1] = this->request << 7 | this->error << 6;
        memcpy(&buf[2], this->commandBuffer, this->commandLength);
    }
}

/**
 * @brief BTI2CPacket::parse parses the packet given by buf. If the packet contains an error, this method returns false.
 * @param buf the buffer containing the packet.
 * @param i2cSize the size of the buffer buf
 * @return returns false on error, true otherwise
 */
bool BTI2CPacket::parse(char *buf, unsigned int i2cSize)
{
    if(i2cSize <= 2) // if a packet is smaller than or equal to 2 byte it cannot contain any information at all
        return false;

    this->read = (buf[0] & 0x80) != 0;
    this->slaveAddress = buf[0] & 0x7F;

    this->request = (buf[1] & 0x80) != 0;
    this->error = (buf[1] & 0x40) != 0;

    if(this->read)
    {
        // the packet is a read
        this->readLength = buf[1] & 0x1F;


        if(this->readLength + 2 > i2cSize) // the packet is too small, it cannot contain a valid response
            return false;

        this->commandLength = i2cSize - 2 - (this->request ? 0 : this->readLength);
        this->commandBuffer = (char*)malloc(this->commandLength);
        memcpy(this->commandBuffer, &buf[2], this->commandLength);

        if(!this->request)
        {
            // the packet is a response, we have to copy the read buffer
            this->readBuffer = (char*)malloc(this->readLength);
            memcpy(this->readBuffer, &buf[2 + this->commandLength], this->readLength);
        }
    }
    else
    {
        // the packet is a write
        this->commandLength = i2cSize - 2;
        this->commandBuffer = (char*)malloc(this->commandLength);
        memcpy(this->commandBuffer, &buf[2], this->commandLength);
    }

    return true;
}
