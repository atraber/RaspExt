
#include "ui/ScriptsTableModel.h"

#include <qdir.h>


ScriptsTableModel::ScriptsTableModel(QObject* parent) : QAbstractTableModel(parent)
{
    QDir directory = QDir("scripts/");
    QStringList entries = directory.entryList(QStringList("*.xml"));

    for(QStringList::iterator it = entries.begin(); it != entries.end(); it++)
    {
        QString name = (*it);

        // remove ".xml" ending
        name.chop(4);

        Script* script = Script::load( name.toStdString() );

        // if script is NULL, then the script could not be loaded. This means its either invalid or could not be opened
        if(script != NULL)
            m_vecScripts.push_back(script);
    }
}

ScriptsTableModel::~ScriptsTableModel()
{
    for(std::vector<Script*>::iterator it = m_vecScripts.begin(); it != m_vecScripts.end(); it++)
    {
        delete (*it);
    }
}

int ScriptsTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_vecScripts.size();
}

int ScriptsTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant ScriptsTableModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid())
         return QVariant();

     if (index.row() >= this->m_vecScripts.size() || index.row() < 0)
         return QVariant();

     if (role == Qt::DisplayRole)
     {
         Script* script = this->m_vecScripts.at(index.row());

         if (index.column() == 0)
             return QString::fromStdString( script->getName() );
         else if (index.column() == 1)
             return QString::fromStdString( script->getDescription() );
     }
     return QVariant();
}

QVariant ScriptsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
     if (role != Qt::DisplayRole)
         return QVariant();

     if (orientation == Qt::Horizontal)
     {
         switch (section)
         {
             case 0:
                 return tr("Name");

             case 1:
                 return tr("Description");

             default:
                 return QVariant();
         }
     }
     return QVariant();
}

Script* ScriptsTableModel::getScript(unsigned int row) const
{
    if(row <= m_vecScripts.size())
        return m_vecScripts.at(row);

    return NULL;
}

void ScriptsTableModel::addScript(Script* script)
{
    beginInsertRows(QModelIndex(), this->rowCount(QModelIndex()), this->rowCount(QModelIndex()));

    m_vecScripts.push_back(script);

    endInsertRows();
}

bool ScriptsTableModel::removeRow(int row, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    QString filename = "scripts/";
    filename.append( m_vecScripts.at(row)->getName().c_str() );
    filename.append(".xml");

    // delete the actual file
    QFile::remove(filename);

    beginRemoveRows(QModelIndex(), row, row);

    m_vecScripts.erase(m_vecScripts.begin() + row);

    endRemoveRows();

    return true;
}

/**
 * @brief ScriptsTableModel::modifyRow
 * This method modifies the row given by the parameter row and sets its content to script.
 * If the given script is not equal to the "old" script in this row, the old script gets deleted.
 * If it is equal, the row content gets updated
 * @param row
 * @param script
 */
void ScriptsTableModel::modifyRow(int row, Script *script)
{
    Script* oldScript = m_vecScripts.at(row);

    if(oldScript != script)
        delete oldScript;

    m_vecScripts[row] = script;

    emit dataChanged(this->index(row, 0), this->index(row, this->columnCount()));
}
