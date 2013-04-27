#ifndef HWOUTPUTLISTENER_H
#define HWOUTPUTLISTENER_H

class HWOutput;

// Interface for output events
class HWOutputListener
{
public:
    /**
     * @brief onOutputChanged gets called from the output object, if a value has changed
     * @param hw the output object which generated the event
     */
    virtual void onOutputChanged(HWOutput* hw) {};
    /**
     * @brief onOutputErrorChanged gets called from the output object, if the error level has changed
     * @param hw the output object which generated the event
     */
    virtual void onOutputErrorChanged(HWOutput* hw) {};
};

#endif // HWOUTPUTLISTENER_H
