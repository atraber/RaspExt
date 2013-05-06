#include "ui/MainWindow.h"
#include "ui_MainWindow.h"
#include "ui/ScriptsTableModel.h"
#include "ui/ScriptConfigTableModel.h"

#include "ui/InputFaderFrame.h"
#include "ui/InputButtonFrame.h"

#include "ui/OutputLEDFrame.h"
#include "ui/OutputRelayFrame.h"
#include "ui/OutputGPOFrame.h"
#include "ui/OutputDCMotorFrame.h"
#include "ui/OutputStepperFrame.h"

#include "ui/ScriptDialog.h"
#include "ui/ConfigDialog.h"

#include "util/Debug.h"

#include <QMessageBox>
#include <QDomDocument>
#include <QFile>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_config(this)
{
    ui->setupUi(this);

    // Overview page
    ui->layoutInput->setAlignment(Qt::AlignTop);
    ui->layoutOutput->setAlignment(Qt::AlignTop);
    ui->layoutVariable->setAlignment(Qt::AlignTop);


    // Script page
    ui->tableScripts->setModel(&m_scriptsModel);
    ui->tableScripts->horizontalHeader()->setStretchLastSection(true);
    ui->tableScripts->setColumnWidth(0, 200);


    ui->tableScriptInput->setModel(&m_scriptInputModel);
    ui->tableScriptInput->horizontalHeader()->setStretchLastSection(true);
    ui->tableScriptInput->setColumnWidth(0, 150);

    ui->tableScriptOutput->setModel(&m_scriptOutputModel);
    ui->tableScriptOutput->horizontalHeader()->setStretchLastSection(true);
    ui->tableScriptOutput->setColumnWidth(0, 150);

    ui->tableScriptVariable->setModel(&m_scriptVariableModel);
    ui->tableScriptVariable->horizontalHeader()->setStretchLastSection(true);
    ui->tableScriptVariable->setColumnWidth(0, 150);

    // Config page
    ui->tableConfig->setModel(&m_configTableModel);
    ui->tableConfig->horizontalHeader()->setStretchLastSection(true);

    // Settings page
    ui->comboStartupConfig->setModel(&m_configTableModel);
    ui->comboStartupScript->setModel(&m_scriptsModel);


    // connect all signals-slots
    connect(ui->buttonCreateScript, SIGNAL(clicked()), this, SLOT(createScript()));
    connect(ui->buttonSelectScript, SIGNAL(clicked()), this, SLOT(selectScript()));
    connect(ui->buttonEditScript, SIGNAL(clicked()), this, SLOT(editScript()));
    connect(ui->buttonDeleteScript, SIGNAL(clicked()), this, SLOT(deleteScript()));

    connect(ui->tableScripts->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateScriptConfig()));

    connect(ui->buttonCreateConfig, SIGNAL(clicked()), this, SLOT(createConfig()));
    connect(ui->buttonEditConfig, SIGNAL(clicked()), this, SLOT(editConfig()));
    connect(ui->buttonSelectConfig, SIGNAL(clicked()), this, SLOT(selectConfig()));
    connect(ui->buttonDeleteConfig, SIGNAL(clicked()), this, SLOT(deleteConfig()));

    connect(ui->buttonStop, SIGNAL(clicked()), this, SLOT(stopScript()));
    connect(ui->buttonPlayPause, SIGNAL(clicked()), this, SLOT(startPauseScript()));


    // load last settings
    QFile file( "defaults.xml" );
    if(!file.open(QIODevice::ReadOnly))
    {
        LOG_WARN(Logger::UI, "Could not open defaults file. Does it exist?");
        return;
    }

    QDomDocument document;
    document.setContent(&file);

    QDomElement docElem = document.documentElement();

    // check if this is a valid defaults file
    if(docElem.tagName().toLower().compare("default") != 0)
    {
        LOG_WARN(Logger::UI, "Invalid defaults file: tag \"default\" is missing");
        return;
    }

    QString defaultScript;

    QDomElement elem = docElem.firstChildElement();
    while(!elem.isNull())
    {
        if(elem.tagName().toLower().compare("config") == 0)
        {
            // only try to load new config if this string is not empty, otherwise this call would fail anyway
            if(!elem.text().isEmpty())
            {
                // try to load the configuration
                m_config.load( elem.text().toStdString() );

                // and select it in the startup config combo box
                int index = ui->comboStartupConfig->findText(elem.text());
                ui->comboStartupConfig->setCurrentIndex(index);

                ui->checkStartupConfig->setChecked(true);
            }
        }
        else if(elem.tagName().toLower().compare("script") == 0)
        {
            defaultScript = elem.text();
        }
        elem = elem.nextSiblingElement();
    }

    file.close();

    m_config.init();

    ui->labelConfig->setText( QString::fromStdString( m_config.getName() ) );

    // Now load the script which was specified in the xml file
    // We cannot load it before as the configuration was not yet initialized and therefore the loading of the script would fail
    // only try to load the given script if this string is not empty, otherwise this call would fail anyway
    if(!defaultScript.isEmpty())
    {
        // try to select the script in the list and load it afterwards
        QModelIndexList matches = m_scriptsModel.match(m_scriptsModel.index(0, 0), Qt::DisplayRole, defaultScript);
        if(matches.size() > 0)
        {
            ui->tableScripts->setCurrentIndex(matches.front());
            this->selectScript();

            ui->comboStartupScript->setCurrentIndex(matches.front().row());

            ui->checkStartupScript->setChecked(true);
        }
    }
}

MainWindow::~MainWindow()
{
    // save last settings
    QFile file( "defaults.xml" );
    if(file.open(QIODevice::WriteOnly))
    {
        QDomDocument document;

        QDomElement defaultElem = document.createElement("default");
        document.appendChild(defaultElem);

        // save specified startup config
        if(ui->checkStartupConfig->isChecked())
        {
            QDomElement config = document.createElement("config");
            QDomText configText = document.createTextNode( ui->comboStartupConfig->currentText() );
            config.appendChild(configText);
            defaultElem.appendChild(config);
        }

        // save specified startup script
        if(ui->checkStartupScript->isChecked())
        {
            QDomElement script = document.createElement("script");
            QDomText scriptText = document.createTextNode( ui->comboStartupScript->currentText() );
            script.appendChild(scriptText);
            defaultElem.appendChild(script);
        }

        file.write(document.toByteArray(4));

        file.close();
    }
    else
    {
        LOG_WARN(Logger::UI, "Could not open defaults file. Do you have read-write permissions to ./defaults.xml?");
    }

    m_config.deinit();
    m_config.clear(); // has to be cleared before ui is deleted, otherwise ui widgets are no longer available and this would lead to a segfault

    delete ui;
}

void MainWindow::updateScriptConfig()
{
    QModelIndexList indices = ui->tableScripts->selectionModel()->selection().indexes();

    Script* script = m_scriptsModel.getScript(indices.front().row());

    std::list<Rule::RequiredInput> listInput;
    std::list<Rule::RequiredOutput> listOutput;
    std::list<Rule::RequiredVariable> listVariable;

    this->getRequiredList(&listInput, &listOutput, &listVariable, script);

    m_scriptInputModel.set( listInput );
    m_scriptOutputModel.set( listOutput );
    m_scriptVariableModel.set( listVariable );
}

void MainWindow::createScript()
{
    Script* script = new Script();
    ScriptDialog* dialog = new ScriptDialog(this, script, &m_config);

    if( dialog->exec() == QDialog::Accepted )
    {
        // save script to hd
        script->save();

        // add it to the gui
        m_scriptsModel.addScript(script);
    }
    else
    {
        // the user did not accept. The created script is no longer needed, so delete it
        delete script;
    }

    delete dialog;
}


void MainWindow::editScript()
{
    QModelIndexList indices = ui->tableScripts->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        int row = indices.front().row();

        Script* script = m_scriptsModel.getScript(row);

        // check if script is selected, if yes, then display a warning and don't do anything
        if(script == m_config.getActiveScript())
        {
            QMessageBox(QMessageBox::Warning, "Warning", "Cannot edit script because it is selected.\nPlease stop the script first", QMessageBox::Ok, this).exec();
            return;
        }

        // check if all required stuff exists
        // if not and the user decides that this is not acceptable, we stop immediately
        if( !this->checkScript(script) )
            return;

        // now we can create the dialog an start editing this script
        ScriptDialog* dialog = new ScriptDialog(this, script, &m_config);

        if( dialog->exec() == QDialog::Accepted)
        {
            // save script to filesystem
            script->save();

            // this updates the description (and name) of a script in the ui
            m_scriptsModel.modifyRow(row, script);

            // this updates the displayed configuration
            this->updateScriptConfig();
        }
        else
        {
            // Modified script was not accepted. As we do not want to have an invalid script (as it may be modified), we reload it
            m_scriptsModel.modifyRow(row, script->load(script->getName()));
        }

        delete dialog;
    }
}

void MainWindow::deleteScript()
{
    QModelIndexList indices = ui->tableScripts->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        // check if script is selected, if yes, then display a warning and don't do anything
        if(m_config.getActiveScript() == m_scriptsModel.getScript(indices.front().row()))
        {
            QMessageBox(QMessageBox::Warning, "Warning", "Cannot delete script because it is selected.\nPlease stop the script first", QMessageBox::Ok, this).exec();
            return;
        }

        m_scriptsModel.removeRow(indices.front().row());
    }
}

void MainWindow::selectScript()
{
    QModelIndexList indices = ui->tableScripts->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        Script* script = m_scriptsModel.getScript(indices.front().row());

        // check if all required stuff exists
        // if not and the user decides that this is not acceptable, we stop immediatly
        if( !this->checkScript(script) )
            return;

        m_config.setActiveScript( script );

        this->updateScriptState();
    }
}

void MainWindow::stopScript()
{
    m_config.stopActiveScript();

    this->updateScriptState();
}

void MainWindow::startPauseScript()
{
    if(m_config.getActiveScriptState() == ConfigManager::Active)
    {
        m_config.pauseActiveScript();

        this->updateScriptState();
    }
    else if(m_config.getActiveScriptState() == ConfigManager::Paused)
    {
        m_config.continueActiveScript();

        this->updateScriptState();
    }
}

void MainWindow::updateScriptState()
{
    switch( m_config.getActiveScriptState() )
    {
    case ConfigManager::Inactive:
        ui->labelState->setText( "Inactive" );
        break;
    case ConfigManager::Active:
        ui->labelState->setText( "Active" );
        break;
    case ConfigManager::Paused:
        ui->labelState->setText( "Paused" );
        break;
    }

    Script* script = m_config.getActiveScript();
    if(script != NULL)
        ui->labelScript->setText( QString::fromStdString( script->getName() ) );
    else
        ui->labelScript->setText("");
}

/**
 * @brief MainWindow::checkScript This function checks all prerequisits of a script (e.g. inputs, outputs, variables).
 * If it finds unmet prerequisits, it asks the user what to do.
 * If everything looks good or the user accepts the unmet dependencies, we return true, false otherwise
 * @param script
 * @return
 */
bool MainWindow::checkScript(Script *script)
{
    std::list<Rule::RequiredInput> listInput;
    std::list<Rule::RequiredOutput> listOutput;
    std::list<Rule::RequiredVariable> listVariable;

    this->getRequiredList(&listInput, &listOutput, &listVariable, script);

    QString strDep;

    for(std::list<Rule::RequiredInput>::iterator it = listInput.begin(); it != listInput.end(); it++)
    {
        if(!it->exists)
        {
            strDep.append( "Input ");
            strDep.append( QString::fromStdString((*it).name) );
            strDep.append( " with type ");
            strDep.append( QString::fromStdString( HWInput::HWInputTypeToString((*it).type) ) );
            strDep.append( "\n" );
        }
    }

    for(std::list<Rule::RequiredOutput>::iterator it = listOutput.begin(); it != listOutput.end(); it++)
    {
        if(!it->exists)
        {
            strDep.append( "Output ");
            strDep.append( QString::fromStdString((*it).name) );
            strDep.append( " with type ");
            strDep.append( QString::fromStdString( HWOutput::HWOutputTypeToString((*it).type) ) );
            strDep.append( "\n" );
        }
    }

    for(std::list<Rule::RequiredVariable>::iterator it = listVariable.begin(); it != listVariable.end(); it++)
    {
        if(!it->exists)
        {
            strDep.append( "Variable " );
            strDep.append( QString::fromStdString((*it).name) );
            strDep.append( "\n" );
        }
    }

    if( strDep.size() != 0 )
    {
        QString message = "Not all required inputs, outputs or variables are present. The following parts are missing:\n\n";

        message.append(strDep);

        message.append("\nDo you want to continue without these parts?");

        QMessageBox messageBox(QMessageBox::Warning, "Error",
                               message,
                               QMessageBox::Yes|QMessageBox::No, this);

        if( messageBox.exec() == QMessageBox::Yes )
            return true;
        else
            return false;
    }

    return true;
}

/**
 * @brief MainWindow::getRequiredList checks the given script for missing objects. It sets every exists field to the corresponding value.
 * @param listInput
 * @param listOutput
 * @param listVariable
 * @param script
 */
void MainWindow::getRequiredList(std::list<Rule::RequiredInput>* listInput,
                                 std::list<Rule::RequiredOutput>* listOutput,
                                 std::list<Rule::RequiredVariable>* listVariable, Script* script)
{
    script->getRequiredList(listInput, listOutput, listVariable);

    // Now we have the list of requirements, we now go through each list and set all exist fields to true for which the objects are present
    for(std::list<Rule::RequiredInput>::iterator it = listInput->begin(); it != listInput->end(); it++)
    {
        HWInput* input = m_config.getInputByName((*it).name);
        if(input != NULL && input->getType() == (*it).type)
            it->exists = true;
        else
            it->exists = false;
    }

    for(std::list<Rule::RequiredOutput>::iterator it = listOutput->begin(); it != listOutput->end(); it++)
    {
        HWOutput* output = m_config.getOutputByName((*it).name);
        if(output != NULL && output->getType() == (*it).type)
            it->exists = true;
        else
            it->exists = false;
    }
}

/**
 * @brief MainWindow::addInput adds the input given by hw to the GUI,
 * displays a frame containing infos about it in the overview tab and registers the GUI element on the input to receive updates.
 * @param hw the HWInput object to display in the GUI
 */
void MainWindow::addInput(HWInput* hw)
{
    InputFrame* frame = NULL;
    switch(hw->getType())
    {
    case HWInput::Button:
        frame = new InputButtonFrame((HWInputButton*)hw);

        break;

    case HWInput::Fader:
        frame = new InputFaderFrame((HWInputFader*)hw);
        break;

    default:
        LOG_WARN(Logger::UI, "Requested GUI-object for unknown input type");
        break;
    }

    if(frame != NULL)
    {
        ui->layoutInput->addWidget(frame);
        m_listInputFrame.push_back(frame);
    }
}

/**
 * @brief MainWindow::addOutput adds the output given by hw to the GUI,
 * displays a frame containing infos about it in the overview tab and registers the GUI element on the output to receive updates.
 * @param hw the HWOutput object to display in the GUI
 */
void MainWindow::addOutput(HWOutput* hw)
{
    OutputFrame* frame = NULL;

    switch(hw->getType())
    {
    case HWOutput::Relay:
        frame = new OutputRelayFrame((HWOutputRelay*)hw);
        break;
    case HWOutput::DCMotor:
        frame = new OutputDCMotorFrame((HWOutputDCMotor*)hw);
        break;
    case HWOutput::LED:
        frame = new OutputLEDFrame((HWOutputLED*)hw);
        break;
    case HWOutput::Stepper:
        frame = new OutputStepperFrame((HWOutputStepper*)hw);
        break;
    case HWOutput::GPO:
        frame = new OutputGPOFrame((HWOutputGPO*)hw);
        break;
    default:
        LOG_WARN(Logger::UI, "Requested GUI-object for unknown output type");
        break;
    }

    if(frame != NULL)
    {
        ui->layoutOutput->addWidget(frame);
        m_listOutputFrame.push_back(frame);
    }
}

/**
 * @brief MainWindow::addVariable adds the variable given by var to the GUI,
 * displays a frame containing infos about it in the overview tab and registers the GUI element on the variable to receive updates.
 * @param var the Variable object to display in the GUI
 */
void MainWindow::addVariable(Variable* var)
{
    VariableFrame* frame = new VariableFrame(var);
    ui->layoutVariable->addWidget(frame);
    m_listVarFrame.push_back( frame );
}

/**
 * @brief MainWindow::removeInput unregisters the GUI element from the object and removes the input object from the GUI.
 * @param hw the object to be removed from the GUI
 */
void MainWindow::removeInput(HWInput* hw)
{
    for(std::list<InputFrame*>::iterator it = m_listInputFrame.begin(); it != m_listInputFrame.end(); it++)
    {
        if((*it)->getHW() == hw)
        {
            ui->layoutInput->removeWidget(*it);
            delete (*it);
            break;
        }
    }
}

/**
 * @brief MainWindow::removeOutput unregisters the GUI element from the object and removes the output object from the GUI.
 * @param hw the object to be removed from the GUI
 */
void MainWindow::removeOutput(HWOutput* hw)
{
    for(std::list<OutputFrame*>::iterator it = m_listOutputFrame.begin(); it != m_listOutputFrame.end(); it++)
    {
        if((*it)->getHW() == hw)
        {
            ui->layoutOutput->removeWidget(*it);
            delete (*it);
            break;
        }
    }
}

/**
 * @brief MainWindow::removeVariable unregisters the GUI element from the object and removes the variable object from the GUI.
 * @param var the object to be removed from the GUI
 */
void MainWindow::removeVariable(Variable* var)
{
    for(std::list<VariableFrame*>::iterator it = m_listVarFrame.begin(); it != m_listVarFrame.end(); it++)
    {
        if((*it)->getVariable() == var)
        {
            ui->layoutVariable->removeWidget(*it);
            delete (*it);
            break;
        }
    }
}

void MainWindow::createConfig()
{
    ConfigDialog* dialog = new ConfigDialog(this, "", &m_config);

    if( dialog->exec() == QDialog::Accepted)
        m_configTableModel.refresh();
}

void MainWindow::editConfig()
{
    QModelIndexList indices = ui->tableConfig->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        std::string name = m_configTableModel.get(indices.front().row());

        if(m_config.getName().compare(name) == 0)
        {
            QMessageBox(QMessageBox::Warning,
                        "Warning",
                        "Cannot edit config because this config is currently selected.\nPlease select a different config first.",
                        QMessageBox::Ok,
                        this).exec();

            return;
        }

        ConfigDialog* dialog = new ConfigDialog(this, name, &m_config);

        if( dialog->exec() == QDialog::Accepted)
            m_configTableModel.refresh();
    }
}

void MainWindow::selectConfig()
{
    QModelIndexList indices = ui->tableConfig->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        std::string name = m_configTableModel.get(indices.front().row());

        // Do nothing if the selected script is currently loaded script
        if(name.compare(m_config.getName()) == 0)
            return;

        // we only allow modifications on the config if no script is loaded and the current config is not selected
        if(m_config.getActiveScriptState() == ConfigManager::Inactive)
        {
            m_config.deinit();
            m_config.clear();

            m_config.load(name);
            m_config.init();

            ui->labelConfig->setText( QString::fromStdString(name) );
        }
        else
        {
            QMessageBox(QMessageBox::Warning,
                        "Warning",
                        "Cannot select config because a script is running.\nPlease stop this script first",
                        QMessageBox::Ok,
                        this).exec();
            return;
        }
    }

    // TODO: think about what to do if the user selects the already loaded config
}

void MainWindow::deleteConfig()
{
    QModelIndexList indices = ui->tableConfig->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        // check if config is selected, if yes, then display a warning and don't do anything
        if(m_config.getActiveScript() == m_scriptsModel.getScript(indices.front().row()))
        {
            QMessageBox(QMessageBox::Warning, "Warning", "Cannot delete config because it is selected.\nPlease select a different config first", QMessageBox::Ok, this).exec();
            return;
        }

        m_configTableModel.removeRow(indices.front().row());
    }
}
