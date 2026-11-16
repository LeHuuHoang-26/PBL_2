#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include "../Models/Manager.h"
#include "../DataStructures/LinkList.h"
#include <string>
#include <QString>

class AuthController {
public:
    static bool login(LinkList<Manager> *managers, const QString &username, const QString &password, QString &errorMessage, Manager *&outManager);
};

#endif // AUTHCONTROLLER_H
