#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include "Manager.h"
#include "LinkList.h"
#include <QString>

class AuthController {
public:
    static bool login(LinkList<Manager> *managers, const QString &username, const QString &password, QString &errorMessage, Manager *&outManager);
};

#endif
