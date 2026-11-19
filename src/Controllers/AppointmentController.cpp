#include "AppointmentController.h"
#include "DoctorController.h"
#include "PatientController.h"
#include "libPBL2.h"
#include "../Models/Doctor.h"
#include "../Models/Patient.h"
#include "../Models/Symptom.h"
#include "../Models/SymptomPatient.h"
#include "../Models/SymptomPatientDetail.h"
#include <QDateTime>
#include <QMap>
#include <QTime>
#include <fstream>
#include <sstream>
#include <cstdio>

static bool compareAppointmentDates(const Appointment *a, const Appointment *b) {
    if (!a || !b) return (a != nullptr) && (b == nullptr);
    int cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
    if (cmp != 0) return cmp < 0;
    return libPBL2::compareNatural(a->getId(), b->getId()) < 0;
}

void AppointmentController::sortAppointmentsByDate(QList<Appointment*> &list) {
    libPBL2::qsort(list, [](const Appointment *a, const Appointment *b) {
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

            if (appDay >= today && appDay <= today.addDays(1)) {
                if (appDt.addSecs(20 * 60) >= now && n->info.getState() == 0) {
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
    return libPBL2::generateNextId(appointments, "A_", [](const Appointment &a) { return a.getId(); });
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
        QDate start = mung1.addDays(-1);
        QDate end = mung1.addDays(-1 + holidayDays - 1);
        if (date >= start && date <= end) {
            return true;
        }
    }
    return false;
}

static QMap<std::string, QList<HolidayEntry>> s_holidayCache;

void AppointmentController::clearHolidayCache() {
    s_holidayCache.clear();
}

static bool matchHolidayInList(const QDate &date, const QList<HolidayEntry> &list, QString *outNote = nullptr) {
    if (!date.isValid()) return false;
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
            if (AppointmentController::isTetHoliday(date, days)) {
                if (outNote) *outNote = QStringLiteral("Tết Nguyên Đán (Nghỉ %1 ngày)").arg(days);
                return true;
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
                    if (outNote) {
                        if (!rest.empty()) *outNote = QString::fromUtf8(rest.c_str());
                        else if (day == 1 && month == 1) *outNote = "Tết Dương Lịch";
                        else if (day == 10 && month == 3) *outNote = "Giỗ Tổ Hùng Vương";
                        else if (day == 30 && month == 4) *outNote = "Ngày Giải phóng miền Nam";
                        else if (day == 1 && month == 5) *outNote = "Ngày Quốc tế Lao động";
                        else if (day == 2 && month == 9) *outNote = "Quốc khánh Việt Nam";
                        else if (day == 1 && month == 9) *outNote = "Nghỉ lễ Quốc khánh";
                        else *outNote = "Nghỉ lễ";
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

bool AppointmentController::isHolidayInList(const QDate &date, const QList<HolidayEntry> &list) {
    return matchHolidayInList(date, list, nullptr);
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
        if (!n->info.isDefault() && n->info.getState() != -2 && n->info.getState() != -1) {
            if (!excludeApptId.empty() && n->info.getId() == excludeApptId) {
                n = n->next;
                continue;
            }
            if (n->info.getDoctorId() == doctorId && libPBL2::compareDates(n->info.getDateGet(), targetTime) == 0) {
                return true;
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
            if (libPBL2::compareDates(an->info.getDateGet(), targetTime) == 0) {
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
    const std::string &configPath,
    const LinkList<Doctor> *doctors,
    const LinkList<Patient> *patients
) {
    if (doctorId.empty()) {
        throw ValidationException("Bác sĩ phụ trách không được để trống.");
    }
    if (doctors) {
        Doctor *doc = DoctorController::findById(const_cast<LinkList<Doctor>*>(doctors), doctorId);
        if (!doc || doc->getState() != 1) {
            throw ValidationException("Bác sĩ được chọn hiện không còn công tác hoặc đã bị xóa khỏi hệ thống.");
        }
    }
    if (patients && !patientId.empty()) {
        Patient *pat = PatientController::findById(const_cast<LinkList<Patient>*>(patients), patientId);
        if (!pat || pat->getState() == -1) {
            throw ValidationException("Hồ sơ bệnh nhân được chọn không tồn tại hoặc đã ngừng hoạt động.");
        }
    }
    if (!targetDateTime.isValid() || targetDateTime <= QDateTime::currentDateTime()) {
        throw ValidationException("Thời gian đặt lịch phải ở tương lai.");
    }
    if (checkHoliday && !configPath.empty()) {
        QString note;
        const auto holidays = loadHolidays(configPath);
        if (matchHolidayInList(targetDateTime.date(), holidays, &note)) {
            std::string msg = "Phòng khám nghỉ lễ vào ngày " + targetDateTime.date().toString("dd/MM/yyyy").toStdString() + " (" + note.toStdString() + "). Vui lòng chọn ngày khác.";
            throw ValidationException(msg);
        }
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
    QString note;
    if (matchHolidayInList(date, list, &note)) {
        return note;
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
        std::string note = split == std::string::npos ? "" : line.substr(split + 1);
        libPBL2::trim(date);
        libPBL2::trim(note);
        list.append({QString::fromUtf8(date.c_str()), QString::fromUtf8(note.c_str())});
    }
    s_holidayCache[configPath] = list;
    return list;
}

bool AppointmentController::saveHolidays(const std::string &configPath, const QList<HolidayEntry> &list) {
    std::string tempPath = configPath + ".tmp";
    std::ofstream file(tempPath);
    if (!file.is_open()) return false;
    auto sanitize = [](const QString &s) {
        QString cleaned = s;
        cleaned.replace('\r', ' ');
        cleaned.replace('\n', ' ');
        return cleaned.trimmed().toUtf8().toStdString();
    };
    try {
        for (const auto &item : list) {
            std::string dStr = sanitize(item.date);
            std::string nStr = sanitize(item.note);
            if (dStr.empty()) continue;
            if (nStr.empty()) {
                file << dStr << "\n";
            } else {
                file << dStr << " - " << nStr << "\n";
            }
        }
        file.flush();
        bool writeOk = file.good();
        file.close();
        if (!writeOk || file.fail()) {
            std::remove(tempPath.c_str());
            return false;
        }
        libPBL2::safeAtomicCommit(tempPath, configPath);
        s_holidayCache[configPath] = list;
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

namespace {

struct SlotHM { int hour; int minute; };
constexpr SlotHM kFixedSlots[] = {
    {7, 0}, {7, 45}, {8, 30}, {9, 15}, {10, 0}, {10, 45},
    {13, 0}, {13, 45}, {14, 30}, {15, 15}, {16, 0}
};

struct SpecScoreEntry {
    std::string spId;
    int score = 0;
};

QList<std::string> buildRankedSpecialtiesForAppointment(
    const std::string &oldSpId,
    const std::string &fallbackSpId,
    const LinkList<SymptomPatientDetail> *symptomPatientDetails,
    const LinkList<Symptom> *symptoms
) {
    QList<SpecScoreEntry> scored;
    if (!oldSpId.empty() && symptomPatientDetails && symptomPatientDetails->getHeader() && symptoms && symptoms->getHeader()) {
        auto *dn = symptomPatientDetails->getHeader()->next;
        while (dn && dn->next) {
            if (dn->info.getSP_id() == oldSpId) {
                std::string sid = dn->info.getS_id();
                auto *sn = symptoms->getHeader()->next;
                while (sn && sn->next) {
                    if (sn->info.getS_id() == sid) {
                        std::string sp = sn->info.getSp_id();
                        int sc = sn->info.getSScore();
                        if (!sp.empty() && sc > 0) {
                            int idx = -1;
                            for (int i = 0; i < scored.size(); ++i) {
                                if (scored[i].spId == sp) { idx = i; break; }
                            }
                            if (idx >= 0) scored[idx].score += sc;
                            else scored.append({sp, sc});
                        }
                        break;
                    }
                    sn = sn->next;
                }
            }
            dn = dn->next;
        }
    }

    if (scored.size() > 1) {
        libPBL2::qsort(scored, [&fallbackSpId](const SpecScoreEntry &a, const SpecScoreEntry &b) {
            if (a.score != b.score) return a.score > b.score;
            bool aOrig = (a.spId == fallbackSpId);
            bool bOrig = (b.spId == fallbackSpId);
            if (aOrig != bOrig) return aOrig;
            return a.spId < b.spId;
        });
    }

    QList<std::string> ranked;
    for (const auto &entry : scored) {
        ranked.append(entry.spId);
    }
    if (!fallbackSpId.empty() && !ranked.contains(fallbackSpId)) {
        ranked.append(fallbackSpId);
    }
    return ranked;
}

bool findReplacementDoctorAndSlot(
    const LinkList<Appointment> *appointments,
    const LinkList<SymptomPatient> *symptomPatients,
    const LinkList<Doctor> *doctors,
    const QList<std::string> &rankedSpecialties,
    const std::string &excludeDoctorId,
    const std::string &patientId,
    const std::string &excludeApptId,
    const Date &oldDate,
    bool allowSameDateTime,
    const QDate &linearStartDate,
    const QList<HolidayEntry> &holidays,
    Doctor *&outDoctor,
    Date &outDate
) {
    outDoctor = nullptr;
    if (!doctors || !doctors->getHeader() || rankedSpecialties.isEmpty()) return false;

    QList<QList<Doctor*>> docsBySpec;
    bool hasAnyCandidate = false;
    for (const auto &spId : rankedSpecialties) {
        QList<Doctor*> specDocs;
        auto *dn = doctors->getHeader()->next;
        while (dn && dn->next) {
            if (!dn->info.isDefault() && dn->info.getState() == 1 &&
                dn->info.getID() != excludeDoctorId && dn->info.getSpId() == spId) {
                specDocs.append(&dn->info);
            }
            dn = dn->next;
        }
        if (specDocs.size() > 1) {
            libPBL2::qsort(specDocs, [](const Doctor *a, const Doctor *b) {
                if (a->getRating() != b->getRating()) return a->getRating() > b->getRating();
                return a->getID() < b->getID();
            });
        }
        if (!specDocs.isEmpty()) hasAnyCandidate = true;
        docsBySpec.append(specDocs);
    }
    if (!hasAnyCandidate) return false;

    QDate oldQDate(oldDate.getYear(), oldDate.getMonth(), oldDate.getDay());
    if (allowSameDateTime && oldQDate.isValid() && !AppointmentController::isHolidayInList(oldQDate, holidays)) {
        for (const auto &specDocs : docsBySpec) {
            for (Doctor *cand : specDocs) {
                if (!AppointmentController::isSlotBooked(appointments, cand->getID(), oldDate, excludeApptId)) {
                    outDoctor = cand;
                    outDate = oldDate;
                    return true;
                }
            }
        }
    }

    QDateTime now = QDateTime::currentDateTime();
    QDate curDate = linearStartDate.isValid() ? linearStartDate : QDate::currentDate();
    if (curDate < now.date()) curDate = now.date();

    for (int searchDays = 0; searchDays < 365; ++searchDays, curDate = curDate.addDays(1)) {
        if (AppointmentController::isHolidayInList(curDate, holidays)) {
            continue;
        }
        for (const auto &slot : kFixedSlots) {
            QDateTime slotDT(curDate, QTime(slot.hour, slot.minute));
            if (slotDT <= now) continue;

            Date candidateDate(curDate.day(), curDate.month(), curDate.year(), slot.hour, slot.minute);
            if (!patientId.empty() && AppointmentController::isPatientSlotBooked(appointments, symptomPatients, patientId, candidateDate, excludeApptId)) {
                continue;
            }
            for (const auto &specDocs : docsBySpec) {
                for (Doctor *cand : specDocs) {
                    if (!AppointmentController::isSlotBooked(appointments, cand->getID(), candidateDate, excludeApptId)) {
                        outDoctor = cand;
                        outDate = candidateDate;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

}

void AppointmentController::rollbackInsertedAppointments(
    LinkList<Appointment> *appointments,
    LinkList<SymptomPatient> *symptomPatients,
    LinkList<SymptomPatientDetail> *symptomPatientDetails,
    const QList<std::string> &insertedApptIds,
    const QList<std::string> &insertedSpIds,
    const QList<Appointment*> &modifiedOldAppts
) {
    for (Appointment *oldA : modifiedOldAppts) {
        if (oldA) oldA->setState(0);
    }
    if (appointments && appointments->getHeader() && !insertedApptIds.isEmpty()) {
        auto *n = appointments->getHeader()->next;
        while (n && n->next) {
            auto *next = n->next;
            if (insertedApptIds.contains(n->info.getId())) {
                appointments->remove(n);
            }
            n = next;
        }
    }
    if (symptomPatientDetails && symptomPatientDetails->getHeader() && !insertedSpIds.isEmpty()) {
        auto *n = symptomPatientDetails->getHeader()->next;
        while (n && n->next) {
            auto *next = n->next;
            if (insertedSpIds.contains(n->info.getSP_id())) {
                symptomPatientDetails->remove(n);
            }
            n = next;
        }
    }
    if (symptomPatients && symptomPatients->getHeader() && !insertedSpIds.isEmpty()) {
        auto *n = symptomPatients->getHeader()->next;
        while (n && n->next) {
            auto *next = n->next;
            if (insertedSpIds.contains(n->info.getSP_id())) {
                symptomPatients->remove(n);
            }
            n = next;
        }
    }
}

QList<Appointment*> AppointmentController::getAffectedAppointmentsForDoctor(
    const LinkList<Appointment> *appointments,
    const std::string &doctorId
) {
    QList<Appointment*> affected;
    if (!appointments || !appointments->getHeader() || doctorId.empty()) return affected;

    Date now = Date::now();
    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getDoctorId() == doctorId) {
            int eff = n->info.computeEffectiveState(now.getDay(), now.getMonth(), now.getYear(), now.getHour(), now.getMinute());
            if (eff == 0) {
                affected.append(&n->info);
            }
        }
        n = n->next;
    }
    sortAppointmentsByDate(affected);
    return affected;
}

int AppointmentController::reassignAppointmentsForDoctor(
    LinkList<Appointment> *appointments,
    LinkList<SymptomPatient> *symptomPatients,
    LinkList<SymptomPatientDetail> *symptomPatientDetails,
    const LinkList<Symptom> *symptoms,
    const LinkList<Doctor> *doctors,
    const LinkList<Patient> *patients,
    const std::string &oldDoctorId,
    const QList<HolidayEntry> &holidays,
    QList<RescheduledResultItem> *outResultItems,
    int *outFailedCount,
    QList<std::string> *outInsertedApptIds,
    QList<std::string> *outInsertedSpIds,
    QList<Appointment*> *outModifiedOldAppts
) {
    if (outFailedCount) *outFailedCount = 0;
    if (!appointments || !symptomPatients || !doctors || oldDoctorId.empty()) return 0;

    QList<Appointment*> affected = getAffectedAppointmentsForDoctor(appointments, oldDoctorId);
    if (affected.isEmpty()) return 0;

    Doctor *oldDoc = DoctorController::findById(const_cast<LinkList<Doctor>*>(doctors), oldDoctorId);
    std::string oldDocSpec = oldDoc ? oldDoc->getSpId() : "";
    QString oldDocName = oldDoc ? QString::fromUtf8(oldDoc->getfullName().c_str()) : QString::fromStdString(oldDoctorId);

    QList<std::string> insertedApptIds;
    QList<std::string> insertedSpIds;
    QList<Appointment*> modifiedOldAppts;
    QList<RescheduledResultItem> localResults;
    int failedCount = 0;

    for (Appointment *oldAppt : affected) {
        if (!oldAppt) continue;
        std::string oldSpId = oldAppt->getSymptomPatientId();
        std::string patientId;
        std::string spSpec = oldDocSpec;
        if (symptomPatients->getHeader()) {
            auto *spn = symptomPatients->getHeader()->next;
            while (spn && spn->next) {
                if (spn->info.getSP_id() == oldSpId) {
                    patientId = spn->info.getP_id();
                    if (spSpec.empty()) spSpec = spn->info.getSp_id();
                    break;
                }
                spn = spn->next;
            }
        }
        if (patientId.empty()) {
            failedCount++;
            continue;
        }

        Date oldD = oldAppt->getDateGet();
        QDate oldQDate(oldD.getYear(), oldD.getMonth(), oldD.getDay());
        QList<std::string> rankedSpecs = buildRankedSpecialtiesForAppointment(oldSpId, spSpec, symptomPatientDetails, symptoms);

        Doctor *newDoc = nullptr;
        Date targetDate;
        bool found = findReplacementDoctorAndSlot(
            appointments, symptomPatients, doctors,
            rankedSpecs, oldDoctorId, patientId, oldAppt->getId(),
            oldD, true, oldQDate, holidays,
            newDoc, targetDate
        );

        if (!found || !newDoc) {
            failedCount++;
            continue;
        }

        oldAppt->setState(-2);
        modifiedOldAppts.append(oldAppt);

        std::string newSpId = SymptomPatient::generateNextId(symptomPatients);
        symptomPatients->insert(SymptomPatient(newSpId, patientId, newDoc->getSpId(), Date::now()));
        insertedSpIds.append(newSpId);

        if (symptomPatientDetails && symptomPatientDetails->getHeader()) {
            QList<std::string> symptomIds;
            auto *dn = symptomPatientDetails->getHeader()->next;
            while (dn && dn->next) {
                if (dn->info.getSP_id() == oldSpId) {
                    symptomIds.append(dn->info.getS_id());
                }
                dn = dn->next;
            }
            for (const auto &sid : symptomIds) {
                symptomPatientDetails->insert(SymptomPatientDetail(newSpId, sid));
            }
        }

        std::string newApptId = generateNextId(appointments);
        appointments->insert(Appointment(newApptId, newSpId, newDoc->getID(), targetDate, 0, -1.0));
        insertedApptIds.append(newApptId);

        Patient *pat = PatientController::findById(const_cast<LinkList<Patient>*>(patients), patientId);
        QString patName = pat ? QString::fromUtf8(pat->getfullName().c_str()) : QString::fromStdString(patientId);
        QString newDocName = QString::fromUtf8(newDoc->getfullName().c_str());

        char oldBuf[32], newBuf[32];
        std::snprintf(oldBuf, sizeof(oldBuf), "%02d/%02d/%04d %02d:%02d",
                      oldD.getDay(), oldD.getMonth(), oldD.getYear(), oldD.getHour(), oldD.getMinute());
        std::snprintf(newBuf, sizeof(newBuf), "%02d/%02d/%04d %02d:%02d",
                      targetDate.getDay(), targetDate.getMonth(), targetDate.getYear(),
                      targetDate.getHour(), targetDate.getMinute());

        RescheduledResultItem rItem;
        rItem.patientName = patName;
        rItem.oldDoctorName = oldDocName;
        rItem.doctorName = newDocName;
        rItem.oldDateTime = QString::fromUtf8(oldBuf);
        rItem.newDateTime = QString::fromUtf8(newBuf);
        localResults.append(rItem);
    }

    if (failedCount > 0) {
        rollbackInsertedAppointments(appointments, symptomPatients, symptomPatientDetails,
                                     insertedApptIds, insertedSpIds, modifiedOldAppts);
        if (outFailedCount) *outFailedCount = failedCount;
        return 0;
    }

    if (outResultItems) *outResultItems = localResults;
    if (outInsertedApptIds) *outInsertedApptIds = insertedApptIds;
    if (outInsertedSpIds) *outInsertedSpIds = insertedSpIds;
    if (outModifiedOldAppts) *outModifiedOldAppts = modifiedOldAppts;
    return localResults.size();
}

int AppointmentController::rescheduleAppointmentsLinear(
    LinkList<Appointment> *appointments,
    LinkList<SymptomPatient> *symptomPatients,
    LinkList<SymptomPatientDetail> *symptomPatientDetails,
    const LinkList<Doctor> *doctors,
    const LinkList<Patient> *patients,
    const QList<Appointment*> &affectedAppointments,
    const QList<HolidayEntry> &prospectiveHolidays,
    QList<RescheduledResultItem> *outResultItems,
    const LinkList<Symptom> *symptoms,
    QList<std::string> *outInsertedApptIds,
    QList<std::string> *outInsertedSpIds,
    QList<Appointment*> *outModifiedOldAppts
) {
    if (!appointments || affectedAppointments.isEmpty()) {
        return 0;
    }

    int successCount = 0;

    for (Appointment *oldAppt : affectedAppointments) {
        if (!oldAppt) continue;
        std::string docId = oldAppt->getDoctorId();
        Date oldD = oldAppt->getDateGet();
        QDate oldQDate(oldD.getYear(), oldD.getMonth(), oldD.getDay());
        if (!oldQDate.isValid()) continue;

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

        Doctor *origDoc = DoctorController::findById(const_cast<LinkList<Doctor>*>(doctors), docId);
        bool origDocActive = (origDoc && origDoc->getState() == 1);
        std::string targetDocId = docId;
        std::string targetSpec = spSpec;
        bool foundSlot = false;
        Date targetCandidateDate;

        if (origDocActive) {
            QDate curDate = oldQDate.addDays(1);
            int searchDays = 0;
            while (!foundSlot && searchDays < 365) {
                searchDays++;
                if (isHolidayInList(curDate, prospectiveHolidays)) {
                    curDate = curDate.addDays(1);
                    continue;
                }
                for (const auto &slot : kFixedSlots) {
                    Date candidateTime(curDate.day(), curDate.month(), curDate.year(),
                                       slot.hour, slot.minute);
                    bool docFree = !isSlotBooked(appointments, docId, candidateTime);
                    bool patientFree = patientId.empty() || !isPatientSlotBooked(appointments, symptomPatients, patientId, candidateTime, oldAppt->getId());
                    if (docFree && patientFree) {
                        targetCandidateDate = candidateTime;
                        foundSlot = true;
                        break;
                    }
                }
                if (!foundSlot) curDate = curDate.addDays(1);
            }
        } else {
            QList<std::string> rankedSpecs = buildRankedSpecialtiesForAppointment(
                oldSpId, origDoc ? origDoc->getSpId() : spSpec, symptomPatientDetails, symptoms);
            Doctor *replacement = nullptr;
            if (findReplacementDoctorAndSlot(
                    appointments, symptomPatients, doctors,
                    rankedSpecs, docId, patientId, oldAppt->getId(),
                    oldD, false, oldQDate.addDays(1), prospectiveHolidays,
                    replacement, targetCandidateDate) && replacement) {
                targetDocId = replacement->getID();
                targetSpec = replacement->getSpId();
                foundSlot = true;
            }
        }

        if (foundSlot) {
            if (!symptomPatients || patientId.empty()) {
                continue;
            }
            oldAppt->setState(-2);
            if (outModifiedOldAppts) outModifiedOldAppts->append(oldAppt);

            std::string newSpId = SymptomPatient::generateNextId(symptomPatients);
            symptomPatients->insert(SymptomPatient(newSpId, patientId, targetSpec, Date::now()));
            if (outInsertedSpIds) outInsertedSpIds->append(newSpId);

            if (symptomPatientDetails && symptomPatientDetails->getHeader()) {
                QList<std::string> symptomIds;
                auto *dn = symptomPatientDetails->getHeader()->next;
                while (dn && dn->next) {
                    if (dn->info.getSP_id() == oldSpId) {
                        symptomIds.append(dn->info.getS_id());
                    }
                    dn = dn->next;
                }
                for (const auto &sid : symptomIds) {
                    symptomPatientDetails->insert(SymptomPatientDetail(newSpId, sid));
                }
            }

            std::string newApptId = generateNextId(appointments);
            appointments->insert(Appointment(newApptId, newSpId, targetDocId, targetCandidateDate, 0, -1.0));
            if (outInsertedApptIds) outInsertedApptIds->append(newApptId);
            successCount++;

            Patient *pat = PatientController::findById(const_cast<LinkList<Patient>*>(patients), patientId);
            QString patName = pat ? QString::fromUtf8(pat->getfullName().c_str()) : QString::fromStdString(patientId);

            Doctor *newDoc = DoctorController::findById(const_cast<LinkList<Doctor>*>(doctors), targetDocId);
            QString docName = newDoc ? QString::fromUtf8(newDoc->getfullName().c_str()) : QString::fromStdString(targetDocId);
            QString oldDocNameStr = (targetDocId != docId && origDoc) ? QString::fromUtf8(origDoc->getfullName().c_str()) : QString();

            char oldBuf[32], newBuf[32];
            std::snprintf(oldBuf, sizeof(oldBuf), "%02d/%02d/%04d %02d:%02d",
                          oldD.getDay(), oldD.getMonth(), oldD.getYear(), oldD.getHour(), oldD.getMinute());
            std::snprintf(newBuf, sizeof(newBuf), "%02d/%02d/%04d %02d:%02d",
                          targetCandidateDate.getDay(), targetCandidateDate.getMonth(), targetCandidateDate.getYear(),
                          targetCandidateDate.getHour(), targetCandidateDate.getMinute());

            if (outResultItems) {
                RescheduledResultItem rItem;
                rItem.patientName = patName;
                rItem.oldDoctorName = oldDocNameStr;
                rItem.doctorName = docName;
                rItem.oldDateTime = QString::fromUtf8(oldBuf);
                rItem.newDateTime = QString::fromUtf8(newBuf);
                outResultItems->append(rItem);
            }
        }
    }

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
