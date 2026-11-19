#include "Permission.h"
#include <memory>

Permission::Permission(std::string perId, std::string perName, std::string desc)
    : Per_id(perId), Per_Name(perName), description(desc) {}

Permission::Permission(const Permission &other)
    : Per_id(other.Per_id), Per_Name(other.Per_Name), description(other.description) {}

std::string Permission::getPer_id() const { return Per_id; }
std::string Permission::getPer_Name() const { return Per_Name; }
std::string Permission::getDescription() const { return description; }

void Permission::setPer_id(const std::string &id) { Per_id = id; }
void Permission::setPer_Name(const std::string &name) { Per_Name = name; }
void Permission::setDescription(const std::string &desc) { description = desc; }

bool Permission::isDefault() const {
    return Per_id.empty() && Per_Name.empty();
}

void Permission::setDefault() {
    Per_id.clear();
    Per_Name.clear();
    description.clear();
}

LinkList<Permission>* Permission::getSystemCatalog() {
    auto catalog = std::make_unique<LinkList<Permission>>();
    catalog->insert(Permission("Per_1", "Cấp quyền tạm thời làm admin có thời hạn", "Cho phép tài khoản tạm thời có toàn quyền quản trị phòng khám."));
    catalog->insert(Permission("Per_2", "Chỉnh sửa thông tin bác sĩ", "Cho phép thêm, sửa, khóa hoặc điều chỉnh hồ sơ bác sĩ."));
    catalog->insert(Permission("Per_3", "Chỉnh sửa thông tin bệnh nhân", "Cho phép quản lý và cập nhật thông tin hồ sơ bệnh nhân."));
    catalog->insert(Permission("Per_4", "Chỉnh sửa thông tin lịch hẹn khám bệnh", "Cho phép lên lịch hẹn, dời lịch, hủy hoặc sửa trạng thái lịch khám."));
    catalog->insert(Permission("Per_5", "Chỉnh sửa lịch nghỉ lễ / Tết", "Cho phép điều chỉnh danh sách các ngày nghỉ lễ của phòng khám."));
    return catalog.release();
}
