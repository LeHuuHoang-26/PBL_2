#include "DoctorView.h"
#include "MainWindow.h"
#include "DoctorController.h"
#include "UIHelper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QLocale>
#include <QHeaderView>
#include <QLabel>
#include <QFrame>
#include <QTimer>

DoctorView::DoctorView(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent), mainWindow(mainWindow) {
    setupUI();
    refresh();
}

void DoctorView::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 32, 36, 32);
    layout->setSpacing(20);

    // Header Title
    auto *h = new QVBoxLayout;
    h->setSpacing(4);
    auto *t = new QLabel("Đội ngũ bác sĩ");
    t->setObjectName("pageTitle");
    auto *s = new QLabel("Danh sách bác sĩ và chuyên khoa đang hoạt động tại phòng khám.");
    s->setObjectName("pageSubtitle");
    h->addWidget(t);
    h->addWidget(s);
    layout->addLayout(h);

    // Actions bar
    auto *actions = new QHBoxLayout;
    search = new QLineEdit;
    search->setPlaceholderText("Tìm bác sĩ hoặc chuyên khoa…");
    addBtn = UIHelper::button("+  Thêm bác sĩ");
    actions->addWidget(search, 1);
    actions->addWidget(addBtn);
    layout->addLayout(actions);

    // Table: không hiển thị mã bác sĩ, có cột xóa không tiêu đề
    table = UIHelper::table({"BÁC SĨ", "CHUYÊN KHOA", "PHÒNG", "PHÍ KHÁM", "ĐÁNH GIÁ", "TRẠNG THÁI", ""});
    table->setCursor(Qt::PointingHandCursor);
    table->setWordWrap(false);
    table->setTextElideMode(Qt::ElideRight);
    table->verticalHeader()->setDefaultSectionSize(48);

    // Căn lề tên trường trùng khớp với dữ liệu bên dưới
    if (table->horizontalHeaderItem(0)) table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(1)) table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(2)) table->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(3)) table->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(4)) table->horizontalHeaderItem(4)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(5)) table->horizontalHeaderItem(5)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(6)) table->horizontalHeaderItem(6)->setTextAlignment(Qt::AlignCenter);

    // Tự động scale các cột sao cho tổng chiều rộng các cột luôn bằng chính xác chiều rộng bảng
    UIHelper::setupTableScaling(table, {0.24, 0.22, 0.13, 0.15, 0.11, 0.15}, 56);

    // Kích hoạt tính năng click tiêu đề cột để QuickSort (ẩn mũi tên chỉ báo theo yêu cầu)
    table->horizontalHeader()->setSectionsClickable(true);
    table->horizontalHeader()->setSortIndicatorShown(false);
    table->horizontalHeader()->setCursor(Qt::PointingHandCursor);
    connect(table->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
        if (logicalIndex == 6) return; // Bỏ qua cột thao tác xóa
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

    // Thanh phân trang (50 người / trang)
    auto *pagination = new QHBoxLayout;
    pagination->setContentsMargins(4, 4, 4, 0);

    totalLabel = new QLabel("Đang hoạt động: 0 / 0 bác sĩ");
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
            refresh();
        }
    });

    connect(nextBtn, &QPushButton::clicked, this, [this] {
        currentPage++;
        refresh();
    });

    connect(search, &QLineEdit::textChanged, this, [this] {
        currentPage = 0;
        refresh();
    });

    if (addBtn) {
        connect(addBtn, &QPushButton::clicked, this, [this] {
            addDoctorDialog();
            refresh();
        });
    }

    // Nháy đúp chuột để chỉnh sửa thông tin bác sĩ (bỏ qua cột xóa)
    connect(table, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        if (col == 6) return;
        if (row < 0 || row >= table->rowCount()) return;
        auto *item = table->item(row, 0);
        if (!item) return;
        std::string id = UIHelper::ss(item->data(Qt::UserRole).toString());
        if (id.empty()) return;
        editDoctorDialog(id);
        refresh();
    });
}

void DoctorView::resetToFirstPage() {
    currentPage = 0;
    refresh();
}

void DoctorView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    currentPage = 0;
    refresh();
}

void DoctorView::refresh() {
    if (!mainWindow || !table) return;
    table->setRowCount(0);

    auto *doctors = mainWindow->getDoctors();
    if (!doctors) return;

    QString q = search ? search->text().trimmed().toLower() : "";
    QList<Doctor*> matched;
    int activeCount = 0;

    auto *n = doctors->getHeader()->next;
    while (n && n->next) {
        if (!n->info.getValid()) {
            n = n->next;
            continue;
        }
        QString name = UIHelper::qs(n->info.getfullName());
        QString spec = mainWindow->specialty(n->info.getSpId());
        QString room = UIHelper::formatRoom(UIHelper::qs(n->info.getRoomId()));
        QString id = UIHelper::qs(n->info.getID());

        if (q.isEmpty() || name.toLower().contains(q) || spec.toLower().contains(q) || room.toLower().contains(q) || id.toLower().contains(q)) {
            matched.append(&n->info);
            if (n->info.getState()) {
                activeCount++;
            }
        }
        n = n->next;
    }

    // Sắp xếp danh sách bác sĩ bằng QuickSort mẫu (Template QuickSort)
    if (sortColumn >= 0) {
        UIHelper::quickSort(matched, [this](const Doctor *a, const Doctor *b) -> bool {
            if (a == b) return false;
            bool asc = (sortOrder == Qt::AscendingOrder);
            switch (sortColumn) {
                case 0: { // Cột 0: BÁC SĨ (Họ và tên gốc D_FullName từ database)
                    int cmp = UIHelper::compareVietnameseNames(UIHelper::qs(a->getfullName()), UIHelper::qs(b->getfullName()));
                    if (cmp != 0) return asc ? (cmp < 0) : (cmp > 0);
                    break;
                }
                case 1: { // Cột 1: CHUYÊN KHOA (Chuyên khoa từ mã Sp_ID gốc)
                    QString specA = mainWindow ? mainWindow->specialty(a->getSpId()) : UIHelper::qs(a->getSpId());
                    QString specB = mainWindow ? mainWindow->specialty(b->getSpId()) : UIHelper::qs(b->getSpId());
                    int cmp = QString::localeAwareCompare(specA, specB);
                    if (cmp != 0) return asc ? (cmp < 0) : (cmp > 0);
                    break;
                }
                case 2: { // Cột 2: PHÒNG (Mã phòng Room_ID gốc từ database)
                    int cmp = QString::localeAwareCompare(UIHelper::qs(a->getRoomId()), UIHelper::qs(b->getRoomId()));
                    if (cmp != 0) return asc ? (cmp < 0) : (cmp > 0);
                    break;
                }
                case 3: { // Cột 3: PHÍ KHÁM (Giá trị Consultation_Fee double gốc từ database)
                    if (a->getFee() != b->getFee())
                        return asc ? (a->getFee() < b->getFee()) : (a->getFee() > b->getFee());
                    break;
                }
                case 4: { // Cột 4: ĐÁNH GIÁ (Giá trị Rating double gốc từ database)
                    if (a->getRating() != b->getRating())
                        return asc ? (a->getRating() < b->getRating()) : (a->getRating() > b->getRating());
                    break;
                }
                case 5: { // Cột 5: TRẠNG THÁI (Giá trị bool D_State gốc từ database: 1 = Hoạt động, 0 = Ngừng hoạt động)
                    if (a->getState() != b->getState())
                        return asc ? (static_cast<int>(a->getState()) < static_cast<int>(b->getState()))
                                   : (static_cast<int>(a->getState()) > static_cast<int>(b->getState()));
                    break;
                }
                default:
                    break;
            }
            return asc ? (a->getID() < b->getID()) : (a->getID() > b->getID());
        });
    }

    int total = matched.size();
    int totalPages = total == 0 ? 1 : (total + PAGE_SIZE - 1) / PAGE_SIZE;
    currentPage = qBound(0, currentPage, totalPages - 1);

    if (totalLabel) {
        totalLabel->setText(QStringLiteral("Đang hoạt động: %1 / %2 bác sĩ").arg(activeCount).arg(total));
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
        Doctor *d = matched[i];
        int r = table->rowCount();
        table->insertRow(r);
        table->setRowHeight(r, 48);

        QString name = UIHelper::qs(d->getfullName());
        QString spec = mainWindow->specialty(d->getSpId());
        QString room = UIHelper::formatRoom(UIHelper::qs(d->getRoomId()));
        QString feeStr = QLocale(QLocale::Vietnamese).toString(d->getFee(), 'f', 0) + " ₫";
        QString ratingStr = QString::number(d->getRating(), 'f', 1) + " ★";
        bool working = d->getState();

        auto *item0 = new QTableWidgetItem(name);
        item0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item0->setData(Qt::UserRole, UIHelper::qs(d->getID()));
        item0->setToolTip("Nháy đúp chuột để sửa thông tin bác sĩ");
        QFont f = item0->font();
        f.setWeight(QFont::DemiBold);
        item0->setFont(f);
        table->setItem(r, 0, item0);

        auto *item1 = new QTableWidgetItem(spec);
        item1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item1->setToolTip("Nháy đúp chuột để sửa thông tin bác sĩ");
        table->setItem(r, 1, item1);

        auto *item2 = new QTableWidgetItem(room);
        item2->setTextAlignment(Qt::AlignCenter);
        item2->setToolTip("Nháy đúp chuột để sửa thông tin bác sĩ");
        table->setItem(r, 2, item2);

        auto *item3 = new QTableWidgetItem(feeStr);
        item3->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        item3->setToolTip("Nháy đúp chuột để sửa thông tin bác sĩ");
        table->setItem(r, 3, item3);

        auto *item4 = new QTableWidgetItem(ratingStr);
        item4->setTextAlignment(Qt::AlignCenter);
        item4->setToolTip("Nháy đúp chuột để sửa thông tin bác sĩ");
        table->setItem(r, 4, item4);

        auto *item5 = new QTableWidgetItem(working ? "Hoạt động" : "Ngừng hoạt động");
        item5->setTextAlignment(Qt::AlignCenter);
        item5->setForeground(working ? QColor("#059669") : QColor("#dc2626"));
        item5->setToolTip("Nháy đúp chuột để sửa thông tin bác sĩ");
        table->setItem(r, 5, item5);

        // Cột 6: Nút xóa hồ sơ bác sĩ (icon thùng rác đỏ)
        auto action = UIHelper::createActionCell(UIHelper::trashIcon(), "Xóa bác sĩ");
        connect(action.second, &QPushButton::clicked, this, [this, d]() {
            if (!mainWindow) return;
            if (!mainWindow->hasPermission("Per_2")) {
                UIHelper::showWarning(this, "Không có quyền", "Bạn không có quyền thực hiện xóa bác sĩ.");
                return;
            }
            QString name = UIHelper::qs(d->getfullName());
            if (!UIHelper::confirmDelete(this, "Xác nhận xóa bác sĩ",
                QStringLiteral("Bạn có chắc chắn muốn xóa bác sĩ \"%1\" khỏi hệ thống?\nThông tin bác sĩ sau khi xóa sẽ được ẩn khỏi danh sách phân công.").arg(name),
                "Xóa bác sĩ", "Không xóa")) {
                return;
            }

            std::string did = d->getID();
            auto *appointments = mainWindow->getAppointments();
            bool hasUpcoming = false;
            if (appointments) {
                auto *an = appointments->getHeader()->next;
                while (an && an->next) {
                    if (an->info.getValid() && an->info.getDoctorId() == did && an->info.getState() == 0) {
                        hasUpcoming = true;
                        break;
                    }
                    an = an->next;
                }
            }
            if (hasUpcoming) {
                if (!UIHelper::confirmWarning(this, "Cảnh báo lịch khám phụ trách",
                    QStringLiteral("Bác sĩ \"%1\" hiện vẫn còn lịch hẹn chưa khám phụ trách.\nBạn vẫn muốn tiếp tục xóa bác sĩ này khỏi hệ thống?").arg(name),
                    "Vẫn tiếp tục xóa", "Quay lại")) {
                    return;
                }
            }

            auto *doctors = mainWindow->getDoctors();
            if (doctors) {
                auto *n = doctors->getHeader()->next;
                while (n && n->next) {
                    if (n->info.getID() == did) {
                        n->info.setValid(false);
                        break;
                    }
                    n = n->next;
                }
            }
            mainWindow->saveAll();
            refresh();
            mainWindow->refreshPage(0);
        });

        table->setCellWidget(r, 6, action.first);
    }
}

namespace {

bool validateDoctorFields(QWidget *parent, const QString &name, const QString &phone) {
    if (name.trimmed().isEmpty() || phone.trimmed().isEmpty()) {
        UIHelper::showWarning(parent, "Thiếu thông tin", "Vui lòng nhập đầy đủ họ tên và số điện thoại.");
        return false;
    }
    if (phone.trimmed().length() != 10) {
        UIHelper::showWarning(parent, "Số điện thoại không hợp lệ", "Số điện thoại phải gồm đúng 10 chữ số.");
        return false;
    }
    return true;
}

struct DoctorFormData {
    QLineEdit *name = nullptr;
    QLineEdit *phone = nullptr;
    QLineEdit *room = nullptr;
    QSpinBox *age = nullptr;
    QComboBox *gender = nullptr;
    QComboBox *specialtyBox = nullptr;
    QComboBox *state = nullptr;
    QDoubleSpinBox *fee = nullptr;
    QDoubleSpinBox *rating = nullptr;
};

QFrame *createDoctorFormCard(DoctorFormData &form,
                             const QString &initName = "", const QString &initPhone = "",
                             const QString &initRoom = "", int initAge = 32,
                             bool initGender = true, const QString &initSpId = "",
                             bool initState = true, double initFee = 200000.0, double initRating = 5.0) {
    auto *card = new QFrame;
    card->setObjectName("doctorCard");
    card->setStyleSheet("QFrame#doctorCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
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

    form.name = new QLineEdit(initName);
    form.name->setPlaceholderText("Ví dụ: Nguyễn Văn An");
    cardLayout->addLayout(makeField("Họ và tên bác sĩ", form.name));

    auto *row1 = new QHBoxLayout;
    row1->setSpacing(14);
    form.phone = new QLineEdit(initPhone);
    form.phone->setPlaceholderText("Ví dụ: 0905123456");
    UIHelper::setupPhoneInput(form.phone);
    form.room = new QLineEdit(initRoom);
    form.room->setPlaceholderText("Ví dụ: Room 101");
    row1->addLayout(makeField("Số điện thoại liên hệ", form.phone), 1);
    row1->addLayout(makeField("Phòng khám (ví dụ: Room 101)", form.room), 1);
    cardLayout->addLayout(row1);

    auto *row2 = new QHBoxLayout;
    row2->setSpacing(14);
    form.age = new QSpinBox;
    form.age->setRange(18, 120);
    form.age->setValue(initAge);
    form.gender = new QComboBox;
    form.gender->addItems({"Nam", "Nữ"});
    form.gender->setCurrentIndex(initGender ? 0 : 1);
    row2->addLayout(makeField("Tuổi", form.age), 1);
    row2->addLayout(makeField("Giới tính", form.gender), 1);
    cardLayout->addLayout(row2);

    auto *row3 = new QHBoxLayout;
    row3->setSpacing(14);
    form.specialtyBox = new QComboBox;
    auto specList = DoctorController::getSpecializationList("Data/Doctor/Specialization.txt");
    for (const auto &item : specList) {
        form.specialtyBox->addItem(item.first, item.second);
    }
    if (!initSpId.isEmpty()) {
        int idx = form.specialtyBox->findData(initSpId);
        if (idx >= 0) form.specialtyBox->setCurrentIndex(idx);
    }
    form.state = new QComboBox;
    form.state->addItems({"Đang làm việc", "Đã nghỉ việc"});
    form.state->setCurrentIndex(initState ? 0 : 1);
    row3->addLayout(makeField("Chuyên khoa", form.specialtyBox), 1);
    row3->addLayout(makeField("Trạng thái công tác", form.state), 1);
    cardLayout->addLayout(row3);

    auto *row4 = new QHBoxLayout;
    row4->setSpacing(14);
    form.fee = new QDoubleSpinBox;
    form.fee->setRange(0, 100000000);
    form.fee->setDecimals(0);
    form.fee->setValue(initFee);
    form.fee->setSuffix(" đ");

    form.rating = new QDoubleSpinBox;
    form.rating->setRange(0, 5);
    form.rating->setSingleStep(0.1);
    form.rating->setValue(initRating);
    form.rating->setSuffix(" ★");

    row4->addLayout(makeField("Phí khám tham chiếu", form.fee), 1);
    row4->addLayout(makeField("Đánh giá chuyên môn", form.rating), 1);
    cardLayout->addLayout(row4);

    return card;
}

} // namespace

void DoctorView::editDoctorDialog(const std::string &id) {
    if (!mainWindow) return;
    Doctor *doctor = mainWindow->findDoctor(id);
    if (!doctor) return;
    if (!mainWindow->hasPermission("Per_2")) {
        UIHelper::showWarning(this, "Không có quyền", "Bạn không có quyền chỉnh sửa thông tin bác sĩ.");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Chỉnh sửa thông tin bác sĩ");
    dialog.setFixedWidth(560);
    dialog.setStyleSheet(UIHelper::dialogFormStyle());

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);

    root->addLayout(UIHelper::createDialogHeader("👨‍⚕️", "Chỉnh sửa thông tin bác sĩ", "Cập nhật chuyên khoa, phòng khám và thông tin làm việc"));

    DoctorFormData form;
    root->addWidget(createDoctorFormCard(form,
                                         UIHelper::qs(doctor->getfullName()),
                                         UIHelper::qs(doctor->getPhone()),
                                         UIHelper::qs(doctor->getRoomId()),
                                         doctor->getAge(),
                                         doctor->getGender(),
                                         UIHelper::qs(doctor->getSpId()),
                                         doctor->getState(),
                                         doctor->getFee(),
                                         doctor->getRating()));

    // Footer Buttons
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->addStretch();

    auto *cancel = new QPushButton("Hủy");
    cancel->setCursor(Qt::PointingHandCursor);
    cancel->setStyleSheet("QPushButton { background: #ffffff; color: #475569; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 9px 22px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #f1f5f9; color: #0f172a; }");

    auto *save = new QPushButton("✓  Lưu thay đổi");
    save->setCursor(Qt::PointingHandCursor);
    save->setStyleSheet("QPushButton { background: #0d9488; color: #ffffff; border: none; border-radius: 8px; padding: 9px 24px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #0f766e; }");

    btnRow->addWidget(cancel);
    btnRow->addWidget(save);
    root->addLayout(btnRow);

    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(save, &QPushButton::clicked, &dialog, [this, &dialog, doctor, &form] {
        if (!validateDoctorFields(&dialog, form.name->text(), form.phone->text())) {
            return;
        }
        doctor->setfullName(UIHelper::ss(form.name->text().trimmed()));
        doctor->setPhone(UIHelper::ss(form.phone->text().trimmed()));
        doctor->setAge(form.age->value());
        doctor->setGender(form.gender->currentIndex() == 0);
        doctor->setSpId(UIHelper::ss(form.specialtyBox->currentData().toString()));
        doctor->setRoomId(UIHelper::ss(form.room->text().trimmed()));
        doctor->setFee(form.fee->value());
        doctor->setRating(form.rating->value());
        doctor->setState(form.state->currentIndex() == 0);

        mainWindow->saveAll();
        refresh();
        dialog.accept();
    });

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, [&dialog]() { dialog.setFocus(); });
    dialog.exec();
}

void DoctorView::addDoctorDialog() {
    if (!mainWindow) return;
    if (!mainWindow->hasPermission("Per_2")) {
        UIHelper::showWarning(this, "Không có quyền", "Bạn không có quyền thêm bác sĩ mới.");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Thêm bác sĩ mới");
    dialog.setFixedWidth(560);
    dialog.setStyleSheet(UIHelper::dialogFormStyle());

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);

    root->addLayout(UIHelper::createDialogHeader("👨‍⚕️", "Thêm bác sĩ mới", "Tạo hồ sơ và thông tin công tác cho bác sĩ mới"));

    std::string newId = DoctorController::generateNextId(mainWindow->getDoctors());
    DoctorFormData form;
    root->addWidget(createDoctorFormCard(form));

    // Footer Buttons
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->addStretch();

    auto *cancel = new QPushButton("Hủy");
    cancel->setCursor(Qt::PointingHandCursor);
    cancel->setStyleSheet("QPushButton { background: #ffffff; color: #475569; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 9px 22px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #f1f5f9; color: #0f172a; }");

    auto *save = new QPushButton("✓  Thêm bác sĩ");
    save->setCursor(Qt::PointingHandCursor);
    save->setStyleSheet("QPushButton { background: #0d9488; color: #ffffff; border: none; border-radius: 8px; padding: 9px 24px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #0f766e; }");

    btnRow->addWidget(cancel);
    btnRow->addWidget(save);
    root->addLayout(btnRow);

    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(save, &QPushButton::clicked, &dialog, [this, &dialog, newId, &form] {
        if (!validateDoctorFields(&dialog, form.name->text(), form.phone->text())) {
            return;
        }

        QString roomStr = form.room->text().trimmed();
        if (roomStr.isEmpty()) {
            roomStr = "Room 101";
        }

        Doctor newDoctor(
            newId,
            UIHelper::ss(form.name->text().trimmed()),
            UIHelper::ss(form.phone->text().trimmed()),
            form.gender->currentIndex() == 0,
            form.age->value(),
            UIHelper::ss(form.specialtyBox->currentData().toString()),
            form.state->currentIndex() == 0,
            UIHelper::ss(roomStr),
            form.fee->value(),
            form.rating->value()
        );

        auto *docList = mainWindow->getDoctors();
        if (docList) {
            docList->insert(newDoctor);
            mainWindow->saveAll();
        }
        dialog.accept();
    });

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, [&dialog]() { dialog.setFocus(); });
    dialog.exec();
}
