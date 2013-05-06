
#include "hw/HWOutputStepperBt.h"
#include "hw/BTThread.h"
#include "ConfigManager.h"
#include "util/Debug.h"

HWOutput* HWOutputStepperBt::load(QDomElement *root)
{
    HWOutputStepperBt* hw = new HWOutputStepperBt();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if( elem.tagName().toLower().compare("slaveaddress") == 0 )
        {
            hw->m_slaveAddress = elem.text().toInt();
        }
        else if( elem.tagName().toLower().compare("btboard") == 0 )
        {
            hw->m_btName = elem.text().toStdString();
        }
        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_slaveAddress > 127 || hw->m_slaveAddress < 0 || hw->m_btName.empty())
    {
        LOG_WARN(Logger::BT, "Invalid parameters");
        return NULL;
    }

    return hw;
}

QDomElement HWOutputStepperBt::save(QDomElement *root, QDomDocument *document)
{
    QDomElement output = HWOutputStepper::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("Bt");
    type.appendChild(typeText);

    output.appendChild(type);

    QDomElement btBoard = document->createElement("BtBoard");
    QDomText btBoardText = document->createTextNode( QString::fromStdString( m_btName ) );
    btBoard.appendChild(btBoardText);

    output.appendChild(btBoard);

    QDomElement slaveAddr = document->createElement("SlaveAddress");
    QDomText slaveAddrText = document->createTextNode(QString::number( m_slaveAddress ));
    slaveAddr.appendChild(slaveAddrText);

    output.appendChild(slaveAddr);

    return output;
}

void HWOutputStepperBt::init(ConfigManager *config)
{
    m_btThread = config->getBTThreadByName(m_btName);

    // if we cannot find the bluetooth board, it does not exist and we should fail
    if(m_btThread == NULL)
    {
        LOG_WARN(Logger::BT, "Bluetooth board does not exist");

        return;
    }

    m_btThread->addInput(this, 1); // 1 Hertz polling frequency
}

void HWOutputStepperBt::deinit(ConfigManager* config)
{
    if(m_btThread != NULL)
    {
        m_btThread->removeInput(this);

        m_btThread = NULL;
    }
}

void HWOutputStepperBt::testBemf()
{
    if(m_btThread != NULL)
        m_btThread->addOutput(std::bind(&HWOutputStepperBt::testBemfI2C, this, std::placeholders::_1));
}

void HWOutputStepperBt::softStop(bool override)
{
    if(override != this->getOverride())
        return;

    if(m_btThread != NULL)
        m_btThread->addOutput(std::bind(&HWOutputStepperBt::softStopI2C, this, std::placeholders::_1, override));
}

void HWOutputStepperBt::setPosition(short position, bool override)
{
    if(override != this->getOverride())
        return;

    if(m_btThread != NULL)
        m_btThread->addOutput(std::bind(&HWOutputStepperBt::setPositionI2C, this, std::placeholders::_1, position, override));
}

void HWOutputStepperBt::setDualPosition(short position1, short position2, unsigned char vmin, unsigned char vmax, bool override)
{
    if(override != this->getOverride())
        return;

    if(m_btThread != NULL)
        m_btThread->addOutput(std::bind(&HWOutputStepperBt::setDualPositionI2C,
                                     this,
                                     std::placeholders::_1,
                                     position1,
                                     position2,
                                     vmin,
                                     vmax,
                                     override));
}

void HWOutputStepperBt::resetPosition(bool override)
{
    if(override != this->getOverride())
        return;

    if(m_btThread != NULL)
        m_btThread->addOutput(std::bind(&HWOutputStepperBt::resetPositionI2C, this, std::placeholders::_1, override));
}

void HWOutputStepperBt::runVelocity(bool override)
{
    if(override != this->getOverride())
        return;

    if(m_btThread != NULL)
        m_btThread->addOutput(std::bind(&HWOutputStepperBt::runVelocityI2C, this, std::placeholders::_1, override));
}

void HWOutputStepperBt::setParam(Param param, bool override)
{
    if(override != this->getOverride())
        return;

    if(m_btThread != NULL)
        m_btThread->addOutput(std::bind(&HWOutputStepperBt::setParamI2C, this, std::placeholders::_1, param, override));
}

void HWOutputStepperBt::refreshFullStatus()
{
    if(m_btThread != NULL)
        m_btThread->addOutput(std::bind(&HWOutputStepperBt::poll, this, std::placeholders::_1));
}

void HWOutputStepperBt::poll(BTThread *btThread)
{
    BTI2CPacket packets[2];

    // GetFullStatus1
    packets[0].read = 1;
    packets[0].request = 1;
    packets[0].slaveAddress = m_slaveAddress;
    packets[0].commandLength = 1;
    packets[0].commandBuffer = (char*)malloc(packets[0].commandLength);

    packets[0].commandBuffer[0] = 0x81; // write GetFullStatus1

    packets[0].readLength = 8;
    packets[0].callbackFunc = std::bind(&HWOutputStepperBt::getFullStatus1Callback, this, std::placeholders::_1, std::placeholders::_2);

    // GetFullStatus2
    packets[1].read = 1;
    packets[1].request = 1;
    packets[1].slaveAddress = m_slaveAddress;
    packets[1].commandLength = 1;
    packets[1].commandBuffer = (char*)malloc(packets[0].commandLength);

    packets[1].commandBuffer[0] = 0xFC; // write GetFullStatus2

    packets[1].readLength = 8;
    packets[1].callbackFunc = std::bind(&HWOutputStepperBt::getFullStatus2Callback, this, std::placeholders::_1, std::placeholders::_2);

    if(m_btThread != NULL)
        m_btThread->sendI2CPackets(packets, 2);
}

void HWOutputStepperBt::getFullStatus1Callback(BTThread* btThread, BTI2CPacket* packet)
{
    if(packet->error || !packet->read || packet->request)
        return;

    char* buf = packet->readBuffer;

    // handle GetFullStatus1
    m_fullStatus.irun = (buf[1] & 0xF0) >> 4;
    m_fullStatus.ihold = buf[1] & 0x0F;
    m_fullStatus.vmax = (buf[2] & 0xF0) >> 4;
    m_fullStatus.vmin = (buf[2] & 0x0F);
    m_fullStatus.accShape = (buf[3] & 0x80) >> 7;
    m_fullStatus.stepMode = (buf[3] & 0x60) >> 5;
    m_fullStatus.shaft = (buf[3] & 0x10) >> 4;
    m_fullStatus.acc = (buf[3] & 0x0F);
    m_fullStatus.vddReset = (buf[4] & 0x80) >> 7;
    m_fullStatus.stepLoss = (buf[4] & 0x40) >> 6;
    m_fullStatus.elDef = (buf[4] & 0x20) >> 5;
    m_fullStatus.uv2 = (buf[4] & 0x10) >> 4;
    m_fullStatus.tsd = (buf[4] & 0x08) >> 3;
    m_fullStatus.tw = (buf[4] & 0x04) >> 2;
    m_fullStatus.tinfo = (buf[4] & 0x03);
    m_fullStatus.motion = (buf[4] & 0xE0) >> 5;
    m_fullStatus.esw = (buf[4] & 0x10) >> 4;
    m_fullStatus.ovc1 = (buf[4] & 0x08) >> 3;
    m_fullStatus.ovc2 = (buf[4] & 0x04) >> 2;
    m_fullStatus.stall = (buf[4] & 0x02) >> 1;
    m_fullStatus.cpfail = (buf[4] & 0x01);
    m_fullStatus.absoluteThreshold = (buf[7] & 0xF0) >> 4;
    m_fullStatus.deltaThreshold = (buf[7] & 0x0F);

    HWOutputStepper::refreshFullStatus();
}

void HWOutputStepperBt::getFullStatus2Callback(BTThread* btThread, BTI2CPacket* packet)
{
    if(packet->error || !packet->read || packet->request)
        return;

    char* buf = packet->readBuffer;

    // handle GetFullStatus2
    m_fullStatus.actualPosition = buf[1] << 8 | buf[2];
    m_fullStatus.targetPosition = buf[3] << 8 | buf[4];
    m_fullStatus.securePosition = (buf[6] & 0x07) << 8 | buf[5];
    m_fullStatus.fs2StallEnabled = (buf[6] & 0xE0) >> 5;
    m_fullStatus.dc100 = (buf[6] & 0x08) >> 3;
    m_fullStatus.absolutStall = (buf[7] & 0x80) >> 7;
    m_fullStatus.deltaStallLow = (buf[7] & 0x40) >> 6;
    m_fullStatus.deltaStallHigh = (buf[7] & 0x20) >> 5;
    m_fullStatus.minSamples = (buf[7] & 0x1C) >> 2;
    m_fullStatus.dc100StallEnable = (buf[7] & 0x02) >> 1;
    m_fullStatus.PWMJitterEnable = (buf[7] & 0x01);

    HWOutputStepper::refreshFullStatus();
}

void HWOutputStepperBt::testBemfI2C(BTThread *btThread)
{
    BTI2CPacket packet;

    packet.read = 0;
    packet.request = 1;
    packet.slaveAddress = m_slaveAddress;
    packet.commandLength = 1;
    packet.commandBuffer = (char*)malloc(packet.commandLength);

    packet.commandBuffer[0] = 0x9F; // send command byte (see page 51 of datasheet)

    if(m_btThread != NULL)
        m_btThread->sendI2CPackets(&packet, 1);
}

void HWOutputStepperBt::softStopI2C(BTThread *btThread, bool override)
{
    HWOutputStepper::softStop(override);

    BTI2CPacket packet;

    packet.read = 0;
    packet.request = 1;
    packet.slaveAddress = m_slaveAddress;
    packet.commandLength = 1;
    packet.commandBuffer = (char*)malloc(packet.commandLength);

    packet.commandBuffer[0] = 0x8F; // send command byte (see page 44 of datasheet)

    if(m_btThread != NULL)
        m_btThread->sendI2CPackets(&packet, 1);
}

void HWOutputStepperBt::setPositionI2C(BTThread *btThread, short position, bool override)
{
    HWOutputStepper::setPosition(position, override);

    BTI2CPacket packet;

    packet.read = 0;
    packet.request = 1;
    packet.slaveAddress = m_slaveAddress;
    packet.commandLength = 5;
    packet.commandBuffer = (char*)malloc(packet.commandLength);

    // send command byte (see page 44 of datasheet)
    packet.commandBuffer[0] = 0x8B;
    packet.commandBuffer[1] = 0xFF;
    packet.commandBuffer[2] = 0xFF;
    packet.commandBuffer[3] = (position & 0xFF00) >> 8;
    packet.commandBuffer[4] = (position & 0x00FF);

    if(m_btThread != NULL)
        m_btThread->sendI2CPackets(&packet, 1);
}

void HWOutputStepperBt::setDualPositionI2C(BTThread *i2cThread,
                                            short position1,
                                            short position2,
                                            unsigned char vmin,
                                            unsigned char vmax,
                                            bool override)
{
    HWOutputStepper::setDualPosition(position1, position2, vmin, vmax, override);

    BTI2CPacket packet;

    packet.read = 0;
    packet.request = 1;
    packet.slaveAddress = m_slaveAddress;
    packet.commandLength = 8;
    packet.commandBuffer = (char*)malloc(packet.commandLength);

    // send command byte (see page 44 of datasheet)
    packet.commandBuffer[0] = 0x88;
    packet.commandBuffer[1] = 0xFF;
    packet.commandBuffer[2] = 0xFF;
    packet.commandBuffer[3] = vmax << 4 | vmin;
    packet.commandBuffer[4] = (position1 & 0xFF00) >> 8;
    packet.commandBuffer[5] = (position1 & 0x00FF);
    packet.commandBuffer[6] = (position2 & 0xFF00) >> 8;
    packet.commandBuffer[7] = (position2 & 0x00FF);

    if(m_btThread != NULL)
        m_btThread->sendI2CPackets(&packet, 1);
}

void HWOutputStepperBt::resetPositionI2C(BTThread *btThread, bool override)
{
    HWOutputStepper::resetPosition(override);

    BTI2CPacket packet;

    packet.read = 0;
    packet.request = 1;
    packet.slaveAddress = m_slaveAddress;
    packet.commandLength = 1;
    packet.commandBuffer = (char*)malloc(packet.commandLength);

    // send command byte (see page 44 of datasheet)
    packet.commandBuffer[0] = 0x86;

    m_btThread->sendI2CPackets(&packet, 1);
}

void HWOutputStepperBt::runVelocityI2C(BTThread *btThread, bool override)
{
    HWOutputStepper::runVelocity(override);

    BTI2CPacket packet;

    packet.read = 0;
    packet.request = 1;
    packet.slaveAddress = m_slaveAddress;
    packet.commandLength = 1;
    packet.commandBuffer = (char*)malloc(packet.commandLength);

    // send command byte (see page 44 of datasheet)
    packet.commandBuffer[0] = 0x97;

    if(m_btThread != NULL)
        m_btThread->sendI2CPackets(&packet, 1);
}

void HWOutputStepperBt::setParamI2C(BTThread *btThread, Param param, bool override)
{
    HWOutputStepper::setParam(param, override);

    // as we need to set EVERY value for these I2C commands,
    // we take the current value from the last FullStatus update and change those specified in param

    BTI2CPacket packets[2];

    packets[0].read = 0;
    packets[0].request = 1;
    packets[0].slaveAddress = m_slaveAddress;
    packets[0].commandLength = 8;
    packets[0].commandBuffer = (char*)malloc(packets[0].commandLength);

    unsigned char* buf = (unsigned char*)packets[0].commandBuffer;

    // send SetStallParam command (see page 49 of datasheet)
    buf[0] = 0x96;
    buf[1] = 0xFF;
    buf[2] = 0xFF;
    buf[3] = (param.irunSet ? param.irun : m_fullStatus.irun) << 4; // irun
    buf[3] = buf[3] | (param.iholdSet ? param.ihold : m_fullStatus.ihold); // ihold
    buf[4] = (param.vmaxSet ? param.vmax : m_fullStatus.vmax) << 4; // vmax
    buf[4] = buf[4] | (param.vminSet ? param.vmin : m_fullStatus.vmin); // vmin
    buf[5] = (param.minSamplesSet ? param.minSamples : m_fullStatus.minSamples) << 5; // minSamples
    buf[5] = buf[5] | ( (param.shaftSet ? param.shaft : m_fullStatus.shaft) << 4 ); // shaft
    buf[5] = buf[5] | (param.accSet ? param.acc : m_fullStatus.acc); // acc
    buf[6] = (param.absoluteThresholdSet ? param.absoluteThreshold : m_fullStatus.absoluteThreshold) << 4; // absoluteThreshold
    buf[6] = buf[6] | (param.deltaThresholdSet ? param.deltaThreshold : m_fullStatus.deltaThreshold); // deltaThreshold
    buf[7] = (param.fs2StallEnabledSet ? param.fs2StallEnabled : m_fullStatus.fs2StallEnabled) << 5; // fs2StallEnabled
    buf[7] = buf[7] | ( (param.accShapeSet ? param.accShape : m_fullStatus.accShape) << 4 ); // accShape
    buf[7] = buf[7] | ( (param.stepModeSet ? param.stepMode : m_fullStatus.stepMode) << 2 ); // stepMode
    buf[7] = buf[7] | ( (param.dc100StallEnableSet ? param.dc100StallEnable : m_fullStatus.dc100StallEnable) << 1 ); // dc100StallEnable
    buf[7] = buf[7] | (param.PWMJitterEnableSet ? param.PWMJitterEnable : m_fullStatus.PWMJitterEnable); // PWMJitterEnable

    packets[1].read = 0;
    packets[1].request = 1;
    packets[1].slaveAddress = m_slaveAddress;
    packets[1].commandLength = 8;
    packets[1].commandBuffer = (char*)malloc(packets[1].commandLength);

    buf = (unsigned char*)packets[1].commandBuffer;

    // send SetMotorParam command (see page 49 of datasheet)
    buf[0] = 0x89;
    buf[1] = 0xFF;
    buf[2] = 0xFF;
    buf[3] = (param.irunSet ? param.irun : m_fullStatus.irun) << 4; // irun
    buf[3] = buf[3] | (param.iholdSet ? param.ihold : m_fullStatus.ihold); // ihold
    buf[4] = (param.vmaxSet ? param.vmax : m_fullStatus.vmax) << 4; // vmax
    buf[4] = buf[4] | (param.vminSet ? param.vmin : m_fullStatus.vmin); // vmin
    buf[5] = ( (param.securePositionSet ? param.securePosition : m_fullStatus.securePosition) & 0x0700) << 5; // securePosition[10:8]
    buf[5] = buf[5] | ( (param.shaftSet ? param.shaft : m_fullStatus.shaft) << 4 ); // shaft
    buf[5] = buf[5] | (param.accSet ? param.acc : m_fullStatus.acc); // acc
    buf[6] = (param.securePositionSet ? param.securePosition : m_fullStatus.securePosition) & 0x00FF; // securePosition[7:0]
    buf[7] = 1 << 7 | 1 << 5 | 1 << 1;
    buf[7] = buf[7] | (param.PWMfreqSet ? param.PWMfreq : 0) << 6; // PWMfreq
    buf[7] = buf[7] | ( (param.accShapeSet ? param.accShape : m_fullStatus.accShape) << 4 ); // accShape
    buf[7] = buf[7] | ( (param.stepModeSet ? param.stepMode : m_fullStatus.stepMode) << 2 ); // stepMode
    buf[7] = buf[7] | (param.PWMJitterEnableSet ? param.PWMJitterEnable : m_fullStatus.PWMJitterEnable); // PWMJitterEnable

    if(m_btThread != NULL)
        m_btThread->sendI2CPackets(packets, 1);
}
