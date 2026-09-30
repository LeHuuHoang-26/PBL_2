#ifndef LIBPBL2_H
#define LIBPBL2_H
#include <string>
#include <fstream>
#include "Manager.h"
#include "LinkList.h"

enum class PermissionType {
    Empty = 0,
    Per_01,
    Per_02,
    Per_03,
    Per_04,
    Per_05,
    Per_06,
    Per_07,
    Per_08,
    Per_00
};

class libPBL2 {
    public:
        static void trim(std::string &);
        struct Permission {
            std::string MA_id;
            PermissionType Per_id;
        };
        static bool isAllowed(const Manager &M, const PermissionType &Per, const LinkList<Permission> *PermissionList = nullptr);
        template <typename T>
        static LinkList<T> *LoadFromFile(const std::string &path);
        template <typename T>
        static T getFromStr(const std::string &str);
        template <typename T>
        static std::string toStr(const T &);
};

template <>
LinkList<Manager>* libPBL2::LoadFromFile<Manager>(const std::string &path);
template <>
LinkList<libPBL2::Permission>* libPBL2::LoadFromFile<libPBL2::Permission>(const std::string &path);
template <>
PermissionType libPBL2::getFromStr<PermissionType>(const std::string &str);
template <>
std::string libPBL2::toStr(const PermissionType &PerT);
template <>
std::string libPBL2::toStr(const ManagerRole &MA_Role);
#endif