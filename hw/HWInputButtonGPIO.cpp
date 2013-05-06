
#include "HWInputButtonGPIO.h"
#include "ConfigManager.h"
#include "hw/GPIOInterruptThread.h"

#include "util/Debug.h"

#include <QDomElement>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "util/Config.h"

HWInputButtonGPIO::HWInputButtonGPIO()
{
    m_fd = -1;
    m_pin = -1;
}

bool HWInputButtonGPIO::init(ConfigManager* config)
{
#ifdef USE_GPIO
    // if the pin number is still -1, we do not have a valid pin number
    pi_assert(m_pin != -1);

    // export it to sysfs
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if(fd < 0)
    {
        LOG_WARN(Logger::Misc, "Could not open sysfs");
        return false;
    }

#define PATH_MAX_LENGTH 100
    char path[PATH_MAX_LENGTH];

    int retval = sprintf(path, "%d", m_pin);
    write(fd, path, retval);
    close(fd);

    // set direction to input
    sprintf(path, "/sys/class/gpio/gpio%d/direction", m_pin);
    fd = open(path, O_WRONLY);
    if(fd < 0)
    {
        LOG_WARN(Logger::Misc, "Could not open sysfs");
        return false;
    }

    // this GPIO port should be an input
    std::string str = "in";
    retval = write(fd, str.c_str(), str.length());
    close(fd);

    if(retval != (int)str.length())
    {
        LOG_WARN(Logger::Misc, "Could not write to sysfs");
        return false;
    }

    // set gpio edge detection to rising and falling
    sprintf(path, "/sys/class/gpio/gpio%d/edge", m_pin);
    fd = open(path, O_WRONLY);
    if(fd < 0)
    {
        LOG_WARN(Logger::Misc, "Could not open sysfs");
        return false;
    }

    // enable edge detection (for interrupts)
    str = "both";
    retval = write(fd, str.c_str(), str.length());
    close(fd);

    if(retval != (int)str.length())
    {
        LOG_WARN(Logger::Misc, "Could not write to sysfs");
        return false;
    }

    sprintf(path, "/sys/class/gpio/gpio%d/value", m_pin);
    m_fd = open(path, O_RDONLY);
    if(m_fd < 0)
    {
        LOG_WARN(Logger::Misc, "Could not open sysfs");
        return false;
    }

    this->handleInterrupt();

    config->getGPIOThread()->addGPIOInterrupt(this);
#endif

    return true;
}

void HWInputButtonGPIO::deinit(ConfigManager* config)
{
#ifdef USE_GPIO
    pi_assert(m_fd >= 0);

    config->getGPIOThread()->removeGPIOInterrupt(this);

    close(m_fd);
    m_fd = -1;
#endif
}

bool HWInputButtonGPIO::handleInterrupt()
{
#ifdef USE_GPIO
    // TODO: check this, I don't like it...
    char buf[3];
    read(m_fd, buf, 3);
    lseek(m_fd, 0, SEEK_SET);

    if(buf[0] == '0')
        m_value = 0;
    else
        m_value = 1;

    this->inputChanged();
#endif

    return true;
}

HWInput* HWInputButtonGPIO::load(QDomElement* root)
{
    HWInputButtonGPIO* hw = new HWInputButtonGPIO();
    QDomElement elem = root->firstChildElement();

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("gpiopin") == 0)
        {
            hw->m_pin = elem.text().toInt();
        }

        elem = elem.nextSiblingElement();
    }

    if(hw->m_pin == -1)
    {
        delete hw;
        return NULL;
    }

    return hw;
}

QDomElement HWInputButtonGPIO::save(QDomElement* root, QDomDocument* document)
{
    QDomElement input = HWInputButton::save(root, document);

    QDomElement type = document->createElement("hwtype");
    QDomText typeText = document->createTextNode("GPIO");
    type.appendChild(typeText);

    input.appendChild(type);

    QDomElement gpiopin = document->createElement("gpiopin");
    QDomText gpiopinText = document->createTextNode(QString::number(m_pin));
    gpiopin.appendChild(gpiopinText);

    input.appendChild(gpiopin);

    return input;
}
