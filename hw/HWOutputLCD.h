#ifndef HWOUTPUTLCD_H
#define HWOUTPUTLCD_H

#include "hw/HWOutput.h"

class HWOUTPUTLCD : public HWOutput
{
public:
    HWOutputLCD();

    static HWOutput* load(QDomElement* root);
    virtual QDomElement save(QDomElement* root, QDomDocument* document);

    bool getValue() const;
    void setValue(bool v);
    bool setOverrideValue(bool v);

protected:
    bool m_value;
};


#endif // HWOUTPUTLCD_H
