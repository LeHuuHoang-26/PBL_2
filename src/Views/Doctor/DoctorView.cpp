#include "DoctorView.h"
#include "AppConstants.h"
#include "libPBL2.h"
#include "MainWindow.h"
#include "AppointmentController.h"
#include "DoctorController.h"
#include "UIHelper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLocale>
#include <QHeaderView>
#include <QScrollBar>
#include <QFrame>
#include <QTimer>
#include <functional>
#include <memory>

namespace {
DoctorSortBy doctorSortByColumn(int col) {
    switch (col) {
        case 0: return DoctorSortBy::Name;
        case 1: return DoctorSortBy::Specialty;
        case 2: return DoctorSortBy::Room;
        case 3: return DoctorSortBy::Fee;
        case 4: return DoctorSortBy::Rating;
        case 5: return DoctorSortBy::State;
        default: return DoctorSortBy::ID;
    }
}

bool hasCrossSpecialtyRoomConflict(
    const LinkList<Doctor> *doctors,
    const std::string &excludeDocId,
    const std::string &spId,
    const QString &roomStr,
    QString &outConflictDoctor
) {
    std::string key = libPBL2::canonicalRoomKey(UIHelper::ss(roomStr));
    if (key.empty() || !doctors || !doctors->getHeader()) return false;
    auto *dn = doctors->getHeader()->next;
    while (dn && dn->next) {
        if (!dn->info.isDefault() && dn->info.getState() != -1 && dn->info.getID() != excludeDocId) {
            if (libPBL2::canonicalRoomKey(dn->info.getRoomId()) == key && dn->info.getSpId() != spId) {
                outConflictDoctor = UIHelper::qs(dn->info.getfullName());
                return true;
            }
        }
        dn = dn->next;
    }
    return false;
}
}

DoctorView::DoctorView(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent), mainWindow(mainWindow) {
    setupUI();
}

void DoctorView::refresh(bool preserveScroll) {
    if (!mainWindow || !table) return;
    if (addBtn) {
        addBtn->setVisible(mainWindow->hasPermission("Per_2"));
    }
    int vScroll = (preserveScroll && table->verticalScrollBar()) ? table->verticalScrollBar()->value() : 0;
    int curRow = preserveScroll ? table->currentRow() : -1;
    table->setUpdatesEnabled(false);
    table->setRowCount(0);

    auto *doctors = mainWindow->getDoctors();
    if (!doctors) {
        table->setUpdatesEnabled(true);
        return;
    }

    QString q = search ? search->text().trimmed().toLower() : "";
    QList<Doctor*> matched;
    int activeCount = 0;

    auto *n = doctors->getHeader()->next;
    while (n && n->next) {
        if (n->info.isDefault() || n->info.getState() == -1) {
            n = n->next;
            continue;
        }
        QString name = UIHelper::qs(n->info.getfullName());
        QString spec = mainWindow->specialty(n->info.getSpId());
        QString rawRoom = UIHelper::qs(n->info.getRoomId());
        QString room = UIHelper::formatRoom(rawRoom);
        QString roomKey = UIHelper::qs(libPBL2::canonicalRoomKey(n->info.getRoomId()));
        QString qRoomKey = UIHelper::qs(libPBL2::canonicalRoomKey(UIHelper::ss(q)));
        QString id = UIHelper::qs(n->info.getID());
        QString phone = UIHelper::qs(n->info.getPhone());

        bool matchRoom = room.toLower().contains(q) || rawRoom.toLower().contains(q) ||
                         (!qRoomKey.isEmpty() && roomKey.contains(qRoomKey));
        if (q.isEmpty() || name.toLower().contains(q) || spec.toLower().contains(q) || matchRoom || id.toLower().contains(q) || phone.contains(q)) {
            matched.append(&n->info);
            if (n->info.getState() == 1) {
                activeCount++;
            }
        }
        n = n->next;
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
        bool working = (d->getState() == 1);

        auto *item0 = new QTableWidgetItem(name);
        item0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item0->setData(Qt::UserRole, UIHelper::qs(d->getID()));

        QFont f = item0->font();
        f.setWeight(QFont::DemiBold);
        item0->setFont(f);
        table->setItem(r, 0, item0);

        auto *item1 = new QTableWidgetItem(spec);
        item1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(r, 1, item1);

        auto *item2 = new QTableWidgetItem(room);
        item2->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 2, item2);

        auto *item3 = new QTableWidgetItem(feeStr);
        item3->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(r, 3, item3);

        auto *item4 = new QTableWidgetItem(ratingStr);
        item4->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 4, item4);

        auto *item5 = new QTableWidgetItem(working ? "🟢" : "🔴");
        item5->setTextAlignment(Qt::AlignCenter);
        item5->setForeground(working ? QColor("#059669") : QColor("#dc2626"));
        table->setItem(r, 5, item5);

        auto action = UIHelper::createActionCell(UIHelper::trashIcon(), "Xóa bác sĩ", table);
        if (mainWindow && !mainWindow->hasPermission("Per_2")) {
            action.second->setToolTip("Bạn không có quyền xóa bác sĩ (Yêu cầu quyền Per_2)");
        }
        connect(action.second, &QPushButton::clicked, this, [this, did = d->getID()]() {
            if (!mainWindow) return;
            if (!mainWindow->hasPermission("Per_2")) {
                UIHelper::showWarning(this, "Không có quyền", "Bạn không có quyền thực hiện xóa bác sĩ.");
                return;
            }
            Doctor *targetDoc = mainWindow->findDoctor(did);
            if (!targetDoc) return;
            QString name = UIHelper::qs(targetDoc->getfullName());

            auto affected = AppointmentController::getAffectedAppointmentsForDoctor(mainWindow->getAppointments(), did);
            if (!affected.isEmpty()) {
                QString warnMsg = QStringLiteral(
                    "Bác sĩ \"%1\" hiện vẫn còn %2 lịch hẹn chưa khám phụ trách.\n\n"
                    "Nếu tiếp tục xóa bác sĩ, hệ thống sẽ tự động chuyển toàn bộ các lịch hẹn này sang bác sĩ phù hợp khác "
                    "(ưu tiên cùng hoặc gần chuyên khoa, giữ nguyên giờ hẹn hoặc tìm khung giờ trống sớm nhất).\n\n"
                    "Bạn có muốn tiếp tục xóa bác sĩ và tự động chuyển ca khám không?"
                ).arg(name).arg(affected.size());
                if (!UIHelper::confirmWarning(this, "Xác nhận xóa bác sĩ & chuyển lịch khám", warnMsg, "Xóa & chuyển lịch", "Hủy bỏ")) {
                    return;
                }
            } else {
                if (!UIHelper::confirmDelete(this, "Xác nhận xóa bác sĩ",
                    QStringLiteral("Bạn có chắc chắn muốn xóa bác sĩ \"%1\" khỏi hệ thống?\nThông tin bác sĩ sau khi xóa sẽ được loại bỏ khỏi danh sách.").arg(name),
                    "Xóa bác sĩ", "Hủy bỏ")) {
                    return;
                }
            }

            targetDoc = mainWindow->findDoctor(did);
            if (!targetDoc) return;
            int oldState = targetDoc->getState();
            targetDoc->setState(-1);

            QList<RescheduledResultItem> resultItems;
            QList<std::string> insertedApptIds;
            QList<std::string> insertedSpIds;
            QList<std::string> modifiedOldApptIds;
            int failedCount = 0;

            try {
                if (!affected.isEmpty()) {
                    auto holidays = AppointmentController::loadHolidays(AppConstants::Paths::HOLIDAYS);
                    AppointmentController::reassignAppointmentsForDoctor(
                        mainWindow->getAppointments(),
                        mainWindow->getSymptomPatients(),
                        mainWindow->getSymptomPatientDetails(),
                        mainWindow->getSymptoms(),
                        mainWindow->getDoctors(),
                        mainWindow->getPatients(),
                        did,
                        holidays,
                        &resultItems,
                        &failedCount,
                        &insertedApptIds,
                        &insertedSpIds,
                        &modifiedOldApptIds
                    );
                    if (failedCount > 0) {
                        targetDoc->setState(oldState);
                        UIHelper::showWarning(this, "Không thể xóa bác sĩ",
                            QStringLiteral("Không thể xóa bác sĩ \"%1\" vì có %2 lịch hẹn chưa tìm được bác sĩ thay thế thuộc các chuyên khoa phù hợp.\nVui lòng bổ sung bác sĩ hoặc xử lý thủ công các lịch hẹn này trước.")
                                .arg(name).arg(failedCount));
                        return;
                    }
                }

                SaveScope scope = affected.isEmpty() ? SaveScope::Doctor : SaveScope::All;
                if (!mainWindow->saveAll(scope, SaveAction::Normal)) {
                    if (Doctor *rbDoc = mainWindow->findDoctor(did)) rbDoc->setState(oldState);
                    if (!affected.isEmpty()) {
                        AppointmentController::rollbackInsertedAppointments(
                            mainWindow->getAppointments(),
                            mainWindow->getSymptomPatients(),
                            mainWindow->getSymptomPatientDetails(),
                            insertedApptIds, insertedSpIds, modifiedOldApptIds
                        );
                    }
                    return;
                }
                refresh(true);
                mainWindow->refreshPage(4);
                mainWindow->refreshPage(0);
                mainWindow->refreshPage(1);
                mainWindow->refreshPage(5);
                if (!affected.isEmpty()) {
                    mainWindow->showRescheduledSummaryDialog(this, resultItems, affected.size(), "Kết quả chuyển bác sĩ khi xóa hồ sơ");
                }
            } catch (const std::exception &e) {
                if (Doctor *rbDoc = mainWindow->findDoctor(did)) rbDoc->setState(oldState);
                AppointmentController::rollbackInsertedAppointments(
                    mainWindow->getAppointments(), mainWindow->getSymptomPatients(), mainWindow->getSymptomPatientDetails(),
                    insertedApptIds, insertedSpIds, modifiedOldApptIds);
                UIHelper::showWarning(this, "Lỗi hệ thống", QString("Không thể xóa bác sĩ: %1").arg(QString::fromUtf8(e.what())));
            }
        });

        table->setCellWidget(r, 6, action.first);
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

void DoctorView::resetToFirstPage() {
    sortColumn = -1;
    sortAscending = false;
    if (mainWindow && mainWindow->getDoctors()) {
        DoctorController::sortDoctors(mainWindow->getDoctors(), DoctorSortBy::ID, false);
    }
    currentPage = 0;
    refresh(false);
}

void DoctorView::reapplyCurrentSort() {
    if (!mainWindow || !mainWindow->getDoctors()) return;
    if (sortColumn < 0 || sortColumn >= 6) {
        DoctorController::sortDoctors(mainWindow->getDoctors(), DoctorSortBy::ID, false);
        return;
    }
    DoctorController::sortDoctors(mainWindow->getDoctors(), doctorSortByColumn(sortColumn), sortAscending);
}

void DoctorView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (event->spontaneous()) return;
    resetToFirstPage();
}

void DoctorView::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 32, 36, 32);
    layout->setSpacing(20);

    auto *h = new QVBoxLayout;
    h->setSpacing(4);
    auto *t = new QLabel("Đội ngũ bác sĩ");
    t->setObjectName("pageTitle");
    auto *s = new QLabel("Danh sách bác sĩ và chuyên khoa đang hoạt động tại phòng khám.");
    s->setObjectName("pageSubtitle");
    h->addWidget(t);
    h->addWidget(s);
    layout->addLayout(h);

    auto *actions = new QHBoxLayout;
    search = new QLineEdit;
    search->setPlaceholderText("Tìm bác sĩ hoặc chuyên khoa…");
    addBtn = UIHelper::button("+  Thêm bác sĩ");

    actions->addWidget(search, 1);
    actions->addWidget(addBtn);
    layout->addLayout(actions);

    table = UIHelper::table({"BÁC SĨ", "CHUYÊN KHOA", "PHÒNG", "PHÍ KHÁM", "ĐÁNH GIÁ", "TRẠNG THÁI", ""}, this);
    table->setWordWrap(false);
    table->setTextElideMode(Qt::ElideRight);
    table->verticalHeader()->setDefaultSectionSize(48);

    if (table->horizontalHeaderItem(0)) table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(1)) table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(2)) table->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(3)) table->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(4)) table->horizontalHeaderItem(4)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(5)) table->horizontalHeaderItem(5)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(6)) table->horizontalHeaderItem(6)->setTextAlignment(Qt::AlignCenter);

    UIHelper::setupTableScaling(table, {0.24, 0.22, 0.13, 0.15, 0.11, 0.15}, 56);

    connect(table->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
        if (logicalIndex < 0 || logicalIndex >= 6) return;
        if (sortColumn == logicalIndex) {
            sortAscending = !sortAscending;
        } else {
            sortColumn = logicalIndex;
            sortAscending = true;
        }
        if (mainWindow && mainWindow->getDoctors()) {
            DoctorController::sortDoctors(mainWindow->getDoctors(), doctorSortByColumn(logicalIndex), sortAscending);
            currentPage = 0;
            refresh(false);
        }
    });

    auto *tableCard = UIHelper::createTableCard(table);
    layout->addWidget(tableCard, 1);

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

    if (addBtn) {
        connect(addBtn, &QPushButton::clicked, this, [this] {
            if (addDoctorDialog()) {
                refresh(false);
                if (mainWindow) {
                    mainWindow->refreshPage(4);
                    mainWindow->refreshPage(0);
                    mainWindow->refreshPage(1);
                    mainWindow->refreshPage(5);
                }
            }
        });
    }

    connect(table, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        if (col == 6) return;
        if (row < 0 || row >= table->rowCount()) return;
        auto *item = table->item(row, 0);
        if (!item) return;
        std::string id = UIHelper::ss(item->data(Qt::UserRole).toString());
        if (id.empty()) return;
        QTimer::singleShot(0, this, [this, id]() {
            editDoctorDialog(id);
            refresh(true);
        });
    });
}

bool DoctorView::addDoctorDialog() {
    if (!mainWindow) return false;
    if (!mainWindow->hasPermission("Per_2")) {
        UIHelper::showWarning(this, "Không có quyền", "Bạn không có quyền thêm bác sĩ mới.");
        return false;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Thêm bác sĩ mới");
    dialog.setFixedWidth(560);
    dialog.setStyleSheet(UIHelper::dialogFormStyle());

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);

    auto *headerBox = new QHBoxLayout;
    headerBox->setSpacing(12);

    auto *icon = new QLabel("👨‍⚕️");
    icon->setFixedSize(46, 46);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("background: #f0fdfa; border: 1.5px solid #99f6e4; border-radius: 12px; font-size: 22px;");

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    auto *title = new QLabel("Thêm bác sĩ mới");
    title->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a;");
    auto *sub = new QLabel("Tạo hồ sơ và thông tin công tác cho bác sĩ mới");
    sub->setStyleSheet("font-size: 12.5px; color: #64748b;");
    titleCol->addWidget(title);
    titleCol->addWidget(sub);

    headerBox->addWidget(icon);
    headerBox->addLayout(titleCol, 1);
    root->addLayout(headerBox);

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

    std::string newId = DoctorController::generateNextId(mainWindow->getDoctors());
    auto *idRow = new QHBoxLayout;
    idRow->setSpacing(14);
    auto *idInput = new QLineEdit(UIHelper::qs(newId));
    idInput->setReadOnly(true);
    idInput->setStyleSheet("QLineEdit { background: #f1f5f9; color: #0d9488; font-weight: 700; border: 1.5px solid #cbd5e1; }");
    idRow->addLayout(makeField("Mã bác sĩ (Tự động cấp)", idInput), 1);

    auto *name = new QLineEdit;
    name->setPlaceholderText("Ví dụ: Nguyễn Văn An");
    idRow->addLayout(makeField("Họ và tên bác sĩ", name), 2);
    cardLayout->addLayout(idRow);

    auto *row1 = new QHBoxLayout;
    row1->setSpacing(14);
    auto *phone = new QLineEdit;
    phone->setPlaceholderText("Ví dụ: 0912345678");
    UIHelper::setupPhoneInput(phone);
    auto *room = new QLineEdit;
    room->setPlaceholderText("Ví dụ: Room 101");
    row1->addLayout(makeField("Số điện thoại liên hệ", phone), 1);
    row1->addLayout(makeField("Phòng khám (ví dụ: Room 101)", room), 1);
    cardLayout->addLayout(row1);

    auto *row2 = new QHBoxLayout;
    row2->setSpacing(14);
    auto *age = new QSpinBox;
    age->setRange(18, 120);
    age->setValue(32);

    auto *gender = new QComboBox;
    gender->addItems({"Nam", "Nữ"});
    gender->setCurrentIndex(0);

    row2->addLayout(makeField("Tuổi", age), 1);
    row2->addLayout(makeField("Giới tính", gender), 1);
    cardLayout->addLayout(row2);

    auto *row3 = new QHBoxLayout;
    row3->setSpacing(14);
    auto *specialtyBox = new QComboBox;
    auto specList = DoctorController::getSpecializationList(AppConstants::Paths::SPECIALIZATION);
    for (const auto &item : specList) {
        specialtyBox->addItem(item.first, item.second);
    }

    auto *state = new QComboBox;
    state->addItems({"Đang công tác", "Tạm ngưng"});
    state->setCurrentIndex(0);

    row3->addLayout(makeField("Chuyên khoa", specialtyBox), 1);
    row3->addLayout(makeField("Trạng thái công tác", state), 1);
    cardLayout->addLayout(row3);

    auto *row4 = new QHBoxLayout;
    row4->setSpacing(14);
    auto *fee = new QDoubleSpinBox;
    fee->setRange(0, 100000000);
    fee->setDecimals(0);
    fee->setValue(200000);
    fee->setSuffix(" đ");

    auto *rating = new QDoubleSpinBox;
    rating->setRange(0, 5);
    rating->setDecimals(1);
    rating->setSingleStep(0.1);
    rating->setValue(5.0);
    rating->setSuffix(" ★");

    row4->addLayout(makeField("Phí khám tham chiếu", fee), 1);
    row4->addLayout(makeField("Đánh giá ban đầu", rating), 1);
    cardLayout->addLayout(row4);

    root->addWidget(card);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->addStretch();

    auto *cancel = new QPushButton("Hủy");
    cancel->setAutoDefault(false);
    cancel->setDefault(false);
    cancel->setCursor(Qt::PointingHandCursor);
    cancel->setStyleSheet("QPushButton { background: #ffffff; color: #475569; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 9px 22px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #f1f5f9; color: #0f172a; }");

    auto *save = new QPushButton("✓  Thêm bác sĩ");
    save->setAutoDefault(false);
    save->setDefault(false);
    save->setCursor(Qt::PointingHandCursor);
    save->setStyleSheet("QPushButton { background: #0d9488; color: #ffffff; border: none; border-radius: 8px; padding: 9px 24px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #0f766e; }");

    btnRow->addWidget(cancel);
    btnRow->addWidget(save);
    root->addLayout(btnRow);

    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(save, &QPushButton::clicked, &dialog, [this, &dialog, newId, name, phone, room, fee, rating, age, gender, specialtyBox, state] {
        QString nameStr = name->text().trimmed();
        QString phoneStr = phone->text().trimmed();
        QString roomStr = room->text().trimmed();
        if (nameStr.isEmpty() || phoneStr.isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng nhập đầy đủ họ tên và số điện thoại.");
            return;
        }
        if (libPBL2::containsForbiddenStorageChars(UIHelper::ss(nameStr)) ||
            libPBL2::containsForbiddenStorageChars(UIHelper::ss(roomStr))) {
            UIHelper::showWarning(&dialog, "Ký tự không hợp lệ", "Họ tên và phòng khám không được chứa ký tự '|' hoặc ký tự xuống dòng.");
            return;
        }
        if (!UIHelper::isValidPhone(phoneStr)) {
            UIHelper::showWarning(&dialog, "Số điện thoại không hợp lệ", "Số điện thoại phải gồm đúng 10 chữ số và bắt đầu bằng số 0 (Ví dụ: 0912345678).");
            phone->setFocus();
            return;
        }
        if (mainWindow && mainWindow->getDoctors()) {
            auto *dn = mainWindow->getDoctors()->getHeader()->next;
            while (dn && dn->next) {
                if (dn->info.getState() != -1 && dn->info.getPhone() == UIHelper::ss(phoneStr)) {
                    UIHelper::showWarning(&dialog, "Trùng thông tin", "Số điện thoại này đã được sử dụng bởi một bác sĩ khác.");
                    phone->setFocus();
                    return;
                }
                dn = dn->next;
            }
        }
        QString spData = specialtyBox->currentData().toString().trimmed();
        if (spData.isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng chọn chuyên khoa hợp lệ cho bác sĩ.");
            return;
        }
        std::string normalizedRoomId = libPBL2::storageRoomId(UIHelper::ss(roomStr));
        if (normalizedRoomId.empty()) {
            UIHelper::showWarning(&dialog, "Phòng khám không hợp lệ", "Vui lòng nhập số hoặc tên phòng khám hợp lệ (Ví dụ: Room 101).");
            room->setFocus();
            return;
        }
        QString conflictDoc;
        if (hasCrossSpecialtyRoomConflict(mainWindow->getDoctors(), "", UIHelper::ss(spData), roomStr, conflictDoc)) {
            UIHelper::showWarning(&dialog, "Trùng phòng khám khác chuyên khoa",
                QStringLiteral("Phòng \"%1\" hiện đang được sử dụng bởi BS. %2 thuộc chuyên khoa khác. Vui lòng chọn phòng khác.")
                    .arg(UIHelper::formatRoom(roomStr), conflictDoc));
            room->setFocus();
            return;
        }
        if (fee->value() <= 0) {
            UIHelper::showWarning(&dialog, "Phí khám không hợp lệ", "Phí khám tham chiếu phải lớn hơn 0 đ.");
            fee->setFocus();
            return;
        }

        Doctor newDoctor(
            newId,
            UIHelper::ss(nameStr),
            UIHelper::ss(phoneStr),
            gender->currentIndex() == 0,
            age->value(),
            UIHelper::ss(spData),
            state->currentIndex() == 0 ? 1 : 0,
            normalizedRoomId,
            fee->value(),
            rating->value()
        );

        try {
            auto *docList = mainWindow->getDoctors();
            if (docList) {
                docList->insert(newDoctor);
                if (!mainWindow->saveAll(SaveScope::Doctor, SaveAction::Normal)) {
                    auto *rn = docList->getHeader()->next;
                    while (rn && rn->next) {
                        if (rn->info.getID() == newId) { docList->remove(rn); break; }
                        rn = rn->next;
                    }
                    return;
                }
            }
            dialog.accept();
            UIHelper::showSuccess(this, "Thành công", "Đã thêm bác sĩ mới thành công.");
        } catch (const AppException &e) {
            UIHelper::showWarning(&dialog, "Lỗi thêm bác sĩ", QString::fromUtf8(e.what()));
        } catch (const std::exception &e) {
            UIHelper::showWarning(&dialog, "Lỗi hệ thống", QString("Đã xảy ra ngoại lệ: %1").arg(e.what()));
        }
    });

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    return dialog.exec() == QDialog::Accepted;
}

void DoctorView::editDoctorDialog(const std::string &id) {
    if (!mainWindow) return;
    Doctor *doctor = mainWindow->findDoctor(id);
    if (!doctor) return;

    bool canEdit = mainWindow->hasPermission("Per_2");

    QDialog dialog(this);
    dialog.setWindowTitle("Chi tiết thông tin bác sĩ & Lịch sử khám");

    int availW = this->width() > 400 ? this->width() : (mainWindow ? (mainWindow->width() - 238) : 1200);
    int availH = this->height() > 300 ? this->height() : (mainWindow ? mainWindow->height() : 800);
    int targetW = static_cast<int>(availW * 0.90);
    int targetH = static_cast<int>(availH * 0.90);
    dialog.setFixedSize(targetW, targetH);

    dialog.setStyleSheet(UIHelper::dialogFormStyle() + QStringLiteral(R"(
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
    )"));

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

    auto *headerBox = new QHBoxLayout;
    headerBox->setSpacing(12);

    auto *iconBadge = new QLabel("👨‍⚕️");
    iconBadge->setFixedSize(40, 40);
    iconBadge->setAlignment(Qt::AlignCenter);
    iconBadge->setStyleSheet("background: #f0fdfa; border: 1.5px solid #99f6e4; border-radius: 10px; font-size: 20px;");

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(0);
    auto *dlgTitle = new QLabel("Hồ sơ bác sĩ");
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

    auto *bodyLayout = new QHBoxLayout;
    bodyLayout->setSpacing(16);

    auto *leftCard = new QFrame;
    leftCard->setObjectName("leftCard");
    leftCard->setFixedWidth(360);
    leftCard->setStyleSheet("QFrame#leftCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
    auto *leftCardLayout = new QVBoxLayout(leftCard);
    leftCardLayout->setContentsMargins(18, 16, 18, 16);
    leftCardLayout->setSpacing(12);

    auto *leftHeader = new QLabel("👨‍⚕️  THÔNG TIN BÁC SĨ");
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

    auto *idField = new QLineEdit(UIHelper::qs(doctor->getID()));
    idField->setEnabled(false);
    idField->setFixedHeight(38);
    idField->setStyleSheet("background: #f8fafc; border: 1.5px solid #e2e8f0; border-radius: 8px; padding: 7px 12px; font-size: 13px; font-weight: 700; color: #0d9488;");
    leftCardLayout->addLayout(makeField("Mã định danh bác sĩ", idField));

    auto *name = new QLineEdit(UIHelper::qs(doctor->getfullName()));
    name->setEnabled(canEdit);
    name->setFixedHeight(38);
    leftCardLayout->addLayout(makeField("Họ và tên bác sĩ", name));

    auto *rowPhoneRoom = new QHBoxLayout;
    rowPhoneRoom->setSpacing(10);
    auto *phone = new QLineEdit(UIHelper::qs(doctor->getPhone()));
    UIHelper::setupPhoneInput(phone);
    phone->setEnabled(canEdit);
    phone->setFixedHeight(38);
    auto *room = new QLineEdit(UIHelper::qs(doctor->getRoomId()));
    room->setEnabled(canEdit);
    room->setFixedHeight(38);
    rowPhoneRoom->addLayout(makeField("Số điện thoại liên hệ", phone), 1);
    rowPhoneRoom->addLayout(makeField("Phòng khám", room), 1);
    leftCardLayout->addLayout(rowPhoneRoom);

    auto *rowAgeGender = new QHBoxLayout;
    rowAgeGender->setSpacing(10);
    auto *age = new QSpinBox;
    age->setRange(18, 120);
    age->setValue(doctor->getAge());
    age->setEnabled(canEdit);
    if (!canEdit) age->setButtonSymbols(QAbstractSpinBox::NoButtons);
    age->setFixedHeight(38);

    auto *gender = new QComboBox;
    gender->addItems({"Nam", "Nữ"});
    gender->setCurrentIndex(doctor->getGender() ? 0 : 1);
    gender->setEnabled(canEdit);
    gender->setFixedHeight(38);

    rowAgeGender->addLayout(makeField("Tuổi", age), 1);
    rowAgeGender->addLayout(makeField("Giới tính", gender), 1);
    leftCardLayout->addLayout(rowAgeGender);

    auto *rowSpecState = new QHBoxLayout;
    rowSpecState->setSpacing(10);
    auto *specialtyBox = new QComboBox;
    auto specList = DoctorController::getSpecializationList(AppConstants::Paths::SPECIALIZATION);
    for (const auto &item : specList) {
        specialtyBox->addItem(item.first, item.second);
    }
    specialtyBox->setCurrentIndex(specialtyBox->findData(UIHelper::qs(doctor->getSpId())));
    specialtyBox->setEnabled(canEdit);
    specialtyBox->setFixedHeight(38);

    auto *state = new QComboBox;
    state->addItems({"Đang công tác", "Tạm ngưng"});
    state->setCurrentIndex(doctor->getState() == 1 ? 0 : 1);
    state->setEnabled(canEdit);
    state->setFixedHeight(38);

    rowSpecState->addLayout(makeField("Chuyên khoa", specialtyBox), 1);
    rowSpecState->addLayout(makeField("Trạng thái công tác", state), 1);
    leftCardLayout->addLayout(rowSpecState);

    auto *rowFeeRating = new QHBoxLayout;
    rowFeeRating->setSpacing(10);
    auto *fee = new QDoubleSpinBox;
    fee->setRange(0, 100000000);
    fee->setDecimals(0);
    fee->setValue(doctor->getFee());
    fee->setSuffix(" đ");
    fee->setEnabled(canEdit);
    if (!canEdit) fee->setButtonSymbols(QAbstractSpinBox::NoButtons);
    fee->setFixedHeight(38);

    auto *rating = new QDoubleSpinBox;
    rating->setRange(0, 5);
    rating->setDecimals(1);
    rating->setSingleStep(0.1);
    rating->setValue(doctor->getRating());
    rating->setSuffix(" ★");
    rating->setEnabled(canEdit);
    if (!canEdit) rating->setButtonSymbols(QAbstractSpinBox::NoButtons);
    rating->setFixedHeight(38);

    rowFeeRating->addLayout(makeField("Phí khám tham chiếu", fee), 1);
    rowFeeRating->addLayout(makeField("Đánh giá chuyên môn", rating), 1);
    leftCardLayout->addLayout(rowFeeRating);

    leftCardLayout->addSpacing(4);

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

        connect(saveBtn, &QPushButton::clicked, &dialog, [this, &dialog, docId = id, name, phone, room, fee, rating, age, gender, specialtyBox, state]() {
            Doctor *doctor = mainWindow ? mainWindow->findDoctor(docId) : nullptr;
            if (!doctor) {
                UIHelper::showWarning(&dialog, "Không tìm thấy", "Hồ sơ bác sĩ không còn tồn tại trong hệ thống.");
                dialog.reject();
                return;
            }
            QString nameStr = name->text().trimmed();
            QString phoneStr = phone->text().trimmed();
            QString roomStr = room->text().trimmed();
            if (nameStr.isEmpty() || phoneStr.isEmpty()) {
                UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng nhập họ tên và điện thoại.");
                return;
            }
            if (libPBL2::containsForbiddenStorageChars(UIHelper::ss(nameStr)) ||
                libPBL2::containsForbiddenStorageChars(UIHelper::ss(roomStr))) {
                UIHelper::showWarning(&dialog, "Ký tự không hợp lệ", "Họ tên và phòng khám không được chứa ký tự '|' hoặc ký tự xuống dòng.");
                return;
            }
            if (!UIHelper::isValidPhone(phoneStr)) {
                UIHelper::showWarning(&dialog, "Số điện thoại không hợp lệ", "Số điện thoại phải gồm đúng 10 chữ số và bắt đầu bằng số 0 (Ví dụ: 0912345678).");
                phone->setFocus();
                return;
            }
            if (mainWindow && mainWindow->getDoctors()) {
                auto *dn = mainWindow->getDoctors()->getHeader()->next;
                while (dn && dn->next) {
                    if (dn->info.getID() != docId && dn->info.getState() != -1 && dn->info.getPhone() == UIHelper::ss(phoneStr)) {
                        UIHelper::showWarning(&dialog, "Trùng thông tin", "Số điện thoại này đã được sử dụng bởi một bác sĩ khác.");
                        phone->setFocus();
                        return;
                    }
                    dn = dn->next;
                }
            }
            QString spData = specialtyBox->currentData().toString().trimmed();
            if (spData.isEmpty()) {
                UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng chọn chuyên khoa hợp lệ cho bác sĩ.");
                return;
            }
            std::string normalizedRoomId = libPBL2::storageRoomId(UIHelper::ss(roomStr));
            if (normalizedRoomId.empty()) {
                UIHelper::showWarning(&dialog, "Phòng khám không hợp lệ", "Vui lòng nhập số hoặc tên phòng khám hợp lệ (Ví dụ: Room 101).");
                room->setFocus();
                return;
            }
            QString conflictDoc;
            if (hasCrossSpecialtyRoomConflict(mainWindow->getDoctors(), docId, UIHelper::ss(spData), roomStr, conflictDoc)) {
                UIHelper::showWarning(&dialog, "Trùng phòng khám khác chuyên khoa",
                    QStringLiteral("Phòng \"%1\" hiện đang được sử dụng bởi BS. %2 thuộc chuyên khoa khác. Vui lòng chọn phòng khác.")
                        .arg(UIHelper::formatRoom(roomStr), conflictDoc));
                room->setFocus();
                return;
            }
            if (fee->value() <= 0) {
                UIHelper::showWarning(&dialog, "Phí khám không hợp lệ", "Phí khám tham chiếu phải lớn hơn 0 đ.");
                fee->setFocus();
                return;
            }

            int newState = (state->currentIndex() == 0) ? 1 : 0;
            std::string newSpId = UIHelper::ss(spData);
            bool isDeactivating = (doctor->getState() == 1 && newState == 0);
            bool isChangingSpec = (doctor->getSpId() != newSpId);
            bool needsReassign = isDeactivating || isChangingSpec;
            QList<Appointment*> affected;
            if (needsReassign) {
                affected = AppointmentController::getAffectedAppointmentsForDoctor(mainWindow->getAppointments(), docId);
                if (!affected.isEmpty()) {
                    QString actionDesc = (isDeactivating && isChangingSpec)
                        ? QStringLiteral("chuyển sang trạng thái \"Tạm ngưng\" và đổi chuyên khoa")
                        : (isDeactivating ? QStringLiteral("chuyển sang trạng thái \"Tạm ngưng\"")
                                          : QStringLiteral("đổi chuyên khoa phụ trách"));
                    QString warnTitle = (isDeactivating && isChangingSpec)
                        ? QStringLiteral("Xác nhận tạm ngưng, đổi chuyên khoa & chuyển lịch khám")
                        : (isDeactivating ? QStringLiteral("Xác nhận tạm ngưng & chuyển lịch khám")
                                          : QStringLiteral("Xác nhận đổi chuyên khoa & chuyển lịch khám"));
                    QString warnMsg = QStringLiteral(
                        "Bác sĩ \"%1\" hiện còn %2 lịch hẹn chưa khám thuộc chuyên khoa cũ.\n\n"
                        "Khi %3, hệ thống sẽ tự động điều phối toàn bộ %2 ca này sang bác sĩ phù hợp khác "
                        "(ưu tiên cùng hoặc gần chuyên khoa, giữ nguyên giờ hẹn hoặc tìm khung giờ trống sớm nhất).\n\n"
                        "Bạn có muốn tiếp tục không?"
                    ).arg(nameStr).arg(affected.size()).arg(actionDesc);
                    if (!UIHelper::confirmWarning(&dialog, warnTitle, warnMsg, "Đồng ý chuyển lịch", "Hủy bỏ")) {
                        return;
                    }
                }
            }

            Doctor oldSnapshot = *doctor;
            QList<RescheduledResultItem> resultItems;
            QList<std::string> insertedApptIds;
            QList<std::string> insertedSpIds;
            QList<std::string> modifiedOldApptIds;
            int failedCount = 0;

            try {
                if (needsReassign && !affected.isEmpty()) {
                    auto holidays = AppointmentController::loadHolidays(AppConstants::Paths::HOLIDAYS);
                    AppointmentController::reassignAppointmentsForDoctor(
                        mainWindow->getAppointments(),
                        mainWindow->getSymptomPatients(),
                        mainWindow->getSymptomPatientDetails(),
                        mainWindow->getSymptoms(),
                        mainWindow->getDoctors(),
                        mainWindow->getPatients(),
                        docId,
                        holidays,
                        &resultItems,
                        &failedCount,
                        &insertedApptIds,
                        &insertedSpIds,
                        &modifiedOldApptIds
                    );
                    if (failedCount > 0) {
                        QString failTitle = isDeactivating ? QStringLiteral("Không thể tạm ngưng bác sĩ")
                                                           : QStringLiteral("Không thể đổi chuyên khoa bác sĩ");
                        UIHelper::showWarning(&dialog, failTitle,
                            QStringLiteral("Không thể thực hiện thay đổi vì có %1 lịch hẹn không tìm được bác sĩ thay thế thuộc các chuyên khoa phù hợp.")
                                .arg(failedCount));
                        return;
                    }
                }

                doctor->setfullName(UIHelper::ss(nameStr));
                doctor->setPhone(UIHelper::ss(phoneStr));
                doctor->setAge(age->value());
                doctor->setGender(gender->currentIndex() == 0);
                doctor->setSpId(newSpId);
                doctor->setRoomId(normalizedRoomId);
                doctor->setFee(fee->value());
                doctor->setRating(rating->value());
                doctor->setState(newState);

                SaveScope scope = (needsReassign && !affected.isEmpty()) ? SaveScope::All : SaveScope::Doctor;
                if (!mainWindow->saveAll(scope, SaveAction::Normal)) {
                    if (Doctor *rbDoc = mainWindow->findDoctor(docId)) *rbDoc = oldSnapshot;
                    if (needsReassign && !affected.isEmpty()) {
                        AppointmentController::rollbackInsertedAppointments(
                            mainWindow->getAppointments(),
                            mainWindow->getSymptomPatients(),
                            mainWindow->getSymptomPatientDetails(),
                            insertedApptIds, insertedSpIds, modifiedOldApptIds
                        );
                    }
                    return;
                }
                mainWindow->refreshPage(4);
                mainWindow->refreshPage(0);
                mainWindow->refreshPage(1);
                mainWindow->refreshPage(5);
                if (needsReassign && !affected.isEmpty()) {
                    QString summaryTitle = isDeactivating
                        ? QStringLiteral("Kết quả chuyển bác sĩ khi tạm ngưng")
                        : QStringLiteral("Kết quả chuyển bác sĩ khi đổi chuyên khoa");
                    mainWindow->showRescheduledSummaryDialog(&dialog, resultItems, affected.size(), summaryTitle);
                }
                UIHelper::showSuccess(&dialog, "Thành công", "Cập nhật thông tin bác sĩ thành công.");
                dialog.accept();
            } catch (const std::exception &e) {
                if (Doctor *rbDoc = mainWindow->findDoctor(docId)) *rbDoc = oldSnapshot;
                AppointmentController::rollbackInsertedAppointments(
                    mainWindow->getAppointments(), mainWindow->getSymptomPatients(), mainWindow->getSymptomPatientDetails(),
                    insertedApptIds, insertedSpIds, modifiedOldApptIds);
                UIHelper::showWarning(&dialog, "Lỗi hệ thống", QString("Không thể lưu thông tin bác sĩ: %1").arg(QString::fromUtf8(e.what())));
            }
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

    auto *rightCard = new QFrame;
    rightCard->setObjectName("rightCard");
    rightCard->setStyleSheet("QFrame#rightCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
    auto *rightCardLayout = new QVBoxLayout(rightCard);
    rightCardLayout->setContentsMargins(18, 16, 18, 16);
    rightCardLayout->setSpacing(10);

    auto *filterBar = new QHBoxLayout;
    filterBar->setSpacing(10);

    auto *searchHist = new QLineEdit;
    searchHist->setFixedHeight(38);
    searchHist->setPlaceholderText("🔍  Tìm theo bệnh nhân, số điện thoại, triệu chứng, ngày khám...");
    searchHist->setStyleSheet("background: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 7px 12px; font-size: 12.5px; color: #0f172a;");
    filterBar->addWidget(searchHist, 1);
    rightCardLayout->addLayout(filterBar);

    auto *histTable = new QTableWidget;
    histTable->setColumnCount(5);
    histTable->setHorizontalHeaderLabels({"NGÀY GIỜ KHÁM", "TÊN BỆNH NHÂN", "SỐ ĐIỆN THOẠI", "TRIỆU CHỨNG", "TRẠNG THÁI"});
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
            font-weight: 700;
            border: none;
            border-bottom: 1.5px solid #cbd5e1;
            padding: 8px;
        }
    )");

    rightCardLayout->addWidget(histTable, 1);

    auto *emptyWidget = new QWidget;
    emptyWidget->setStyleSheet("background: transparent;");
    auto *emptyLay = new QVBoxLayout(emptyWidget);
    emptyLay->setAlignment(Qt::AlignCenter);
    emptyLay->setSpacing(8);

    auto *emptyIcon = new QLabel("📋");
    emptyIcon->setStyleSheet("font-size: 36px;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    auto *emptyTitle = new QLabel("Chưa có lịch sử khám bệnh");
    emptyTitle->setStyleSheet("font-size: 14px; font-weight: 700; color: #0f172a;");
    emptyTitle->setAlignment(Qt::AlignCenter);
    auto *emptySub = new QLabel("Bác sĩ này chưa có ca khám nào trong hệ thống hoặc không khớp bộ lọc.");
    emptySub->setStyleSheet("font-size: 12px; color: #64748b;");
    emptySub->setAlignment(Qt::AlignCenter);

    emptyLay->addWidget(emptyIcon);
    emptyLay->addWidget(emptyTitle);
    emptyLay->addWidget(emptySub);
    rightCardLayout->addWidget(emptyWidget, 1);
    emptyWidget->hide();

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
    pageInfoLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #0f172a; padding: 0 6px;");
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

    const int PAGE_SIZE = 20;
    struct HistViewState {
        int sortCol = -1;
        bool sortAsc = false;
        int currentPage = 0;
    };
    auto viewState = std::make_shared<HistViewState>();

    auto reloadHistoryPtr = std::make_shared<std::function<void()>>();
    auto reloadHistory = [reloadHistoryPtr]() { if (*reloadHistoryPtr) (*reloadHistoryPtr)(); };

    *reloadHistoryPtr = [this, docId = id, histTable, emptyWidget, searchHist, countLabel, viewState, prevPageBtn, pageInfoLabel, nextPageBtn, PAGE_SIZE]() {
        if (!mainWindow || !mainWindow->getAppointments()) return;

        QList<Appointment*> allAppts;

        int totalCount = 0;

        auto *n = mainWindow->getAppointments()->getHeader()->next;
        while (n && n->next) {
            if (n->info.getDoctorId() == docId) {
                allAppts.append(&n->info);
                totalCount++;
            }
            n = n->next;
        }

        QString q = searchHist->text().trimmed().toLower();

        QList<Appointment*> filtered;
        for (auto *a : allAppts) {
            if (!q.isEmpty()) {
                QString aid = UIHelper::qs(a->getId()).toLower();
                Patient *p = mainWindow->findPatientForAppointment(*a);
                QString patName = p ? UIHelper::qs(p->getfullName()) : "";
                QString patPhone = p ? UIHelper::qs(p->getPhone()) : "";
                QString sym = mainWindow->getSymptomNamesForAppointment(*a);
                Date ad = a->getDateGet();
                QString dateStr = QString::asprintf("%02d/%02d/%04d %02d:%02d", ad.getDay(), ad.getMonth(), ad.getYear(), ad.getHour(), ad.getMinute());

                QString fullSearch = (aid + " " + patName + " " + patPhone + " " + sym + " " + dateStr).toLower();
                if (!fullSearch.contains(q)) {
                    continue;
                }
            }
            filtered.append(a);
        }

        QDateTime nowDt = QDateTime::currentDateTime();
        int curD = nowDt.date().day(), curM = nowDt.date().month(), curY = nowDt.date().year();
        int curH = nowDt.time().hour(), curMin = nowDt.time().minute(), curSec = nowDt.time().second();

        int col = viewState->sortCol;
        bool asc = viewState->sortAsc;
        libPBL2::qsort(filtered, [this, col, asc, curD, curM, curY, curH, curMin, curSec](const Appointment *a, const Appointment *b) -> bool {
            if (!a || !b) return false;
            int cmp = 0;
            switch (col) {
                case 0:
                    cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
                    break;
                case 1: {
                    Patient *pa = mainWindow ? mainWindow->findPatientForAppointment(*a) : nullptr;
                    Patient *pb = mainWindow ? mainWindow->findPatientForAppointment(*b) : nullptr;
                    cmp = libPBL2::compareVietnameseNames(pa ? pa->getfullName() : "", pb ? pb->getfullName() : "");
                    break;
                }
                case 2: {
                    Patient *pa = mainWindow ? mainWindow->findPatientForAppointment(*a) : nullptr;
                    Patient *pb = mainWindow ? mainWindow->findPatientForAppointment(*b) : nullptr;
                    cmp = QString::compare(pa ? UIHelper::qs(pa->getPhone()) : "", pb ? UIHelper::qs(pb->getPhone()) : "");
                    break;
                }
                case 3: {
                    QString sa = mainWindow ? mainWindow->getSymptomNamesForAppointment(*a) : "";
                    QString sb = mainWindow ? mainWindow->getSymptomNamesForAppointment(*b) : "";
                    cmp = libPBL2::compareVietnameseStrings(UIHelper::ss(sa), UIHelper::ss(sb));
                    break;
                }
                case 4: {
                    int sa = a->computeEffectiveState(curD, curM, curY, curH, curMin, curSec);
                    int sb = b->computeEffectiveState(curD, curM, curY, curH, curMin, curSec);
                    cmp = (sa < sb) ? -1 : ((sa > sb) ? 1 : 0);
                    break;
                }
                default:
                    cmp = libPBL2::compareNatural(a->getId(), b->getId());
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
        if (totalFiltered == totalCount) {
            countLabel->setText(QStringLiteral("Đang hiển thị %1 - %2 / Tổng %3 ca khám").arg(startIndex + 1).arg(endIndex).arg(totalFiltered));
        } else {
            countLabel->setText(QStringLiteral("Đang hiển thị %1 - %2 / %3 kết quả (Tổng %4 ca)").arg(startIndex + 1).arg(endIndex).arg(totalFiltered).arg(totalCount));
        }

        emptyWidget->hide();
        histTable->show();
        histTable->setRowCount(0);

        for (int i = startIndex; i < endIndex; ++i) {
            auto *a = filtered[i];
            int r = histTable->rowCount();
            histTable->insertRow(r);
            histTable->setRowHeight(r, 46);

            Date ad = a->getDateGet();
            Patient *p = mainWindow->findPatientForAppointment(*a);

            auto *dtItem = new QTableWidgetItem(QString::asprintf("%02d/%02d/%04d %02d:%02d", ad.getDay(), ad.getMonth(), ad.getYear(), ad.getHour(), ad.getMinute()));
            dtItem->setTextAlignment(Qt::AlignCenter);
            dtItem->setData(Qt::UserRole, UIHelper::qs(a->getId()));
            histTable->setItem(r, 0, dtItem);

            QString patName = p ? UIHelper::qs(p->getfullName()) : "—";
            auto *patItem = new QTableWidgetItem(patName);
            patItem->setTextAlignment(Qt::AlignCenter);
            histTable->setItem(r, 1, patItem);

            QString patPhone = p ? UIHelper::qs(p->getPhone()) : "—";
            auto *phoneItem = new QTableWidgetItem(patPhone);
            phoneItem->setTextAlignment(Qt::AlignCenter);
            histTable->setItem(r, 2, phoneItem);

            QString symText = mainWindow->getSymptomNamesForAppointment(*a);
            auto *symItem = new QTableWidgetItem(symText);
            symItem->setTextAlignment(Qt::AlignCenter);
            histTable->setItem(r, 3, symItem);

            int st = a->computeEffectiveState(curD, curM, curY, curH, curMin, curSec);
            QString badgeText = "● Chưa khám";
            QString badgeStyle = "QLabel { background: #fffbeb; color: #d97706; border: 1px solid #fde68a; border-radius: 10px; font-size: 11px; font-weight: 700; padding: 3px 8px; }";
            if (st == 1) {
                badgeText = "● Đã khám";
                badgeStyle = "QLabel { background: #ecfdf5; color: #059669; border: 1px solid #a7f3d0; border-radius: 10px; font-size: 11px; font-weight: 700; padding: 3px 8px; }";
            } else if (st == -1) {
                badgeText = "● Trễ hẹn";
                badgeStyle = "QLabel { background: #fff7ed; color: #ea580c; border: 1px solid #ffedd5; border-radius: 10px; font-size: 11px; font-weight: 700; padding: 3px 8px; }";
            } else if (st == -2) {
                badgeText = "● Đã dời lịch";
                badgeStyle = "QLabel { background: #f1f5f9; color: #64748b; border: 1px solid #cbd5e1; border-radius: 10px; font-size: 11px; font-weight: 700; padding: 3px 8px; }";
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

    connect(searchHist, &QLineEdit::textChanged, &dialog, [viewState, reloadHistory]() {
        viewState->currentPage = 0;
        reloadHistory();
    });

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

    connect(histTable, &QTableWidget::cellDoubleClicked, &dialog, [this, histTable, reloadHistory](int row, int) {
        if (row >= 0 && histTable->item(row, 0)) {
            std::string apptId = UIHelper::ss(histTable->item(row, 0)->data(Qt::UserRole).toString());
            if (!apptId.empty() && mainWindow) {
                mainWindow->editAppointmentDialog(apptId);
                reloadHistory();
            }
        }
    });

    reloadHistory();

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, &dialog, [&dialog]() { dialog.setFocus(); });
    dialog.exec();
}
