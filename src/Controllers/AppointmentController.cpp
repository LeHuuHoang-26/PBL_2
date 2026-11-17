#include "AppointmentController.h"
#include "libPBL2.h"
#include "../Models/Doctor.h"
#include "../Models/Patient.h"
#include "../Models/SymptomPatient.h"
#include "../Models/SymptomPatientDetail.h"
#include <QDateTime>
#include <QTime>
#include <fstream>
#include <sstream>
#include <cstdio>

static bool compareAppointmentDates(const Appointment *a, const Appointment *b) {
    if (!a || !b) return a != nullptr;
    Date da = a->getDateGet();
    Date db = b->getDateGet();
    if (da.getYear() != db.getYear()) return da.getYear() < db.getYear();
    if (da.getMonth() != db.getMonth()) return da.getMonth() < db.getMonth();
    if (da.getDay() != db.getDay()) return da.getDay() < db.getDay();
    if (da.getHour() != db.getHour()) return da.getHour() < db.getHour();
    if (da.getMinute() != db.getMinute()) return da.getMinute() < db.getMinute();
    return libPBL2::compareNatural(a->getId(), b->getId()) < 0;
}

void AppointmentController::sortAppointmentsByDate(QList<Appointment*> &list) {
    libPBL2::qsort(list, [](const Appointment *a, const Appointment *b) {
        if (!a || !b) return false;
        return compareAppointmentDates(a, b);
    });
}

QList<Appointment*> AppointmentController::getAppointmentsForDate(const LinkList<Appointment> *appointments, const QDate &targetDate) {
    QList<Appointment*> list;
    if (!appointments || !appointments->getHeader()) return list;

    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() != -2) {
            Date d = n->info.getDateGet();
            if (d.getYear() == targetDate.year() &&
                d.getMonth() == targetDate.month() &&
                d.getDay() == targetDate.day()) {
                list.append(&n->info);
            }
        }
        n = n->next;
    }
    sortAppointmentsByDate(list);
    return list;
}

QList<Appointment*> AppointmentController::getUpcomingAppointments(const LinkList<Appointment> *appointments, const QDate &today) {
    QList<Appointment*> list;
    if (!appointments || !appointments->getHeader()) return list;

    QDateTime now = QDateTime::currentDateTime();

    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() != -2) {
            Date d = n->info.getDateGet();
            QDate appDay(d.getYear(), d.getMonth(), d.getDay());
            QTime appTime(d.getHour(), d.getMinute());
            QDateTime appDt(appDay, appTime);

            // Quá hạn hoặc đã khám thì không hiển thị trong lịch hẹn sắp tới
            if (appDay >= today && appDay <= today.addDays(1)) {
                if (appDt >= now && n->info.getState() == 0) {
                    list.append(&n->info);
                }
            }
        }
        n = n->next;
    }
    sortAppointmentsByDate(list);
    return list;
}

void AppointmentController::getMonthDayCounts(const LinkList<Appointment> *appointments, int year, int month, int dayCounts[32]) {
    for (int i = 0; i < 32; ++i) dayCounts[i] = 0;
    if (!appointments || !appointments->getHeader()) return;

    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() != -2) {
            Date d = n->info.getDateGet();
            if (d.getYear() == year && d.getMonth() == month) {
                int day = d.getDay();
                if (day >= 1 && day <= 31) {
                    dayCounts[day]++;
                }
            }
        }
        n = n->next;
    }
}

std::string AppointmentController::generateNextId(const LinkList<Appointment> *appointments) {
    int maxId = 0;
    if (appointments && appointments->getHeader()) {
        auto *n = appointments->getHeader()->next;
        while (n && n->next) {
            std::string id = n->info.getId();
            if (id.rfind("A_", 0) == 0) {
                try {
                    int val = std::stoi(id.substr(2));
                    if (val > maxId) maxId = val;
                } catch (const std::invalid_argument &) {
                    // Chuỗi ID chứa ký tự không hợp lệ, bỏ qua bản ghi này
                } catch (const std::out_of_range &) {
                    // Giá trị ID vượt ngưỡng số nguyên, bỏ qua bản ghi này
                }
            }
            n = n->next;
        }
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "A_%03d", maxId + 1);
    return std::string(buf);
}

QDate AppointmentController::getLunarNewYear(int year) {
    switch (year) {
        case 2020: return QDate(2020, 1, 25);
        case 2021: return QDate(2021, 2, 12);
        case 2022: return QDate(2022, 2, 1);
        case 2023: return QDate(2023, 1, 22);
        case 2024: return QDate(2024, 2, 10);
        case 2025: return QDate(2025, 1, 29);
        case 2026: return QDate(2026, 2, 17);
        case 2027: return QDate(2027, 2, 6);
        case 2028: return QDate(2028, 1, 26);
        case 2029: return QDate(2029, 2, 13);
        case 2030: return QDate(2030, 2, 3);
        case 2031: return QDate(2031, 1, 23);
        case 2032: return QDate(2032, 2, 11);
        case 2033: return QDate(2033, 1, 31);
        case 2034: return QDate(2034, 2, 19);
        case 2035: return QDate(2035, 2, 8);
        case 2036: return QDate(2036, 1, 28);
        case 2037: return QDate(2037, 2, 15);
        case 2038: return QDate(2038, 2, 4);
        case 2039: return QDate(2039, 1, 24);
        case 2040: return QDate(2040, 2, 12);
        case 2041: return QDate(2041, 2, 1);
        case 2042: return QDate(2042, 1, 22);
        case 2043: return QDate(2043, 2, 10);
        case 2044: return QDate(2044, 1, 30);
        case 2045: return QDate(2045, 2, 17);
        default: break;
    }
    return QDate(year, 2, 1);
}

bool AppointmentController::isTetHoliday(const QDate &date, int holidayDays) {
    if (holidayDays <= 0) holidayDays = 5;
    for (int y = date.year() - 1; y <= date.year() + 1; ++y) {
        QDate mung1 = getLunarNewYear(y);
        if (!mung1.isValid()) continue;
        // 5 ngày nghỉ Tết theo quy chuẩn: từ 30 Tết (mung1 - 1 ngày) đến hết Mùng 4 Tết (mung1 + 3 ngày)
        QDate start = mung1.addDays(-1);
        QDate end = mung1.addDays(-1 + holidayDays - 1);
        if (date >= start && date <= end) {
            return true;
        }
    }
    return false;
}

#include <QMap>

static QMap<std::string, QList<HolidayEntry>> s_holidayCache;

void AppointmentController::clearHolidayCache() {
    s_holidayCache.clear();
}

bool AppointmentController::isHolidayInList(const QDate &date, const QList<HolidayEntry> &list) {
    if (!date.isValid()) return false;
    for (const auto &item : list) {
        std::string value = item.date.toStdString();
        std::string rest = item.note.toStdString();
        libPBL2::trim(value);
        libPBL2::trim(rest);

        // Kiểm tra dịp Tết Nguyên Đán
        std::string lowerVal = value;
        for (char &c : lowerVal) c = std::tolower(static_cast<unsigned char>(c));
        if (lowerVal.rfind("tet", 0) == 0) {
            int days = 5;
            if (!rest.empty()) {
                std::stringstream ss(rest);
                int parsedDays = 0;
                if (ss >> parsedDays && parsedDays > 0) {
                    days = parsedDays;
                }
            }
            if (isTetHoliday(date, days)) {
                return true;
            }
            continue;
        }

        // Ngày nghỉ cố định dạng ngày/tháng (ví dụ: 1/1, 30/4, 1/5, 2/9, 10/3 hoặc 2/9/2026)
        if (value.find('/') != std::string::npos) {
            int day = 0, month = 0, year = 0;
            char slash1 = 0, slash2 = 0;
            std::stringstream parsed(value);
            if (parsed >> day >> slash1 >> month && slash1 == '/') {
                if (parsed >> slash2 >> year && slash2 == '/') {
                    if (day == date.day() && month == date.month() && year == date.year()) {
                        return true;
                    }
                } else {
                    if (day == date.day() && month == date.month()) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool AppointmentController::isHoliday(const QDate &date, const std::string &configPath) {
    const auto list = loadHolidays(configPath);
    return isHolidayInList(date, list);
}

bool AppointmentController::isSlotBooked(const LinkList<Appointment> *appointments, const std::string &doctorId, const Date &targetTime, const std::string &excludeApptId) {
    if (!appointments || !appointments->getHeader() || doctorId.empty() || targetTime.isDefault()) {
        return false;
    }
    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() != -2) {
            if (!excludeApptId.empty() && n->info.getId() == excludeApptId) {
                n = n->next;
                continue;
            }
            if (n->info.getDoctorId() == doctorId && n->info.getState() != -1) {
                Date ad = n->info.getDateGet();
                if (ad.getYear() == targetTime.getYear() &&
                    ad.getMonth() == targetTime.getMonth() &&
                    ad.getDay() == targetTime.getDay() &&
                    ad.getHour() == targetTime.getHour() &&
                    ad.getMinute() == targetTime.getMinute()) {
                    return true;
                }
            }
        }
        n = n->next;
    }
    return false;
}

bool AppointmentController::isPatientSlotBooked(
    const LinkList<Appointment> *appointments,
    const LinkList<SymptomPatient> *symptomPatients,
    const std::string &patientId,
    const Date &targetTime,
    const std::string &excludeApptId
) {
    if (!appointments || !appointments->getHeader() || !symptomPatients || !symptomPatients->getHeader() || patientId.empty() || targetTime.isDefault()) {
        return false;
    }
    auto *an = appointments->getHeader()->next;
    while (an && an->next) {
        if (!an->info.isDefault() && an->info.getState() != -2 && an->info.getState() != -1) {
            if (!excludeApptId.empty() && an->info.getId() == excludeApptId) {
                an = an->next;
                continue;
            }
            Date ad = an->info.getDateGet();
            if (ad.getYear() == targetTime.getYear() &&
                ad.getMonth() == targetTime.getMonth() &&
                ad.getDay() == targetTime.getDay() &&
                ad.getHour() == targetTime.getHour() &&
                ad.getMinute() == targetTime.getMinute()) {

                std::string spId = an->info.getSymptomPatientId();
                auto *spn = symptomPatients->getHeader()->next;
                while (spn && spn->next) {
                    if (spn->info.getSP_id() == spId) {
                        if (spn->info.getP_id() == patientId) {
                            return true;
                        }
                        break;
                    }
                    spn = spn->next;
                }
            }
        }
        an = an->next;
    }
    return false;
}

void AppointmentController::validateBooking(
    const LinkList<Appointment> *appointments,
    const LinkList<SymptomPatient> *symptomPatients,
    const std::string &patientId,
    const std::string &doctorId,
    const QDateTime &targetDateTime,
    const std::string &excludeApptId,
    bool checkHoliday,
    const std::string &configPath
) {
    if (doctorId.empty()) {
        throw ValidationException("Bác sĩ phụ trách không được để trống.");
    }
    if (!targetDateTime.isValid() || targetDateTime <= QDateTime::currentDateTime()) {
        throw ValidationException("Thời gian đặt lịch phải ở tương lai.");
    }
    if (checkHoliday && !configPath.empty() && isHoliday(targetDateTime.date(), configPath)) {
        QString note = getHolidayNote(targetDateTime.date(), configPath);
        std::string msg = "Phòng khám nghỉ lễ vào ngày " + targetDateTime.date().toString("dd/MM/yyyy").toStdString() + " (" + note.toStdString() + "). Vui lòng chọn ngày khác.";
        throw ValidationException(msg);
    }
    Date targetDate = Date::loadFromString(targetDateTime.toString("yyyy-MM-dd HH:mm").toStdString());
    if (isSlotBooked(appointments, doctorId, targetDate, excludeApptId)) {
        throw ValidationException("Bác sĩ đã có lịch hẹn khác vào khung giờ này. Vui lòng chọn khung giờ hoặc bác sĩ khác.");
    }
    if (!patientId.empty() && symptomPatients && isPatientSlotBooked(appointments, symptomPatients, patientId, targetDate, excludeApptId)) {
        throw ValidationException("Bệnh nhân đã có một lịch khám khác vào cùng khung giờ này (" +
                                  targetDateTime.toString("dd/MM/yyyy lúc HH:mm").toStdString() +
                                  "). Bệnh nhân có thể khám nhiều chuyên khoa trong cùng một ngày nhưng không được trùng cùng một khung giờ.");
    }
}

QString AppointmentController::getHolidayNoteFromList(const QDate &date, const QList<HolidayEntry> &list) {
    if (!date.isValid()) return "Nghỉ lễ";
    for (const auto &item : list) {
        std::string value = item.date.toStdString();
        std::string rest = item.note.toStdString();
        libPBL2::trim(value);
        libPBL2::trim(rest);

        std::string lowerVal = value;
        for (char &c : lowerVal) c = std::tolower(static_cast<unsigned char>(c));
        if (lowerVal.rfind("tet", 0) == 0) {
            int days = 5;
            if (!rest.empty()) {
                std::stringstream ss(rest);
                int parsedDays = 0;
                if (ss >> parsedDays && parsedDays > 0) {
                    days = parsedDays;
                }
            }
            if (isTetHoliday(date, days)) {
                return QStringLiteral("Tết Nguyên Đán (Nghỉ %1 ngày)").arg(days);
            }
            continue;
        }

        if (value.find('/') != std::string::npos) {
            int day = 0, month = 0, year = 0;
            char slash1 = 0, slash2 = 0;
            std::stringstream parsed(value);
            if (parsed >> day >> slash1 >> month && slash1 == '/') {
                bool match = false;
                if (parsed >> slash2 >> year && slash2 == '/') {
                    match = (day == date.day() && month == date.month() && year == date.year());
                } else {
                    match = (day == date.day() && month == date.month());
                }
                if (match) {
                    if (!rest.empty()) {
                        return QString::fromUtf8(rest.c_str());
                    }
                    if (day == 1 && month == 1) return "Tết Dương Lịch";
                    if (day == 10 && month == 3) return "Giỗ Tổ Hùng Vương";
                    if (day == 30 && month == 4) return "Ngày Giải phóng miền Nam";
                    if (day == 1 && month == 5) return "Ngày Quốc tế Lao động";
                    if (day == 2 && month == 9) return "Quốc khánh Việt Nam";
                    if (day == 1 && month == 9) return "Nghỉ lễ Quốc khánh";
                    return "Nghỉ lễ";
                }
            }
        }
    }
    return "Nghỉ lễ";
}

QString AppointmentController::getHolidayNote(const QDate &date, const std::string &configPath) {
    const auto list = loadHolidays(configPath);
    return getHolidayNoteFromList(date, list);
}

QList<HolidayEntry> AppointmentController::loadHolidays(const std::string &configPath) {
    if (s_holidayCache.contains(configPath)) {
        return s_holidayCache[configPath];
    }
    QList<HolidayEntry> list;
    std::ifstream file(configPath);
    if (!file.is_open()) return list;
    std::string line;
    while (std::getline(file, line)) {
        libPBL2::trim(line);
        if (line.empty()) continue;
        size_t split = line.find('-');
        std::string date = split == std::string::npos ? line : line.substr(0, split);
        std::string note = split == std::string::npos ? "Nghỉ lễ" : line.substr(split + 1);
        libPBL2::trim(date);
        libPBL2::trim(note);
        list.append({QString::fromUtf8(date.c_str()), QString::fromUtf8(note.c_str())});
    }
    s_holidayCache[configPath] = list;
    return list;
}

bool AppointmentController::saveHolidays(const std::string &configPath, const QList<HolidayEntry> &list) {
    s_holidayCache[configPath] = list;
    std::string tempPath = configPath + ".tmp";
    std::ofstream file(tempPath);
    if (!file.is_open()) return false;
    try {
        for (const auto &item : list) {
            file << item.date.toUtf8().toStdString() << " - " << item.note.toUtf8().toStdString() << "\n";
        }
        file.close();
        libPBL2::safeAtomicCommit(tempPath, configPath);
        return true;
    } catch (...) {
        file.close();
        std::remove(tempPath.c_str());
        return false;
    }
}

QList<Appointment*> AppointmentController::getAffectedAppointmentsForHoliday(
    const LinkList<Appointment> *appointments,
    const QList<HolidayEntry> &currentHolidays,
    const QList<HolidayEntry> &prospectiveHolidays
) {
    QList<Appointment*> affectedList;
    if (!appointments || !appointments->getHeader()) return affectedList;

    QDate today = QDate::currentDate();
    QTime nowTime = QTime::currentTime();

    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() == 0) {
            Date d = n->info.getDateGet();
            QDate apptDate(d.getYear(), d.getMonth(), d.getDay());

            // Chỉ xét các ca hẹn trong tương lai hoặc hôm nay nhưng chưa quá giờ hẹn
            bool isFutureOrToday = false;
            if (apptDate > today) {
                isFutureOrToday = true;
            } else if (apptDate == today) {
                QTime apptTime(d.getHour(), d.getMinute());
                if (apptTime > nowTime) {
                    isFutureOrToday = true;
                }
            }

            if (isFutureOrToday) {
                bool wasHoliday = isHolidayInList(apptDate, currentHolidays);
                bool willBeHoliday = isHolidayInList(apptDate, prospectiveHolidays);

                if (!wasHoliday && willBeHoliday) {
                    affectedList.append(&n->info);
                }
            }
        }
        n = n->next;
    }

    sortAppointmentsByDate(affectedList);
    return affectedList;
}

int AppointmentController::rescheduleAppointmentsLinear(
    LinkList<Appointment> *appointments,
    LinkList<SymptomPatient> *symptomPatients,
    LinkList<SymptomPatientDetail> *symptomPatientDetails,
    const LinkList<Doctor> *doctors,
    const LinkList<Patient> *patients,
    const QList<Appointment*> &affectedAppointments,
    const QList<HolidayEntry> &prospectiveHolidays,
    QString &outSummaryReport,
    QList<RescheduledResultItem> *outResultItems
) {
    if (!appointments || affectedAppointments.isEmpty()) {
        outSummaryReport = "Không có cuộc hẹn nào cần dời.";
        return 0;
    }

    const QStringList fixedSlots = {
        "07:00", "07:45", "08:30", "09:15", "10:00", "10:45",
        "13:00", "13:45", "14:30", "15:15", "16:00"
    };

    int successCount = 0;
    QStringList detailLogs;

    for (Appointment *oldAppt : affectedAppointments) {
        if (!oldAppt) continue;
        std::string docId = oldAppt->getDoctorId();
        Date oldD = oldAppt->getDateGet();
        QDate oldQDate(oldD.getYear(), oldD.getMonth(), oldD.getDay());

        // Lấy thông tin bệnh nhân và chuyên khoa từ phiếu SymptomPatient cũ trước để kiểm tra trùng giờ khám
        std::string oldSpId = oldAppt->getSymptomPatientId();
        std::string patientId = "";
        std::string spSpec = "";
        if (symptomPatients && symptomPatients->getHeader()) {
            auto *spn = symptomPatients->getHeader()->next;
            while (spn && spn->next) {
                if (spn->info.getSP_id() == oldSpId) {
                    patientId = spn->info.getP_id();
                    spSpec = spn->info.getSp_id();
                    break;
                }
                spn = spn->next;
            }
        }

        // Bắt đầu tìm kiếm từ ngày tiếp theo (> ngày bị khóa)
        QDate curDate = oldQDate.addDays(1);
        bool foundSlot = false;
        Date targetCandidateDate;
        int maxSearchDays = 365;
        int searchDays = 0;

        while (!foundSlot && searchDays < maxSearchDays) {
            searchDays++;
            // Bỏ qua nếu là ngày nghỉ lễ
            if (isHolidayInList(curDate, prospectiveHolidays)) {
                curDate = curDate.addDays(1);
                continue;
            }

            // Quét tuyến tính qua 11 slot trong ngày curDate để truy vết lịch ca khám của bác sĩ đó
            for (const QString &slotStr : fixedSlots) {
                QTime slotTime = QTime::fromString(slotStr, "HH:mm");
                Date candidateTime(curDate.day(), curDate.month(), curDate.year(),
                                   slotTime.hour(), slotTime.minute());

                // Nếu ca này của bác sĩ chưa bị đặt VÀ bệnh nhân chưa có lịch khám vào giờ này: chèn vào!
                bool docFree = !isSlotBooked(appointments, docId, candidateTime);
                bool patientFree = patientId.empty() || !isPatientSlotBooked(appointments, symptomPatients, patientId, candidateTime, oldAppt->getId());
                if (docFree && patientFree) {
                    targetCandidateDate = candidateTime;
                    foundSlot = true;
                    break;
                }
            }

            if (!foundSlot) {
                curDate = curDate.addDays(1);
            }
        }

        if (foundSlot) {
            // 1. Đổi trạng thái lịch cũ thành -2 (Đã dời lịch)
            oldAppt->setState(-2);

            // 2. Tạo phiếu SymptomPatient mới
            std::string newSpId = SymptomPatient::generateNextId(symptomPatients);
            if (symptomPatients) {
                symptomPatients->insert(SymptomPatient(newSpId, patientId, spSpec, Date::now()));
            }

            // 4. Sao chép chi tiết triệu chứng sang phiếu mới
            if (symptomPatientDetails && symptomPatientDetails->getHeader()) {
                auto *dn = symptomPatientDetails->getHeader()->next;
                while (dn && dn->next) {
                    if (dn->info.getSP_id() == oldSpId) {
                        symptomPatientDetails->insert(SymptomPatientDetail(newSpId, dn->info.getS_id()));
                    }
                    dn = dn->next;
                }
            }

            // 5. Tạo Appointment mới và chèn ngay vào danh sách appointments
            std::string newApptId = generateNextId(appointments);
            appointments->insert(Appointment(newApptId, newSpId, docId, targetCandidateDate, 0));

            successCount++;

            // 6. Ghi lại thông tin để báo cáo
            QString patName = QString::fromStdString(patientId);
            if (patients && patients->getHeader()) {
                auto *pn = patients->getHeader()->next;
                while (pn && pn->next) {
                    if (pn->info.getID() == patientId) {
                        patName = QString::fromUtf8(pn->info.getfullName().c_str());
                        break;
                    }
                    pn = pn->next;
                }
            }

            QString docName = QString::fromStdString(docId);
            if (doctors && doctors->getHeader()) {
                auto *dn = doctors->getHeader()->next;
                while (dn && dn->next) {
                    if (dn->info.getID() == docId) {
                        docName = QString::fromUtf8(dn->info.getfullName().c_str());
                        break;
                    }
                    dn = dn->next;
                }
            }

            char oldBuf[32], newBuf[32];
            std::snprintf(oldBuf, sizeof(oldBuf), "%02d/%02d/%04d %02d:%02d", 
                          oldD.getDay(), oldD.getMonth(), oldD.getYear(), oldD.getHour(), oldD.getMinute());
            std::snprintf(newBuf, sizeof(newBuf), "%02d/%02d/%04d %02d:%02d", 
                          targetCandidateDate.getDay(), targetCandidateDate.getMonth(), targetCandidateDate.getYear(), 
                          targetCandidateDate.getHour(), targetCandidateDate.getMinute());

            detailLogs.append(QString("• Bệnh nhân: %1 (Mã hẹn cũ: %2)\n  Bác sĩ: %3\n  Lịch cũ: %4 ➔ Lịch mới: %5\n")
                                  .arg(patName, QString::fromStdString(oldAppt->getId()), docName, oldBuf, newBuf));

            if (outResultItems) {
                RescheduledResultItem rItem;
                rItem.oldApptId = QString::fromStdString(oldAppt->getId());
                rItem.newApptId = QString::fromStdString(newApptId);
                rItem.patientName = patName;
                rItem.doctorName = docName;
                rItem.oldDateTime = QString::fromUtf8(oldBuf);
                rItem.newDateTime = QString::fromUtf8(newBuf);
                outResultItems->append(rItem);
            }
        }
    }

    outSummaryReport = QString("Đã dời thành công %1/%2 cuộc hẹn sang các ca khám kế tiếp:\n\n%3")
                           .arg(successCount)
                           .arg(affectedAppointments.size())
                           .arg(detailLogs.join("\n"));
    return successCount;
}

int AppointmentController::syncExpiredAppointments(LinkList<Appointment> *appointments) {
    if (!appointments || !appointments->getHeader()) return 0;
    Date now = Date::now();
    int changed = 0;
    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() == 0) {
            int effState = n->info.computeEffectiveState(now.getDay(), now.getMonth(), now.getYear(), now.getHour(), now.getMinute());
            if (effState == -1) {
                n->info.setState(-1);
                changed++;
            }
        }
        n = n->next;
    }
    return changed;
}
