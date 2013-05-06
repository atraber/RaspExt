
#include "hw/HWInputButtonBtGPIO.h"
#include "ConfigManager.h"
#include "hw/BTThread.h"
#include "util/Debug.h"

#include <QDomElement>

HWInputButtonBtGPIO::HWInputButtonBtGPIO()
{
    m_pinGroup = -1;
    m_pin = -1;
    m_btThread = NULL;
}

HWInput* HWInputButtonBtGPIO::load(QDomElement *root)
{
    HWInputButtonBtGPIO* hw = new HWInputButtonBtGPIO();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if( elem.tagName().toLower().compare("pingroup") == 0 )
        {
            hw->m_pinGroup = elem.text().toInt();
        }
        else if( elem.tagName().toLower().compare("pin") == 0 )
        {
            hw->m_pin = elem.text().toInt();
        }
        else if( elem.tagName().toLower().compare("btboard") == 0 )
        {
            hw->m_btName = elem.text().toStdString();
        }

        elem = elem.nextSiblingElement();
    }

    // check for invalid parameters
    if( hw->m_pin > 7 || hw->m_pinGroup > 4 || hw->m_btName.empty())
    {
        LOG_WARN(Logger::BT, "Invalid bluetooth parameters");
        return NULL;
    }

    return hw;
}

QDomElement HWInputButtonBtGPIO::save(QDomElement* root, QDomDocument* document)
{
    QDomElement input = HWInputButton::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("BtGPIO");
    type.appendChild(typeText);

    input.appendChild(type);

    QDomElement pinGroup = document->createElement("PinGroup");
    QDomText pinGroupText = document->createTextNode(QString::number( m_pinGroup ));
    pinGroup.appendChild(pinGroupText);

    input.appendChild(pinGroup);

    QDomElement pin = document->createElement("Pin");
    QDomText pinText = document->createTextNode(QString::number( m_pin ));
    pin.appendChild(pinText);

    input.appendChild(pin);

    QDomElement btBoard = document->createElement("BtBoard");
    QDomText btBoardText = document->createTextNode( QString::fromStdString( m_btName ) );
    btBoard.appendChild(btBoardText);

    input.appendChild(btBoard);

    return input;
}

bool HWInputButtonBtGPIO::init(ConfigManager* config)
{
    m_btThread = config->getBTThreadByName(m_btName);

    // if we cannot find the bluetooth board, it does not exist and we should fail
    if(m_btThread == NULL)
    {
        LOG_WARN(Logger::BT, "Bluetooth board does not exist");

        return false;
    }

    m_btThread->addGPInput(this);

    return true;
}

void HWInputButtonBtGPIO::deinit(ConfigManager* config)
{
    if(m_btThread != NULL)
    {
        m_btThread->removeGPInput(this);

        m_btThread = NULL;
    }
}

void HWInputButtonBtGPIO::setValue(bool value)
{
    // only do something if the value has really changed
    if(m_value != value)
    {
        m_value = value;
        this->inputChanged();
    }
}
