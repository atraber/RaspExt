#ifndef CONDITIONTABLEMODEL_H
#define CONDITIONTABLEMODEL_H

#include <QAbstractTableModel>

#include "script/Rule.h"

class ConditionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ConditionTableModel(QObject *parent, Rule* rule);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    Condition* get(unsigned int row) const;

    void removeRow(int row, const QModelIndex &parent = QModelIndex());
    void add(Condition* condition, int row = -1);
    void editRow(int row, Condition* condition);

private:
    Rule* m_rule;
};

#endif // CONDITIONTABLEMODEL_H
