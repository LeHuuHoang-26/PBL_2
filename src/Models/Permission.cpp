#include "Permission.h"
#include "libPBL2.h"
#include "AppConstants.h"
#include <fstream>
#include <memory>
#include <sstream>

Permission::Permission(std::string perId, std::string perName, std::string desc)
    : Per_id(perId), Per_Name(perName), description(desc) {}

Permission::Permission(const Permission &other)
    : Per_id(other.Per_id), Per_Name(other.Per_Name), description(other.description) {}

std::string Permission::getPer_id() const { return Per_id; }
std::string Permission::getPer_Name() const { return Per_Name; }

bool Permission::isDefault() const {
    return Per_id.empty() && Per_Name.empty();
}

void Permission::setDefault() {
    Per_id.clear();
    Per_Name.clear();
    description.clear();
}

LinkList<Permission>* Permission::getSystemCatalog(const std::string &path) {
    auto catalog = std::make_unique<LinkList<Permission>>();
    const std::string cfgPath = path.empty() ? std::string(AppConstants::Paths::PERMISSION_CATALOG) : path;
    std::ifstream in(cfgPath);
    if (in.is_open()) {
        std::string line;
        while (std::getline(in, line)) {
            libPBL2::trim(line);
            if (line.empty() || line.find("Per_ID") != std::string::npos || line.find("---") != std::string::npos) {
                continue;
            }
            std::stringstream ss(line);
            std::string perId, perName, desc;
            if (!std::getline(ss, perId, '|') || !std::getline(ss, perName, '|')) {
                continue;
            }
            std::getline(ss, desc);
            libPBL2::trim(perId);
            libPBL2::trim(perName);
            libPBL2::trim(desc);
            if (!perId.empty() && !perName.empty()) {
                catalog->insert(Permission(perId, perName, desc));
            }
        }
        in.close();
    }

    if (catalog->getCount() == 0) {
        catalog->insert(Permission("Per_1", "Cấp quyền tạm thời làm admin có thời hạn", "Cho phép tài khoản tạm thời có toàn quyền quản trị phòng khám."));
        catalog->insert(Permission("Per_2", "Chỉnh sửa thông tin bác sĩ", "Cho phép thêm, sửa, khóa hoặc điều chỉnh hồ sơ bác sĩ."));
        catalog->insert(Permission("Per_3", "Chỉnh sửa thông tin bệnh nhân", "Cho phép quản lý và cập nhật thông tin hồ sơ bệnh nhân."));
        catalog->insert(Permission("Per_4", "Chỉnh sửa thông tin lịch hẹn khám bệnh", "Cho phép lên lịch hẹn, dời lịch, hủy hoặc sửa trạng thái lịch khám."));
        catalog->insert(Permission("Per_5", "Chỉnh sửa lịch nghỉ lễ / Tết", "Cho phép điều chỉnh danh sách các ngày nghỉ lễ của phòng khám."));
    }
    return catalog.release();
}
