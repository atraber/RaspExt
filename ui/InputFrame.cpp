
#include "ui/InputFrame.h"
#include "util/Debug.h"


InputFrame::InputFrame(HWInput *hw)
{
    m_hw = hw;
    m_GUIUpdatePending = false;
    QObject::connect(this, SIGNAL(onInputChangedSignal()), this, SLOT(onInputChangedGUI()), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(onInputErrorChangedSignal()), this, SLOT(onInputErrorChangedGUI()), Qt::QueuedConnection);
}

void InputFrame::onInputChanged(HWInput *hw)
{
    pi_assert(hw == m_hw);

    if(!m_GUIUpdatePending)
    {
        m_GUIUpdatePending = true;
        emit onInputChangedSignal();
    }
}

void InputFrame::onInputErrorChanged(HWInput *hw)
{
    pi_assert(hw == m_hw);

    emit onInputErrorChangedSignal();
}

void InputFrame::onInputChangedGUI()
{
    m_GUIUpdatePending = false;
}

void InputFrame::onInputErrorChangedGUI()
{
    switch(m_hw->getErrorLevel())
    {
    case HWInput::OK:
        this->setStyleSheet("background-color: rgb(213, 255, 213);");
        break;

    case HWInput::Warning:
        this->setStyleSheet("background-color: rgb(252, 255, 193);");
        break;

    case HWInput::Critical:
        this->setStyleSheet("background-color: rgb(255, 170, 0);");
        break;

    case HWInput::Failure:
        this->setStyleSheet("background-color: rgb(255, 152, 101);");
        break;
    }
}
