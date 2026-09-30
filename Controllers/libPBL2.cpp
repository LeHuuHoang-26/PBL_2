#include "libPBL2.h"
#include <string>
#include <fstream>
#include "Manager.h"
#include "LinkList.h"
using namespace std;

void libPBL2::trim(string &str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == string::npos) {
        str.clear();
        return;
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    str = str.substr(start, end - start + 1);
}

bool libPBL2::isAllowed(const Manager &M, const PermissionType &Per, const LinkList<Permission> *PermissionList) {
    if (!M.isActive())
        return false;

    if (Per == PermissionType::Empty)
        return true;

    if (M.getRole() == ManagerRole::Administrator)
        return true;

    if (PermissionList == nullptr)
        return false;

    LinkList<Permission>::Node *tmp = PermissionList->getHeader();
    while (tmp->next != nullptr) {
        if (M.getID() == tmp->info.MA_id && tmp->info.Per_id == Per)
            return true;
        tmp = tmp->next;
    }

    return false;
}

template<>
LinkList<Manager> *libPBL2::LoadFromFile(const string &path) {
    LinkList<Manager> *ManagerList = new LinkList<Manager>;
    string tmp;

    ifstream f(path);
    if(!f.is_open())
        return nullptr;
    
    getline(f, tmp);
    getline(f, tmp);

    int flag = 1;
    while (flag) {
        flag = 0;
        Manager M(f);
        if (!M.isDefault()) {
            flag = 1;
            ManagerList->insert(M);
        } 
    }
    return ManagerList;
}

template<>
LinkList<libPBL2::Permission> *libPBL2::LoadFromFile(const string &path) {
    LinkList<libPBL2::Permission> *PerList = new LinkList<libPBL2::Permission>;
    string tmp;

    ifstream f(path);
    if (!f.is_open())
        return nullptr;

    getline(f, tmp);
    getline(f, tmp);

    while (1) {
        libPBL2::Permission tmpPer;
        if (!getline(f, tmpPer.MA_id, '|')) {
            break;
        }

        if (!getline(f, tmp, '\n')) {
            break;
        }

        libPBL2::trim(tmpPer.MA_id);
        libPBL2::trim(tmp);
        tmpPer.Per_id = libPBL2::getFromStr<PermissionType>(tmp);

        PerList->insert(tmpPer);
    }
    return PerList;
}

template<>
PermissionType libPBL2::getFromStr<PermissionType>(const std::string &str) {
    if (str == "Per_00") return PermissionType::Per_00;
    if (str == "Per_01") return PermissionType::Per_01;
    if (str == "Per_02") return PermissionType::Per_02;
    if (str == "Per_03") return PermissionType::Per_03;
    if (str == "Per_04") return PermissionType::Per_04;
    if (str == "Per_05") return PermissionType::Per_05;
    if (str == "Per_06") return PermissionType::Per_06;
    if (str == "Per_07") return PermissionType::Per_07;
    if (str == "Per_08") return PermissionType::Per_08;
    return PermissionType::Empty;
}

template <>
std::string libPBL2::toStr(const PermissionType &PerT) {
    if (PerT == PermissionType::Per_00) return "Per_00";
    if (PerT == PermissionType::Per_01) return "Per_01";
    if (PerT == PermissionType::Per_02) return "Per_02";
    if (PerT == PermissionType::Per_03) return "Per_03";
    if (PerT == PermissionType::Per_04) return "Per_04";
    if (PerT == PermissionType::Per_05) return "Per_05";
    if (PerT == PermissionType::Per_06) return "Per_06";
    if (PerT == PermissionType::Per_07) return "Per_07";
    if (PerT == PermissionType::Per_08) return "Per_08";
    return "Empty";
}

template <>
std::string libPBL2::toStr(const ManagerRole &MA_Role) {
    if (MA_Role == ManagerRole::Administrator) return "Administrator";
    if (MA_Role == ManagerRole::Receptionist || MA_Role == ManagerRole::SpecialReceptionist) return "Receptionist";
    return "Empty";
}

