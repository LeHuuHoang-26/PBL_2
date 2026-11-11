#include "PatientView.h"
#include "libPBL2.h"
#include "MainWindow.h"
#include "PatientController.h"
#include "UIHelper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialog>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QHeaderView>
#include <QLabel>
#include <QFrame>
#include <QTimer>

PatientView::PatientView(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent), mainWindow(mainWindow) {
    if (mainWindow && mainWindow->getPatients()) {
        libPBL2::sortPatients(mainWindow->getPatients(), libPBL2::PatientSortBy::ID, false);
    }
    setupUI();
    refresh(false);
}

void PatientView::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 32, 36, 32);
    layout->setSpacing(20);

    // Header Title
    auto *h = new QVBoxLayout;
    h->setSpacing(4);
    auto *t = new QLabel("Bệnh nhân");
    t->setObjectName("pageTitle");
    auto *s = new QLabel("Quản lý danh sách hồ sơ người bệnh.");
    s->setObjectName("pageSubtitle");
    h->addWidget(t);
    h->addWidget(s);
    layout->addLayout(h);

    // Actions bar
    auto *actions = new QHBoxLayout;
    search = new QLineEdit;
    search->setPlaceholderText("Tìm theo tên, điện thoại hoặc CCCD…");

    addBtn = UIHelper::button("＋  Thêm bệnh nhân");

    actions->addWidget(search, 1);
    actions->addWidget(addBtn);
    layout->addLayout(actions);

    if (mainWindow && !mainWindow->hasPermission("Per_3")) {
        addBtn->hide();
    }

    // Table: không hiển thị mã bệnh nhân, không hiển thị triệu chứng, có cột xóa không tiêu đề
    table = UIHelper::table({"HỌ VÀ TÊN", "ĐIỆN THOẠI", "CCCD", "TUỔI", "GIỚI TÍNH", ""});
    table->setWordWrap(false);
    table->setTextElideMode(Qt::ElideRight);
    table->verticalHeader()->setDefaultSectionSize(48);

    // Căn lề tên trường (Header) trùng khớp với dữ liệu bên dưới giống trang tổng quát
    if (table->horizontalHeaderItem(0)) table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(1)) table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(2)) table->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(3)) table->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(4)) table->horizontalHeaderItem(4)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(5)) table->horizontalHeaderItem(5)->setTextAlignment(Qt::AlignCenter);

    // Tự động scale các cột sao cho tổng chiều rộng các cột luôn bằng chính xác chiều rộng bảng
    UIHelper::setupTableScaling(table, {0.28, 0.21, 0.25, 0.11, 0.15}, 56);

    connect(table->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
        if (logicalIndex < 0 || logicalIndex >= 5) return;
        if (sortColumn == logicalIndex) {
            sortAscending = !sortAscending;
        } else {
            sortColumn = logicalIndex;
            sortAscending = true;
        }

        libPBL2::PatientSortBy criteria = libPBL2::PatientSortBy::Name;
        switch (logicalIndex) {
            case 0: criteria = libPBL2::PatientSortBy::Name; break;
            case 1: criteria = libPBL2::PatientSortBy::Phone; break;
            case 2: criteria = libPBL2::PatientSortBy::CCCD; break;
            case 3: criteria = libPBL2::PatientSortBy::Age; break;
            case 4: criteria = libPBL2::PatientSortBy::Gender; break;
            default: return;
        }

        if (mainWindow && mainWindow->getPatients()) {
            libPBL2::sortPatients(mainWindow->getPatients(), criteria, sortAscending);
            currentPage = 0;
            refresh(false);
        }
    });

    // Khung thẻ bo tròn hoàn thiện 4 góc, chống tràn viền
    auto *tableCard = UIHelper::createTableCard(table);
    layout->addWidget(tableCard, 1);

    // Thanh phân trang (50 người / trang)
    auto *pagination = new QHBoxLayout;
    pagination->setContentsMargins(4, 4, 4, 0);

    totalLabel = new QLabel("Tổng số: 0 bệnh nhân");
    totalLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #475569;");
    pagination->addWidget(totalLabel);

    pagination->addStretch();

    prevBtn = UIHelper::button("‹  Trang trước", "secondary");
    nextBtn = UIHelper::button("Trang sau  ›", "secondary");
    pageStatus = new QLabel("Trang 1 / 1");
    pageStatus->setStyleSheet("font-size: 12.5px; font-weight: 700; color: #0f172a; padding: 0 12px;");

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

    connect(search, &QLineEdit::textChanged, this, [this] {
        currentPage = 0;
        refresh(false);
    });

    connect(addBtn, &QPushButton::clicked, this, [this] {
        addPatientDialog();
        refresh(false);
    });

    // Tích hợp sửa hồ sơ khi click 2 lần chuột trái (double-click) vào hàng bệnh nhân (bỏ qua cột xóa)
    connect(table, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        if (col == 5) return;
        if (row < 0 || row >= table->rowCount()) return;
        auto *item = table->item(row, 0);
        if (!item) return;
        std::string id = UIHelper::ss(item->data(Qt::UserRole).toString());
        if (id.empty()) return;
        editPatientDialog(id);
        refresh(true);
    });
}

void PatientView::reapplyCurrentSort() {
    if (!mainWindow || !mainWindow->getPatients()) return;
    if (sortColumn < 0) {
        libPBL2::sortPatients(mainWindow->getPatients(), libPBL2::PatientSortBy::ID, false);
        return;
    }
    libPBL2::PatientSortBy criteria = libPBL2::PatientSortBy::Name;
    switch (sortColumn) {
        case 0: criteria = libPBL2::PatientSortBy::Name; break;
        case 1: criteria = libPBL2::PatientSortBy::Phone; break;
        case 2: criteria = libPBL2::PatientSortBy::CCCD; break;
        case 3: criteria = libPBL2::PatientSortBy::Age; break;
        case 4: criteria = libPBL2::PatientSortBy::Gender; break;
        default: return;
    }
    libPBL2::sortPatients(mainWindow->getPatients(), criteria, sortAscending);
}

void PatientView::resetToFirstPage() {
    currentPage = 0;
    sortColumn = -1;
    sortAscending = false;
    if (mainWindow && mainWindow->getPatients()) {
        libPBL2::sortPatients(mainWindow->getPatients(), libPBL2::PatientSortBy::ID, false);
    }
    refresh();
}

void PatientView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    currentPage = 0;
    sortColumn = -1;
    sortAscending = false;
    if (mainWindow && mainWindow->getPatients()) {
        libPBL2::sortPatients(mainWindow->getPatients(), libPBL2::PatientSortBy::ID, false);
    }
    refresh();
}

void PatientView::refresh(bool preserveScroll) {
    if (!mainWindow || !table) return;
    int vScroll = (preserveScroll && table->verticalScrollBar()) ? table->verticalScrollBar()->value() : 0;
    int curRow = preserveScroll ? table->currentRow() : -1;
    table->setUpdatesEnabled(false);
    table->setRowCount(0);

    auto *patients = mainWindow->getPatients();
    if (!patients) {
        table->setUpdatesEnabled(true);
        return;
    }

    QString q = search ? search->text().trimmed().toLower() : "";
    QList<Patient*> matched;

    auto *n = patients->getHeader()->next;
    while (n && n->next) {
        QString name = UIHelper::qs(n->info.getfullName());
        QString id = UIHelper::qs(n->info.getID());
        QString phone = UIHelper::qs(n->info.getPhone());
        QString cccd = UIHelper::qs(n->info.getCCCD());

        if (q.isEmpty() || name.toLower().contains(q) || id.toLower().contains(q) || phone.contains(q) || cccd.contains(q)) {
            matched.append(&n->info);
        }
        n = n->next;
    }

    int total = matched.size();
    int totalPages = total == 0 ? 1 : (total + PAGE_SIZE - 1) / PAGE_SIZE;
    currentPage = qBound(0, currentPage, totalPages - 1);

    if (totalLabel) {
        totalLabel->setText(QStringLiteral("Tổng số: %1 bệnh nhân").arg(total));
    }
    if (pageStatus) {
        pageStatus->setText(QStringLiteral("Trang %1 / %2").arg(currentPage + 1).arg(totalPages));
    }
    if (prevBtn) {
        prevBtn->setEnabled(currentPage > 0);
    }
    if (nextBtn) {
        nextBtn->setEnabled(currentPage < totalPages - 1);
    }

    int start = currentPage * PAGE_SIZE;
    int end = qMin(start + PAGE_SIZE, total);

    for (int i = start; i < end; ++i) {
        Patient *p = matched[i];
        int r = table->rowCount();
        table->insertRow(r);
        table->setRowHeight(r, 48);

        QString name = UIHelper::qs(p->getfullName());
        QString phone = UIHelper::qs(p->getPhone());
        QString cccd = UIHelper::qs(p->getCCCD());

        auto *item0 = new QTableWidgetItem(name);
        item0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item0->setData(Qt::UserRole, UIHelper::qs(p->getID()));
        QFont f = item0->font();
        f.setWeight(QFont::DemiBold);
        item0->setFont(f);
        table->setItem(r, 0, item0);

        auto *item1 = new QTableWidgetItem(phone);
        item1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(r, 1, item1);

        auto *item2 = new QTableWidgetItem(cccd);
        item2->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(r, 2, item2);

        auto *item3 = new QTableWidgetItem(QString::number(p->getAge()));
        item3->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 3, item3);

        auto *item4 = new QTableWidgetItem(p->getGender() ? "Nam" : "Nữ");
        item4->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 4, item4);

        // Cột Xóa: không có tên trường, chỉ có icon thùng rác đỏ
        auto action = UIHelper::createActionCell(UIHelper::trashIcon(), "Xóa hồ sơ bệnh nhân");
        if (mainWindow && !mainWindow->hasPermission("Per_3")) {
            action.second->setToolTip("Bạn không có quyền xóa hồ sơ bệnh nhân (Yêu cầu quyền Per_3)");
        }
        connect(action.second, &QPushButton::clicked, this, [this, p]() {
            if (!mainWindow) return;
            if (!mainWindow->hasPermission("Per_3")) {
                UIHelper::showWarning(this, "Không có quyền", "Bạn không có quyền thực hiện xóa hồ sơ bệnh nhân.");
                return;
            }
            QString name = UIHelper::qs(p->getfullName());
            if (!UIHelper::confirmDelete(this, "Xác nhận xóa hồ sơ bệnh nhân",
                QStringLiteral("Bạn có chắc chắn muốn xóa hồ sơ bệnh nhân \"%1\" khỏi hệ thống?\nHồ sơ sau khi xóa sẽ được loại bỏ khỏi danh sách quản lý.").arg(name),
                "Xóa hồ sơ", "Không xóa")) {
                return;
            }

            std::string pid = p->getID();
            auto *appointments = mainWindow->getAppointments();
            bool hasUpcoming = false;
            if (appointments) {
                auto *an = appointments->getHeader()->next;
                while (an && an->next) {
                    if (mainWindow->getPatientIdForAppointment(an->info) == pid && an->info.getState() == 0) {
                        hasUpcoming = true;
                        break;
                    }
                    an = an->next;
                }
            }
            if (hasUpcoming) {
                if (!UIHelper::confirmWarning(this, "Cảnh báo lịch khám chưa hoàn tất",
                    QStringLiteral("Bệnh nhân \"%1\" hiện vẫn còn lịch hẹn chưa khám trong hệ thống.\nBạn vẫn muốn tiếp tục xóa hồ sơ này?").arg(name),
                    "Vẫn tiếp tục xóa", "Quay lại")) {
                    return;
                }
            }

            auto *patients = mainWindow->getPatients();
            if (patients) {
                auto *n = patients->getHeader()->next;
                while (n && n->next) {
                    if (n->info.getID() == pid) {
                        patients->remove(n);
                        break;
                    }
                    n = n->next;
                }
            }

            // Dọn dẹp SymptomPatient và Appointment liên kết với bệnh nhân bị xóa
            auto *symptomPatients = mainWindow->getSymptomPatients();
            if (symptomPatients && symptomPatients->getHeader()) {
                auto *spNode = symptomPatients->getHeader()->next;
                while (spNode && spNode->next) {
                    if (spNode->info.getP_id() == pid) {
                        std::string spId = spNode->info.getSP_id();
                        if (appointments && appointments->getHeader()) {
                            auto *aNode = appointments->getHeader()->next;
                            while (aNode && aNode->next) {
                                if (aNode->info.getSymptomPatientId() == spId) {
                                    auto *toDelA = aNode;
                                    aNode = aNode->next;
                                    appointments->remove(toDelA);
                                    continue;
                                }
                                aNode = aNode->next;
                            }
                        }
                        auto *toDelSp = spNode;
                        spNode = spNode->next;
                        symptomPatients->remove(toDelSp);
                        continue;
                    }
                    spNode = spNode->next;
                }
            }
            mainWindow->saveAll(SaveScope::All, SaveAction::Normal);
            refresh(true);
            mainWindow->refreshPage(0);
        });

        table->setCellWidget(r, 5, action.first);
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

void PatientView::addPatientDialog() {
    if (!mainWindow) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Thêm bệnh nhân mới");
    dialog.setFixedWidth(540);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #f8fafc;
            font-family: 'Segoe UI', sans-serif;
        }
        QLineEdit, QSpinBox, QComboBox {
            background: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            color: #0f172a;
            min-height: 20px;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
            border: 1.5px solid #0d9488;
            background: #ffffff;
        }
        QSpinBox {
            padding-right: 30px;
        }
        QSpinBox::up-button {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 25px;
            border-left: 1.5px solid #94a3b8;
            border-bottom: 1px solid #94a3b8;
            border-top-right-radius: 7px;
            background: #e2e8f0;
        }
        QSpinBox::up-button:hover {
            background: #0d9488;
        }
        QSpinBox::up-button:hover QSpinBox::up-arrow {
            image: url(:/assets/textures/arrow_up_white.png);
        }
        QSpinBox::up-button:pressed {
            background: #0f766e;
        }
        QSpinBox::up-arrow {
            image: url(:/assets/textures/arrow_up_dark.png);
            width: 9px;
            height: 9px;
        }
        QSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            width: 25px;
            border-left: 1.5px solid #94a3b8;
            border-bottom-right-radius: 7px;
            background: #e2e8f0;
        }
        QSpinBox::down-button:hover {
            background: #0d9488;
        }
        QSpinBox::down-button:hover QSpinBox::down-arrow {
            image: url(:/assets/textures/arrow_down_white.png);
        }
        QSpinBox::down-button:pressed {
            background: #0f766e;
        }
        QSpinBox::down-arrow {
            image: url(:/assets/textures/arrow_down_dark.png);
            width: 9px;
            height: 9px;
        }
        QComboBox {
            padding-right: 28px;
        }
        QComboBox:disabled {
            background: #f1f5f9;
            color: #94a3b8;
            border-color: #e2e8f0;
        }
        QComboBox::drop-down {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 28px;
            border: none;
            background: transparent;
        }
        QComboBox::drop-down:disabled {
            width: 0px;
            border: none;
        }
        QComboBox::down-arrow {
            image: url(:/assets/textures/arrow_down_dark.png);
            width: 11px;
            height: 11px;
            margin-right: 8px;
        }
        QComboBox::down-arrow:disabled {
            image: none;
        }
        QComboBox QAbstractItemView {
            background: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            selection-background-color: #0d9488;
            selection-color: #ffffff;
            outline: none;
            padding: 4px;
        }
        QComboBox QAbstractItemView::item {
            min-height: 28px;
            padding: 4px 10px;
            border-radius: 6px;
            color: #0f172a;
            background-color: #ffffff;
        }
        QComboBox QAbstractItemView::item:hover {
            background-color: #f0fdfa;
            color: #0f766e;
        }
        QComboBox QAbstractItemView::item:selected {
            background-color: #0d9488;
            color: #ffffff;
        }
    )");

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);

    // Header Banner
    auto *headerBox = new QHBoxLayout;
    headerBox->setSpacing(12);

    auto *icon = new QLabel("➕");
    icon->setFixedSize(46, 46);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("background: #f0fdfa; border: 1.5px solid #99f6e4; border-radius: 12px; font-size: 20px;");

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    auto *title = new QLabel("Thêm bệnh nhân mới");
    title->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a;");
    auto *sub = new QLabel("Nhập thông tin hồ sơ và định danh của người bệnh");
    sub->setStyleSheet("font-size: 12.5px; color: #64748b;");
    titleCol->addWidget(title);
    titleCol->addWidget(sub);

    headerBox->addWidget(icon);
    headerBox->addLayout(titleCol, 1);
    root->addLayout(headerBox);

    // Form Card Container
    auto *card = new QFrame;
    card->setObjectName("patientCard");
    card->setStyleSheet("QFrame#patientCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 20);
    cardLayout->setSpacing(14);

    auto makeField = [](const QString &labelText, QWidget *inputWidget) {
        auto *col = new QVBoxLayout;
        col->setSpacing(5);
        auto *lbl = new QLabel(labelText);
        lbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
        col->addWidget(lbl);
        col->addWidget(inputWidget);
        return col;
    };

    auto *name = new QLineEdit;
    name->setPlaceholderText("Ví dụ: Nguyễn Văn An");
    cardLayout->addLayout(makeField("Họ và tên bệnh nhân", name));

    auto *row1 = new QHBoxLayout;
    row1->setSpacing(14);
    auto *phone = new QLineEdit;
    phone->setPlaceholderText("Ví dụ: 0905123456");
    UIHelper::setupPhoneInput(phone);

    auto *cccd = new QLineEdit;
    cccd->setPlaceholderText("Ví dụ: 048096001234");
    UIHelper::setupCccdInput(cccd);

    row1->addLayout(makeField("Số điện thoại liên hệ", phone), 1);
    row1->addLayout(makeField("Số CCCD / Định danh", cccd), 1);
    cardLayout->addLayout(row1);

    auto *row2 = new QHBoxLayout;
    row2->setSpacing(14);
    auto *age = new QSpinBox;
    age->setRange(1, 130);
    age->setValue(30);
    auto *gender = new QComboBox;
    gender->addItems({"Nam", "Nữ"});
    row2->addLayout(makeField("Tuổi", age), 1);
    row2->addLayout(makeField("Giới tính", gender), 1);
    cardLayout->addLayout(row2);

    root->addWidget(card);

    // Footer Buttons
    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();
    auto *cancel = new QPushButton("Hủy");
    cancel->setAutoDefault(false);
    cancel->setDefault(false);
    cancel->setCursor(Qt::PointingHandCursor);
    cancel->setStyleSheet("QPushButton { background: #ffffff; color: #475569; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 9px 22px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #f1f5f9; color: #0f172a; }");

    auto *save = new QPushButton("Tạo hồ sơ");
    save->setAutoDefault(false);
    save->setDefault(false);
    save->setCursor(Qt::PointingHandCursor);
    save->setStyleSheet("QPushButton { background: #0d9488; color: #ffffff; border: none; border-radius: 8px; padding: 9px 24px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #0f766e; }");

    buttons->addWidget(cancel);
    buttons->addWidget(save);
    root->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(save, &QPushButton::clicked, &dialog, [this, &dialog, name, phone, cccd, age, gender] {
        QString nameStr = name->text().trimmed();
        QString phoneStr = phone->text().trimmed();
        QString cccdStr = cccd->text().trimmed();

        if (nameStr.isEmpty() || phoneStr.isEmpty() || cccdStr.isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng nhập đủ họ tên, điện thoại và CCCD.");
            return;
        }

        if (!UIHelper::isValidPhone(phoneStr)) {
            UIHelper::showWarning(&dialog, "Số điện thoại không hợp lệ", "Số điện thoại phải gồm đúng 10 chữ số và bắt đầu bằng số 0 (Ví dụ: 0905123456).");
            phone->setFocus();
            return;
        }

        if (!UIHelper::isValidCccd(cccdStr)) {
            UIHelper::showWarning(&dialog, "Số CCCD không hợp lệ", "Số CCCD phải gồm đúng 12 chữ số và bắt đầu bằng số 0 (Ví dụ: 048096001234).");
            cccd->setFocus();
            return;
        }

        auto *patients = mainWindow->getPatients();
        if (patients) {
            auto *n = patients->getHeader()->next;
            while (n && n->next) {
                if (n->info.getPhone() == UIHelper::ss(phoneStr) || n->info.getCCCD() == UIHelper::ss(cccdStr)) {
                    UIHelper::showWarning(&dialog, "Trùng hồ sơ", "Số điện thoại hoặc CCCD đã tồn tại.");
                    return;
                }
                n = n->next;
            }
        }

        std::string id = Patient::generateNextId(patients);
        if (patients) {
            patients->insert(Patient(id, UIHelper::ss(nameStr), UIHelper::ss(phoneStr), gender->currentIndex() == 0, age->value(), UIHelper::ss(cccdStr)));
        }

        mainWindow->saveAll(SaveScope::Patient, SaveAction::Normal);
        refresh();
        dialog.accept();
    });
    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, [&dialog]() { dialog.setFocus(); });
    dialog.exec();
}

void PatientView::editPatientDialog(const std::string &id) {
    if (!mainWindow) return;
    Patient *patient = mainWindow->findPatient(id);
    if (!patient) return;

    bool canEdit = mainWindow->hasPermission("Per_3");

    QDialog dialog(this);
    dialog.setWindowTitle("Chi tiết thông tin bệnh nhân & Lịch sử khám");

    // Mở rộng cố định bằng đúng 9/10 (90%) so với vùng nội dung bên trong (không tính thanh menu bên trái)
    int availW = this->width() > 400 ? this->width() : (mainWindow ? (mainWindow->width() - 238) : 1200);
    int availH = this->height() > 300 ? this->height() : (mainWindow ? mainWindow->height() : 800);
    int targetW = static_cast<int>(availW * 0.90);
    int targetH = static_cast<int>(availH * 0.90);
    dialog.setFixedSize(targetW, targetH);

    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #f8fafc;
            font-family: 'Segoe UI', sans-serif;
        }
        QLineEdit, QSpinBox, QComboBox {
            background: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 7px 12px;
            font-size: 13px;
            color: #0f172a;
            min-height: 20px;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
            border: 1.5px solid #0d9488;
            background: #ffffff;
        }
        QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled {
            background: #f1f5f9;
            color: #64748b;
            border-color: #e2e8f0;
        }
        QSpinBox {
            padding-right: 30px;
        }
        QSpinBox::up-button {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 25px;
            border-left: 1.5px solid #94a3b8;
            border-bottom: 1px solid #94a3b8;
            border-top-right-radius: 7px;
            background: #e2e8f0;
        }
        QSpinBox::up-button:hover { background: #0d9488; }
        QSpinBox::up-button:hover QSpinBox::up-arrow { image: url(:/assets/textures/arrow_up_white.png); }
        QSpinBox::up-button:pressed { background: #0f766e; }
        QSpinBox::up-arrow { image: url(:/assets/textures/arrow_up_dark.png); width: 9px; height: 9px; }
        QSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            width: 25px;
            border-left: 1.5px solid #94a3b8;
            border-bottom-right-radius: 7px;
            background: #e2e8f0;
        }
        QSpinBox::down-button:hover { background: #0d9488; }
        QSpinBox::down-button:hover QSpinBox::down-arrow { image: url(:/assets/textures/arrow_down_white.png); }
        QSpinBox::down-button:pressed { background: #0f766e; }
        QSpinBox::down-arrow { image: url(:/assets/textures/arrow_down_dark.png); width: 9px; height: 9px; }
        QComboBox { padding-right: 28px; }
        QComboBox::drop-down { subcontrol-origin: border; subcontrol-position: top right; width: 28px; border: none; background: transparent; }
        QComboBox::drop-down:disabled { width: 0px; }
        QComboBox::down-arrow { image: url(:/assets/textures/arrow_down_dark.png); width: 11px; height: 11px; margin-right: 8px; }
        QComboBox::down-arrow:disabled { image: none; }
        QComboBox QAbstractItemView {
            background: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            selection-background-color: #0d9488;
            selection-color: #ffffff;
            outline: none;
            padding: 4px;
        }
        QComboBox QAbstractItemView::item { min-height: 28px; padding: 4px 10px; border-radius: 6px; color: #0f172a; background-color: #ffffff; }
        QComboBox QAbstractItemView::item:hover { background-color: #f0fdfa; color: #0f766e; }
        QComboBox QAbstractItemView::item:selected { background-color: #0d9488; color: #ffffff; }
        QScrollBar:vertical, QScrollBar:horizontal {
            width: 6px;
            height: 6px;
            background: #f1f5f9;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: #cbd5e1;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
            background: #94a3b8;
        }
    )");

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

    // 1. Thanh Header Banner trên cùng
    auto *headerBox = new QHBoxLayout;
    headerBox->setSpacing(12);

    auto *iconBadge = new QLabel("👤");
    iconBadge->setFixedSize(40, 40);
    iconBadge->setAlignment(Qt::AlignCenter);
    iconBadge->setStyleSheet("background: #f0fdfa; border: 1.5px solid #99f6e4; border-radius: 10px; font-size: 20px;");

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(0);
    auto *dlgTitle = new QLabel("Hồ sơ bệnh nhân");
    dlgTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a;");
    titleCol->addWidget(dlgTitle);

    auto *roleBadge = new QLabel(canEdit ? "● Có quyền sửa hồ sơ" : "● Chỉ xem");
    roleBadge->setStyleSheet(canEdit
        ? "background: #eff6ff; color: #2563eb; border: 1px solid #bfdbfe; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;"
        : "background: #f1f5f9; color: #475569; border: 1px solid #cbd5e1; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;");

    headerBox->addWidget(iconBadge);
    headerBox->addLayout(titleCol, 1);
    headerBox->addWidget(roleBadge, 0, Qt::AlignVCenter);
    root->addLayout(headerBox);

    auto *divider = new QFrame;
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("background-color: #e2e8f0; max-height: 1px; border: none;");
    root->addWidget(divider);

    // 2. Thân chính chia 2 cột (QHBoxLayout)
    auto *bodyLayout = new QHBoxLayout;
    bodyLayout->setSpacing(16);

    // --- CỘT TRÁI: THÔNG TIN BỆNH NHÂN (~320px) ---
    auto *leftCard = new QFrame;
    leftCard->setObjectName("leftCard");
    leftCard->setFixedWidth(320);
    leftCard->setStyleSheet("QFrame#leftCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
    auto *leftCardLayout = new QVBoxLayout(leftCard);
    leftCardLayout->setContentsMargins(18, 16, 18, 16);
    leftCardLayout->setSpacing(12);

    auto *leftHeader = new QLabel("👤  THÔNG TIN BỆNH NHÂN");
    leftHeader->setStyleSheet("font-size: 11.5px; font-weight: 800; color: #0f766e; letter-spacing: 0.6px; text-transform: uppercase;");
    leftCardLayout->addWidget(leftHeader);

    auto makeField = [](const QString &labelText, QWidget *inputWidget) {
        auto *col = new QVBoxLayout;
        col->setSpacing(5);
        auto *lbl = new QLabel(labelText);
        lbl->setStyleSheet("font-size: 11.5px; font-weight: 700; color: #475569;");
        col->addWidget(lbl);
        col->addWidget(inputWidget);
        return col;
    };

    auto *idField = new QLineEdit(UIHelper::qs(patient->getID()));
    idField->setEnabled(false);
    idField->setFixedHeight(38);
    idField->setStyleSheet("background: #f8fafc; border: 1.5px solid #e2e8f0; border-radius: 8px; padding: 7px 12px; font-size: 13px; font-weight: 750; color: #0d9488;");
    leftCardLayout->addLayout(makeField("Mã định danh bệnh nhân", idField));

    auto *name = new QLineEdit(UIHelper::qs(patient->getfullName()));
    name->setEnabled(canEdit);
    name->setFixedHeight(38);
    leftCardLayout->addLayout(makeField("Họ và tên bệnh nhân", name));

    auto *phone = new QLineEdit(UIHelper::qs(patient->getPhone()));
    UIHelper::setupPhoneInput(phone);
    phone->setEnabled(canEdit);
    phone->setFixedHeight(38);
    leftCardLayout->addLayout(makeField("Số điện thoại liên hệ", phone));

    auto *cccd = new QLineEdit(UIHelper::qs(patient->getCCCD()));
    UIHelper::setupCccdInput(cccd);
    cccd->setEnabled(canEdit);
    cccd->setFixedHeight(38);
    leftCardLayout->addLayout(makeField("Số CCCD / Định danh", cccd));

    auto *rowAgeGender = new QHBoxLayout;
    rowAgeGender->setSpacing(10);

    auto *age = new QSpinBox;
    age->setRange(1, 130);
    age->setValue(patient->getAge());
    age->setEnabled(canEdit);
    age->setFixedHeight(38);

    auto *gender = new QComboBox;
    gender->addItems({"Nam", "Nữ"});
    gender->setCurrentIndex(patient->getGender() ? 0 : 1);
    gender->setEnabled(canEdit);
    gender->setFixedHeight(38);

    rowAgeGender->addLayout(makeField("Tuổi", age), 1);
    rowAgeGender->addLayout(makeField("Giới tính", gender), 1);
    leftCardLayout->addLayout(rowAgeGender);

    leftCardLayout->addSpacing(4);

    // Hàng nút bấm ở đáy cột trái
    auto *leftBtnBox = new QHBoxLayout;
    leftBtnBox->setSpacing(8);

    if (canEdit) {
        auto *cancelBtn = new QPushButton("Hủy");
        cancelBtn->setAutoDefault(false);
        cancelBtn->setDefault(false);
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setFixedHeight(36);
        cancelBtn->setStyleSheet("QPushButton { background: #ffffff; color: #475569; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 7px 16px; font-weight: 700; font-size: 12.5px; } QPushButton:hover { background: #f1f5f9; color: #0f172a; }");
        connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

        auto *saveBtn = new QPushButton("✓  Lưu thay đổi");
        saveBtn->setAutoDefault(false);
        saveBtn->setDefault(false);
        saveBtn->setCursor(Qt::PointingHandCursor);
        saveBtn->setFixedHeight(36);
        saveBtn->setStyleSheet("QPushButton { background: #0d9488; color: #ffffff; border: none; border-radius: 8px; padding: 7px 18px; font-weight: 700; font-size: 12.5px; } QPushButton:hover { background: #0f766e; }");

        connect(saveBtn, &QPushButton::clicked, &dialog, [this, &dialog, patient, name, phone, cccd, age, gender]() {
            QString nameStr = name->text().trimmed();
            QString phoneStr = phone->text().trimmed();
            QString cccdStr = cccd->text().trimmed();

            if (nameStr.isEmpty() || phoneStr.isEmpty() || cccdStr.isEmpty()) {
                UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng nhập đủ họ tên, điện thoại và CCCD.");
                return;
            }

            if (!UIHelper::isValidPhone(phoneStr)) {
                UIHelper::showWarning(&dialog, "Số điện thoại không hợp lệ", "Số điện thoại phải gồm đúng 10 chữ số và bắt đầu bằng số 0 (Ví dụ: 0905123456).");
                phone->setFocus();
                return;
            }

            if (!UIHelper::isValidCccd(cccdStr)) {
                UIHelper::showWarning(&dialog, "Số CCCD không hợp lệ", "Số CCCD phải gồm đúng 12 chữ số và bắt đầu bằng số 0 (Ví dụ: 048096001234).");
                cccd->setFocus();
                return;
            }

            auto *patients = mainWindow->getPatients();
            if (patients) {
                auto *n = patients->getHeader()->next;
                while (n && n->next) {
                    if (&n->info != patient && (n->info.getPhone() == UIHelper::ss(phoneStr) || n->info.getCCCD() == UIHelper::ss(cccdStr))) {
                        UIHelper::showWarning(&dialog, "Trùng hồ sơ", "Số điện thoại hoặc CCCD đã tồn tại trên một bệnh nhân khác.");
                        return;
                    }
                    n = n->next;
                }
            }

            patient->setfullName(UIHelper::ss(nameStr));
            patient->setPhone(UIHelper::ss(phoneStr));
            patient->setCCCD(UIHelper::ss(cccdStr));
            patient->setAge(age->value());
            patient->setGender(gender->currentIndex() == 0);

            mainWindow->saveAll(SaveScope::Patient, SaveAction::Normal);
            UIHelper::showSuccess(&dialog, "Thành công", "Cập nhật thông tin bệnh nhân thành công.");
            dialog.accept();
        });

        leftBtnBox->addWidget(cancelBtn);
        leftBtnBox->addWidget(saveBtn);
    } else {
        auto *closeBtn = new QPushButton("Đóng");
        closeBtn->setAutoDefault(false);
        closeBtn->setDefault(false);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setFixedHeight(36);
        closeBtn->setStyleSheet("QPushButton { background: #0d9488; color: #ffffff; border: none; border-radius: 8px; padding: 7px 24px; font-weight: 700; font-size: 12.5px; } QPushButton:hover { background: #0f766e; }");
        connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        leftBtnBox->addStretch();
        leftBtnBox->addWidget(closeBtn);
    }
    leftCardLayout->addLayout(leftBtnBox);
    leftCardLayout->addStretch(1);

    bodyLayout->addWidget(leftCard);

    // --- CỘT PHẢI: LỊCH SỬ KHÁM CỦA BỆNH NHÂN CÓ SEARCH & SORT ---
    auto *rightCard = new QFrame;
    rightCard->setObjectName("rightCard");
    rightCard->setStyleSheet("QFrame#rightCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
    auto *rightCardLayout = new QVBoxLayout(rightCard);
    rightCardLayout->setContentsMargins(18, 16, 18, 16);
    rightCardLayout->setSpacing(10);

    // Thanh công cụ: Tìm kiếm + Lọc trạng thái + Nút Đặt lịch khám mới (căn ngang bằng trên cùng)
    auto *filterBar = new QHBoxLayout;
    filterBar->setSpacing(10);

    auto *searchHist = new QLineEdit;
    searchHist->setFixedHeight(38);
    searchHist->setPlaceholderText("🔍  Tìm theo bác sĩ, chuyên khoa, phòng khám, triệu chứng, ngày khám...");
    searchHist->setStyleSheet("background: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 7px 12px; font-size: 12.5px; color: #0f172a;");
    filterBar->addWidget(searchHist, 1);

    auto *quickBookBtn = new QPushButton("＋  Đặt lịch khám mới");
    quickBookBtn->setAutoDefault(false);
    quickBookBtn->setDefault(false);
    quickBookBtn->setCursor(Qt::PointingHandCursor);
    quickBookBtn->setFixedHeight(38);
    quickBookBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #0d9488;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 0 16px;
            font-size: 12.5px;
            font-weight: 750;
        }
        QPushButton:hover {
            background-color: #0f766e;
        }
    )");
    filterBar->addWidget(quickBookBtn);
    rightCardLayout->addLayout(filterBar);

    // Bảng danh sách lịch sử khám (5 cột, không có cột thao tác)
    auto *histTable = new QTableWidget;
    histTable->setColumnCount(5);
    histTable->setHorizontalHeaderLabels({"NGÀY GIỜ KHÁM", "BÁC SĨ PHỤ TRÁCH", "PHÒNG", "TRIỆU CHỨNG", "TRẠNG THÁI"});
    histTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    histTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    histTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    histTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    histTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    histTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    histTable->verticalHeader()->setVisible(false);
    histTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    histTable->setSelectionMode(QAbstractItemView::SingleSelection);
    histTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    histTable->setShowGrid(false);
    histTable->setAlternatingRowColors(true);
    histTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #ffffff;
            alternate-background-color: #f8fafc;
            border: 1px solid #e2e8f0;
            border-radius: 8px;
            outline: none;
        }
        QTableWidget::item {
            padding: 4px 8px;
            color: #0f172a;
            border-bottom: 1px solid #f1f5f9;
        }
        QTableWidget::item:selected {
            background-color: #f0fdfa;
            color: #0f766e;
        }
        QHeaderView::section {
            background-color: #f8fafc;
            color: #475569;
            font-size: 11px;
            font-weight: 750;
            border: none;
            border-bottom: 1.5px solid #cbd5e1;
            padding: 8px;
        }
    )");

    rightCardLayout->addWidget(histTable, 1);

    // Empty state khi không có lịch sử khám
    auto *emptyWidget = new QWidget;
    emptyWidget->setStyleSheet("background: transparent;");
    auto *emptyLay = new QVBoxLayout(emptyWidget);
    emptyLay->setAlignment(Qt::AlignCenter);
    emptyLay->setSpacing(8);

    auto *emptyIcon = new QLabel("📋");
    emptyIcon->setStyleSheet("font-size: 36px;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    auto *emptyTitle = new QLabel("Chưa có lịch sử khám bệnh");
    emptyTitle->setStyleSheet("font-size: 14px; font-weight: 750; color: #0f172a;");
    emptyTitle->setAlignment(Qt::AlignCenter);
    auto *emptySub = new QLabel("Bệnh nhân này chưa có ca khám nào trong hệ thống hoặc không khớp bộ lọc.");
    emptySub->setStyleSheet("font-size: 12px; color: #64748b;");
    emptySub->setAlignment(Qt::AlignCenter);

    emptyLay->addWidget(emptyIcon);
    emptyLay->addWidget(emptyTitle);
    emptyLay->addWidget(emptySub);
    rightCardLayout->addWidget(emptyWidget, 1);
    emptyWidget->hide();

    // Thanh trạng thái & phân trang (Stack 20 ca/trang) dưới chân cột phải
    auto *histFooter = new QHBoxLayout;
    histFooter->setContentsMargins(0, 4, 0, 0);
    histFooter->setSpacing(12);

    auto *leftFooterLay = new QVBoxLayout;
    leftFooterLay->setSpacing(2);
    auto *countLabel = new QLabel("Đang hiển thị 0 ca khám");
    countLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
    auto *hintLabel = new QLabel("💡 Nhấp đúp chuột vào hàng để xem hoặc dời lịch khám");
    hintLabel->setStyleSheet("font-size: 11.5px; color: #0d9488; font-weight: 600;");
    leftFooterLay->addWidget(countLabel);
    leftFooterLay->addWidget(hintLabel);
    histFooter->addLayout(leftFooterLay, 1);

    auto *paginationBox = new QHBoxLayout;
    paginationBox->setSpacing(8);

    auto *prevPageBtn = new QPushButton("‹  Trang trước");
    prevPageBtn->setAutoDefault(false);
    prevPageBtn->setDefault(false);
    prevPageBtn->setCursor(Qt::PointingHandCursor);
    prevPageBtn->setFixedHeight(32);
    prevPageBtn->setStyleSheet(R"(
        QPushButton {
            background: #ffffff;
            color: #0f766e;
            border: 1.5px solid #cbd5e1;
            border-radius: 6px;
            padding: 0 12px;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton:hover {
            background: #f0fdfa;
            border-color: #0d9488;
        }
        QPushButton:disabled {
            background: #f8fafc;
            color: #94a3b8;
            border-color: #e2e8f0;
        }
    )");

    auto *pageInfoLabel = new QLabel("Trang 1 / 1");
    pageInfoLabel->setStyleSheet("font-size: 12px; font-weight: 750; color: #0f172a; padding: 0 6px;");
    pageInfoLabel->setAlignment(Qt::AlignCenter);

    auto *nextPageBtn = new QPushButton("Trang sau  ›");
    nextPageBtn->setAutoDefault(false);
    nextPageBtn->setDefault(false);
    nextPageBtn->setCursor(Qt::PointingHandCursor);
    nextPageBtn->setFixedHeight(32);
    nextPageBtn->setStyleSheet(R"(
        QPushButton {
            background: #ffffff;
            color: #0f766e;
            border: 1.5px solid #cbd5e1;
            border-radius: 6px;
            padding: 0 12px;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton:hover {
            background: #f0fdfa;
            border-color: #0d9488;
        }
        QPushButton:disabled {
            background: #f8fafc;
            color: #94a3b8;
            border-color: #e2e8f0;
        }
    )");

    paginationBox->addWidget(prevPageBtn);
    paginationBox->addWidget(pageInfoLabel);
    paginationBox->addWidget(nextPageBtn);
    histFooter->addLayout(paginationBox, 0);

    rightCardLayout->addLayout(histFooter);

    bodyLayout->addWidget(rightCard, 1);
    root->addLayout(bodyLayout);

    // --- LOGIC QUẢN LÝ DỮ LIỆU LỊCH SỬ KHÁM (SEARCH, SORT, PHÂN TRANG 20 CA/TRANG) ---
    const int PAGE_SIZE = 20; // Giới hạn 20 ca khám mỗi trang
    struct HistViewState {
        int sortCol = 0;       // Mặc định sắp xếp theo ngày giờ khám
        bool sortAsc = false;  // Mới nhất lên đầu
        int currentPage = 0;   // Trang hiện tại (0-based)
    };
    auto viewState = std::make_shared<HistViewState>();

    auto reloadHistoryPtr = std::make_shared<std::function<void()>>();
    auto reloadHistory = [reloadHistoryPtr]() { if (*reloadHistoryPtr) (*reloadHistoryPtr)(); };

    *reloadHistoryPtr = [this, patient, histTable, emptyWidget, searchHist, countLabel, viewState, prevPageBtn, pageInfoLabel, nextPageBtn, reloadHistory, PAGE_SIZE]() {
        if (!mainWindow || !mainWindow->getAppointments()) return;

        std::string pId = patient->getID();
        QList<Appointment*> allAppts;

        int totalCount = 0;

        auto *n = mainWindow->getAppointments()->getHeader()->next;
        while (n && n->next) {
            std::string apptPid = mainWindow->getPatientIdForAppointment(n->info);
            if (apptPid == pId && n->info.getState() != -2) {
                allAppts.append(&n->info);
                totalCount++;
            }
            n = n->next;
        }

        // Lọc theo search text
        QString q = searchHist->text().trimmed().toLower();

        QList<Appointment*> filtered;
        for (auto *a : allAppts) {
            if (!q.isEmpty()) {
                Doctor *d = mainWindow->findDoctor(a->getDoctorId());
                QString docName = d ? UIHelper::qs(d->getfullName()) : "";
                QString spec = d ? mainWindow->specialty(d->getSpId()) : "";
                QString room = d ? UIHelper::formatRoom(UIHelper::qs(d->getRoomId())) : "";
                QString sym = mainWindow->getSymptomNamesForAppointment(*a);
                Date ad = a->getDateGet();
                QString dateStr = QString::asprintf("%02d/%02d/%04d %02d:%02d", ad.getDay(), ad.getMonth(), ad.getYear(), ad.getHour(), ad.getMinute());

                QString fullSearch = (docName + " " + spec + " " + room + " " + sym + " " + dateStr).toLower();
                if (!fullSearch.contains(q)) {
                    continue;
                }
            }
            filtered.append(a);
        }

        // Sắp xếp (Sort)
        int col = viewState->sortCol;
        bool asc = viewState->sortAsc;
        libPBL2::qsort(filtered, [this, col, asc](const Appointment *a, const Appointment *b) -> bool {
            if (!a || !b) return false;
            int cmp = 0;
            switch (col) {
                case 0:
                    cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
                    break;
                case 1: {
                    Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                    Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                    cmp = libPBL2::compareVietnameseNames(da ? da->getfullName() : "", db ? db->getfullName() : "");
                    break;
                }
                case 2: {
                    Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                    Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                    int roomA = da ? libPBL2::extractRoomNumber(da->getRoomId()) : -1;
                    int roomB = db ? libPBL2::extractRoomNumber(db->getRoomId()) : -1;
                    if (roomA != roomB) {
                        cmp = (roomA < roomB) ? -1 : 1;
                    } else {
                        cmp = QString::compare(da ? UIHelper::qs(da->getRoomId()) : "", db ? UIHelper::qs(db->getRoomId()) : "");
                    }
                    break;
                }
                case 3: {
                    QString sa = mainWindow->getSymptomNamesForAppointment(*a);
                    QString sb = mainWindow->getSymptomNamesForAppointment(*b);
                    cmp = QString::compare(sa, sb, Qt::CaseInsensitive);
                    break;
                }
                case 4:
                    cmp = (a->getState() < b->getState()) ? -1 : ((a->getState() > b->getState()) ? 1 : 0);
                    break;
                default:
                    cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
                    break;
            }
            if (cmp == 0) cmp = libPBL2::compareNatural(a->getId(), b->getId());
            return asc ? (cmp < 0) : (cmp > 0);
        });

        int totalFiltered = filtered.size();
        int totalPages = (totalFiltered + PAGE_SIZE - 1) / PAGE_SIZE;
        if (totalPages <= 0) totalPages = 1;

        if (viewState->currentPage >= totalPages) viewState->currentPage = totalPages - 1;
        if (viewState->currentPage < 0) viewState->currentPage = 0;

        prevPageBtn->setEnabled(viewState->currentPage > 0);
        nextPageBtn->setEnabled(viewState->currentPage < totalPages - 1);
        pageInfoLabel->setText(QStringLiteral("Trang %1 / %2").arg(viewState->currentPage + 1).arg(totalPages));

        if (totalFiltered == 0) {
            countLabel->setText(QStringLiteral("Đang hiển thị 0 / %1 ca khám").arg(totalCount));
            histTable->hide();
            emptyWidget->show();
            return;
        }

        int startIndex = viewState->currentPage * PAGE_SIZE;
        int endIndex = qMin(startIndex + PAGE_SIZE, totalFiltered);
        countLabel->setText(QStringLiteral("Đang hiển thị %1 - %2 / Tổng %3 ca khám").arg(startIndex + 1).arg(endIndex).arg(totalCount));

        emptyWidget->hide();
        histTable->show();
        histTable->setRowCount(0);

        for (int i = startIndex; i < endIndex; ++i) {
            auto *a = filtered[i];
            int r = histTable->rowCount();
            histTable->insertRow(r);
            histTable->setRowHeight(r, 46);

            Date ad = a->getDateGet();
            Doctor *d = mainWindow->findDoctor(a->getDoctorId());

            // 0. Ngày giờ khám (căn giữa)
            auto *dtItem = new QTableWidgetItem(QString::asprintf("%02d/%02d/%04d %02d:%02d", ad.getDay(), ad.getMonth(), ad.getYear(), ad.getHour(), ad.getMinute()));
            dtItem->setTextAlignment(Qt::AlignCenter);
            dtItem->setData(Qt::UserRole, UIHelper::qs(a->getId()));
            histTable->setItem(r, 0, dtItem);

            // 1. Bác sĩ phụ trách (chỉ hiển thị họ tên, căn giữa)
            QString docText = d ? QStringLiteral("BS. %1").arg(UIHelper::qs(d->getfullName())) : UIHelper::qs(a->getDoctorId());
            auto *docItem = new QTableWidgetItem(docText);
            docItem->setTextAlignment(Qt::AlignCenter);
            histTable->setItem(r, 1, docItem);

            // 2. Phòng khám (căn giữa)
            QString roomText = d ? UIHelper::formatRoom(UIHelper::qs(d->getRoomId())) : "—";
            auto *roomItem = new QTableWidgetItem(roomText);
            roomItem->setTextAlignment(Qt::AlignCenter);
            histTable->setItem(r, 2, roomItem);

            // 3. Triệu chứng (căn giữa)
            QString symText = mainWindow->getSymptomNamesForAppointment(*a);
            if (symText.isEmpty()) symText = "Chưa ghi nhận";
            auto *symItem = new QTableWidgetItem(symText);
            symItem->setTextAlignment(Qt::AlignCenter);
            histTable->setItem(r, 3, symItem);

            // 4. Trạng thái ca khám (căn giữa)
            int st = a->getState();
            if (st == 0) {
                QDateTime now = QDateTime::currentDateTime();
                QDateTime apptDt(QDate(ad.getYear(), ad.getMonth(), ad.getDay()), QTime(ad.getHour(), ad.getMinute()));
                if (apptDt < now) st = -1;
            }
            QString badgeText = "● Chưa khám";
            QString badgeStyle = "QLabel { background: #fffbeb; color: #d97706; border: 1px solid #fde68a; border-radius: 10px; font-size: 11px; font-weight: 750; padding: 3px 8px; }";
            if (st == 1) {
                badgeText = "● Đã khám";
                badgeStyle = "QLabel { background: #ecfdf5; color: #059669; border: 1px solid #a7f3d0; border-radius: 10px; font-size: 11px; font-weight: 750; padding: 3px 8px; }";
            } else if (st == -1) {
                badgeText = "● Trễ hẹn";
                badgeStyle = "QLabel { background: #fff7ed; color: #ea580c; border: 1px solid #ffedd5; border-radius: 10px; font-size: 11px; font-weight: 750; padding: 3px 8px; }";
            }
            auto *badgeLbl = new QLabel(badgeText);
            badgeLbl->setStyleSheet(badgeStyle);
            badgeLbl->setAlignment(Qt::AlignCenter);
            auto *badgeWrap = new QWidget;
            auto *bwl = new QHBoxLayout(badgeWrap);
            bwl->setContentsMargins(4, 0, 4, 0);
            bwl->setAlignment(Qt::AlignCenter);
            bwl->addWidget(badgeLbl);
            histTable->setCellWidget(r, 4, badgeWrap);
        }
    };

    // Kết nối Search (reset về trang đầu)
    connect(searchHist, &QLineEdit::textChanged, &dialog, [viewState, reloadHistory]() {
        viewState->currentPage = 0;
        reloadHistory();
    });

    // Kết nối Sort khi bấm vào Header cột (reset về trang đầu)
    connect(histTable->horizontalHeader(), &QHeaderView::sectionClicked, &dialog, [viewState, reloadHistory](int logicalIndex) {
        if (logicalIndex < 0 || logicalIndex >= 5) return;
        if (viewState->sortCol == logicalIndex) {
            viewState->sortAsc = !viewState->sortAsc;
        } else {
            viewState->sortCol = logicalIndex;
            viewState->sortAsc = (logicalIndex == 0) ? false : true;
        }
        viewState->currentPage = 0;
        reloadHistory();
    });

    // Kết nối nút phân trang
    connect(prevPageBtn, &QPushButton::clicked, &dialog, [viewState, reloadHistory]() {
        if (viewState->currentPage > 0) {
            viewState->currentPage--;
            reloadHistory();
        }
    });
    connect(nextPageBtn, &QPushButton::clicked, &dialog, [viewState, reloadHistory]() {
        viewState->currentPage++;
        reloadHistory();
    });

    // Double-click vào hàng mở chi tiết ca khám
    connect(histTable, &QTableWidget::cellDoubleClicked, &dialog, [this, histTable, reloadHistory](int row, int /*col*/) {
        if (row >= 0 && histTable->item(row, 0)) {
            std::string apptId = UIHelper::ss(histTable->item(row, 0)->data(Qt::UserRole).toString());
            if (!apptId.empty() && mainWindow) {
                mainWindow->editAppointmentDialog(apptId);
                reloadHistory();
            }
        }
    });

    // Nút "＋ Đặt lịch khám mới"
    connect(quickBookBtn, &QPushButton::clicked, &dialog, [this, reloadHistory]() {
        if (mainWindow) {
            mainWindow->showBookingDialog(QDate::currentDate());
            reloadHistory();
        }
    });

    reloadHistory();

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, [&dialog]() { dialog.setFocus(); });
    dialog.exec();
}
