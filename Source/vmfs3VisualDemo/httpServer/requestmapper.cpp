#include <QCoreApplication>
#include <QThread>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include "requestmapper.h"
#include "filelogger.h"
#include <QDebug>
#include "fsdatactrl.h"

RequestMapper::RequestMapper(QObject* parent, MainWindow *pMain)
    :HttpRequestHandler(parent)
{
    m_pMainWindow = pMain;
//    qDebug("RequestMapper: created");
}


RequestMapper::~RequestMapper()
{
//    qDebug("RequestMapper: deleted");
}

int clipStructConvertJson2Struct(const QByteArray jsonData, FsClipDescribeStruct &clipDescStruct)
{
    int ret = 0;

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseJsonErr);
    if(!(parseJsonErr.error == QJsonParseError::NoError))
    {
        qDebug() << ("解析json文件错误！");
        ret = -1;
        goto EXIT;
    }

    do{
        QJsonObject jsonObject = document.object();

        // journal
        {}

        // clipHead
        if(jsonObject.contains(QStringLiteral("clipHead")))
        {
            QJsonValue jsonValueList = jsonObject.value(QStringLiteral("clipHead"));
            QJsonObject item = jsonValueList.toObject();
            _FsClipHead *pClipHead = &clipDescStruct.clipHead;
            /*
            "clipHead": {
                    "maxTime": 1578984942.0,
                    "datSpLen": 535744512.0,
                    "coverCnt": 2.0,
                    "datSpUseLen": 13973481.0,
                    "clipStatus": 0.0,
                    "resv[5]": 0.0,
                    "minTime": 1578984914.0,
                    "streamId": 109.0,
                    "clipId": 6989.0,
                    "perClipId": 6927.0,
                    "nextClipId": 0.0,
                    "checkSum": 595.0
                }
            */
            pClipHead->clipId       = fsUint32(item["clipId"].toInt());
            pClipHead->perClipId    = fsUint32(item["perClipId"].toInt());
            pClipHead->nextClipId   = fsUint32(item["nextClipId"].toInt());
            pClipHead->streamId     = fsUint32(item["streamId"].toInt());
            pClipHead->minTime      = fsUint32(item["minTime"].toInt());
            pClipHead->maxTime      = fsUint32(item["maxTime"].toInt());
            pClipHead->datSpLen     = fsUint32(item["datSpLen"].toInt());
            pClipHead->datSpUseLen  = fsUint32(item["datSpUseLen"].toInt());
            pClipHead->coverCnt     = fsUint32(item["coverCnt"].toInt());
            pClipHead->clipStatus   = fsUint32(item["clipStatus"].toInt());
            pClipHead->checkSum     = fsUint32(item["checkSum"].toInt());
        }

        // keyFrameHead
        {}

        // keyFrameIndex
        {}

        // keyFrameInfo
        {}
    }while(0);

EXIT:
    return ret;
}

void RequestMapper::service(HttpRequest& request, HttpResponse& response)
{
    QByteArray path   = request.getPath();
    QByteArray method = request.getMethod();
    QByteArray body   = request.getBody();

    FsClipDescribeStruct clipDescStruct;
    memset(&clipDescStruct, 0x00, sizeof(clipDescStruct));

    //    qDebug("RequestMapper: path=%s",path.data());
    //    qDebug("RequestMapper: clipid=%s", clipId.data());
    //    qDebug("RequestMapper: method=%s",method.data());
    //    qDebug("RequestMapper: body=%s",body.data());

    if ( !clipStructConvertJson2Struct(body, clipDescStruct) )
    {
        // conver success, update tablemodel clip cache
        m_pMainWindow->updateClipInfo(clipDescStruct);
    }

    response.setStatus(200);

//    qDebug("RequestMapper: finished request");
}
