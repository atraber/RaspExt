
#include "ui/OutputFrame.h"
#include "util/Debug.h"


OutputFrame::OutputFrame(HWOutput *hw)
{
    m_hw = hw;
    QObject::connect(this, SIGNAL(onOutputChangedSignal()), this, SLOT(onOutputChangedGUI()), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(onOutputErrorChangedSignal()), this, SLOT(onOutnputErrorChangedGUI()), Qt::QueuedConnection);
}

void OutputFrame::onOutputChanged(HWOutput *hw)
{
    pi_assert(hw == m_hw);
    emit onOutputChangedSignal();
}

void OutputFrame::onOutputChangedGUI()
{

}

void OutputFrame::onOutputErrorChanged(HWOutput *hw)
{
    pi_assert(hw == m_hw);
    emit onOutnputErrorChangedGUI();
}

void OutputFrame::onOutnputErrorChangedGUI()
{
    switch(m_hw->getErrorLevel())
    {
    case HWOutput::OK:
        this->setStyleSheet("background-color: rgb(213, 255, 213);");
        break;

    case HWOutput::Warning:
        this->setStyleSheet("background-color: rgb(252, 255, 193);");
        break;

    case HWOutput::Critical:
        this->setStyleSheet("background-color: rgb(255, 170, 0);");
        break;

    case HWOutput::Failure:
        this->setStyleSheet("background-color: rgb(255, 152, 101);");
        break;
    }
}
