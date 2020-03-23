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

#include "customtablemodel.h"
#include <QtCore/QVector>
#include <QtCore/QTime>
#include <QtCore/QRect>
#include <QtCore/QRandomGenerator>
#include <QtGui/QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>

struct sortData{
    QVariant data;
    qreal streamId;
};

/***************************************************************************************
 *
 ****************************************************************************************/


CustomTableModel::CustomTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    m_columnCount = 0;
    m_rowCount = 64;

    // m_tableHeader
    {
        m_tableHeader.clear();               m_columnCount = 0;
//        m_tableHeader.append("checkSum");    m_columnCount++;
        m_tableHeader.append("clipId");      m_columnCount++;
//        m_tableHeader.append("clipStatus");  m_columnCount++;
        m_tableHeader.append("perClipId");   m_columnCount++;
        m_tableHeader.append("nextClipId");  m_columnCount++;
        m_tableHeader.append("streamId");    m_columnCount++;
        m_tableHeader.append("minTime");     m_columnCount++;
        m_tableHeader.append("maxTime");     m_columnCount++;
        m_tableHeader.append("datSpLen(MB)");    m_columnCount++;
        m_tableHeader.append("datSpUseLen(MB)"); m_columnCount++;
//        m_tableHeader.append("coverCnt");    m_columnCount++;
    }

    // clip cache
    {
        m_clipDataHash.clear();
        m_clipDataTableList.clear();
    }

    // timer to reflush tableview
    m_ptimer = new QTimer(this);
    m_ptimer->setInterval(1000);
    connect(m_ptimer, SIGNAL(timeout()), this, SLOT(timeHit()));
    m_ptimer->start();
}

CustomTableModel::~CustomTableModel()
{
    qDeleteAll(m_clipDataHash);
}

void CustomTableModel::Sleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

int CustomTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
//    qDebug() << "table tmp row count:" << m_clipDataTableList.count();
//    qDebug() << "table real row count:" << m_clipDataHash.size();
    return m_clipDataTableList.count();
//    return m_rowCount;
}

int CustomTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
//    qDebug() << "table column count:" << m_columnCount;
    return m_columnCount;
}

QVariant CustomTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        QString header;
        switch(section)
        {
            case 0:  header = m_tableHeader.at(section); break;
            case 1:  header = m_tableHeader.at(section); break;
            case 2:  header = m_tableHeader.at(section); break;
            case 3:  header = m_tableHeader.at(section); break;
            case 4:  header = m_tableHeader.at(section); break;
            case 5:  header = m_tableHeader.at(section); break;
            case 6:  header = m_tableHeader.at(section); break;
            case 7:  header = m_tableHeader.at(section); break;
            case 8:  header = m_tableHeader.at(section); break;
            case 9:  header = m_tableHeader.at(section); break;
            case 10: header = m_tableHeader.at(section); break;
            case 11: header = m_tableHeader.at(section); break;
            default: header = ""; break;
        }
        return header;
    }
    else
        return QString("%1").arg(section + 1);
}

QVariant CustomTableModel::getClipInfoByColumnIndex_ForTableView(const int streamId, const int column) const
{
    QVariant var;
    FsClipDescribeStruct *pClip = nullptr;
    pClip = m_clipDataHash.value(streamId);
    if(!pClip)
        goto EXIT;

    switch (column) {
        case 0:  var.setValue(pClip->clipHead.clipId);      break;
        case 1:  var.setValue(pClip->clipHead.perClipId);   break;
        case 2:  var.setValue(pClip->clipHead.nextClipId);  break;
        case 3:  var.setValue(pClip->clipHead.streamId);    break;
        case 4:
        {
            var.setValue( QDateTime::fromTime_t(pClip->clipHead.minTime).toString("yyyy-MM-dd HH:mm:ss") );
            break;
        }
        case 5:
        {
            var.setValue( QDateTime::fromTime_t(pClip->clipHead.maxTime).toString("yyyy-MM-dd HH:mm:ss") );
            break;
        }
        case 6:
        {
            double tmpd = pClip->clipHead.datSpLen / 1024.0 / 1024.0;
            var.setValue(tmpd);
            break;
        }
        case 7:
        {
            double tmpd = pClip->clipHead.datSpUseLen / 1024.0 / 1024.0;
            var.setValue(tmpd);
            break;
        }
        default:
            break;
    }

EXIT:
    return var;
}

void CustomTableModel::setClipInfoByColumnIndex_ForTableView(const int streamId, const int column, const QVariant &value) const
{
    FsClipDescribeStruct *pClip = nullptr;
    pClip = m_clipDataHash.value(streamId);
    if(!pClip)
        goto EXIT;

    switch (column) {
        case 0:  pClip->clipHead.clipId      = fsUint32(value.toInt());      break;
        case 1:  pClip->clipHead.perClipId   = fsUint32(value.toInt());      break;
        case 2:  pClip->clipHead.nextClipId  = fsUint32(value.toInt());      break;
        case 3:  pClip->clipHead.streamId    = fsUint32(value.toInt());      break;
        case 4:  pClip->clipHead.minTime     = fsUint32(value.toInt());      break;
        case 5:  pClip->clipHead.maxTime     = fsUint32(value.toInt());      break;
        case 6:  pClip->clipHead.datSpLen    = fsUint32(value.toDouble());   break;
        case 7:  pClip->clipHead.datSpUseLen = fsUint32(value.toDouble());   break;
        default:
            break;
    }

EXIT:
    return ;
}

QVariant CustomTableModel::data(const QModelIndex &index, int role) const
{
    int streamId = 0;
    if(m_clipDataTableList.size() <= index.row())
    {
        goto EXIT;
    }

    streamId = m_clipDataTableList.at(index.row());
    if(streamId <= 0)
    {
        goto EXIT;
    }

//    qDebug() << "table model data display clipId:" << clipId;
    if (role == Qt::DisplayRole) {
        return getClipInfoByColumnIndex_ForTableView(streamId, index.column());
    } else if (role == Qt::EditRole) {
        return getClipInfoByColumnIndex_ForTableView(streamId, index.column());
    }

EXIT:
    return QVariant();
}

bool CustomTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int clipId = 0;
    clipId = m_clipDataTableList[index.row()];

    if (index.isValid() && role == Qt::EditRole) {
        setClipInfoByColumnIndex_ForTableView(clipId, index.column(), value);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags CustomTableModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool sortClipDescend(struct sortData &A, struct sortData &B)
{
    if( A.data > B.data )
        return true;
    return false;
}

bool sortClipAescend(struct sortData &A, struct sortData &B)
{
    if( A.data < B.data )
        return true;
    return false;
}

void CustomTableModel::sort(int column, Qt::SortOrder order)
{
//    qDebug() << "sort " << " column: " << column << " order:" << order;

    struct sortData tmpSortData;
    QList<struct sortData> tmpList;

    tmpList.clear();
    for (ClipDataTableListIter cIter =  m_clipDataTableList.begin();
                               cIter != m_clipDataTableList.end();
                               cIter++)
    {
        tmpSortData.data = getClipInfoByColumnIndex_ForTableView(*cIter, column);
        tmpSortData.streamId = *cIter;
        tmpList.append(tmpSortData);
    }

    if( order == Qt::DescendingOrder)
    {
        // 降序 从大到小
        qSort(tmpList.begin(), tmpList.end(), sortClipDescend);
    }
    else if( order == Qt::AscendingOrder)
    {
        // 升序 从小到大
        qSort(tmpList.begin(), tmpList.end(), sortClipAescend);
    }
    else
    {
        return;
    }

    m_clipDataTableList.clear();
    for (QList<struct sortData>::iterator cIter =  tmpList.begin();
                                          cIter != tmpList.end();
                                          cIter++)
    {
        m_clipDataTableList.append(cIter->streamId);
    }

    timeHit();
}

void CustomTableModel::timeHit()
{
//    qDebug() << "timeHit" << m_rowCount - 1 << " " << m_columnCount - 1;
    QModelIndex topLeft = createIndex(0, 0);
    QModelIndex bottomRight = createIndex(m_rowCount - 1, m_columnCount - 1);
    emit dataChanged(topLeft, bottomRight);;
}


bool CustomTableModel::updateClipInfo(FsClipDescribeStruct &clipDescStruct)
{
    bool ret = false;
    int streamId = 0;
    int indexRow = 0;
    FsClipDescribeStruct *pClip = NULL;

    streamId = int(clipDescStruct.clipHead.streamId);
    if(streamId <= 0)
    {
        qDebug() << "invaild streamId:" << streamId;
        goto EXIT;
    }

    // update old data
    for (ClipDataTableListIter cIter = m_clipDataTableList.begin();
                               cIter != m_clipDataTableList.end();
                               cIter++)
    {
        if (*cIter == streamId)
        {
            pClip = m_clipDataHash.value(streamId);
            *pClip = clipDescStruct;
            ret = true;
            goto EXIT;
        }
        indexRow++;
    }

    // insert new data
    if(1)
    {
        pClip = new FsClipDescribeStruct;
        if(!pClip)
        {
            qDebug() << "new FsClipDescribeStruct fail!!!";
            goto EXIT;
        }
        memcpy(pClip, &clipDescStruct, sizeof(FsClipDescribeStruct));
        m_clipDataTableList.append(streamId);
        m_clipDataHash.insert(streamId, pClip);

        emit rowChange();
        ret = true;
    }

EXIT:
    return ret;
}
