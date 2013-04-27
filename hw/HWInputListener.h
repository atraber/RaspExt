#ifndef HWINPUTLISTENER_H
#define HWINPUTLISTENER_H

class HWInput;

// Interface for input events
class HWInputListener
{
public:
    /**
     * @brief onInputChanged gets called from the input object, if a value has changed
     * @param hw the input object which generated the event
     */
    virtual void onInputChanged(HWInput* hw) {};
    /**
     * @brief onInputErrorChanged gets called from the input object, if the error level has changed
     * @param hw the input object which generated the event
     */
    virtual void onInputErrorChanged(HWInput* hw) {};
};

#endif // HWINPUTLISTENER_H
