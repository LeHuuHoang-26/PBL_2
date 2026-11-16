#include "Permission.h"
#include "libPBL2.h"

using namespace std;

// ==========================================
// Constructors & Destructor
// ==========================================
Permission::Permission(string perId, string perName, string desc)
    : Per_id(perId), Per_Name(perName), description(desc) {}

Permission::Permission(const Permission &other)
    : Per_id(other.Per_id), Per_Name(other.Per_Name), description(other.description) {}

// ==========================================
// Getters & Setters
// ==========================================
string Permission::getPer_id() const { return Per_id; }
string Permission::getPer_Name() const { return Per_Name; }
string Permission::getDescription() const { return description; }

void Permission::setPer_id(const string &id) { Per_id = id; }
void Permission::setPer_Name(const string &name) { Per_Name = name; }
void Permission::setDescription(const string &desc) { description = desc; }

// ==========================================
// State Helpers
// ==========================================
bool Permission::isDefault() const {
    return Per_id.empty() && Per_Name.empty();
}

void Permission::setDefault() {
    Per_id.clear();
    Per_Name.clear();
    description.clear();
}

// ==========================================
// Type Conversions
// ==========================================
string Permission::permissionTypeToString(PermissionType per) {
    switch (per) {
        case PermissionType::Per_1: return "Per_1";
        case PermissionType::Per_2: return "Per_2";
        case PermissionType::Per_3: return "Per_3";
        case PermissionType::Per_4: return "Per_4";
        case PermissionType::Per_5: return "Per_5";
        default:                    return "Empty";
    }
}

// ==========================================
// System Permission Catalog
// ==========================================
LinkList<Permission>* Permission::getSystemCatalog() {
    auto *catalog = new LinkList<Permission>;
    catalog->insert(Permission("Per_1", "Cấp quyền tạm thời làm admin có thời hạn", "Cho phép tài khoản tạm thời có toàn quyền quản trị phòng khám."));
    catalog->insert(Permission("Per_2", "Chỉnh sửa thông tin bác sĩ", "Cho phép thêm, sửa, khóa hoặc điều chỉnh hồ sơ bác sĩ."));
    catalog->insert(Permission("Per_3", "Chỉnh sửa thông tin bệnh nhân", "Cho phép quản lý và cập nhật thông tin hồ sơ bệnh nhân."));
    catalog->insert(Permission("Per_4", "Chỉnh sửa thông tin lịch hẹn khám bệnh", "Cho phép lên lịch hẹn, dời lịch, hủy hoặc sửa trạng thái lịch khám."));
    catalog->insert(Permission("Per_5", "Chỉnh sửa lịch nghỉ lễ / Tết", "Cho phép điều chỉnh danh sách các ngày nghỉ lễ của phòng khám."));
    return catalog;
}
