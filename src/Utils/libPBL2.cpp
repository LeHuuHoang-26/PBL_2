#include "libPBL2.h"
#include "Manager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

namespace libPBL2 {

void trim(string &str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == string::npos) {
        str.clear();
        return;
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    str = str.substr(start, end - start + 1);
}

void filterNewline(string &str) {
    string res = "";
    for (char c : str) {
        if (c != '\r' && c != '\n') {
            res += c;
        }
    }
    str = res;
}

size_t utf8Length(const string &str) {
    size_t len = 0;
    for (unsigned char c : str) {
        if ((c & 0xC0) != 0x80) {
            len++;
        }
    }
    return len;
}

string padRightUtf8(const string &str, size_t targetWidth) {
    size_t ulen = utf8Length(str);
    if (ulen >= targetWidth) return str;
    return str + string(targetWidth - ulen, ' ');
}

bool isAllowed(const Manager &M, PermissionType Per, const LinkList<Permission> *PermissionList) {
    if (!M.isActive())
        return false;

    if (Per == PermissionType::Empty)
        return true;

    if (M.getRole() == ManagerRole::Administrator)
        return true;

    if (PermissionList == nullptr || PermissionList->getHeader() == nullptr)
        return false;

    LinkList<Permission>::Node *tmp = PermissionList->getHeader()->next;
    while (tmp != nullptr && tmp->next != nullptr) {
        if (M.getID() == tmp->info.MA_id && tmp->info.Per_id == Per)
            return true;
        tmp = tmp->next;
    }

    return false;
}

string getProjectRoot() {
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH) return ".";
    string path(buffer);
    size_t pos = path.find_last_of("/\\");
    string exeDir = (pos == string::npos) ? "." : path.substr(0, pos);

    fs::path p(exeDir);
    if (p.filename() == "bin") {
        return p.parent_path().string();
    }
    return exeDir;
}

string getDataPath(const string &subPath) {
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string exeDir = ".";
    if (len > 0 && len < MAX_PATH) {
        string path(buffer);
        size_t pos = path.find_last_of("/\\");
        exeDir = (pos == string::npos) ? "." : path.substr(0, pos);
    }

    // 1. Kiểm tra vị trí chuẩn: ../data/<subPath> (khi exe chạy từ bin/ trong cấu trúc project)
    fs::path p1 = fs::path(exeDir) / ".." / "data" / subPath;
    if (fs::exists(p1)) {
        return fs::weakly_canonical(p1).string();
    }

    // 2. Kiểm tra vị trí đóng gói: data/<subPath> cùng cấp với .exe (khi deploy/phân phối)
    fs::path p2 = fs::path(exeDir) / "data" / subPath;
    if (fs::exists(p2)) {
        return fs::weakly_canonical(p2).string();
    }

    // 3. Kiểm tra working directory hiện tại: data/<subPath>
    fs::path p3 = fs::path("data") / subPath;
    if (fs::exists(p3)) {
        return fs::weakly_canonical(p3).string();
    }

    // 4. Kiểm tra working directory cha: ../data/<subPath>
    fs::path p4 = fs::path("..") / "data" / subPath;
    if (fs::exists(p4)) {
        return fs::weakly_canonical(p4).string();
    }

    // Mặc định trả về đường dẫn chuẩn dạng canonical
    return fs::weakly_canonical(p1).string();
}

} // namespace libPBL2


