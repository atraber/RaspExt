#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "hw/HWInput.h"
#include "hw/HWOutput.h"
#include "script/Variable.h"
#include "hw/Config.h"

#include <list>
#include <QFrame>

class MainWindow;
class GPIOInterruptThread;
class I2CThread;
class BTThread;
class RuleTimerThread;
class Script;
class SoundManager;

/**
 * @brief The ConfigManager class manages the configuration and hardware related stuff.
 * In addition, it controlls the currently selected script.
 * The ConfigManager class is deeply linked with MainWindow, as for example MainWindow gets called every time, an input, output or variable gets added.
 * MainWindow then creates the corresponding GUI-object and links it to the HW-object.
 */
class ConfigManager
{
public:
    enum ScriptState
    {
        Inactive,
        Active,
        Paused
    };

    ConfigManager(MainWindow* win);
    ~ConfigManager();

    void init();
    void deinit();

    bool load(std::string name);
    void clear();
    std::string getName() const { return m_config.getName();}

    ScriptState getActiveScriptState() const { return m_scriptState;}
    void stopActiveScript();
    void pauseActiveScript();
    void continueActiveScript();

    void setActiveScript(Script* script);
    Script* getActiveScript() const { return m_activeScript;}

    std::list<HWInput*> getInputList() const;
    std::list<HWOutput*> getOutputList() const;

    HWInput* getInputByName(std::string str);
    HWOutput* getOutputByName(std::string str);
    Variable* getVariableByName(std::string str);

    GPIOInterruptThread* getGPIOThread();
    I2CThread* getI2CThread();
    BTThread* getBTThreadByName(std::string str);
    BTThread* getBTThreadByAddr(std::string addr);
    RuleTimerThread* getRuleTimerThread();
    SoundManager* getSoundManager() const { return m_soundManager;}

private:
    bool addVariable(Variable* var);

    void removeVariable(Variable* var);

    ScriptState m_scriptState;
    Script* m_activeScript;

    MainWindow* m_mainWindow;
    Config m_config;
    std::list<Variable*> m_listVariable;

    GPIOInterruptThread* m_gpioThread;
    I2CThread* m_i2cThread;
    RuleTimerThread* m_ruleTimer;

    SoundManager* m_soundManager;
};

#endif // CONFIGMANAGER_H
