#ifndef REQUESTMAPPER_H
#define REQUESTMAPPER_H

#include "mainwindow.h"
#include "httprequesthandler.h"

/**
  The request mapper dispatches incoming HTTP requests to controller classes
  depending on the requested path.
*/

class RequestMapper : public HttpRequestHandler {
    Q_OBJECT
    Q_DISABLE_COPY(RequestMapper)
public:

    /**
      Constructor.
      @param parent Parent object
    */
    RequestMapper(QObject* parent = NULL, MainWindow *pMain = NULL);

    /**
      Destructor.
    */
    ~RequestMapper();

    /**
      Dispatch incoming HTTP requests to different controllers depending on the URL.
      @param request The received HTTP request
      @param response Must be used to return the response
    */
    void service(HttpRequest& request, HttpResponse& response);

private:
    MainWindow *m_pMainWindow;
};

#endif // REQUESTMAPPER_H
