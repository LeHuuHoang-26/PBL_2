#include "AccountView.h"
#include "MainWindow.h"
#include "ManagerController.h"
#include "UIHelper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QLabel>
#include <QFrame>
#include <QHeaderView>
#include <QShowEvent>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QAction>
#include <QIcon>
#include <QTimer>

struct PermissionDef {
    const char *id;
    const char *name;
};

static const PermissionDef permissionDefinitions[] = {
    {"Per_1", "Cấp quyền tạm thời làm admin có thời hạn"},
    {"Per_2", "Chỉnh sửa thông tin bác sĩ"},
    {"Per_3", "Chỉnh sửa thông tin bệnh nhân"},
    {"Per_4", "Chỉnh sửa thông tin lịch hẹn khám bệnh"},
    {"Per_5", "Chỉnh sửa lịch nghỉ lễ / Tết"}
};
static const int permissionDefinitionCount = 5;

AccountView::AccountView(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent), mainWindow(mainWindow) {
    setupUI();
    refresh();
}

void AccountView::resetToFirstPage() {
    currentPage = 0;
    refresh();
}

void AccountView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    currentPage = 0;
    refresh();
}

void AccountView::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 32, 36, 32);
    layout->setSpacing(16);

    // Header Title
    auto *h = new QVBoxLayout;
    h->setSpacing(4);
    auto *t = new QLabel("Quản lý tài khoản");
    t->setObjectName("pageTitle");
    auto *s = new QLabel("Quản lý người dùng nhân viên và phân quyền bảo mật cho hệ thống.");
    s->setObjectName("pageSubtitle");
    h->addWidget(t);
    h->addWidget(s);
    layout->addLayout(h);

    // Actions bar: Bỏ "Khóa / mở khóa tài khoản", chỉ giữ lại "+ Thêm quản lý"
    auto *actions = new QHBoxLayout;
    addBtn = UIHelper::button("＋  Thêm quản lý");
    actions->addWidget(addBtn);
    actions->addStretch();
    layout->addLayout(actions);

    // Table: Bỏ cột mã, đặt các cột thẳng hàng tương tự tổng quan
    table = UIHelper::table({"HỌ VÀ TÊN", "TÀI KHOẢN", "VAI TRÒ", "TRẠNG THÁI", ""});
    table->setCursor(Qt::PointingHandCursor);
    table->setWordWrap(false);
    table->setTextElideMode(Qt::ElideRight);
    table->verticalHeader()->setDefaultSectionSize(48);

    if (table->horizontalHeaderItem(0)) table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(1)) table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(2)) table->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(3)) table->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(4)) table->horizontalHeaderItem(4)->setTextAlignment(Qt::AlignCenter);

    // Tự động scale các cột sao cho tổng chiều rộng các cột luôn bằng chính xác chiều rộng bảng
    UIHelper::setupTableScaling(table, {0.30, 0.26, 0.26, 0.18}, 56);

    // Kích hoạt tính năng click tiêu đề cột để QuickSort
    table->horizontalHeader()->setSectionsClickable(true);
    table->horizontalHeader()->setSortIndicatorShown(false);
    table->horizontalHeader()->setCursor(Qt::PointingHandCursor);
    connect(table->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
        if (logicalIndex == 4) return; // Bỏ qua cột thao tác xóa
        if (sortColumn == logicalIndex) {
            sortOrder = (sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            sortColumn = logicalIndex;
            sortOrder = Qt::AscendingOrder;
        }
        currentPage = 0;
        refresh();
    });

    // Khung thẻ bo tròn hoàn thiện 4 góc, chống tràn viền
    auto *tableCard = UIHelper::createTableCard(table);
    layout->addWidget(tableCard, 1);

    // Pagination: Thêm page với stack là 8
    auto *pagination = new QHBoxLayout;
    pagination->setContentsMargins(4, 4, 4, 0);

    totalLabel = new QLabel("Tổng số: 0 tài khoản");
    totalLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #475569;");
    pagination->addWidget(totalLabel);

    pagination->addStretch();

    prevBtn = UIHelper::button("‹  Trang trước", "secondary");
    nextBtn = UIHelper::button("Trang sau  ›", "secondary");
    pageStatus = new QLabel("Trang 1 / 1");
    pageStatus->setStyleSheet("font-size: 12.5px; font-weight: 700; color: #0f172a; padding: 0 10px;");

    pagination->addWidget(prevBtn);
    pagination->addWidget(pageStatus);
    pagination->addWidget(nextBtn);
    layout->addLayout(pagination);

    connect(prevBtn, &QPushButton::clicked, this, [this] {
        if (currentPage > 0) {
            currentPage--;
            refresh();
        }
    });

    connect(nextBtn, &QPushButton::clicked, this, [this] {
        currentPage++;
        refresh();
    });

    connect(addBtn, &QPushButton::clicked, this, [this] {
        addManagerDialog();
        refresh();
    });

    // Dup click vào hàng thì có thể sửa thông tin (bỏ qua cột xóa)
    connect(table, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        if (col == 4) return;
        if (row < 0 || row >= table->rowCount()) return;
        auto *item = table->item(row, 0);
        if (!item) return;
        std::string id = UIHelper::ss(item->data(Qt::UserRole).toString());
        if (id.empty()) return;
        editManagerDialog(id);
    });
}

void AccountView::refresh() {
    if (!mainWindow || !table) return;
    table->setRowCount(0);

    auto *managers = mainWindow->getManagers();
    if (!managers) return;

    QList<Manager*> allList;
    auto *n = managers->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getValid()) {
            allList.append(&n->info);
        }
        n = n->next;
    }

    // Sắp xếp danh sách tài khoản bằng QuickSort mẫu (Template QuickSort)
    if (sortColumn >= 0) {
        UIHelper::quickSort(allList, [this](const Manager *a, const Manager *b) -> bool {
            if (a == b) return false;
            bool asc = (sortOrder == Qt::AscendingOrder);
            switch (sortColumn) {
                case 0: { // Họ và tên
                    int cmp = UIHelper::compareVietnameseNames(UIHelper::qs(a->getfullName()), UIHelper::qs(b->getfullName()));
                    if (cmp != 0) return asc ? (cmp < 0) : (cmp > 0);
                    break;
                }
                case 1: { // Tài khoản (Username)
                    int cmp = QString::localeAwareCompare(UIHelper::qs(a->getUsername()), UIHelper::qs(b->getUsername()));
                    if (cmp != 0) return asc ? (cmp < 0) : (cmp > 0);
                    break;
                }
                case 2: { // Vai trò
                    if (static_cast<int>(a->getRole()) != static_cast<int>(b->getRole()))
                        return asc ? (static_cast<int>(a->getRole()) < static_cast<int>(b->getRole()))
                                   : (static_cast<int>(a->getRole()) > static_cast<int>(b->getRole()));
                    break;
                }
                case 3: { // Trạng thái
                    if (a->getState() != b->getState())
                        return asc ? (a->getState() < b->getState()) : (a->getState() > b->getState());
                    break;
                }
                default:
                    break;
            }
            return asc ? (a->getID() < b->getID()) : (a->getID() > b->getID());
        });
    }

    int total = allList.size();
    int totalPages = total == 0 ? 1 : (total + PAGE_SIZE - 1) / PAGE_SIZE;
    currentPage = qBound(0, currentPage, totalPages - 1);

    if (totalLabel) {
        totalLabel->setText(QStringLiteral("Tổng số: %1 tài khoản").arg(total));
    }
    if (pageStatus) {
        pageStatus->setText(QStringLiteral("Trang %1 / %2").arg(currentPage + 1).arg(totalPages));
    }
    if (prevBtn) prevBtn->setEnabled(currentPage > 0);
    if (nextBtn) nextBtn->setEnabled(currentPage < totalPages - 1);

    int start = currentPage * PAGE_SIZE;
    int end = qMin(start + PAGE_SIZE, total);

    for (int i = start; i < end; ++i) {
        Manager *m = allList[i];
        int r = table->rowCount();
        table->insertRow(r);
        table->setRowHeight(r, 48);

        // Cột 0: Họ và tên (Lưu ID vào UserRole để double click lấy ra)
        auto *item0 = new QTableWidgetItem(UIHelper::qs(m->getfullName()));
        item0->setData(Qt::UserRole, UIHelper::qs(m->getID()));
        item0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QFont f = item0->font();
        f.setWeight(QFont::DemiBold);
        item0->setFont(f);
        item0->setForeground(QColor("#0f172a"));
        table->setItem(r, 0, item0);

        // Cột 1: Tài khoản
        auto *item1 = new QTableWidgetItem(UIHelper::qs(m->getUsername()));
        item1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item1->setForeground(QColor("#334155"));
        table->setItem(r, 1, item1);

        // Cột 2: Vai trò
        auto *item2 = new QTableWidgetItem(ManagerController::roleName(m->getRole()));
        item2->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item2->setForeground(QColor("#334155"));
        table->setItem(r, 2, item2);

        // Cột 3: Trạng thái (thẳng hàng giữa với tiêu đề TRẠNG THÁI)
        bool active = m->isActive();
        auto *item3 = new QTableWidgetItem(active ? "● Hoạt động" : "● Đã khóa");
        item3->setTextAlignment(Qt::AlignCenter);
        item3->setForeground(active ? QColor("#059669") : QColor("#dc2626"));
        QFont sf = item3->font();
        sf.setWeight(QFont::Bold);
        item3->setFont(sf);
        table->setItem(r, 3, item3);

        // Cột 4: Nút xóa tài khoản (icon thùng rác đỏ)
        auto action = UIHelper::createActionCell(UIHelper::trashIcon(), "Xóa tài khoản");
        connect(action.second, &QPushButton::clicked, this, [this, m]() {
            if (!mainWindow) return;
            Manager *cur = mainWindow->getCurrentManager();
            if (cur && cur->getID() == m->getID()) {
                UIHelper::showWarning(this, "Không thể xóa", "Bạn không thể xóa tài khoản hiện đang đăng nhập vào hệ thống.");
                return;
            }
            if (ManagerController::isRootAdmin(m)) {
                UIHelper::showWarning(this, "Không thể xóa", "Không thể xóa tài khoản Quản trị viên gốc của hệ thống.");
                return;
            }
            QString name = UIHelper::qs(m->getfullName());
            if (!UIHelper::confirmDelete(this, "Xác nhận xóa tài khoản",
                QStringLiteral("Bạn có chắc chắn muốn xóa tài khoản \"%1\" khỏi hệ thống?\nTài khoản sau khi xóa sẽ bị vô hiệu hóa hoàn toàn.").arg(name),
                "Xóa tài khoản", "Không xóa")) {
                return;
            }
            m->setValid(false);
            mainWindow->saveAll();
            refresh();
        });
        table->setCellWidget(r, 4, action.first);
    }
}

namespace {

struct PermCheckItem {
    QCheckBox *box = nullptr;
    QSpinBox *spin = nullptr;
};

struct ManagerFormData {
    QLineEdit *name = nullptr;
    QLineEdit *phone = nullptr;
    QSpinBox *age = nullptr;
    QComboBox *gender = nullptr;
    QLineEdit *username = nullptr;
    QLineEdit *password = nullptr;
    QComboBox *role = nullptr;
    QComboBox *statusCombo = nullptr;
};

QFrame *createManagerFormCard(ManagerFormData &form,
                              bool isEditMode = false,
                              const QString &initName = "",
                              const QString &initPhone = "",
                              int initAge = 28,
                              bool initGender = true,
                              const QString &initUsername = "",
                              const QString &initPassword = "",
                              ManagerRole initRole = ManagerRole::Receptionist,
                              bool initActive = true,
                              bool disableUsername = false,
                              bool disableRole = false,
                              bool disableStatus = false) {
    auto *card = new QFrame;
    card->setObjectName("managerCard");
    card->setStyleSheet("QFrame#managerCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 20);
    cardLayout->setSpacing(12);

    auto makeField = [](const QString &labelText, QWidget *inputWidget) {
        auto *col = new QVBoxLayout;
        col->setSpacing(5);
        auto *lbl = new QLabel(labelText);
        lbl->setStyleSheet("font-weight: 700; font-size: 12px; color: #475569;");
        col->addWidget(lbl);
        col->addWidget(inputWidget);
        return col;
    };

    form.name = new QLineEdit(initName);
    form.phone = new QLineEdit(initPhone);
    form.phone->setPlaceholderText("Ví dụ: 0905123456");
    UIHelper::setupPhoneInput(form.phone);
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(12);
    row1->addLayout(makeField("HỌ VÀ TÊN *", form.name), 3);
    row1->addLayout(makeField("SỐ ĐIỆN THOẠI *", form.phone), 2);
    cardLayout->addLayout(row1);

    form.age = new QSpinBox;
    form.age->setRange(18, 120);
    form.age->setValue(initAge);

    form.gender = new QComboBox;
    form.gender->addItems({"Nam", "Nữ"});
    form.gender->setCurrentIndex(initGender ? 0 : 1);

    form.username = new QLineEdit(initUsername);
    if (disableUsername) {
        form.username->setEnabled(false);
    }

    auto *row2 = new QHBoxLayout;
    row2->setSpacing(12);
    row2->addLayout(makeField("TUỔI", form.age), 1);
    row2->addLayout(makeField("GIỚI TÍNH", form.gender), 1);
    row2->addLayout(makeField("TÊN ĐĂNG NHẬP *", form.username), 2);
    cardLayout->addLayout(row2);

    form.password = new QLineEdit(initPassword);
    form.password->setEchoMode(QLineEdit::Normal);
    auto *toggleAction = form.password->addAction(UIHelper::createEyeIcon(true), QLineEdit::TrailingPosition);
    QObject::connect(toggleAction, &QAction::triggered, form.password, [pwd = form.password, toggleAction]() {
        bool isNormal = (pwd->echoMode() == QLineEdit::Normal);
        pwd->setEchoMode(isNormal ? QLineEdit::Password : QLineEdit::Normal);
        toggleAction->setIcon(UIHelper::createEyeIcon(!isNormal));
    });
    cardLayout->addLayout(makeField("MẬT KHẨU *", form.password));

    form.role = new QComboBox;
    form.role->addItems({"Lễ tân", "Lễ tân chuyên trách", "Quản trị viên"});
    int roleIdx = (initRole == ManagerRole::Administrator) ? 2 :
                  ((initRole == ManagerRole::SpecialReceptionist) ? 1 : 0);
    form.role->setCurrentIndex(roleIdx);
    if (disableRole) {
        form.role->setEnabled(false);
    }

    if (isEditMode) {
        form.statusCombo = new QComboBox;
        form.statusCombo->addItems({"Hoạt động", "Đã khóa"});
        form.statusCombo->setCurrentIndex(initActive ? 0 : 1);
        if (disableStatus) {
            form.statusCombo->setEnabled(false);
        }

        auto *row3 = new QHBoxLayout;
        row3->setSpacing(12);
        row3->addLayout(makeField("VAI TRÒ HỆ THỐNG", form.role), 1);
        row3->addLayout(makeField("TRẠNG THÁI TÀI KHOẢN", form.statusCombo), 1);
        cardLayout->addLayout(row3);
    } else {
        cardLayout->addLayout(makeField("VAI TRÒ HỆ THỐNG", form.role));
    }

    return card;
}

bool validateManagerInput(QWidget *parent, LinkList<Manager> *managers,
                          const QString &name, const QString &phone,
                          const QString &username, const QString &password,
                          const std::string &excludeId = "") {
    if (name.trimmed().isEmpty() || phone.trimmed().isEmpty() ||
        username.trimmed().isEmpty() || password.trimmed().isEmpty()) {
        UIHelper::showWarning(parent, "Thiếu thông tin", "Vui lòng nhập đầy đủ các thông tin bắt buộc.");
        return false;
    }
    if (phone.trimmed().length() != 10) {
        UIHelper::showWarning(parent, "Số điện thoại không hợp lệ", "Số điện thoại phải gồm đúng 10 chữ số.");
        return false;
    }

    std::string newUsername = UIHelper::ss(username.trimmed());
    if (managers) {
        auto *n = managers->getHeader()->next;
        while (n && n->next) {
            if (n->info.getValid() && (excludeId.empty() || n->info.getID() != excludeId) &&
                n->info.getUsername() == newUsername) {
                UIHelper::showWarning(parent, "Trùng tài khoản", "Tên đăng nhập này đã được sử dụng. Vui lòng chọn tên khác.");
                return false;
            }
            n = n->next;
        }
    }
    return true;
}

void buildPermissionsSection(QVBoxLayout *layout, const std::string &managerId, QList<PermCheckItem> &checks) {
    auto *permDivider = new QFrame;
    permDivider->setFrameShape(QFrame::HLine);
    permDivider->setStyleSheet("background-color: #f1f5f9; max-height: 1px; border: none; margin: 4px 0;");
    layout->addWidget(permDivider);

    auto *permTitle = new QLabel("Phân quyền nâng cao (Quản trị viên gốc)");
    permTitle->setStyleSheet("font-size: 13px; font-weight: 750; color: #0d9488;");
    layout->addWidget(permTitle);

    QStringList granted = ManagerController::getGrantedPermissions(managerId, "Data/PermissionManager.txt");

    for (int i = 0; i < permissionDefinitionCount; ++i) {
        const auto &entry = permissionDefinitions[i];
        auto *check = new QCheckBox(QString::fromUtf8(entry.name));
        check->setStyleSheet("QCheckBox { font-size: 12.5px; color: #334155; } QCheckBox:hover { color: #0d9488; }");
        check->setChecked(granted.contains(QString::fromUtf8(entry.id)));
        check->setProperty("permissionId", QString::fromUtf8(entry.id));
        layout->addWidget(check);

        QSpinBox *duration = nullptr;
        if (std::string(entry.id) == "Per_1") {
            auto *durRow = new QHBoxLayout;
            durRow->setContentsMargins(20, 0, 0, 0);
            auto *durLbl = new QLabel("Thời hạn hiệu lực (ngày):");
            durLbl->setStyleSheet("font-size: 12px; color: #64748b; font-weight: 600;");
            duration = new QSpinBox;
            duration->setRange(1, 3650);
            duration->setValue(30);
            durRow->addWidget(durLbl);
            durRow->addWidget(duration);
            durRow->addStretch();
            layout->addLayout(durRow);

            durLbl->setVisible(check->isChecked());
            duration->setVisible(check->isChecked());
            QObject::connect(check, &QCheckBox::toggled, [durLbl, duration](bool c) {
                durLbl->setVisible(c);
                duration->setVisible(c);
            });
        }
        checks.append({check, duration});
    }
}

void savePermissionsFromChecks(const std::string &managerId, const QList<PermCheckItem> &checks) {
    QList<PermissionRecord> newGrants;
    for (const auto &entry : checks) {
        if (entry.box && entry.box->isChecked()) {
            std::string expiry;
            if (entry.spin) {
                expiry = UIHelper::ss(QDateTime::currentDateTime().addDays(entry.spin->value()).toString(Qt::ISODate));
            }
            newGrants.append({managerId, UIHelper::ss(entry.box->property("permissionId").toString()), expiry});
        }
    }
    ManagerController::savePermissions(managerId, newGrants, "Data/PermissionManager.txt");
}

} // namespace

void AccountView::addManagerDialog() {
    if (!mainWindow) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Thêm tài khoản quản lý mới");
    dialog.setFixedWidth(540);
    dialog.setStyleSheet(UIHelper::dialogFormStyle());

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);

    root->addLayout(UIHelper::createDialogHeader("➕", "Thêm tài khoản quản lý mới", "Tạo tài khoản người dùng nhân viên mới cho hệ thống"));

    ManagerFormData form;
    root->addWidget(createManagerFormCard(form, false));

    // Buttons
    auto *btnBox = new QHBoxLayout;
    btnBox->addStretch();
    auto *cancelBtn = new QPushButton("Hủy bỏ");
    cancelBtn->setStyleSheet("QPushButton { background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 8px; padding: 9px 20px; font-size: 13px; font-weight: 600; color: #475569; } QPushButton:hover { background: #e2e8f0; color: #1e293b; }");
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    auto *saveBtn = new QPushButton("＋  Tạo tài khoản");
    saveBtn->setStyleSheet("QPushButton { background: #0d9488; border: none; border-radius: 8px; padding: 9px 22px; font-size: 13px; font-weight: 700; color: white; } QPushButton:hover { background: #0f766e; }");

    connect(saveBtn, &QPushButton::clicked, &dialog, [this, &dialog, &form] {
        auto *managers = mainWindow->getManagers();
        if (!validateManagerInput(&dialog, managers, form.name->text(), form.phone->text(), form.username->text(), form.password->text())) {
            return;
        }

        std::string id = ManagerController::generateNextId(managers);
        ManagerRole r = form.role->currentIndex() == 2 ? ManagerRole::Administrator :
                       (form.role->currentIndex() == 1 ? ManagerRole::SpecialReceptionist : ManagerRole::Receptionist);

        if (managers) {
            managers->insert(Manager(id, UIHelper::ss(form.name->text().trimmed()), UIHelper::ss(form.phone->text().trimmed()),
                                     form.gender->currentIndex() == 0, form.age->value(),
                                     UIHelper::ss(form.username->text().trimmed()), UIHelper::ss(form.password->text()), r, 1));
        }
        mainWindow->saveAll();
        dialog.accept();
        UIHelper::showSuccess(this, "Thành công", "Tài khoản quản lý mới đã được tạo thành công.");
    });

    btnBox->addWidget(cancelBtn);
    btnBox->addWidget(saveBtn);
    root->addLayout(btnBox);

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, [&dialog]() { dialog.setFocus(); });
    dialog.exec();
}

void AccountView::editManagerDialog(const std::string &managerId) {
    if (!mainWindow) return;

    auto *managers = mainWindow->getManagers();
    if (!managers) return;

    Manager *targetMgr = ManagerController::findById(managers, managerId);
    if (!targetMgr) return;

    bool isTargetRoot = (targetMgr->getID() == "MA_001");
    bool isCurrentRoot = mainWindow->isRootAdmin();
    bool isSelf = (mainWindow->getCurrentManager() && mainWindow->getCurrentManager()->getID() == targetMgr->getID());

    // Nếu sửa tài khoản MA_001 mà bản thân không phải root admin thì không có quyền
    if (isTargetRoot && !isCurrentRoot) {
        UIHelper::showWarning(this, "Không có quyền", "Chỉ quản trị viên gốc mới có quyền chỉnh sửa tài khoản này.");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Chỉnh sửa thông tin tài khoản");
    dialog.setFixedWidth(540);
    dialog.setStyleSheet(UIHelper::dialogFormStyle());

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);

    root->addLayout(UIHelper::createDialogHeader("👤", "Chỉnh sửa thông tin tài khoản", "Cập nhật thông tin quản lý, vai trò và trạng thái hoạt động"));

    ManagerFormData form;
    auto *card = createManagerFormCard(form, true,
                                       UIHelper::qs(targetMgr->getfullName()),
                                       UIHelper::qs(targetMgr->getPhone()),
                                       targetMgr->getAge(),
                                       targetMgr->getGender(),
                                       UIHelper::qs(targetMgr->getUsername()),
                                       UIHelper::qs(targetMgr->getPassword()),
                                       targetMgr->getRole(),
                                       targetMgr->isActive(),
                                       isTargetRoot,
                                       isTargetRoot,
                                       isTargetRoot || isSelf);

    auto *checks = new QList<PermCheckItem>;
    if (isCurrentRoot && !isTargetRoot) {
        auto *cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
        if (cardLayout) {
            buildPermissionsSection(cardLayout, managerId, *checks);
        }
    }
    root->addWidget(card);

    // Buttons
    auto *btnBox = new QHBoxLayout;
    btnBox->addStretch();
    auto *cancelBtn = new QPushButton("Hủy bỏ");
    cancelBtn->setStyleSheet("QPushButton { background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 8px; padding: 9px 20px; font-size: 13px; font-weight: 600; color: #475569; } QPushButton:hover { background: #e2e8f0; color: #1e293b; }");
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    auto *saveBtn = new QPushButton("✓  Lưu thay đổi");
    saveBtn->setStyleSheet("QPushButton { background: #0d9488; border: none; border-radius: 8px; padding: 9px 22px; font-size: 13px; font-weight: 700; color: white; } QPushButton:hover { background: #0f766e; }");

    connect(saveBtn, &QPushButton::clicked, &dialog, [this, &dialog, targetMgr, managerId, &form, isTargetRoot, isSelf, isCurrentRoot, checks] {
        auto *managers = mainWindow->getManagers();
        if (!validateManagerInput(&dialog, managers, form.name->text(), form.phone->text(), form.username->text(), form.password->text(), targetMgr->getID())) {
            return;
        }

        std::string newUsername = UIHelper::ss(form.username->text().trimmed());
        targetMgr->setfullName(UIHelper::ss(form.name->text().trimmed()));
        targetMgr->setPhone(UIHelper::ss(form.phone->text().trimmed()));
        targetMgr->setAge(form.age->value());
        targetMgr->setGender(form.gender->currentIndex() == 0);
        targetMgr->setUsername(newUsername);
        targetMgr->setPassword(UIHelper::ss(form.password->text().trimmed()));

        if (!isTargetRoot) {
            ManagerRole r = (form.role->currentIndex() == 2) ? ManagerRole::Administrator :
                           ((form.role->currentIndex() == 1) ? ManagerRole::SpecialReceptionist : ManagerRole::Receptionist);
            targetMgr->setRole(r);
        }

        if (!isTargetRoot && !isSelf && form.statusCombo) {
            targetMgr->setState(form.statusCombo->currentIndex() == 0 ? 1 : 0);
        }

        if (isCurrentRoot && !isTargetRoot) {
            savePermissionsFromChecks(managerId, *checks);
        }

        mainWindow->saveAll();
        refresh();
        dialog.accept();
        UIHelper::showSuccess(this, "Thành công", "Thông tin tài khoản đã được cập nhật thành công.");
    });

    btnBox->addWidget(cancelBtn);
    btnBox->addWidget(saveBtn);
    root->addLayout(btnBox);

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, [&dialog]() { dialog.setFocus(); });
    dialog.exec();
}
