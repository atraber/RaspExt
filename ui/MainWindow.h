#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>

#include "hw/HWInput.h"
#include "hw/HWOutput.h"
#include "script/Variable.h"
#include "ConfigManager.h"

#include "ui/InputFrame.h"
#include "ui/OutputFrame.h"
#include "ui/VariableFrame.h"
#include "ui/ScriptsTableModel.h"
#include "ui/ScriptConfigTableModel.h"
#include "ui/ConfigTableModel.h"

namespace Ui {
    class MainWindow;
}

/**
 * @brief The MainWindow class opens a window containing all relevant information for RASP,
 * like a live overview, script selecting and editing and config.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void addInput(HWInput* hw);
    void addOutput(HWOutput* hw);
    void addVariable(Variable* var);
    void removeInput(HWInput* hw);
    void removeOutput(HWOutput* hw);
    void removeVariable(Variable* var);

private slots:
    void updateScriptConfig();

    void createScript();
    void editScript();
    void selectScript();
    void deleteScript();

    void stopScript();
    void startPauseScript();

    void createConfig();
    void editConfig();
    void selectConfig();
    void deleteConfig();

    void updateErrorFacilities();

private:
    void updateScriptState();
    bool checkScript(Script* script);
    void getRequiredList(std::list<Rule::RequiredInput>* listInput,
                         std::list<Rule::RequiredOutput>* listOutput,
                         std::list<Rule::RequiredVariable>* listVariable,
                         Script* script);

    Ui::MainWindow *ui;
    ScriptsTableModel m_scriptsModel;
    ScriptInputTableModel m_scriptInputModel;
    ScriptOutputTableModel m_scriptOutputModel;
    ScriptVariableTableModel m_scriptVariableModel;
    QStringListModel m_listLevelModel;
    QStringListModel m_listFacilityModel;

    ConfigTableModel m_configTableModel;

    ConfigManager m_config;

    std::list<InputFrame*> m_listInputFrame;
    std::list<OutputFrame*> m_listOutputFrame;
    std::list<VariableFrame*> m_listVarFrame;
};

#endif // MAINWINDOW_H
