/**
 *  Copyright 2020 by Yuri Alexandrov <evilruff@gmail.com>
 *
 * This file is part of some open source application.
 *
 * Some open source application is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QLogView.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

#ifndef _xValueListModel_h_
#define _xValueListModel_h_ 1

#include <QObject>
#include <QAbstractTableModel>

class xValueListModel : public QAbstractTableModel {
    Q_OBJECT
public:
    xValueListModel(QObject *pParent = nullptr);
    ~xValueListModel();

};

typedef std::function<QVariant( int /*row*/,int /*column*/, int /*role*/)> DataFunction;
typedef std::function<Qt::ItemFlags(int /*row*/, int /*column*/, Qt::ItemFlags /*flags*/)> FlagsFunction;
typedef std::function<QVariant( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/)> HeaderFunction;

template <typename T> class xValueCollection: public xValueListModel {
public:

    xValueCollection(QObject *pParent = nullptr) :
        xValueListModel(pParent) {

    }

    ~xValueCollection() {

    }

    void        setItem(int nIndex, const T & item) {
        if ((nIndex < 0) || (nIndex >= m_items.size()))
            return;
        
        m_items[nIndex] = item;
        dataChanged(index(nIndex, 0), index(nIndex, 0));        
    }

    void        setItems(const QVector<T> & items) {
        emit layoutAboutToBeChanged();
        m_items = items;
        emit layoutChanged();
    }

    void        appendItems(const QVector<T> & items) {
        beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + items.size());
        m_items << items;
        endInsertRows();
    }

    void        appendItem(const T & item) {
        beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + 1);
        m_items << item;
        endInsertRows();
    }

    void        insertItem(const T & item, int before) {
        beginInsertRows(QModelIndex(), before, before + 1);
        m_items.insert(before, item);
        endInsertRows();
    }

    int         indexOf(const T & item) const {
        QVector<T>::const_iterator it = std::find(m_items.begin(), m_items.end(), item);
        if (it == m_items.end())
            return -1;

        return std::distance(m_items.begin(), it);
    }

    bool        contains(const T & item) const {
        return (indexOf(item) >= 0);
    }

    bool        removeItem(const T & item) {
        int index = indexOf(item);
        if (index == -1)
            return false;
        
        beginRemoveRows(QModelIndex(), index, index);
        m_items.remove(index);
        endInsertRows();

        return true;
    }

    void        clear() {
        emit layoutAboutToBeChanged();
        m_items.clear();
        emit layoutChanged();
    }

    void        setDataCallback(DataFunction f, int role = -1) {
        funcDataCallbacks[role] = f;
    }

    void        setHeaderCallback(HeaderFunction f, int role = -1) {
        funcHeaderCallbacks[role] = f;
    }

    void        setFlagsCallback(FlagsFunction f) {
        funcFlags = f;
    }

    void        setColumntCount(int cnt) {
        m_columnCount = cnt;
    }
        
    virtual int      rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return m_items.count();
    }

    virtual int      columnCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);       
        return m_columnCount;
    }

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if ((index.row() >= rowCount()) || (index.column() >= columnCount()))
            return QVariant();

        if (funcDataCallbacks.contains(role)) {
            return funcDataCallbacks[role](index.row(), index.column(), role);
        } else if (funcDataCallbacks.contains(-1)) {
            return funcDataCallbacks[-1](index.row(), index.column(), role);
        }

        return QVariant();
    }

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
    
        if (funcHeaderCallbacks.contains(role)) {
            return funcHeaderCallbacks[role](section, orientation, role);
        }
        else if (funcHeaderCallbacks.contains(-1)) {
            return funcHeaderCallbacks[-1](section, orientation, role);
        }

        return QVariant();
    }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const override {
        Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

        if ((index.row() >= rowCount()) || (index.column() >= columnCount()))
            return defaultFlags;
    
        if (funcFlags)
            return funcFlags(index.row(), index.column(), defaultFlags);
        
        return defaultFlags;
    }

    const T &   itemAt(int row) {
        if (row >= m_items.size())
            return m_emptyValue;

        return m_items[row];
    }

    const QVector<T>  & items() const {
        return m_items;
    }

protected:
    const T                     m_emptyValue;
    QVector<T>                  m_items;
    int                         m_columnCount   = 0;

    QMap<int, DataFunction>     funcDataCallbacks;
    QMap<int, HeaderFunction>   funcHeaderCallbacks;
    FlagsFunction               funcFlags = nullptr;
};



#endif