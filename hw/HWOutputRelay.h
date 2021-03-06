#ifndef HWOUTPUTRELAY_H
#define HWOUTPUTRELAY_H

#include "hw/HWOutput.h"

class HWOutputRelay : public HWOutput
{
public:
    HWOutputRelay();

    static HWOutput* load(QDomElement* root);
    virtual QDomElement save(QDomElement* root, QDomDocument* document);

    bool getValue() const;
    void setValue(bool v);
    bool setOverrideValue(bool v);

protected:
    bool m_value;
};

#endif // HWOUTPUTRELAY_H
