#ifndef INPUTFRAME_H
#define INPUTFRAME_H

#include <QFrame>

#include "hw/HWInput.h"
#include "hw/HWInputListener.h"

class InputFrame : public QFrame, public HWInputListener
{
    Q_OBJECT
public:
    InputFrame(HWInput* hw);

    HWInput* getHW() const { return m_hw;}

signals:
    void onInputChangedSignal();
    void onInputErrorChangedSignal();

protected slots:
    virtual void onInputChangedGUI();
    virtual void onInputErrorChangedGUI();

private:
    void onInputChanged(HWInput *hw);
    void onInputErrorChanged(HWInput *hw);

    HWInput* m_hw;
    bool m_GUIUpdatePending;
};

#endif // INPUTFRAME_H
