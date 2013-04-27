
#include "hw/HWInput.h"
#include "hw/HWInputListener.h"

#include "hw/HWInputButton.h"
#include "hw/HWInputFader.h"

#include "util/Debug.h"

#include <QDomDocument>

HWInput::HWInput()
{
    m_errorLevel = OK;
    m_bOverride = false;
}

HWInput::~HWInput()
{
}

bool HWInput::init(ConfigManager *config)
{
    return true;
}

void HWInput::deinit(ConfigManager *config)
{

}

HWInput* HWInput::load(QDomElement *root)
{
    QDomElement elem = root->firstChildElement();

    HWInput* hw = NULL;
    std::string name;

    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("type") == 0)
        {
            pi_assert(hw == NULL);

            HWInputType type = StringToHWInputType(elem.text().toStdString());

            switch(type)
            {
            case Button:
                hw = HWInputButton::load(root);
                break;
            case Fader:
                hw = HWInputFader::load(root);
                break;
            }
        }
        else if(elem.tagName().toLower().compare("name") == 0)
        {
            name = elem.text().toStdString();
        }

        elem = elem.nextSiblingElement();
    }

    if(hw != NULL)
    {
        if(name.empty())
        {
            pi_warn("Could not create new input, name was empty");

            delete hw;
            return NULL;
        }

        hw->setName(name);
    }

    return hw;
}

QDomElement HWInput::save(QDomElement *root, QDomDocument *document)
{
    QDomElement input = document->createElement("input");

    QDomElement name = document->createElement("name");
    QDomText nameText = document->createTextNode(this->getName().c_str());
    name.appendChild(nameText);

    input.appendChild(name);

    root->appendChild(input);

    return input;
}

void HWInput::setOverride(bool b)
{
    m_bOverride = b;
}

bool HWInput::getOverride() const
{
    return m_bOverride;
}

/**
 * @brief HWInput::registerInputListener registers the object listener for the onInputChanged event.
 * This method calls the event handler immediately once after registration
 * @param listener
 */
void HWInput::registerInputListener(HWInputListener *listener)
{
    // TODO: check if already registered

    m_listListeners.push_back(listener);

    listener->onInputChanged(this);
    listener->onInputErrorChanged(this);
}

/**
 * @brief HWInput::registerInputListener unregisters an object for the onInputChanged event.
 * @param listener
 */
void HWInput::unregisterInputListener(HWInputListener *listener)
{
    // TODO: check if registered, otherwise raise error

    m_listListeners.remove(listener);
}

/**
 * @brief HWInput::inputChanged calls all registered inputListener, so that they can detect that this input has changed.
 */
void HWInput::inputChanged()
{
    for(std::list<HWInputListener*>::iterator it = m_listListeners.begin(); it != m_listListeners.end(); it++)
    {
        (*it)->onInputChanged(this);
    }
}

/**
 * @brief HWInput::inputChanged calls all registered inputListener, so that they can detect that the error level for this input has changed.
 */
void HWInput::errorLevelChanged()
{
    for(std::list<HWInputListener*>::iterator it = m_listListeners.begin(); it != m_listListeners.end(); it++)
    {
        (*it)->onInputErrorChanged(this);
    }
}

void
HWInput::handleError(bool errorOccurred, bool catastrophic)
{
    ErrorLevel newLevel = m_errorLevel;
    if(errorOccurred)
    {
        if(catastrophic)
            newLevel = Failure;
        else if(m_errorLevel == OK)
            newLevel = Warning;
        else if(m_errorLevel == Warning)
            newLevel = Critical;
    }
    else
    {
        if(m_errorLevel == Failure)
            newLevel = Critical;
        else if(m_errorLevel == Critical)
            newLevel = Warning;
        else if(m_errorLevel == Warning)
            newLevel = OK;
    }

    if(newLevel != m_errorLevel)
    {
        m_errorLevel = newLevel;
        this->errorLevelChanged();
    }
}

std::string HWInput::HWInputTypeToString(HWInputType type)
{
    switch(type)
    {
    case Button:
        return "Button";
        break;
    case Fader:
        return "Fader";
        break;
    default:
        pi_warn("Invalid type");
        return "";
        break;
    }
}

HWInput::HWInputType HWInput::StringToHWInputType(std::string str)
{
    const char* cstr = str.c_str();
    if( strcasecmp(cstr, "button") == 0)
        return Button;
    else if( strcasecmp(cstr, "fader") == 0)
        return Fader;
    else
        return EINVALID;
}
