
#include "ui/ConditionTableModel.h"
#include "script/Action.h"
#include "script/Condition.h"


ConditionTableModel::ConditionTableModel(QObject* parent, Rule* rule) : QAbstractTableModel(parent)
{
    m_rule = rule;
}

int ConditionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_rule->m_listConditions.size();
}

int ConditionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant ConditionTableModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid())
         return QVariant();

     if (index.row() >= m_rule->m_listConditions.size() || index.row() < 0)
         return QVariant();

     if (role == Qt::DisplayRole)
     {
         Condition* condition = m_rule->m_listConditions.at(index.row());

         if (index.column() == 0)
             return QString::fromStdString( condition->getDescription() );
     }
     return QVariant();
}

QVariant ConditionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
     if (role != Qt::DisplayRole)
         return QVariant();

     if (orientation == Qt::Horizontal) {
         switch (section) {
             case 0:
                 return tr("Description");

             default:
                 return QVariant();
         }
     }
     return QVariant();
}

void ConditionTableModel::removeRow(int row, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows(QModelIndex(), row, row);

    m_rule->m_listConditions.erase( m_rule->m_listConditions.begin() + row);

    endRemoveRows();
}

void ConditionTableModel::add(Condition *condition)
{
    beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount());

    m_rule->addCondition(condition);

    endInsertRows();
}

void ConditionTableModel::editRow(int row, Condition *condition)
{
    m_rule->editCondition( m_rule->m_listConditions.at( row ), condition );
}

Condition* ConditionTableModel::get(unsigned int row) const
{
    return m_rule->m_listConditions.at(row);
}
