
#include "ui/ActionTableModel.h"
#include "script/Action.h"

ActionTableModel::ActionTableModel(QObject* parent, Rule* rule) : QAbstractTableModel(parent)
{
    m_rule = rule;
}

int ActionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_rule->m_listActions.size();
}

int ActionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant ActionTableModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid())
         return QVariant();

     if (index.row() >= m_rule->m_listActions.size() || index.row() < 0)
         return QVariant();

     if (role == Qt::DisplayRole)
     {
         Action* action = m_rule->m_listActions.at(index.row());

         if (index.column() == 0)
             return QString::fromStdString( action->getDescription() );
     }
     return QVariant();
}

QVariant ActionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

bool ActionTableModel::removeRow(int row, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows(QModelIndex(), row, row);

    m_rule->m_listActions.erase( m_rule->m_listActions.begin() + row);

    endRemoveRows();

    return true;
}


void ActionTableModel::add(Action* action)
{
    beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount());

    m_rule->addAction(action);

    endInsertRows();
}

void ActionTableModel::edit(int row, Action *action)
{
    m_rule->editAction( m_rule->m_listActions.at( row ), action );
}

Action* ActionTableModel::get(unsigned int row) const
{
    return m_rule->m_listActions.at(row);
}

void ActionTableModel::swap(int src, int dst)
{
    beginMoveRows(QModelIndex(), src, src, QModelIndex(), dst);

    std::swap(m_rule->m_listActions[src], m_rule->m_listActions[dst] );

    endMoveRows();
}
