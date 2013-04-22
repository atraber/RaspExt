#ifndef ACTIONTABLEMODEL_H
#define ACTIONTABLEMODEL_H

#include <QAbstractTableModel>

#include "script/Rule.h"

class ActionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ActionTableModel(QObject *parent, Rule* rule);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    Action* get(unsigned int row) const;

    bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    void add(Action* action);
    void edit(int row, Action* action);
    void swap(int src, int dst);

private:
    Rule* m_rule;
};

#endif // ACTIONTABLEMODEL_H
