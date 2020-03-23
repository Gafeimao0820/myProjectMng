/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CUSTOMTABLEMODEL_H
#define CUSTOMTABLEMODEL_H

#include <QtCore/QAbstractTableModel>
#include <QtCore/QHash>
#include <QtCore/QRect>
#include <QTimer>
#include <QObject>

#include "fsdatactrl.h"

typedef  QHash<qint32, FsClipDescribeStruct*> ClipDescStructCacheHash;
typedef  QHash<qint32, FsClipDescribeStruct*>::iterator ClipDescStructCacheHashIter;
typedef  QList<qint32> ClipDataTableLis;
typedef  QList<qint32>::iterator ClipDataTableListIter;

class CustomTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit CustomTableModel(QObject *parent = 0);
    virtual ~CustomTableModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    // clipinfo conver method
    QVariant getClipInfoByColumnIndex_ForTableView(const int streamId, const int column) const;
    void setClipInfoByColumnIndex_ForTableView(const int streamId, const int column, const QVariant &value) const;

    // extern api
    bool updateClipInfo(FsClipDescribeStruct &clipDescStruct);

    // comment method
    void Sleep(int msec);

signals:
    void rowChange();

    // slot
private slots:
    void timeHit();    

private:
    ClipDataTableLis m_clipDataTableList;       // use for table view
    ClipDescStructCacheHash m_clipDataHash;     // cache clip info
    QList<QString> m_tableHeader;
    int m_columnCount;
    int m_rowCount;         // record current usable clip count.
    QTimer *m_ptimer;       // use for time to reflush.
};

#endif // CUSTOMTABLEMODEL_H
