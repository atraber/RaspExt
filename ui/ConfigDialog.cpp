
#include "ui/ConfigDialog.h"
#include "hw/HWInput.h"
#include "hw/HWOutput.h"
#include "hw/BTClassicThread.h"
#include "ConfigManager.h"
#include "hw/I2CThread.h"
#include "util/Debug.h"

#include "hw/HWInputButton.h"
#include "hw/HWInputButtonI2C.h"
#include "hw/HWInputButtonBt.h"
#include "hw/HWInputButtonBtGPIO.h"
#include "hw/HWInputFader.h"
#include "hw/HWInputFaderI2C.h"
#include "hw/HWInputFaderBt.h"

#include "hw/HWOutputDCMotor.h"
#include "hw/HWOutputDCMotorI2C.h"
#include "hw/HWOutputDCMotorBt.h"
#include "hw/HWOutputStepper.h"
#include "hw/HWOutputStepperI2C.h"
#include "hw/HWOutputStepperBt.h"
#include "hw/HWOutputLED.h"
#include "hw/HWOutputLEDI2C.h"
#include "hw/HWOutputLEDBt.h"
#include "hw/HWOutputRelay.h"
#include "hw/HWOutputRelayI2C.h"
#include "hw/HWOutputRelayBt.h"
#include "hw/HWOutputGPO.h"
#include "hw/HWOutputGPOI2C.h"
#include "hw/HWOutputGPOBt.h"

#include "ui/I2CScanDialog.h"
#include "ui/BTScanDialog.h"

#include "ui_ConfigBTDialog.h"
#include "ui_ConfigDialog.h"
#include "ui_ConfigInputDialog.h"
#include "ui_ConfigOutputDialog.h"

#include <QMessageBox>

ConfigDialog::ConfigDialog(QWidget *parent, std::string name, ConfigManager* configManager) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    m_configManager = configManager;

    ui->setupUi(this);

    // only try to load config if the name is not empty
    // empty name means a new config
    if( !name.empty() )
        m_config.load(name);


    // fill in values
    ui->editName->setText( QString::fromStdString( m_config.getName() ) );


    // set table models
    m_inputTableModel = new ConfigInputTableModel(this, &m_config);
    ui->tableInputs->setModel(m_inputTableModel);
    ui->tableInputs->horizontalHeader()->setStretchLastSection(true);

    m_outputTableModel = new ConfigOutputTableModel(this, &m_config);
    ui->tableOutputs->setModel(m_outputTableModel);
    ui->tableOutputs->horizontalHeader()->setStretchLastSection(true);

    m_btThreadTableModel = new ConfigBTThreadTableModel(this, &m_config);
    ui->tableBtBoards->setModel(m_btThreadTableModel);
    ui->tableBtBoards->horizontalHeader()->setStretchLastSection(true);


    // connect all signals - slots
    connect(ui->buttonAddInput, SIGNAL(clicked()), this, SLOT(addInput()));
    connect(ui->buttonEditInput, SIGNAL(clicked()), this, SLOT(editInput()));
    connect(ui->buttonDeleteInput, SIGNAL(clicked()), this, SLOT(deleteInput()));
    connect(ui->buttonAddOutput, SIGNAL(clicked()), this, SLOT(addOutput()));
    connect(ui->buttonEditOutput, SIGNAL(clicked()), this, SLOT(editOutput()));
    connect(ui->buttonDeleteOutput, SIGNAL(clicked()), this, SLOT(deleteOutput()));
    connect(ui->buttonAddBt, SIGNAL(clicked()), this, SLOT(addBt()));
    connect(ui->buttonEditBt, SIGNAL(clicked()), this, SLOT(editBt()));
    connect(ui->buttonDeleteBt, SIGNAL(clicked()), this, SLOT(deleteBt()));

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(okPressed()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

ConfigDialog::~ConfigDialog()
{
    delete this->ui;

    delete m_inputTableModel;
    delete m_outputTableModel;
    delete m_btThreadTableModel;
}

/**
 * @brief ConfigDialog::i2cScan opens a dialog for i2c scanning
 * @return returns -1 if the user did not select an i2c address or pressed cancel, the selected address otherwise
 */
int ConfigDialog::i2cScan()
{
    I2CScanDialog dialog(this);

    m_configManager->getI2CThread()->addOutput( std::bind(&I2CScanDialog::i2cScan, &dialog, std::placeholders::_1) );

    if( dialog.exec() == QDialog::Accepted)
        return dialog.getAddress();

    return -1;
}

int ConfigDialog::btI2CScan(std::string name)
{
    std::string addr;

    // first find the address for this name
    for(std::list<BTThread*>::iterator it = m_config.m_listBTThread.begin(); it != m_config.m_listBTThread.end(); it++)
    {
        if( strcasecmp((*it)->getName().c_str(), name.c_str()) == 0 )
        {
            addr = (*it)->getBTAddr();
            break;
        }
    }

    // give up if we did not find an address, this means that we have been given an invalid argument
    if(addr.empty())
        return -1;

    // look in existing threads if we find this address
    BTThread* btThread = m_configManager->getBTThreadByAddr(addr);
    bool bOwn = false;

    if(btThread == NULL)
    {
        // apparently we did not find it, so we must create it ourselfs
        bOwn = true;

        btThread = new BTClassicThread();
        btThread->setBTAddr(addr);

        btThread->start();
    }

    I2CScanDialog dialog(this);

    btThread->addOutput( std::bind(&I2CScanDialog::btI2CScan, &dialog, std::placeholders::_1) );

    int slaveAddress = -1;
    if( dialog.exec() == QDialog::Accepted)
        slaveAddress = dialog.getAddress();

    if(bOwn)
    {
        // since we have created it only for this purpose we have to delete it now
        btThread->kill();
        delete btThread;
    }

    return slaveAddress;
}

void ConfigDialog::addInput()
{
    ConfigInputDialog dialog(this);

    if( dialog.exec() == QDialog::Accepted)
    {
        m_inputTableModel->add( dialog.assemble() );
    }
}

void ConfigDialog::editInput()
{
    QModelIndexList indices = ui->tableInputs->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        int row = indices.front().row();

        ConfigInputDialog dialog(this);

        dialog.edit( m_inputTableModel->get(row) );

        if( dialog.exec() == QDialog::Accepted)
        {
            m_inputTableModel->modifyRow(row, dialog.assemble());
        }
    }
}

void ConfigDialog::deleteInput()
{
    QModelIndexList indices = ui->tableInputs->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        int row = indices.front().row();

        m_inputTableModel->removeRow(row);
    }
}

void ConfigDialog::addOutput()
{
    ConfigOutputDialog dialog(this);

    if( dialog.exec() == QDialog::Accepted)
    {
        m_outputTableModel->add( dialog.assemble() );
    }
}

void ConfigDialog::editOutput()
{
    QModelIndexList indices = ui->tableOutputs->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        int row = indices.front().row();

        ConfigOutputDialog dialog(this);

        dialog.edit( m_outputTableModel->get(row) );

        if( dialog.exec() == QDialog::Accepted)
        {
            m_outputTableModel->modifyRow(row, dialog.assemble());
        }
    }
}

void ConfigDialog::deleteOutput()
{
    QModelIndexList indices = ui->tableOutputs->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        int row = indices.front().row();

        m_outputTableModel->removeRow(row);
    }
}

void ConfigDialog::addBt()
{
    ConfigBTDialog dialog(this);

    if( dialog.exec() == QDialog::Accepted)
    {
        m_btThreadTableModel->add( dialog.assemble() );
    }
}

void ConfigDialog::editBt()
{
    QModelIndexList indices = ui->tableBtBoards->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        int row = indices.front().row();

        ConfigBTDialog dialog(this);

        dialog.edit( m_btThreadTableModel->get(row) );

        if( dialog.exec() == QDialog::Accepted)
        {
            m_btThreadTableModel->modifyRow(row, dialog.assemble());
        }
    }
}

void ConfigDialog::deleteBt()
{
    QModelIndexList indices = ui->tableBtBoards->selectionModel()->selection().indexes();

    if(indices.size() != 0)
    {
        int row = indices.front().row();

        m_btThreadTableModel->removeRow(row);
    }
}

void ConfigDialog::okPressed()
{
    if( ui->editName->text().size() == 0 )
    {
        // name is empty, we cannot save a script with an empty name
        QMessageBox(QMessageBox::Warning,
                    "Empty Name",
                    "The name of a config cannot be empty! Please fill in a name",
                    QMessageBox::Ok,
                    this).exec();
        return;
    }

    m_config.setName( ui->editName->text().toStdString() );

    m_config.save();

    this->done(Accepted);
}





ConfigInputTableModel::ConfigInputTableModel(QObject *parent, Config *config) : QAbstractTableModel(parent)
{
    m_config = config;
}

int ConfigInputTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_config->m_listInput.size();
}

int ConfigInputTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant ConfigInputTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= m_config->m_listInput.size() || index.row() < 0)
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        HWInput* hw = *std::next(m_config->m_listInput.begin(), index.row());

        if(index.column() == 0)
            return QString::fromStdString( hw->getName() );
        else
            return QString::fromStdString( HWInput::HWInputTypeToString( hw->getType() ) );
    }

    return QVariant();
}

QVariant ConfigInputTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if(orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return "Name";
        case 1:
            return "Type";
        default:
            return QVariant();
        }
    }

    return QVariant();
}

bool ConfigInputTableModel::removeRow(int row, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows(QModelIndex(), row, row);

    // delete the variable, as it is no longer needed
    std::list<HWInput*>::iterator it = std::next(m_config->m_listInput.begin(), row);
    delete *it;

    m_config->m_listInput.erase(it);

    endRemoveRows();

    return true;
}

HWInput* ConfigInputTableModel::get(int row) const
{
    return *std::next(m_config->m_listInput.begin(), row);
}

void ConfigInputTableModel::add(HWInput *hw)
{
    if(hw != NULL)
    {
        beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount());

        m_config->m_listInput.push_back(hw);

        endInsertRows();
    }
    else
    {
        LOG_WARN(Logger::UI, "Received null argument");
    }
}

void ConfigInputTableModel::modifyRow(int row, HWInput *hw)
{
    std::list<HWInput*>::iterator it = std::next(m_config->m_listInput.begin(), row);
    HWInput* old = *it;

    if(hw != old)
        delete old;

    *it = hw;

    emit dataChanged(this->index(row, 0), this->index(row, this->columnCount()));
}


ConfigOutputTableModel::ConfigOutputTableModel(QObject *parent, Config *config) : QAbstractTableModel(parent)
{
    m_config = config;
}

int ConfigOutputTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_config->m_listOutput.size();
}

int ConfigOutputTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant ConfigOutputTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= m_config->m_listOutput.size() || index.row() < 0)
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        HWOutput* hw = *std::next(m_config->m_listOutput.begin(), index.row());

        if(index.column() == 0)
            return QString::fromStdString( hw->getName() );
        else
            return QString::fromStdString( HWOutput::HWOutputTypeToString( hw->getType() ) );
    }

    return QVariant();
}

QVariant ConfigOutputTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if(orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return "Name";
        case 1:
            return "Type";
        default:
            return QVariant();
        }
    }

    return QVariant();
}

bool ConfigOutputTableModel::removeRow(int row, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows(QModelIndex(), row, row);

    // delete the variable, as it is no longer needed
    std::list<HWOutput*>::iterator it = std::next(m_config->m_listOutput.begin(), row);
    delete *it;

    m_config->m_listOutput.erase(it);

    endRemoveRows();

    return true;
}

HWOutput* ConfigOutputTableModel::get(int row) const
{
    return *std::next(m_config->m_listOutput.begin(), row);
}

void ConfigOutputTableModel::add(HWOutput *hw)
{
    if(hw != NULL)
    {
        beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount());

        m_config->m_listOutput.push_back(hw);

        endInsertRows();
    }
    else
    {
        LOG_WARN(Logger::UI, "Received null argument");
    }
}

void ConfigOutputTableModel::modifyRow(int row, HWOutput *hw)
{
    std::list<HWOutput*>::iterator it = std::next(m_config->m_listOutput.begin(), row);
    HWOutput* old = *it;

    if(hw != old)
        delete old;

    *it = hw;

    emit dataChanged(this->index(row, 0), this->index(row, this->columnCount()));
}


ConfigBTThreadTableModel::ConfigBTThreadTableModel(QObject *parent, Config *config) : QAbstractTableModel(parent)
{
    m_config = config;
}

int ConfigBTThreadTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_config->m_listBTThread.size();
}

int ConfigBTThreadTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant ConfigBTThreadTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= m_config->m_listBTThread.size() || index.row() < 0)
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        BTThread* bt = *std::next(m_config->m_listBTThread.begin(), index.row());

        if(index.column() == 0)
            return QString::fromStdString( bt->getName() );
        else
            return QString::fromStdString( bt->getBTAddr() );
    }

    return QVariant();
}

QVariant ConfigBTThreadTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if(orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return "Name";
        case 1:
            return "Address";
        default:
            return QVariant();
        }
    }

    return QVariant();
}

bool ConfigBTThreadTableModel::removeRow(int row, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows(QModelIndex(), row, row);

    // delete the variable, as it is no longer needed
    std::list<BTThread*>::iterator it = std::next(m_config->m_listBTThread.begin(), row);
    delete *it;

    m_config->m_listBTThread.erase(it);

    endRemoveRows();

    return true;
}

void ConfigBTThreadTableModel::add(BTThread *btThread)
{
    if(btThread != NULL)
    {
        beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount());

        m_config->m_listBTThread.push_back(btThread);

        endInsertRows();
    }
    else
    {
        LOG_WARN(Logger::UI, "Received null argument");
    }
}

void ConfigBTThreadTableModel::modifyRow(int row, BTThread *btThread)
{
    std::list<BTThread*>::iterator it = std::next(m_config->m_listBTThread.begin(), row);
    BTThread* oldBt = *it;

    if(btThread != oldBt)
        delete oldBt;

    *it = btThread;

    emit dataChanged(this->index(row, 0), this->index(row, this->columnCount()));
}

BTThread* ConfigBTThreadTableModel::get(int row) const
{
    return *std::next(m_config->m_listBTThread.begin(), row);
}






ConfigBTDialog::ConfigBTDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigBTDialog)
{
    ui->setupUi(this);

    // connect all signals - slots
    connect(ui->buttonScan, SIGNAL(clicked()), this, SLOT(btScan()));

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));


    // TODO: check missing if bluetooth address is correct
}

ConfigBTDialog::~ConfigBTDialog()
{
    delete ui;
}

void ConfigBTDialog::btScan()
{
    BTScanDialog dialog(this);

    if( dialog.exec() == QDialog::Accepted)
    {
        std::pair<std::string, std::string> pair = dialog.getNameAddr();

        ui->editName->setText( QString::fromStdString( pair.first ) );
        ui->editAddr->setText( QString::fromStdString( pair.second ) );
    }
}

void ConfigBTDialog::edit(BTThread *btThread)
{
    ui->editName->setText( QString::fromStdString( btThread->getName() ) );
    ui->editAddr->setText( QString::fromStdString( btThread->getBTAddr() ) );
}

BTThread* ConfigBTDialog::assemble() const
{
    BTThread* btThread = new BTClassicThread();

    btThread->setName( ui->editName->text().toStdString() );
    btThread->setBTAddr( ui->editAddr->text().toStdString() );

    return btThread;
}


ConfigInputDialog::ConfigInputDialog(ConfigDialog *parent) :
    QDialog(parent),
    ui(new Ui::ConfigInputDialog)
{
    m_baseWidget = NULL;

    ui->setupUi(this);

    // connect all signals - slots
    connect(ui->comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(comboChanged(int)));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    // default values
    this->comboChanged( ui->comboType->currentIndex() );
}

ConfigInputDialog::~ConfigInputDialog()
{
    if(m_baseWidget != NULL)
        delete m_baseWidget;

    delete ui;
}

void ConfigInputDialog::edit(HWInput *hw)
{
    ui->editName->setText( QString::fromStdString( hw->getName() ) );
    ui->comboType->setCurrentIndex( hw->getType() );

    if(m_baseWidget != NULL)
    {
        m_baseWidget->edit(hw);
    }
}

HWInput* ConfigInputDialog::assemble() const
{
    HWInput* hw = NULL;

    if(m_baseWidget != NULL)
        hw = m_baseWidget->assemble();

    if(hw != NULL)
    {
        hw->setName( ui->editName->text().toStdString() );
    }

    return hw;
}

void ConfigInputDialog::comboChanged(int index)
{
    // delete old baseWidget if it exists
    if(m_baseWidget != NULL)
    {
        ui->gridLayout->removeWidget(m_baseWidget);
        delete m_baseWidget;
        m_baseWidget = NULL;
    }

    HWInput::HWInputType type = (HWInput::HWInputType)index;
    switch( type )
    {
    case HWInput::Button:
        m_baseWidget = new ConfigInputButtonWidget(this, (ConfigDialog*)this->parent());
        break;
    case HWInput::Fader:
        m_baseWidget = new ConfigInputFaderWidget(this, (ConfigDialog*)this->parent());
        break;
    }

    // add new baseWidget, this should never be NULL. If it is, somewhere something went wrong
    if(m_baseWidget != NULL)
    {
        ui->gridLayout->addWidget(m_baseWidget, 2, 0, 1, 2);
    }
}


ConfigOutputDialog::ConfigOutputDialog(ConfigDialog *parent) :
    QDialog(parent),
    ui(new Ui::ConfigOutputDialog)
{
    m_baseWidget = NULL;

    ui->setupUi(this);

    // connect all signals - slots
    connect(ui->comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(comboChanged(int)));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    // default values
    this->comboChanged( ui->comboType->currentIndex() );
}

ConfigOutputDialog::~ConfigOutputDialog()
{
    if(m_baseWidget != NULL)
        delete m_baseWidget;

    delete ui;
}

void ConfigOutputDialog::edit(HWOutput *hw)
{
    ui->editName->setText( QString::fromStdString( hw->getName() ) );
    ui->comboType->setCurrentIndex( hw->getType() );

    if(m_baseWidget != NULL)
    {
        m_baseWidget->edit(hw);
    }
}

HWOutput* ConfigOutputDialog::assemble() const
{
    HWOutput* hw = NULL;

    if(m_baseWidget != NULL)
        hw = m_baseWidget->assemble();

    if(hw != NULL)
    {
        hw->setName( ui->editName->text().toStdString() );
    }

    return hw;
}

void ConfigOutputDialog::comboChanged(int index)
{
    // delete old baseWidget if it exists
    if(m_baseWidget != NULL)
    {
        ui->gridLayout->removeWidget(m_baseWidget);
        delete m_baseWidget;
        m_baseWidget = NULL;
    }

    HWOutput::HWOutputType type = (HWOutput::HWOutputType)index;
    switch( type )
    {
    case HWOutput::DCMotor:
        m_baseWidget = new ConfigOutputDCMotorWidget(this, (ConfigDialog*)this->parent());
        break;
    case HWOutput::Stepper:
        m_baseWidget = new ConfigOutputStepperWidget(this, (ConfigDialog*)this->parent());
        break;
    case HWOutput::LED:
        m_baseWidget = new ConfigOutputLEDWidget(this, (ConfigDialog*)this->parent());
        break;
    case HWOutput::Relay:
        m_baseWidget = new ConfigOutputRelayWidget(this, (ConfigDialog*)this->parent());
        break;
    case HWOutput::GPO:
        m_baseWidget = new ConfigOutputGPOWidget(this, (ConfigDialog*)this->parent());
        break;
    }

    // add new baseWidget, this should never be NULL. If it is, somewhere something went wrong
    if(m_baseWidget != NULL)
    {
        ui->gridLayout->addWidget(m_baseWidget, 2, 0, 1, 2);
    }
}


ConfigInputButtonWidget::ConfigInputButtonWidget(QWidget *parent, ConfigDialog* configDialog) : IConfigInputWidget(parent)
{
    m_configDialog = configDialog;

    QLabel* label = new QLabel("Select hardware", this);
    m_comboType = new QComboBox(this);
    m_comboType->addItem("Dummy");
    m_comboType->addItem("I2C");
    m_comboType->addItem("Bluetooth I2C");
    m_comboType->addItem("Bluetooth");

    m_labelBtBoard = new QLabel("Select bluetooth board", this);
    m_comboBtBoard = new QComboBox(this);

    // add all bluetooth boards to the combo box
    const std::list<BTThread*>* listBT = m_configDialog->getListBTThread();
    for(std::list<BTThread*>::const_iterator it = listBT->begin(); it != listBT->end(); it++)
        m_comboBtBoard->addItem( QString::fromStdString( (*it)->getName() ) );

    m_labelI2CAddr = new QLabel("Select I2C address", this);
    m_spinI2CAddr = new QSpinBox(this);
    m_spinI2CAddr->setMinimum(0);
    m_spinI2CAddr->setMaximum(127);

    m_labelPort = new QLabel("Select pin", this);
    m_spinPort = new QSpinBox(this);
    m_spinPort->setMinimum(0);
    m_spinPort->setMaximum(15);

    m_buttonI2CScan = new QPushButton("Scan I2C", this);


    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // now add everything to the layout
    layout->addWidget(label, 0, 0);
    layout->addWidget(m_comboType, 0, 1);

    layout->addWidget(m_labelBtBoard, 1, 0);
    layout->addWidget(m_comboBtBoard, 1, 1);
    layout->addWidget(m_labelI2CAddr, 2, 0);
    layout->addWidget(m_spinI2CAddr, 2, 1);
    layout->addWidget(m_labelPort, 3, 0);
    layout->addWidget(m_spinPort, 3, 1);

    layout->addWidget(m_buttonI2CScan, 4, 1);


    this->setLayout(layout);

    // now connect all signals - slots
    connect(m_comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(m_buttonI2CScan, SIGNAL(clicked()), this, SLOT(i2cScan()));

    // set default
    this->typeChanged( m_comboType->currentIndex() );
}

void ConfigInputButtonWidget::typeChanged(int index)
{
    HWInput::HWType type = (HWInput::HWType)index;
    switch(type)
    {
    case HWInput::Dummy:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->hide();
        m_spinI2CAddr->hide();
        m_labelPort->hide();
        m_spinPort->hide();
        m_buttonI2CScan->hide();
        break;
    case HWInput::I2C:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelPort->show();
        m_spinPort->show();
        m_buttonI2CScan->show();
        break;
    case HWInput::BtI2C:
        m_labelBtBoard->show();
        m_comboBtBoard->show();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelPort->show();
        m_spinPort->show();
        m_buttonI2CScan->show();
        break;
    case HWInput::Bt:
        m_labelBtBoard->show();
        m_comboBtBoard->show();
        m_labelI2CAddr->hide();
        m_spinI2CAddr->hide();
        m_labelPort->show();
        m_spinPort->show();
        m_buttonI2CScan->hide();
        break;
    }
}

void ConfigInputButtonWidget::i2cScan()
{
    int slaveAddress = -1;

    HWInput::HWType type = (HWInput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWInput::I2C:
        slaveAddress = m_configDialog->i2cScan();
        break;
    case HWInput::BtI2C:
        slaveAddress = m_configDialog->btI2CScan( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    if(slaveAddress != -1)
        m_spinI2CAddr->setValue( slaveAddress );
}

void ConfigInputButtonWidget::edit(HWInput *hw)
{
    m_comboType->setCurrentIndex( hw->getHWType() );

    const char* btName = NULL;
    switch(hw->getHWType())
    {
    case HWInput::I2C:
        m_spinI2CAddr->setValue( ((HWInputButtonI2C*)hw)->getSlaveAddress() );
        m_spinPort->setValue( ((HWInputButtonI2C*)hw)->getPort() );
        break;
    case HWInput::BtI2C:
        m_spinI2CAddr->setValue( ((HWInputButtonBt*)hw)->getSlaveAddress() );
        m_spinPort->setValue( ((HWInputButtonBt*)hw)->getPort() );
        btName = ((HWInputButtonBt*)hw)->getBTName().c_str();
        break;
    case HWInput::Bt:
        m_spinPort->setValue( ((HWInputButtonBtGPIO*)hw)->getPin() );
        btName = ((HWInputButtonBtGPIO*)hw)->getBTName().c_str();
        break;
    }


    if(btName != NULL)
    {
        for(unsigned int i = 0; i < m_comboBtBoard->count();i ++)
        {
            if(m_comboBtBoard->itemText(i).compare( btName ) == 0)
            {
                m_comboBtBoard->setCurrentIndex(i);
                break;
            }
        }
    }
}

HWInput* ConfigInputButtonWidget::assemble()
{
    HWInputButton* hw = NULL;

    HWInput::HWType type = (HWInput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWInput::Dummy:
        hw = new HWInputButton();
        break;
    case HWInput::I2C:
        hw = new HWInputButtonI2C();
        ((HWInputButtonI2C*)hw)->setPort( m_spinPort->value() );
        ((HWInputButtonI2C*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        break;
    case HWInput::BtI2C:
        hw = new HWInputButtonBt();
        ((HWInputButtonBt*)hw)->setPort( m_spinPort->value() );
        ((HWInputButtonBt*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWInputButtonBt*)hw)->setBTName( m_comboBtBoard->currentText().toStdString() );
        break;
    case HWInput::Bt:
        hw = new HWInputButtonBtGPIO();
        ((HWInputButtonBtGPIO*)hw)->setPinGroup(2); // set to 2 as its the only one supported as of now
        ((HWInputButtonBtGPIO*)hw)->setPin( m_spinPort->value() );
        ((HWInputButtonBtGPIO*)hw)->setBTName( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    return hw;
}

ConfigInputFaderWidget::ConfigInputFaderWidget(QWidget *parent, ConfigDialog* configDialog) : IConfigInputWidget(parent)
{
    m_configDialog = configDialog;

    QLabel* label = new QLabel("Select hardware", this);
    m_comboType = new QComboBox(this);
    m_comboType->addItem("Dummy");
    m_comboType->addItem("I2C");
    m_comboType->addItem("Bluetooth I2C");

    m_labelBtBoard = new QLabel("Select bluetooth board", this);
    m_comboBtBoard = new QComboBox(this);

    // add all bluetooth boards to the combo box
    const std::list<BTThread*>* listBT = m_configDialog->getListBTThread();
    for(std::list<BTThread*>::const_iterator it = listBT->begin(); it != listBT->end(); it++)
        m_comboBtBoard->addItem( QString::fromStdString( (*it)->getName() ) );

    m_labelI2CAddr = new QLabel("Select I2C address", this);
    m_spinI2CAddr = new QSpinBox(this);
    m_spinI2CAddr->setMinimum(0);
    m_spinI2CAddr->setMaximum(127);

    m_labelChannel = new QLabel("Select channel", this);
    m_spinChannel = new QSpinBox(this);
    m_spinChannel->setMinimum(0);
    m_spinChannel->setMaximum(7);

    m_buttonI2CScan = new QPushButton("Scan I2C", this);


    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // now add everything to the layout
    layout->addWidget(label, 0, 0);
    layout->addWidget(m_comboType, 0, 1);

    layout->addWidget(m_labelBtBoard, 1, 0);
    layout->addWidget(m_comboBtBoard, 1, 1);
    layout->addWidget(m_labelI2CAddr, 2, 0);
    layout->addWidget(m_spinI2CAddr, 2, 1);
    layout->addWidget(m_labelChannel, 3, 0);
    layout->addWidget(m_spinChannel, 3, 1);

    layout->addWidget(m_buttonI2CScan, 4, 1);


    this->setLayout(layout);

    // now connect all signals - slots
    connect(m_comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(m_buttonI2CScan, SIGNAL(clicked()), this, SLOT(i2cScan()));

    // set default
    this->typeChanged( m_comboType->currentIndex() );
}

void ConfigInputFaderWidget::typeChanged(int index)
{
    HWInput::HWType type = (HWInput::HWType)index;
    switch(type)
    {
    case HWInput::Dummy:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->hide();
        m_spinI2CAddr->hide();
        m_labelChannel->hide();
        m_spinChannel->hide();
        m_buttonI2CScan->hide();
        break;
    case HWInput::I2C:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelChannel->show();
        m_spinChannel->show();
        m_buttonI2CScan->show();
        break;
    case HWInput::BtI2C:
        m_labelBtBoard->show();
        m_comboBtBoard->show();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelChannel->show();
        m_spinChannel->show();
        m_buttonI2CScan->show();
        break;
    }
}

void ConfigInputFaderWidget::i2cScan()
{
    int slaveAddress = -1;

    HWInput::HWType type = (HWInput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWInput::I2C:
        slaveAddress = m_configDialog->i2cScan();
        break;
    case HWInput::BtI2C:
        slaveAddress = m_configDialog->btI2CScan( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    if(slaveAddress != -1)
        m_spinI2CAddr->setValue( slaveAddress );
}

void ConfigInputFaderWidget::edit(HWInput *hw)
{
    m_comboType->setCurrentIndex( hw->getHWType() );

    const char* btName = NULL;
    switch(hw->getHWType())
    {
    case HWInput::I2C:
        m_spinI2CAddr->setValue( ((HWInputFaderI2C*)hw)->getSlaveAddress() );
        m_spinChannel->setValue( ((HWInputFaderI2C*)hw)->getChannel() );
        break;
    case HWInput::BtI2C:
        m_spinI2CAddr->setValue( ((HWInputFaderBt*)hw)->getSlaveAddress() );
        m_spinChannel->setValue( ((HWInputFaderBt*)hw)->getChannel() );
        btName = ((HWInputFaderBt*)hw)->getBTName().c_str();
        break;
    }


    if(btName != NULL)
    {
        for(unsigned int i = 0; i < m_comboBtBoard->count();i ++)
        {
            if(m_comboBtBoard->itemText(i).compare( btName ) == 0)
            {
                m_comboBtBoard->setCurrentIndex(i);
                break;
            }
        }
    }
}

HWInput* ConfigInputFaderWidget::assemble()
{
    HWInputFader* hw = NULL;

    HWInput::HWType type = (HWInput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWInput::Dummy:
        hw = new HWInputFader();
        break;
    case HWInput::I2C:
        hw = new HWInputFaderI2C();
        ((HWInputFaderI2C*)hw)->setChannel( m_spinChannel->value() );
        ((HWInputFaderI2C*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        break;
    case HWInput::BtI2C:
        hw = new HWInputFaderBt();
        ((HWInputFaderBt*)hw)->setChannel( m_spinChannel->value() );
        ((HWInputFaderBt*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWInputFaderBt*)hw)->setBTName( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    return hw;
}




ConfigOutputDCMotorWidget::ConfigOutputDCMotorWidget(QWidget *parent, ConfigDialog* configDialog) : IConfigOutputWidget(parent)
{
    m_configDialog = configDialog;

    QLabel* label = new QLabel("Select hardware", this);
    m_comboType = new QComboBox(this);
    m_comboType->addItem("Dummy");
    m_comboType->addItem("I2C");
    m_comboType->addItem("Bluetooth I2C");

    m_labelBtBoard = new QLabel("Select bluetooth board", this);
    m_comboBtBoard = new QComboBox(this);

    // add all bluetooth boards to the combo box
    const std::list<BTThread*>* listBT = m_configDialog->getListBTThread();
    for(std::list<BTThread*>::const_iterator it = listBT->begin(); it != listBT->end(); it++)
        m_comboBtBoard->addItem( QString::fromStdString( (*it)->getName() ) );

    m_labelI2CAddr = new QLabel("Select I2C address", this);
    m_spinI2CAddr = new QSpinBox(this);
    m_spinI2CAddr->setMinimum(0);
    m_spinI2CAddr->setMaximum(127);

    m_buttonI2CScan = new QPushButton("Scan I2C", this);


    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // now add everything to the layout
    layout->addWidget(label, 0, 0);
    layout->addWidget(m_comboType, 0, 1);

    layout->addWidget(m_labelBtBoard, 1, 0);
    layout->addWidget(m_comboBtBoard, 1, 1);
    layout->addWidget(m_labelI2CAddr, 2, 0);
    layout->addWidget(m_spinI2CAddr, 2, 1);

    layout->addWidget(m_buttonI2CScan, 4, 1);


    this->setLayout(layout);

    // now connect all signals - slots
    connect(m_comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(m_buttonI2CScan, SIGNAL(clicked()), this, SLOT(i2cScan()));

    // set default
    this->typeChanged( m_comboType->currentIndex() );
}

void ConfigOutputDCMotorWidget::typeChanged(int index)
{
    HWOutput::HWType type = (HWOutput::HWType)index;
    switch(type)
    {
    case HWOutput::Dummy:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->hide();
        m_spinI2CAddr->hide();
        m_buttonI2CScan->hide();
        break;
    case HWOutput::I2C:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_buttonI2CScan->show();
        break;
    case HWOutput::BtI2C:
        m_labelBtBoard->show();
        m_comboBtBoard->show();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_buttonI2CScan->show();
        break;
    }
}

void ConfigOutputDCMotorWidget::i2cScan()
{
    int slaveAddress = -1;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::I2C:
        slaveAddress = m_configDialog->i2cScan();
        break;
    case HWOutput::BtI2C:
        slaveAddress = m_configDialog->btI2CScan( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    if(slaveAddress != -1)
        m_spinI2CAddr->setValue( slaveAddress );
}

void ConfigOutputDCMotorWidget::edit(HWOutput *hw)
{
    m_comboType->setCurrentIndex( hw->getHWType() );

    const char* btName = NULL;
    switch(hw->getHWType())
    {
    case HWOutput::I2C:
        m_spinI2CAddr->setValue( ((HWOutputDCMotorI2C*)hw)->getSlaveAddress() );
        break;
    case HWOutput::BtI2C:
        m_spinI2CAddr->setValue( ((HWOutputDCMotorBt*)hw)->getSlaveAddress() );
        btName = ((HWOutputDCMotorBt*)hw)->getBTName().c_str();
        break;
    }


    if(btName != NULL)
    {
        for(unsigned int i = 0; i < m_comboBtBoard->count();i ++)
        {
            if(m_comboBtBoard->itemText(i).compare( btName ) == 0)
            {
                m_comboBtBoard->setCurrentIndex(i);
                break;
            }
        }
    }
}

HWOutput* ConfigOutputDCMotorWidget::assemble()
{
    HWOutputDCMotor* hw = NULL;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::Dummy:
        hw = new HWOutputDCMotor();
        break;
    case HWOutput::I2C:
        hw = new HWOutputDCMotorI2C();
        ((HWOutputDCMotorI2C*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        break;
    case HWOutput::BtI2C:
        hw = new HWOutputDCMotorBt();
        ((HWOutputDCMotorBt*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWOutputDCMotorBt*)hw)->setBTName( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    return hw;
}


ConfigOutputStepperWidget::ConfigOutputStepperWidget(QWidget *parent, ConfigDialog* configDialog) : IConfigOutputWidget(parent)
{
    m_configDialog = configDialog;

    QLabel* label = new QLabel("Select hardware", this);
    m_comboType = new QComboBox(this);
    m_comboType->addItem("Dummy");
    m_comboType->addItem("I2C");
    m_comboType->addItem("Bluetooth I2C");

    m_labelBtBoard = new QLabel("Select bluetooth board", this);
    m_comboBtBoard = new QComboBox(this);

    // add all bluetooth boards to the combo box
    const std::list<BTThread*>* listBT = m_configDialog->getListBTThread();
    for(std::list<BTThread*>::const_iterator it = listBT->begin(); it != listBT->end(); it++)
        m_comboBtBoard->addItem( QString::fromStdString( (*it)->getName() ) );

    m_labelI2CAddr = new QLabel("Select I2C address", this);
    m_spinI2CAddr = new QSpinBox(this);
    m_spinI2CAddr->setMinimum(0);
    m_spinI2CAddr->setMaximum(127);

    m_buttonI2CScan = new QPushButton("Scan I2C", this);


    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // now add everything to the layout
    layout->addWidget(label, 0, 0);
    layout->addWidget(m_comboType, 0, 1);

    layout->addWidget(m_labelBtBoard, 1, 0);
    layout->addWidget(m_comboBtBoard, 1, 1);
    layout->addWidget(m_labelI2CAddr, 2, 0);
    layout->addWidget(m_spinI2CAddr, 2, 1);

    layout->addWidget(m_buttonI2CScan, 4, 1);


    this->setLayout(layout);

    // now connect all signals - slots
    connect(m_comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(m_buttonI2CScan, SIGNAL(clicked()), this, SLOT(i2cScan()));

    // set default
    this->typeChanged( m_comboType->currentIndex() );
}

void ConfigOutputStepperWidget::typeChanged(int index)
{
    HWOutput::HWType type = (HWOutput::HWType)index;
    switch(type)
    {
    case HWOutput::Dummy:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->hide();
        m_spinI2CAddr->hide();
        m_buttonI2CScan->hide();
        break;
    case HWOutput::I2C:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_buttonI2CScan->show();
        break;
    case HWOutput::BtI2C:
        m_labelBtBoard->show();
        m_comboBtBoard->show();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_buttonI2CScan->show();
        break;
    }
}

void ConfigOutputStepperWidget::i2cScan()
{
    int slaveAddress = -1;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::I2C:
        slaveAddress = m_configDialog->i2cScan();
        break;
    case HWOutput::BtI2C:
        slaveAddress = m_configDialog->btI2CScan( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    if(slaveAddress != -1)
        m_spinI2CAddr->setValue( slaveAddress );
}

void ConfigOutputStepperWidget::edit(HWOutput *hw)
{
    m_comboType->setCurrentIndex( hw->getHWType() );

    const char* btName = NULL;
    switch(hw->getHWType())
    {
    case HWOutput::I2C:
        m_spinI2CAddr->setValue( ((HWOutputStepperI2C*)hw)->getSlaveAddress() );
        break;
    case HWOutput::BtI2C:
        m_spinI2CAddr->setValue( ((HWOutputStepperBt*)hw)->getSlaveAddress() );
        btName = ((HWOutputStepperBt*)hw)->getBTName().c_str();
        break;
    }


    if(btName != NULL)
    {
        for(unsigned int i = 0; i < m_comboBtBoard->count();i ++)
        {
            if(m_comboBtBoard->itemText(i).compare( btName ) == 0)
            {
                m_comboBtBoard->setCurrentIndex(i);
                break;
            }
        }
    }
}

HWOutput* ConfigOutputStepperWidget::assemble()
{
    HWOutputStepper* hw = NULL;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::Dummy:
        hw = new HWOutputStepper();
        break;
    case HWOutput::I2C:
        hw = new HWOutputStepperI2C();
        ((HWOutputStepperI2C*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        break;
    case HWOutput::BtI2C:
        hw = new HWOutputStepperBt();
        ((HWOutputStepperBt*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWOutputStepperBt*)hw)->setBTName( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    return hw;
}


ConfigOutputLEDWidget::ConfigOutputLEDWidget(QWidget *parent, ConfigDialog* configDialog) : IConfigOutputWidget(parent)
{
    m_configDialog = configDialog;

    QLabel* label = new QLabel("Select hardware", this);
    m_comboType = new QComboBox(this);
    m_comboType->addItem("Dummy");
    m_comboType->addItem("I2C");
    m_comboType->addItem("Bluetooth I2C");

    m_labelBtBoard = new QLabel("Select bluetooth board", this);
    m_comboBtBoard = new QComboBox(this);

    // add all bluetooth boards to the combo box
    const std::list<BTThread*>* listBT = m_configDialog->getListBTThread();
    for(std::list<BTThread*>::const_iterator it = listBT->begin(); it != listBT->end(); it++)
        m_comboBtBoard->addItem( QString::fromStdString( (*it)->getName() ) );

    m_labelI2CAddr = new QLabel("Select I2C address", this);
    m_spinI2CAddr = new QSpinBox(this);
    m_spinI2CAddr->setMinimum(0);
    m_spinI2CAddr->setMaximum(127);

    m_labelChannel = new QLabel("Select channel", this);
    m_spinChannel = new QSpinBox(this);
    m_spinChannel->setMinimum(0);
    m_spinChannel->setMaximum(15);

    m_buttonI2CScan = new QPushButton("Scan I2C", this);


    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // now add everything to the layout
    layout->addWidget(label, 0, 0);
    layout->addWidget(m_comboType, 0, 1);

    layout->addWidget(m_labelBtBoard, 1, 0);
    layout->addWidget(m_comboBtBoard, 1, 1);
    layout->addWidget(m_labelI2CAddr, 2, 0);
    layout->addWidget(m_spinI2CAddr, 2, 1);

    layout->addWidget(m_labelChannel, 3, 0);
    layout->addWidget(m_spinChannel, 3, 1);

    layout->addWidget(m_buttonI2CScan, 4, 1);


    this->setLayout(layout);

    // now connect all signals - slots
    connect(m_comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(m_buttonI2CScan, SIGNAL(clicked()), this, SLOT(i2cScan()));

    // set default
    this->typeChanged( m_comboType->currentIndex() );
}

void ConfigOutputLEDWidget::typeChanged(int index)
{
    HWOutput::HWType type = (HWOutput::HWType)index;
    switch(type)
    {
    case HWOutput::Dummy:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->hide();
        m_spinI2CAddr->hide();
        m_labelChannel->hide();
        m_spinChannel->hide();
        m_buttonI2CScan->hide();
        break;
    case HWOutput::I2C:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelChannel->show();
        m_spinChannel->show();
        m_buttonI2CScan->show();
        break;
    case HWOutput::BtI2C:
        m_labelBtBoard->show();
        m_comboBtBoard->show();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelChannel->show();
        m_spinChannel->show();
        m_buttonI2CScan->show();
        break;
    }
}

void ConfigOutputLEDWidget::i2cScan()
{
    int slaveAddress = -1;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::I2C:
        slaveAddress = m_configDialog->i2cScan();
        break;
    case HWOutput::BtI2C:
        slaveAddress = m_configDialog->btI2CScan( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    if(slaveAddress != -1)
        m_spinI2CAddr->setValue( slaveAddress );
}

void ConfigOutputLEDWidget::edit(HWOutput *hw)
{
    m_comboType->setCurrentIndex( hw->getHWType() );

    const char* btName = NULL;
    switch(hw->getHWType())
    {
    case HWOutput::I2C:
        m_spinI2CAddr->setValue( ((HWOutputLEDI2C*)hw)->getSlaveAddress() );
        m_spinChannel->setValue( ((HWOutputLEDI2C*)hw)->getChannel() );
        break;
    case HWOutput::BtI2C:
        m_spinI2CAddr->setValue( ((HWOutputLEDBt*)hw)->getSlaveAddress() );
        m_spinChannel->setValue( ((HWOutputLEDBt*)hw)->getChannel() );
        btName = ((HWOutputLEDBt*)hw)->getBTName().c_str();
        break;
    }


    if(btName != NULL)
    {
        for(unsigned int i = 0; i < m_comboBtBoard->count();i ++)
        {
            if(m_comboBtBoard->itemText(i).compare( btName ) == 0)
            {
                m_comboBtBoard->setCurrentIndex(i);
                break;
            }
        }
    }
}

HWOutput* ConfigOutputLEDWidget::assemble()
{
    HWOutputLED* hw = NULL;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::Dummy:
        hw = new HWOutputLED();
        break;
    case HWOutput::I2C:
        hw = new HWOutputLEDI2C();
        ((HWOutputLEDI2C*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWOutputLEDI2C*)hw)->setChannel( m_spinChannel->value() );
        break;
    case HWOutput::BtI2C:
        hw = new HWOutputLEDBt();
        ((HWOutputLEDBt*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWOutputLEDBt*)hw)->setChannel( m_spinChannel->value() );
        ((HWOutputLEDBt*)hw)->setBTName( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    return hw;
}


ConfigOutputRelayWidget::ConfigOutputRelayWidget(QWidget *parent, ConfigDialog* configDialog) : IConfigOutputWidget(parent)
{
    m_configDialog = configDialog;

    QLabel* label = new QLabel("Select hardware", this);
    m_comboType = new QComboBox(this);
    m_comboType->addItem("Dummy");
    m_comboType->addItem("I2C");
    m_comboType->addItem("Bluetooth I2C");

    m_labelBtBoard = new QLabel("Select bluetooth board", this);
    m_comboBtBoard = new QComboBox(this);

    // add all bluetooth boards to the combo box
    const std::list<BTThread*>* listBT = m_configDialog->getListBTThread();
    for(std::list<BTThread*>::const_iterator it = listBT->begin(); it != listBT->end(); it++)
        m_comboBtBoard->addItem( QString::fromStdString( (*it)->getName() ) );

    m_labelI2CAddr = new QLabel("Select I2C address", this);
    m_spinI2CAddr = new QSpinBox(this);
    m_spinI2CAddr->setMinimum(0);
    m_spinI2CAddr->setMaximum(127);

    m_labelChannel = new QLabel("Select channel", this);
    m_spinChannel = new QSpinBox(this);
    m_spinChannel->setMinimum(0);
    m_spinChannel->setMaximum(15);

    m_buttonI2CScan = new QPushButton("Scan I2C", this);


    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // now add everything to the layout
    layout->addWidget(label, 0, 0);
    layout->addWidget(m_comboType, 0, 1);

    layout->addWidget(m_labelBtBoard, 1, 0);
    layout->addWidget(m_comboBtBoard, 1, 1);
    layout->addWidget(m_labelI2CAddr, 2, 0);
    layout->addWidget(m_spinI2CAddr, 2, 1);

    layout->addWidget(m_labelChannel, 3, 0);
    layout->addWidget(m_spinChannel, 3, 1);

    layout->addWidget(m_buttonI2CScan, 4, 1);


    this->setLayout(layout);

    // now connect all signals - slots
    connect(m_comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(m_buttonI2CScan, SIGNAL(clicked()), this, SLOT(i2cScan()));

    // set default
    this->typeChanged( m_comboType->currentIndex() );
}

void ConfigOutputRelayWidget::typeChanged(int index)
{
    HWOutput::HWType type = (HWOutput::HWType)index;
    switch(type)
    {
    case HWOutput::Dummy:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->hide();
        m_spinI2CAddr->hide();
        m_labelChannel->hide();
        m_spinChannel->hide();
        m_buttonI2CScan->hide();
        break;
    case HWOutput::I2C:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelChannel->show();
        m_spinChannel->show();
        m_buttonI2CScan->show();
        break;
    case HWOutput::BtI2C:
        m_labelBtBoard->show();
        m_comboBtBoard->show();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelChannel->show();
        m_spinChannel->show();
        m_buttonI2CScan->show();
        break;
    }
}

void ConfigOutputRelayWidget::i2cScan()
{
    int slaveAddress = -1;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::I2C:
        slaveAddress = m_configDialog->i2cScan();
        break;
    case HWOutput::BtI2C:
        slaveAddress = m_configDialog->btI2CScan( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    if(slaveAddress != -1)
        m_spinI2CAddr->setValue( slaveAddress );
}

void ConfigOutputRelayWidget::edit(HWOutput *hw)
{
    m_comboType->setCurrentIndex( hw->getHWType() );

    const char* btName = NULL;
    switch(hw->getHWType())
    {
    case HWOutput::I2C:
        m_spinI2CAddr->setValue( ((HWOutputRelayI2C*)hw)->getSlaveAddress() );
        m_spinChannel->setValue( ((HWOutputRelayI2C*)hw)->getChannel() );
        break;
    case HWOutput::BtI2C:
        m_spinI2CAddr->setValue( ((HWOutputRelayBt*)hw)->getSlaveAddress() );
        m_spinChannel->setValue( ((HWOutputRelayBt*)hw)->getChannel() );
        btName = ((HWOutputRelayBt*)hw)->getBTName().c_str();
        break;
    }


    if(btName != NULL)
    {
        for(unsigned int i = 0; i < m_comboBtBoard->count();i ++)
        {
            if(m_comboBtBoard->itemText(i).compare( btName ) == 0)
            {
                m_comboBtBoard->setCurrentIndex(i);
                break;
            }
        }
    }
}

HWOutput* ConfigOutputRelayWidget::assemble()
{
    HWOutputRelay* hw = NULL;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::Dummy:
        hw = new HWOutputRelay();
        break;
    case HWOutput::I2C:
        hw = new HWOutputRelayI2C();
        ((HWOutputRelayI2C*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWOutputRelayI2C*)hw)->setChannel( m_spinChannel->value() );
        break;
    case HWOutput::BtI2C:
        hw = new HWOutputRelayBt();
        ((HWOutputRelayBt*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWOutputRelayBt*)hw)->setChannel( m_spinChannel->value() );
        ((HWOutputRelayBt*)hw)->setBTName( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    return hw;
}


ConfigOutputGPOWidget::ConfigOutputGPOWidget(QWidget *parent, ConfigDialog* configDialog) : IConfigOutputWidget(parent)
{
    m_configDialog = configDialog;

    QLabel* label = new QLabel("Select hardware", this);
    m_comboType = new QComboBox(this);
    m_comboType->addItem("Dummy");
    m_comboType->addItem("I2C");
    m_comboType->addItem("Bluetooth I2C");

    m_labelBtBoard = new QLabel("Select bluetooth board", this);
    m_comboBtBoard = new QComboBox(this);

    // add all bluetooth boards to the combo box
    const std::list<BTThread*>* listBT = m_configDialog->getListBTThread();
    for(std::list<BTThread*>::const_iterator it = listBT->begin(); it != listBT->end(); it++)
        m_comboBtBoard->addItem( QString::fromStdString( (*it)->getName() ) );

    m_labelI2CAddr = new QLabel("Select I2C address", this);
    m_spinI2CAddr = new QSpinBox(this);
    m_spinI2CAddr->setMinimum(0);
    m_spinI2CAddr->setMaximum(127);

    m_labelPort = new QLabel("Select channel", this);
    m_spinPort = new QSpinBox(this);
    m_spinPort->setMinimum(0);
    m_spinPort->setMaximum(15);

    m_buttonI2CScan = new QPushButton("Scan I2C", this);


    QGridLayout* layout = new QGridLayout(this);

    // remove spacing around widget, it looks kind of odd otherwise
    layout->setContentsMargins(0, 0, 0, 0);

    // now add everything to the layout
    layout->addWidget(label, 0, 0);
    layout->addWidget(m_comboType, 0, 1);

    layout->addWidget(m_labelBtBoard, 1, 0);
    layout->addWidget(m_comboBtBoard, 1, 1);
    layout->addWidget(m_labelI2CAddr, 2, 0);
    layout->addWidget(m_spinI2CAddr, 2, 1);

    layout->addWidget(m_labelPort, 3, 0);
    layout->addWidget(m_spinPort, 3, 1);

    layout->addWidget(m_buttonI2CScan, 4, 1);


    this->setLayout(layout);

    // now connect all signals - slots
    connect(m_comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(m_buttonI2CScan, SIGNAL(clicked()), this, SLOT(i2cScan()));

    // set default
    this->typeChanged( m_comboType->currentIndex() );
}

void ConfigOutputGPOWidget::typeChanged(int index)
{
    HWOutput::HWType type = (HWOutput::HWType)index;
    switch(type)
    {
    case HWOutput::Dummy:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->hide();
        m_spinI2CAddr->hide();
        m_labelPort->hide();
        m_spinPort->hide();
        m_buttonI2CScan->hide();
        break;
    case HWOutput::I2C:
        m_labelBtBoard->hide();
        m_comboBtBoard->hide();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelPort->show();
        m_spinPort->show();
        m_buttonI2CScan->show();
        break;
    case HWOutput::BtI2C:
        m_labelBtBoard->show();
        m_comboBtBoard->show();
        m_labelI2CAddr->show();
        m_spinI2CAddr->show();
        m_labelPort->show();
        m_spinPort->show();
        m_buttonI2CScan->show();
        break;
    }
}

void ConfigOutputGPOWidget::i2cScan()
{
    int slaveAddress = -1;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::I2C:
        slaveAddress = m_configDialog->i2cScan();
        break;
    case HWOutput::BtI2C:
        slaveAddress = m_configDialog->btI2CScan( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    if(slaveAddress != -1)
        m_spinI2CAddr->setValue( slaveAddress );
}

void ConfigOutputGPOWidget::edit(HWOutput *hw)
{
    m_comboType->setCurrentIndex( hw->getHWType() );

    const char* btName = NULL;
    switch(hw->getHWType())
    {
    case HWOutput::I2C:
        m_spinI2CAddr->setValue( ((HWOutputGPOI2C*)hw)->getSlaveAddress() );
        m_spinPort->setValue( ((HWOutputGPOI2C*)hw)->getPort() );
        break;
    case HWOutput::BtI2C:
        m_spinI2CAddr->setValue( ((HWOutputGPOBt*)hw)->getSlaveAddress() );
        m_spinPort->setValue( ((HWOutputGPOBt*)hw)->getPort() );
        btName = ((HWOutputGPOBt*)hw)->getBTName().c_str();
        break;
    }


    if(btName != NULL)
    {
        for(unsigned int i = 0; i < m_comboBtBoard->count();i ++)
        {
            if(m_comboBtBoard->itemText(i).compare( btName ) == 0)
            {
                m_comboBtBoard->setCurrentIndex(i);
                break;
            }
        }
    }
}

HWOutput* ConfigOutputGPOWidget::assemble()
{
    HWOutputGPO* hw = NULL;

    HWOutput::HWType type = (HWOutput::HWType)m_comboType->currentIndex();
    switch(type)
    {
    case HWOutput::Dummy:
        hw = new HWOutputGPO();
        break;
    case HWOutput::I2C:
        hw = new HWOutputGPOI2C();
        ((HWOutputGPOI2C*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWOutputGPOI2C*)hw)->setPort( m_spinPort->value() );
        break;
    case HWOutput::BtI2C:
        hw = new HWOutputGPOBt();
        ((HWOutputGPOBt*)hw)->setSlaveAddress( m_spinI2CAddr->value() );
        ((HWOutputGPOBt*)hw)->setPort( m_spinPort->value() );
        ((HWOutputGPOBt*)hw)->setBTName( m_comboBtBoard->currentText().toStdString() );
        break;
    }

    return hw;
}
