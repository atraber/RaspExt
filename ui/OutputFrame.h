#ifndef OUTPUTFRAME_H
#define OUTPUTFRAME_H

#include <QFrame>

#include "hw/HWOutput.h"
#include "hw/HWOutputListener.h"

class OutputFrame : public QFrame, public HWOutputListener
{
    Q_OBJECT
public:
    OutputFrame(HWOutput* hw);

    HWOutput* getHW() const { return m_hw;}

signals:
    void onOutputChangedSignal();
    void onOutputErrorChangedSignal();

protected slots:
    virtual void onOutputChangedGUI();
    virtual void onOutputErrorChangedGUI();

private:
    void onOutputChanged(HWOutput* hw);
    void onOutputErrorChanged(HWOutput *hw);

    HWOutput* m_hw;
};


#endif // OUTPUTFRAME_H
