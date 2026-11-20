#include "AccountView.h"
#include "libPBL2.h"
#include "MainWindow.h"
#include "ManagerController.h"
#include "Permission.h"
#include "PermissionManager.h"
#include "UIHelper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QScrollBar>
#include <QFrame>
#include <QHeaderView>
#include <QShowEvent>
#include <QDateTime>
#include <QAction>
#include <QTimer>
#include <memory>

namespace {
AccountSortBy accountSortByColumn(int col) {
    switch (col) {
        case 0: return AccountSortBy::Name;
        case 1: return AccountSortBy::Username;
        case 2: return AccountSortBy::Role;
        case 3: return AccountSortBy::State;
        default: return AccountSortBy::ID;
    }
}
}

AccountView::AccountView(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent), mainWindow(mainWindow) {
    sortColumn = -1;
    sortAscending = false;
    setupUI();
}

void AccountView::refresh(bool preserveScroll) {
    if (!mainWindow || !table) return;
    int vScroll = (preserveScroll && table->verticalScrollBar()) ? table->verticalScrollBar()->value() : 0;
    int curRow = preserveScroll ? table->currentRow() : -1;
    table->setUpdatesEnabled(false);
    table->setRowCount(0);

    auto *managers = mainWindow->getManagers();
    if (!managers) {
        table->setUpdatesEnabled(true);
        return;
    }

    Manager *curUser = mainWindow->getCurrentManager();
    const auto *pRecs = mainWindow->getPermissionRecords();
    bool isAdmin = mainWindow->isAdministrator();

    if (addBtn) {
        addBtn->setVisible(isAdmin);
    }
    if (subtitleLabel) {
        subtitleLabel->setText(isAdmin
            ? "Quản lý người dùng nhân viên và phân quyền bảo mật cho hệ thống."
            : "Xem và cập nhật thông tin tài khoản cá nhân của bạn.");
    }

    QList<Manager*> allList;
    auto *n = managers->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && ManagerController::canViewAccount(curUser, &n->info, pRecs)) {
            allList.append(&n->info);
        }
        n = n->next;
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

        auto *item0 = new QTableWidgetItem(UIHelper::qs(m->getfullName()));
        item0->setData(Qt::UserRole, UIHelper::qs(m->getID()));
        item0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QFont f = item0->font();
        f.setWeight(QFont::DemiBold);
        item0->setFont(f);
        item0->setForeground(QColor("#0f172a"));
        table->setItem(r, 0, item0);

        auto *item1 = new QTableWidgetItem(UIHelper::qs(m->getUsername()));
        item1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item1->setForeground(QColor("#334155"));
        table->setItem(r, 1, item1);

        auto *item2 = new QTableWidgetItem(ManagerController::displayRoleName(m, pRecs));
        item2->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item2->setForeground(QColor("#334155"));
        table->setItem(r, 2, item2);

        bool active = m->isActive();
        auto *item3 = new QTableWidgetItem(active ? "● Hoạt động" : "● Đã khóa");
        item3->setTextAlignment(Qt::AlignCenter);
        item3->setForeground(active ? QColor("#059669") : QColor("#dc2626"));
        QFont sf = item3->font();
        sf.setWeight(QFont::Bold);
        item3->setFont(sf);
        table->setItem(r, 3, item3);

        bool isSelf = curUser && curUser->getID() == m->getID();
        bool isTargetRoot = ManagerController::isRootAdmin(m);
        bool canDeleteAccount = isAdmin && !isSelf && !isTargetRoot;
        if (canDeleteAccount) {
            auto action = UIHelper::createActionCell(UIHelper::trashIcon(), "Xóa tài khoản", table);
            connect(action.second, &QPushButton::clicked, this, [this, mid = m->getID(), name = UIHelper::qs(m->getfullName())]() {
                if (!mainWindow) return;
                auto *managers = mainWindow->getManagers();
                if (!managers) return;
                Manager *target = ManagerController::findById(managers, mid);
                if (!target) return;
                if (!mainWindow->isAdministrator()) {
                    UIHelper::showWarning(this, "Không có quyền", "Chỉ quản trị viên mới có quyền xóa tài khoản.");
                    return;
                }
                Manager *cur = mainWindow->getCurrentManager();
                if (cur && cur->getID() == mid) {
                    UIHelper::showWarning(this, "Không thể xóa", "Bạn không thể xóa tài khoản hiện đang đăng nhập vào hệ thống.");
                    return;
                }
                if (ManagerController::isRootAdmin(target)) {
                    UIHelper::showWarning(this, "Không thể xóa", "Không thể xóa tài khoản Quản trị viên gốc của hệ thống.");
                    return;
                }
                const auto *recs = mainWindow->getPermissionRecords();
                if (!mainWindow->isRootAdmin() &&
                    ManagerController::getPrivilegeLevel(cur, recs) <= ManagerController::getPrivilegeLevel(target, recs)) {
                    UIHelper::showWarning(this, "Không có quyền", "Bạn chỉ có quyền xóa các tài khoản có quyền thấp hơn.");
                    return;
                }
                if (!UIHelper::confirmDelete(this, "Xác nhận xóa tài khoản",
                    QStringLiteral("Bạn có chắc chắn muốn xóa tài khoản \"%1\" khỏi hệ thống?\nTài khoản sau khi xóa sẽ bị xóa khỏi danh sách.").arg(name),
                    "Xóa tài khoản", "Hủy bỏ")) {
                    return;
                }
                try {
                    Manager backupMgr = *target;
                    LinkList<PermissionManager> backupPerms;
                    auto *n = managers->getHeader()->next;
                    while (n && n->next) {
                        if (n->info.getID() == mid) {
                            managers->remove(n);
                            break;
                        }
                        n = n->next;
                    }
                    auto *pRecords = mainWindow->getPermissionRecords();
                    if (pRecords && pRecords->getHeader()) {
                        auto *pn = pRecords->getHeader()->next;
                        while (pn && pn->next) {
                            if (pn->info.getMA_id() == mid) {
                                backupPerms.insert(pn->info);
                                auto *toDel = pn;
                                pn = pn->next;
                                pRecords->remove(toDel);
                                continue;
                            }
                            pn = pn->next;
                        }
                    }
                    if (!mainWindow->saveAll(SaveScope::Account, SaveAction::Normal)) {
                        managers->insert(backupMgr);
                        if (pRecords && backupPerms.getHeader()) {
                            auto *bn = backupPerms.getHeader()->next;
                            while (bn && bn->next) {
                                pRecords->insert(bn->info);
                                bn = bn->next;
                            }
                        }
                        return;
                    }
                    refresh(true);
                } catch (const std::exception &e) {
                    UIHelper::showWarning(this, "Lỗi hệ thống", QString("Không thể xóa tài khoản: %1").arg(QString::fromUtf8(e.what())));
                }
            });
            table->setCellWidget(r, 4, action.first);
        }
    }

    table->setUpdatesEnabled(true);
    if (preserveScroll) {
        if (table->verticalScrollBar()) table->verticalScrollBar()->setValue(vScroll);
        if (curRow >= 0 && curRow < table->rowCount()) table->setCurrentCell(curRow, 0);
        QTimer::singleShot(0, table, [this, vScroll, curRow]() {
            if (table && table->verticalScrollBar()) table->verticalScrollBar()->setValue(qBound(0, vScroll, table->verticalScrollBar()->maximum()));
            if (table && curRow >= 0 && curRow < table->rowCount()) table->setCurrentCell(curRow, 0);
        });
    }
}

void AccountView::resetToFirstPage() {
    sortColumn = -1;
    sortAscending = false;
    if (mainWindow && mainWindow->getManagers()) {
        ManagerController::sortManagers(mainWindow->getManagers(), AccountSortBy::ID, false, mainWindow->getPermissionRecords());
    }
    currentPage = 0;
    refresh(false);
}

void AccountView::reapplyCurrentSort() {
    if (!mainWindow || !mainWindow->getManagers()) return;
    if (sortColumn < 0 || sortColumn >= 4) {
        ManagerController::sortManagers(mainWindow->getManagers(), AccountSortBy::ID, false, mainWindow->getPermissionRecords());
        return;
    }
    ManagerController::sortManagers(mainWindow->getManagers(), accountSortByColumn(sortColumn), sortAscending, mainWindow->getPermissionRecords());
}

void AccountView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (event->spontaneous()) return;
    resetToFirstPage();
}

void AccountView::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 32, 36, 32);
    layout->setSpacing(16);

    auto *h = new QVBoxLayout;
    h->setSpacing(4);
    auto *t = new QLabel("Quản lý tài khoản");
    t->setObjectName("pageTitle");
    subtitleLabel = new QLabel("Quản lý người dùng nhân viên và phân quyền bảo mật cho hệ thống.");
    subtitleLabel->setObjectName("pageSubtitle");
    h->addWidget(t);
    h->addWidget(subtitleLabel);
    layout->addLayout(h);

    auto *actions = new QHBoxLayout;
    addBtn = UIHelper::button("＋  Thêm quản lý");
    actions->addWidget(addBtn);
    actions->addStretch();
    layout->addLayout(actions);

    table = UIHelper::table({"HỌ VÀ TÊN", "TÀI KHOẢN", "VAI TRÒ", "TRẠNG THÁI", ""}, this);
    table->setWordWrap(false);
    table->setTextElideMode(Qt::ElideRight);
    table->verticalHeader()->setDefaultSectionSize(48);

    if (table->horizontalHeaderItem(0)) table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(1)) table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(2)) table->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(3)) table->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(4)) table->horizontalHeaderItem(4)->setTextAlignment(Qt::AlignCenter);

    UIHelper::setupTableScaling(table, {0.30, 0.26, 0.26, 0.18}, 56);

    connect(table->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
        if (logicalIndex < 0 || logicalIndex >= 4) return;
        if (sortColumn == logicalIndex) {
            sortAscending = !sortAscending;
        } else {
            sortColumn = logicalIndex;
            sortAscending = true;
        }
        if (mainWindow && mainWindow->getManagers()) {
            ManagerController::sortManagers(mainWindow->getManagers(), accountSortByColumn(logicalIndex), sortAscending, mainWindow->getPermissionRecords());
            currentPage = 0;
            refresh(false);
        }
    });

    auto *tableCard = UIHelper::createTableCard(table);
    layout->addWidget(tableCard, 1);

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
            refresh(false);
        }
    });

    connect(nextBtn, &QPushButton::clicked, this, [this] {
        currentPage++;
        refresh(false);
    });

    connect(addBtn, &QPushButton::clicked, this, [this] {
        if (addManagerDialog()) {
            refresh(false);
        }
    });

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

bool AccountView::addManagerDialog() {
    if (!mainWindow) return false;
    if (!mainWindow->isAdministrator()) {
        UIHelper::showWarning(this, "Không có quyền", "Chỉ quản trị viên mới có quyền tạo tài khoản mới.");
        return false;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Thêm tài khoản quản lý mới");
    dialog.setFixedWidth(540);
    dialog.setStyleSheet(UIHelper::dialogFormStyle());

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);

    auto *headerBox = new QHBoxLayout;
    headerBox->setSpacing(12);

    auto *icon = new QLabel("➕");
    icon->setFixedSize(46, 46);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("background: #f0fdfa; border: 1.5px solid #99f6e4; border-radius: 12px; font-size: 20px;");

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    auto *title = new QLabel("Thêm tài khoản quản lý mới");
    title->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a;");
    auto *sub = new QLabel("Tạo tài khoản người dùng nhân viên mới cho hệ thống");
    sub->setStyleSheet("font-size: 12.5px; color: #64748b;");
    titleCol->addWidget(title);
    titleCol->addWidget(sub);

    headerBox->addWidget(icon);
    headerBox->addLayout(titleCol, 1);
    root->addLayout(headerBox);

    auto *card = new QFrame;
    card->setObjectName("addCard");
    card->setStyleSheet("QFrame#addCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
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

    auto *name = new QLineEdit;
    auto *phone = new QLineEdit;
    UIHelper::setupPhoneInput(phone);
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(12);
    row1->addLayout(makeField("HỌ VÀ TÊN *", name), 3);
    row1->addLayout(makeField("SỐ ĐIỆN THOẠI *", phone), 2);
    cardLayout->addLayout(row1);

    auto *age = new QSpinBox;
    age->setRange(18, 120);
    age->setValue(28);

    auto *gender = new QComboBox;
    gender->addItems({"Nam", "Nữ"});

    auto *username = new QLineEdit;

    auto *row2 = new QHBoxLayout;
    row2->setSpacing(12);
    row2->addLayout(makeField("TUỔI", age), 1);
    row2->addLayout(makeField("GIỚI TÍNH", gender), 1);
    row2->addLayout(makeField("TÊN ĐĂNG NHẬP *", username), 2);
    cardLayout->addLayout(row2);

    auto *password = new QLineEdit;
    password->setEchoMode(QLineEdit::Password);
    auto *toggleAddAction = password->addAction(UIHelper::createEyeIcon(false), QLineEdit::TrailingPosition);
    connect(toggleAddAction, &QAction::triggered, password, [password, toggleAddAction]() {
        bool isNormal = (password->echoMode() == QLineEdit::Normal);
        password->setEchoMode(isNormal ? QLineEdit::Password : QLineEdit::Normal);
        toggleAddAction->setIcon(UIHelper::createEyeIcon(!isNormal));
    });
    cardLayout->addLayout(makeField("MẬT KHẨU *", password));

    auto *role = new QComboBox;
    role->addItem("Lễ tân", static_cast<int>(ManagerRole::Receptionist));
    role->addItem("Lễ tân chuyên trách", static_cast<int>(ManagerRole::SpecialReceptionist));
    if (mainWindow->isRootAdmin()) {
        role->addItem("Quản trị viên", static_cast<int>(ManagerRole::Administrator));
    }
    cardLayout->addLayout(makeField("VAI TRÒ HỆ THỐNG", role));

    root->addWidget(card);

    auto *btnBox = new QHBoxLayout;
    btnBox->addStretch();
    auto *cancelBtn = new QPushButton("Hủy bỏ");
    cancelBtn->setAutoDefault(false);
    cancelBtn->setDefault(false);
    cancelBtn->setStyleSheet("QPushButton { background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 8px; padding: 9px 20px; font-size: 13px; font-weight: 600; color: #475569; } QPushButton:hover { background: #e2e8f0; color: #1e293b; }");
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    auto *saveBtn = new QPushButton("＋  Tạo tài khoản");
    saveBtn->setAutoDefault(false);
    saveBtn->setDefault(false);
    saveBtn->setStyleSheet("QPushButton { background: #0d9488; border: none; border-radius: 8px; padding: 9px 22px; font-size: 13px; font-weight: 700; color: white; } QPushButton:hover { background: #0f766e; }");

    connect(saveBtn, &QPushButton::clicked, &dialog, [this, &dialog, name, phone, username, password, age, gender, role] {
        QString nameStr = name->text().trimmed();
        QString userStr = username->text().trimmed();
        QString phoneStr = phone->text().trimmed();
        QString passStr = password->text().trimmed();
        if (nameStr.isEmpty() || phoneStr.isEmpty() ||
            userStr.isEmpty() || passStr.isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng nhập đầy đủ các thông tin bắt buộc.");
            return;
        }
        if (libPBL2::containsForbiddenStorageChars(UIHelper::ss(nameStr)) ||
            libPBL2::containsForbiddenStorageChars(UIHelper::ss(userStr)) ||
            libPBL2::containsForbiddenStorageChars(UIHelper::ss(passStr))) {
            UIHelper::showWarning(&dialog, "Ký tự không hợp lệ", "Họ tên, tên đăng nhập và mật khẩu không được chứa ký tự '|' hoặc ký tự xuống dòng.");
            return;
        }
        if (userStr.contains(' ')) {
            UIHelper::showWarning(&dialog, "Tên đăng nhập không hợp lệ", "Tên đăng nhập không được chứa khoảng trắng.");
            username->setFocus();
            return;
        }
        if (!UIHelper::isValidPhone(phoneStr)) {
            UIHelper::showWarning(&dialog, "Số điện thoại không hợp lệ", "Số điện thoại phải gồm đúng 10 chữ số và bắt đầu bằng số 0 (Ví dụ: 0905123456).");
            phone->setFocus();
            return;
        }
        if (ManagerController::isHiddenAccount(UIHelper::ss(userStr))) {
            UIHelper::showWarning(&dialog, "Tên đăng nhập không hợp lệ", "Tên đăng nhập này được dành riêng cho hệ thống.");
            username->setFocus();
            return;
        }
        auto *managers = mainWindow->getManagers();
        if (managers) {
            auto *n = managers->getHeader()->next;
            while (n && n->next) {
                if (n->info.getUsername() == UIHelper::ss(userStr)) {
                    UIHelper::showWarning(&dialog, "Trùng tài khoản", "Tên đăng nhập này đã được sử dụng. Vui lòng chọn tên khác.");
                    username->setFocus();
                    return;
                }
                if (n->info.getPhone() == UIHelper::ss(phoneStr)) {
                    UIHelper::showWarning(&dialog, "Trùng số điện thoại", "Số điện thoại này đã được sử dụng bởi một tài khoản nhân viên khác.");
                    phone->setFocus();
                    return;
                }
                n = n->next;
            }
        }

        std::string id = ManagerController::generateNextId(managers);
        ManagerRole r = static_cast<ManagerRole>(role->currentData().toInt());
        if (r == ManagerRole::Administrator && !mainWindow->isRootAdmin()) {
            UIHelper::showWarning(&dialog, "Không có quyền", "Chỉ Quản trị viên gốc mới có quyền tạo tài khoản Quản trị viên.");
            return;
        }

        try {
            if (managers) {
                managers->insert(Manager(id, UIHelper::ss(name->text().trimmed()), UIHelper::ss(phoneStr), gender->currentIndex() == 0, age->value(), UIHelper::ss(userStr), UIHelper::ss(passStr), r, 1));
            }
            if (!mainWindow->saveAll(SaveScope::Account, SaveAction::Normal)) {
                if (managers && managers->getHeader()) {
                    auto *rn = managers->getHeader()->next;
                    while (rn && rn->next) {
                        if (rn->info.getID() == id) { managers->remove(rn); break; }
                        rn = rn->next;
                    }
                }
                return;
            }
            dialog.accept();
            UIHelper::showSuccess(this, "Thành công", "Tài khoản quản lý mới đã được tạo thành công.");
        } catch (const AppException &e) {
            UIHelper::showWarning(&dialog, "Lỗi tạo tài khoản", QString::fromUtf8(e.what()));
        } catch (const std::exception &e) {
            UIHelper::showWarning(&dialog, "Lỗi hệ thống", QString("Đã xảy ra ngoại lệ: %1").arg(e.what()));
        }
    });

    btnBox->addWidget(cancelBtn);
    btnBox->addWidget(saveBtn);
    root->addLayout(btnBox);

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, &dialog, [&dialog]() { dialog.setFocus(); });
    return dialog.exec() == QDialog::Accepted;
}

void AccountView::editManagerDialog(const std::string &managerId) {
    if (!mainWindow) return;

    auto *managers = mainWindow->getManagers();
    if (!managers) return;

    Manager *targetMgr = ManagerController::findById(managers, managerId);
    if (!targetMgr) return;

    Manager *curMgr = mainWindow->getCurrentManager();
    const auto *pRecs = mainWindow->getPermissionRecords();
    if (!ManagerController::canViewAccount(curMgr, targetMgr, pRecs)) {
        UIHelper::showWarning(this, "Không có quyền", "Bạn không có quyền xem thông tin tài khoản này.");
        return;
    }

    bool isTargetRoot = ManagerController::isRootAdmin(targetMgr);
    bool isCurrentRoot = mainWindow->isRootAdmin();
    bool isSelf = (curMgr && curMgr->getID() == targetMgr->getID());

    QDialog dialog(this);
    dialog.setWindowTitle("Chỉnh sửa thông tin tài khoản");
    dialog.setFixedWidth(540);
    dialog.setStyleSheet(UIHelper::dialogFormStyle());

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);

    auto *headerBox = new QHBoxLayout;
    headerBox->setSpacing(12);

    auto *icon = new QLabel("👤");
    icon->setFixedSize(46, 46);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("background: #f0fdfa; border: 1.5px solid #99f6e4; border-radius: 12px; font-size: 22px;");

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    auto *title = new QLabel("Chỉnh sửa thông tin tài khoản");
    title->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a;");
    auto *sub = new QLabel((isSelf && !isCurrentRoot)
        ? "Cập nhật thông tin cá nhân và mật khẩu tài khoản của bạn"
        : "Cập nhật thông tin quản lý, vai trò và trạng thái hoạt động");
    sub->setStyleSheet("font-size: 12.5px; color: #64748b;");
    sub->setWordWrap(true);
    titleCol->addWidget(title);
    titleCol->addWidget(sub);

    headerBox->addWidget(icon);
    headerBox->addLayout(titleCol, 1);
    root->addLayout(headerBox);

    auto *card = new QFrame(&dialog);
    card->setObjectName("editCard");
    card->setStyleSheet("QFrame#editCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
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

    auto *name = new QLineEdit(UIHelper::qs(targetMgr->getfullName()));
    auto *phone = new QLineEdit(UIHelper::qs(targetMgr->getPhone()));
    UIHelper::setupPhoneInput(phone);
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(12);
    row1->addLayout(makeField("HỌ VÀ TÊN *", name), 3);
    row1->addLayout(makeField("SỐ ĐIỆN THOẠI *", phone), 2);
    cardLayout->addLayout(row1);

    auto *age = new QSpinBox;
    age->setRange(18, 120);
    age->setValue(targetMgr->getAge());

    auto *gender = new QComboBox;
    gender->addItems({"Nam", "Nữ"});
    gender->setCurrentIndex(targetMgr->getGender() ? 0 : 1);

    auto *username = new QLineEdit(UIHelper::qs(targetMgr->getUsername()));
    if (isTargetRoot) {
        username->setEnabled(false);
    }

    auto *row2 = new QHBoxLayout;
    row2->setSpacing(12);
    row2->addLayout(makeField("TUỔI", age), 1);
    row2->addLayout(makeField("GIỚI TÍNH", gender), 1);
    row2->addLayout(makeField("TÊN ĐĂNG NHẬP *", username), 2);
    cardLayout->addLayout(row2);

    auto *password = new QLineEdit(UIHelper::qs(targetMgr->getPassword()));
    password->setEchoMode(QLineEdit::Password);
    auto *toggleEditAction = password->addAction(UIHelper::createEyeIcon(false), QLineEdit::TrailingPosition);
    connect(toggleEditAction, &QAction::triggered, password, [password, toggleEditAction]() {
        bool isNormal = (password->echoMode() == QLineEdit::Normal);
        password->setEchoMode(isNormal ? QLineEdit::Password : QLineEdit::Normal);
        toggleEditAction->setIcon(UIHelper::createEyeIcon(!isNormal));
    });
    cardLayout->addLayout(makeField("MẬT KHẨU *", password));

    auto *role = new QComboBox;
    role->addItems({"Lễ tân", "Lễ tân chuyên trách", "Quản trị viên"});
    int roleIdx = (targetMgr->getRole() == ManagerRole::Administrator) ? 2 :
                  ((targetMgr->getRole() == ManagerRole::SpecialReceptionist) ? 1 : 0);
    role->setCurrentIndex(roleIdx);
    if (isTargetRoot || isSelf || !isCurrentRoot) {
        role->setEnabled(false);
    }

    auto *statusCombo = new QComboBox;
    statusCombo->addItems({"Hoạt động", "Đã khóa"});
    statusCombo->setCurrentIndex(targetMgr->isActive() ? 0 : 1);
    if (isTargetRoot || isSelf) {
        statusCombo->setEnabled(false);
    }

    auto *row3 = new QHBoxLayout;
    row3->setSpacing(12);
    row3->addLayout(makeField("VAI TRÒ HỆ THỐNG", role), 1);
    row3->addLayout(makeField("TRẠNG THÁI TÀI KHOẢN", statusCombo), 1);
    cardLayout->addLayout(row3);

    struct PermCheckItem {
        QCheckBox *box = nullptr;
        QSpinBox *spin = nullptr;
        int baseDays = 30;
        QDateTime baseExpiry;
        bool initiallyGranted = false;
        bool toggledOff = false;
    };
    auto checks = std::make_shared<QList<PermCheckItem>>();

    auto computePer1Expiry = [](const PermCheckItem &item) -> QDateTime {
        if (!item.spin) return QDateTime();
        if (item.initiallyGranted && !item.toggledOff && item.baseExpiry.isValid()) {
            return item.baseExpiry.addDays(item.spin->value() - item.baseDays);
        }
        return QDateTime::currentDateTime().addDays(item.spin->value());
    };

    if (isCurrentRoot && !isTargetRoot && !isSelf) {
        auto *permSection = new QWidget(card);
        auto *permLayout = new QVBoxLayout(permSection);
        permLayout->setContentsMargins(0, 0, 0, 0);
        permLayout->setSpacing(10);

        auto *permDivider = new QFrame(permSection);
        permDivider->setFrameShape(QFrame::HLine);
        permDivider->setStyleSheet("background-color: #f1f5f9; max-height: 1px; border: none; margin: 4px 0;");
        permLayout->addWidget(permDivider);

        auto *permTitle = new QLabel("Phân quyền nâng cao (Quản trị viên gốc)", permSection);
        permTitle->setStyleSheet("font-size: 13px; font-weight: 700; color: #0d9488;");
        permLayout->addWidget(permTitle);

        QStringList granted = ManagerController::getGrantedPermissions(managerId, mainWindow ? mainWindow->getPermissionRecords() : nullptr);

        std::unique_ptr<LinkList<Permission>> catalog;
        try {
            catalog.reset(Permission::getSystemCatalog());
        } catch (const std::exception &e) {
            UIHelper::showWarning(this, "Lỗi phân quyền", QString("Không thể nạp danh mục quyền: %1").arg(QString::fromUtf8(e.what())));
        }
        if (catalog && catalog->getHeader()) {
            auto *node = catalog->getHeader()->next;
            while (node && node->next) {
                const auto &entry = node->info;
                std::string pId = entry.getPer_id();
                QString pName = QString::fromUtf8(entry.getPer_Name().c_str());

                auto *check = new QCheckBox(pName, permSection);
                check->setStyleSheet("QCheckBox { font-size: 12.5px; color: #334155; } QCheckBox:hover { color: #0d9488; }");
                check->setChecked(granted.contains(QString::fromUtf8(pId.c_str())));
                check->setProperty("permissionId", QString::fromUtf8(pId.c_str()));

                auto *rowLayout = new QHBoxLayout;
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                rowLayout->addWidget(check);

                QLabel *badgeLabel = nullptr;
                QSpinBox *duration = nullptr;
                int baseDays = 30;
                QDateTime baseExpiry;
                bool initGranted = false;

                if (pId == "Per_1") {
                    badgeLabel = new QLabel(permSection);
                    badgeLabel->setStyleSheet("font-size: 11.5px; font-weight: 700; color: #0f766e; background-color: #ccfbf1; border: 1px solid #99f6e4; border-radius: 6px; padding: 2px 8px;");
                    badgeLabel->setVisible(false);
                    rowLayout->addWidget(badgeLabel);

                    QDateTime now = QDateTime::currentDateTime();
                    QDateTime currentExpiry = ManagerController::permissionExpiry(managerId, pId, mainWindow ? mainWindow->getPermissionRecords() : nullptr);
                    if (granted.contains(QString::fromUtf8(pId.c_str())) && currentExpiry.isValid() && currentExpiry > now) {
                        qint64 secsRemaining = now.secsTo(currentExpiry);
                        int remainingDays = static_cast<int>((secsRemaining + 86399) / 86400);
                        if (remainingDays < 1) remainingDays = 1;
                        baseDays = remainingDays;
                        baseExpiry = currentExpiry;
                        initGranted = true;
                        badgeLabel->setText(currentExpiry.toString("dd-MM-yyyy | HH:mm"));
                        badgeLabel->setVisible(check->isChecked());
                    } else {
                        baseDays = 30;
                        baseExpiry = now.addDays(30);
                        badgeLabel->setText(baseExpiry.toString("dd-MM-yyyy | HH:mm"));
                        badgeLabel->setVisible(check->isChecked());
                    }
                }
                rowLayout->addStretch();
                permLayout->addLayout(rowLayout);

                int itemIndex = checks->size();
                if (pId == "Per_1") {
                    auto *durRow = new QHBoxLayout;
                    durRow->setContentsMargins(20, 0, 0, 0);
                    auto *durLbl = new QLabel("Thời hạn hiệu lực (ngày):", permSection);
                    durLbl->setStyleSheet("font-size: 12px; color: #64748b; font-weight: 600;");
                    duration = new QSpinBox(permSection);
                    duration->setRange(1, qMax(3650, baseDays));
                    duration->setValue(baseDays);
                    durRow->addWidget(durLbl);
                    durRow->addWidget(duration);
                    durRow->addStretch();
                    permLayout->addLayout(durRow);

                    durLbl->setVisible(check->isChecked());
                    duration->setVisible(check->isChecked());

                    auto updateBadge = [checks, itemIndex, badgeLabel, computePer1Expiry]() {
                        if (!badgeLabel || itemIndex >= checks->size()) return;
                        const auto &it = (*checks)[itemIndex];
                        if (it.box && it.box->isChecked()) {
                            QDateTime newExp = computePer1Expiry(it);
                            badgeLabel->setText(newExp.toString("dd-MM-yyyy | HH:mm"));
                            badgeLabel->setVisible(true);
                        } else {
                            badgeLabel->setVisible(false);
                        }
                    };

                    connect(duration, qOverload<int>(&QSpinBox::valueChanged), &dialog, updateBadge);
                    connect(check, &QCheckBox::toggled, &dialog, [checks, itemIndex, durLbl, duration, updateBadge](bool c) {
                        if (!c && itemIndex < checks->size()) {
                            (*checks)[itemIndex].toggledOff = true;
                        }
                        durLbl->setVisible(c);
                        duration->setVisible(c);
                        updateBadge();
                    });
                }
                checks->append({check, duration, baseDays, baseExpiry, initGranted, false});
                node = node->next;
            }
        }

        cardLayout->addWidget(permSection);
        auto syncPermSectionVisibility = [permSection, &dialog](int idx) {
            permSection->setVisible(idx != 2);
            dialog.adjustSize();
        };
        permSection->setVisible(role->currentIndex() != 2);
        connect(role, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, syncPermSectionVisibility);
    }

    root->addWidget(card);

    auto *btnBox = new QHBoxLayout;
    btnBox->addStretch();
    auto *cancelBtn = new QPushButton("Hủy bỏ");
    cancelBtn->setAutoDefault(false);
    cancelBtn->setDefault(false);
    cancelBtn->setStyleSheet("QPushButton { background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 8px; padding: 9px 20px; font-size: 13px; font-weight: 600; color: #475569; } QPushButton:hover { background: #e2e8f0; color: #1e293b; }");
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    auto *saveBtn = new QPushButton("✓  Lưu thay đổi");
    saveBtn->setAutoDefault(false);
    saveBtn->setDefault(false);
    saveBtn->setStyleSheet("QPushButton { background: #0d9488; border: none; border-radius: 8px; padding: 9px 22px; font-size: 13px; font-weight: 700; color: white; } QPushButton:hover { background: #0f766e; }");

    connect(saveBtn, &QPushButton::clicked, &dialog, [this, &dialog, managerId, name, phone, age, gender, username, password, role, statusCombo, isTargetRoot, isSelf, isCurrentRoot, checks, computePer1Expiry] {
        auto *managers = mainWindow->getManagers();
        Manager *targetMgr = ManagerController::findById(managers, managerId);
        if (!targetMgr) {
            UIHelper::showWarning(&dialog, "Không tìm thấy", "Tài khoản không còn tồn tại trong hệ thống.");
            dialog.reject();
            return;
        }

        QString nameStr = name->text().trimmed();
        QString userStr = username->text().trimmed();
        QString phoneStr = phone->text().trimmed();
        QString passStr = password->text().trimmed();
        if (nameStr.isEmpty() || phoneStr.isEmpty() ||
            userStr.isEmpty() || passStr.isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng nhập đầy đủ các thông tin bắt buộc.");
            return;
        }
        if (libPBL2::containsForbiddenStorageChars(UIHelper::ss(nameStr)) ||
            libPBL2::containsForbiddenStorageChars(UIHelper::ss(userStr)) ||
            libPBL2::containsForbiddenStorageChars(UIHelper::ss(passStr))) {
            UIHelper::showWarning(&dialog, "Ký tự không hợp lệ", "Họ tên, tên đăng nhập và mật khẩu không được chứa ký tự '|' hoặc ký tự xuống dòng.");
            return;
        }
        if (userStr.contains(' ')) {
            UIHelper::showWarning(&dialog, "Tên đăng nhập không hợp lệ", "Tên đăng nhập không được chứa khoảng trắng.");
            username->setFocus();
            return;
        }
        if (!UIHelper::isValidPhone(phoneStr)) {
            UIHelper::showWarning(&dialog, "Số điện thoại không hợp lệ", "Số điện thoại phải gồm đúng 10 chữ số và bắt đầu bằng số 0 (Ví dụ: 0905123456).");
            phone->setFocus();
            return;
        }
        if (ManagerController::isHiddenAccount(UIHelper::ss(userStr))) {
            UIHelper::showWarning(&dialog, "Tên đăng nhập không hợp lệ", "Tên đăng nhập này được dành riêng cho hệ thống.");
            username->setFocus();
            return;
        }

        std::string newUsername = UIHelper::ss(userStr);
        std::string newPhone = UIHelper::ss(phoneStr);
        if (managers) {
            auto *n = managers->getHeader()->next;
            while (n && n->next) {
                if (n->info.getID() != managerId) {
                    if (n->info.getUsername() == newUsername) {
                        UIHelper::showWarning(&dialog, "Trùng tài khoản", "Tên đăng nhập này đã được sử dụng. Vui lòng chọn tên khác.");
                        username->setFocus();
                        return;
                    }
                    if (n->info.getPhone() == newPhone) {
                        UIHelper::showWarning(&dialog, "Trùng số điện thoại", "Số điện thoại này đã được sử dụng bởi một tài khoản nhân viên khác.");
                        phone->setFocus();
                        return;
                    }
                }
                n = n->next;
            }
        }

        try {
            Manager oldSnapshot = *targetMgr;
            LinkList<PermissionManager> oldPermsBackup;
            if (isCurrentRoot && !isTargetRoot && !isSelf && mainWindow && mainWindow->getPermissionRecords() && mainWindow->getPermissionRecords()->getHeader()) {
                auto *pn = mainWindow->getPermissionRecords()->getHeader()->next;
                while (pn && pn->next) {
                    if (pn->info.getMA_id() == managerId) {
                        oldPermsBackup.insert(pn->info);
                    }
                    pn = pn->next;
                }
            }

            targetMgr->setfullName(UIHelper::ss(nameStr));
            targetMgr->setPhone(newPhone);
            targetMgr->setAge(age->value());
            targetMgr->setGender(gender->currentIndex() == 0);
            if (!isTargetRoot) {
                targetMgr->setUsername(newUsername);
            }
            targetMgr->setPassword(UIHelper::ss(passStr));

            ManagerRole newRole = targetMgr->getRole();
            if (isCurrentRoot && !isTargetRoot && !isSelf) {
                newRole = (role->currentIndex() == 2) ? ManagerRole::Administrator :
                         ((role->currentIndex() == 1) ? ManagerRole::SpecialReceptionist : ManagerRole::Receptionist);
                targetMgr->setRole(newRole);
            }

            if (!isTargetRoot && !isSelf) {
                targetMgr->setState(statusCombo->currentIndex() == 0 ? 1 : 0);
            }

            if (isCurrentRoot && !isTargetRoot && !isSelf) {
                LinkList<PermissionManager> newGrants;
                if (newRole != ManagerRole::Administrator) {
                    for (const auto &entry : *checks) {
                        if (entry.box && entry.box->isChecked()) {
                            std::string expiry;
                            if (entry.spin) {
                                QDateTime finalExpiry = computePer1Expiry(entry);
                                expiry = UIHelper::ss(finalExpiry.toString(Qt::ISODate));
                            }
                            newGrants.insert(PermissionManager(managerId, UIHelper::ss(entry.box->property("permissionId").toString()), expiry));
                        }
                    }
                }
                try {
                    if (mainWindow && mainWindow->getPermissionRecords()) {
                        ManagerController::savePermissions(managerId, newGrants, mainWindow->getPermissionRecords());
                    }
                } catch (const std::exception &e) {
                    *targetMgr = oldSnapshot;
                    try {
                        if (mainWindow && mainWindow->getPermissionRecords()) {
                            ManagerController::savePermissions(managerId, oldPermsBackup, mainWindow->getPermissionRecords());
                        }
                    } catch (...) {}
                    UIHelper::showWarning(&dialog, "Lỗi phân quyền", QString("Không thể cập nhật phân quyền: %1").arg(QString::fromUtf8(e.what())));
                    return;
                }
            }

            if (!mainWindow->saveAll(SaveScope::Account, SaveAction::Normal)) {
                if (Manager *rollbackMgr = ManagerController::findById(managers, managerId)) {
                    *rollbackMgr = oldSnapshot;
                }
                if (isCurrentRoot && !isTargetRoot && !isSelf && mainWindow && mainWindow->getPermissionRecords()) {
                    try {
                        ManagerController::savePermissions(managerId, oldPermsBackup, mainWindow->getPermissionRecords());
                    } catch (...) {}
                }
                return;
            }
            if (isSelf && mainWindow && mainWindow->getCurrentManager()) {
                if (Manager *freshSelf = ManagerController::findById(managers, managerId)) {
                    *mainWindow->getCurrentManager() = *freshSelf;
                }
                mainWindow->updateUserProfileDisplay();
                mainWindow->refreshPage(0);
            }
            refresh(true);
            dialog.accept();
            UIHelper::showSuccess(this, "Thành công", "Thông tin tài khoản đã được cập nhật thành công.");
        } catch (const std::exception &e) {
            UIHelper::showWarning(&dialog, "Lỗi hệ thống", QString("Không thể lưu thay đổi tài khoản: %1").arg(QString::fromUtf8(e.what())));
        }
    });

    btnBox->addWidget(cancelBtn);
    btnBox->addWidget(saveBtn);
    root->addLayout(btnBox);

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, &dialog, [&dialog]() { dialog.setFocus(); });
    dialog.exec();
}
