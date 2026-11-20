#include "AppointmentController.h"
#include "DoctorController.h"
#include "PatientController.h"
#include "libPBL2.h"
#include "Doctor.h"
#include "Patient.h"
#include "Symptom.h"
#include "SymptomPatient.h"
#include "SymptomPatientDetail.h"
#include <QDateTime>
#include <QMap>
#include <QTime>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace {

constexpr double kPi = 3.14159265358979323846;

long long jdFromDate(int dd, int mm, int yy) {
    int a = (14 - mm) / 12;
    int y = yy + 4800 - a;
    int m = mm + 12 * a - 3;
    long long jd = dd + (153 * m + 2) / 5 + 365LL * y + y / 4 - y / 100 + y / 400 - 32045;
    if (jd < 2299161) {
        jd = dd + (153 * m + 2) / 5 + 365LL * y + y / 4 - 32083;
    }
    return jd;
}

QDate jdToDate(long long jd) {
    long long a, b, c;
    if (jd > 2299160) {
        a = jd + 32044;
        b = (4 * a + 3) / 146097;
        c = a - (b * 146097) / 4;
    } else {
        b = 0;
        c = jd + 32082;
    }
    long long d = (4 * c + 3) / 1461;
    long long e = c - (1461 * d) / 4;
    long long m = (5 * e + 2) / 153;
    int day = static_cast<int>(e - (153 * m + 2) / 5 + 1);
    int month = static_cast<int>(m + 3 - 12 * (m / 10));
    int year = static_cast<int>(b * 100 + d - 4800 + m / 10);
    return QDate(year, month, day);
}

long long getNewMoonDay(int k, double timeZone) {
    double T = k / 1236.85;
    double T2 = T * T;
    double T3 = T2 * T;
    double dr = kPi / 180.0;
    double Jd1 = 2415020.75933 + 29.53058868 * k + 0.0001178 * T2 - 0.000000155 * T3;
    Jd1 += 0.00033 * std::sin((166.56 + 132.87 * T - 0.009173 * T2) * dr);
    double M = 359.2242 + 29.10535608 * k - 0.0000333 * T2 - 0.00000347 * T3;
    double Mpr = 306.0253 + 385.81691806 * k + 0.0107306 * T2 + 0.00001236 * T3;
    double F = 21.2964 + 390.67050646 * k - 0.0016528 * T2 - 0.00000239 * T3;
    double C1 = (0.1734 - 0.000393 * T) * std::sin(M * dr) + 0.0021 * std::sin(2 * dr * M);
    C1 = C1 - 0.4068 * std::sin(Mpr * dr) + 0.0161 * std::sin(dr * 2 * Mpr);
    C1 = C1 - 0.0004 * std::sin(dr * 3 * Mpr);
    C1 = C1 + 0.0104 * std::sin(dr * 2 * F) - 0.0051 * std::sin(dr * (M + Mpr));
    C1 = C1 - 0.0074 * std::sin(dr * (M - Mpr)) + 0.0004 * std::sin(dr * (2 * F + M));
    C1 = C1 - 0.0004 * std::sin(dr * (2 * F - M)) - 0.0006 * std::sin(dr * (2 * F + Mpr));
    C1 = C1 + 0.0010 * std::sin(dr * (2 * F - Mpr)) + 0.0005 * std::sin(dr * (2 * Mpr + M));
    double deltat;
    if (T < -11.0) {
        deltat = 0.001 + 0.000839 * T + 0.0002261 * T2 - 0.00000845 * T3 - 0.000000081 * T * T3;
    } else {
        deltat = -0.000278 + 0.000265 * T + 0.000262 * T2;
    }
    double JdNew = Jd1 + C1 - deltat;
    return static_cast<long long>(std::floor(JdNew + 0.5 + timeZone / 24.0));
}

int getSunLongitude(long long jdn, double timeZone) {
    double T = (jdn - 2451545.5 - timeZone / 24.0) / 36525.0;
    double T2 = T * T;
    double dr = kPi / 180.0;
    double M = 357.52910 + 35999.05030 * T - 0.0001559 * T2 - 0.00000048 * T * T2;
    double L0 = 280.46645 + 36000.76983 * T + 0.0003032 * T2;
    double DL = (1.914600 - 0.004817 * T - 0.000014 * T2) * std::sin(dr * M);
    DL += (0.019993 - 0.000101 * T) * std::sin(dr * 2 * M) + 0.000290 * std::sin(dr * 3 * M);
    double L = (L0 + DL) * dr;
    L -= kPi * 2.0 * std::floor(L / (kPi * 2.0));
    return static_cast<int>(std::floor(L / kPi * 6.0));
}

long long getLunarMonth11(int yy, double timeZone) {
    long long off = jdFromDate(31, 12, yy) - 2415021;
    int k = static_cast<int>(std::floor(off / 29.530588853));
    long long nm = getNewMoonDay(k, timeZone);
    int sunLong = getSunLongitude(nm, timeZone);
    if (sunLong >= 9) {
        nm = getNewMoonDay(k - 1, timeZone);
    }
    return nm;
}

int getLeapMonthOffset(long long a11, double timeZone) {
    int k = static_cast<int>(std::floor((a11 - 2415021.076998695) / 29.530588853 + 0.5));
    int last = 0;
    int i = 1;
    int arc = getSunLongitude(getNewMoonDay(k + i, timeZone), timeZone);
    do {
        last = arc;
        i++;
        arc = getSunLongitude(getNewMoonDay(k + i, timeZone), timeZone);
    } while (arc != last && i < 14);
    return i - 1;
}

QDate convertLunarToSolarVN(int lunarDay, int lunarMonth, int lunarYear, bool lunarLeap = false) {
    constexpr double kTimeZoneVN = 7.0;
    long long a11, b11;
    if (lunarMonth < 11) {
        a11 = getLunarMonth11(lunarYear - 1, kTimeZoneVN);
        b11 = getLunarMonth11(lunarYear, kTimeZoneVN);
    } else {
        a11 = getLunarMonth11(lunarYear, kTimeZoneVN);
        b11 = getLunarMonth11(lunarYear + 1, kTimeZoneVN);
    }
    int k = static_cast<int>(std::floor(0.5 + (a11 - 2415021.076998695) / 29.530588853));
    int off = lunarMonth - 11;
    if (off < 0) off += 12;
    if (b11 - a11 > 365) {
        int leapOff = getLeapMonthOffset(a11, kTimeZoneVN);
        int leapMonth = leapOff - 2;
        if (leapMonth < 0) leapMonth += 12;
        if (lunarLeap && lunarMonth != leapMonth) {
            return QDate();
        }
        if (lunarLeap || off >= leapOff) {
            off += 1;
        }
    }
    long long monthStart = getNewMoonDay(k + off, kTimeZoneVN);
    return jdToDate(monthStart + lunarDay - 1);
}

struct SlotHM { int hour; int minute; };
constexpr SlotHM kFixedSlots[] = {
    {7, 0}, {7, 45}, {8, 30}, {9, 15}, {10, 0}, {10, 45},
    {13, 0}, {13, 45}, {14, 30}, {15, 15}, {16, 0}
};

struct SpecScoreEntry {
    std::string spId;
    int score = 0;
};

static bool compareAppointmentDates(const Appointment *a, const Appointment *b) {
    if (!a || !b) return (a != nullptr) && (b == nullptr);
    int cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
    if (cmp != 0) return cmp < 0;
    return libPBL2::compareNatural(a->getId(), b->getId()) < 0;
}

bool parseHolidayDateParts(const QString &rawDate, int &outDay, int &outMonth, int &outYear, bool &outHasYear) {
    outDay = outMonth = outYear = 0;
    outHasYear = false;
    QString trimmed = rawDate.trimmed();
    QStringList parts = trimmed.split('/');
    if (parts.size() != 2 && parts.size() != 3) return false;
    for (const QString &p : parts) {
        if (p.isEmpty()) return false;
        for (const QChar &ch : p) {
            if (ch < '0' || ch > '9') return false;
        }
    }
    bool okD = false, okM = false;
    outDay = parts[0].toInt(&okD);
    outMonth = parts[1].toInt(&okM);
    if (!okD || !okM || outMonth < 1 || outMonth > 12 || outDay < 1) return false;
    if (parts.size() == 3) {
        if (parts[2].length() != 4) return false;
        bool okY = false;
        outYear = parts[2].toInt(&okY);
        if (!okY || outYear < AppConstants::MIN_CALENDAR_YEAR || outYear > AppConstants::MAX_CALENDAR_YEAR) return false;
        if (!QDate::isValid(outYear, outMonth, outDay)) return false;
        outHasYear = true;
    } else {
        if (!QDate::isValid(2024, outMonth, outDay)) return false;
    }
    return true;
}

QList<std::string> buildRankedSpecialtiesForAppointment(
    const std::string &oldSpId,
    const std::string &fallbackSpId,
    const LinkList<SymptomPatientDetail> *symptomPatientDetails,
    const LinkList<Symptom> *symptoms
) {
    QList<std::string> symptomIds;
    if (!oldSpId.empty() && symptomPatientDetails && symptomPatientDetails->getHeader()) {
        auto *dn = symptomPatientDetails->getHeader()->next;
        while (dn && dn->next) {
            if (dn->info.getSP_id() == oldSpId) {
                symptomIds.append(dn->info.getS_id());
            }
            dn = dn->next;
        }
    }
    return AppointmentController::rankSpecialtiesBySymptoms(symptomIds, symptoms, fallbackSpId);
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
                return libPBL2::compareNatural(a->getID(), b->getID()) < 0;
            });
        }
        if (!specDocs.isEmpty()) hasAnyCandidate = true;
        docsBySpec.append(specDocs);
    }
    if (!hasAnyCandidate) return false;

    QDateTime now = QDateTime::currentDateTime();
    QDate oldQDate(oldDate.getYear(), oldDate.getMonth(), oldDate.getDay());
    QDateTime oldDT(oldQDate, QTime(oldDate.getHour(), oldDate.getMinute()));

    if (allowSameDateTime && oldDT.isValid() && oldDT > now && !AppointmentController::isHolidayInList(oldQDate, holidays)) {
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

    QDate curDate = linearStartDate.isValid() ? linearStartDate : now.date();
    if (curDate < now.date()) curDate = now.date();

    for (int searchDays = 0; searchDays < 365; ++searchDays, curDate = curDate.addDays(1)) {
        if (AppointmentController::isHolidayInList(curDate, holidays)) {
            continue;
        }
        for (const auto &slot : kFixedSlots) {
            QDateTime slotDT(curDate, QTime(slot.hour, slot.minute));
            if (slotDT <= now) continue;
            if (oldDT.isValid() && curDate == oldQDate && slotDT <= oldDT) continue;

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

Appointment *AppointmentController::findById(LinkList<Appointment> *appointments, const std::string &id) {
    if (id.empty() || !appointments || !appointments->getHeader()) return nullptr;
    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getId() == id) return &n->info;
        n = n->next;
    }
    return nullptr;
}

void AppointmentController::sortAppointmentsById(LinkList<Appointment> &list, bool ascending) {
    libPBL2::qsort(list, [ascending](const Appointment &a, const Appointment &b) {
        int cmp = libPBL2::compareNatural(a.getId(), b.getId());
        if (cmp == 0) cmp = libPBL2::compareDates(a.getDateGet(), b.getDateGet());
        return ascending ? (cmp < 0) : (cmp > 0);
    });
}

void AppointmentController::sortAppointmentsById(LinkList<Appointment> *list, bool ascending) {
    if (list) sortAppointmentsById(*list, ascending);
}

QStringList AppointmentController::getFixedTimeSlots() {
    QStringList slotsList;
    slotsList.reserve(static_cast<int>(sizeof(kFixedSlots) / sizeof(kFixedSlots[0])));
    for (const auto &s : kFixedSlots) {
        slotsList.append(QString::asprintf("%02d:%02d", s.hour, s.minute));
    }
    return slotsList;
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
                if (appDt.addSecs(AppConstants::APPOINTMENT_GRACE_MINUTES * 60) >= now && n->info.getState() == 0) {
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
    if (year < 1900 || year > 2199) return QDate();
    return convertLunarToSolarVN(1, 1, year, false);
}

QDate AppointmentController::getHungKingsDay(int year) {
    if (year < 1900 || year > 2199) return QDate();
    return convertLunarToSolarVN(10, 3, year, false);
}

bool AppointmentController::isTetHoliday(const QDate &date, int holidayDays) {
    if (!date.isValid()) return false;
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

QString AppointmentController::canonicalHolidayKey(const QString &rawDate) {
    QString s = rawDate.trimmed().toLower();
    if (s == "tet") return QStringLiteral("tet");
    if (s == "gioto") return QStringLiteral("gioto");
    int d = 0, m = 0, y = 0;
    bool hasY = false;
    if (parseHolidayDateParts(rawDate, d, m, y, hasY)) {
        if (hasY) return QStringLiteral("%1/%2/%3").arg(d).arg(m).arg(y);
        return QStringLiteral("%1/%2").arg(d).arg(m);
    }
    return s;
}

bool AppointmentController::holidaysConflict(const QString &dateA, const QString &dateB) {
    QString keyA = canonicalHolidayKey(dateA);
    QString keyB = canonicalHolidayKey(dateB);
    if (keyA == keyB) return true;

    int dA = 0, mA = 0, yA = 0, dB = 0, mB = 0, yB = 0;
    bool hasYA = false, hasYB = false;
    if (parseHolidayDateParts(dateA, dA, mA, yA, hasYA) &&
        parseHolidayDateParts(dateB, dB, mB, yB, hasYB)) {
        if (dA == dB && mA == mB && (!hasYA || !hasYB || yA == yB)) {
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
        std::string value = item.date.toUtf8().toStdString();
        std::string rest = item.note.toUtf8().toStdString();
        libPBL2::trim(value);
        libPBL2::trim(rest);

        std::string lowerVal = value;
        for (char &c : lowerVal) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lowerVal == "tet") {
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
        if (lowerVal == "gioto") {
            QDate gioToDate = AppointmentController::getHungKingsDay(date.year());
            if (gioToDate.isValid() && date == gioToDate) {
                if (outNote) {
                    *outNote = rest.empty()
                        ? QStringLiteral("Giỗ Tổ Hùng Vương (10/3 Âm lịch)")
                        : QString::fromUtf8(rest.c_str());
                }
                return true;
            }
            continue;
        }

        int day = 0, month = 0, year = 0;
        bool hasYear = false;
        if (parseHolidayDateParts(QString::fromUtf8(value.c_str()), day, month, year, hasYear)) {
            bool match = hasYear
                ? (day == date.day() && month == date.month() && year == date.year())
                : (day == date.day() && month == date.month());
            if (match) {
                if (outNote) {
                    if (!rest.empty()) *outNote = QString::fromUtf8(rest.c_str());
                    else if (day == 1 && month == 1) *outNote = QStringLiteral("Tết Dương Lịch");
                    else if (day == 30 && month == 4) *outNote = QStringLiteral("Ngày Giải phóng miền Nam");
                    else if (day == 1 && month == 5) *outNote = QStringLiteral("Ngày Quốc tế Lao động");
                    else if (day == 2 && month == 9) *outNote = QStringLiteral("Quốc khánh Việt Nam");
                    else if (day == 1 && month == 9) *outNote = QStringLiteral("Nghỉ lễ Quốc khánh");
                    else *outNote = QStringLiteral("Nghỉ lễ");
                }
                return true;
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
    if (patients) {
        if (patientId.empty()) {
            throw ValidationException("Hồ sơ bệnh nhân không được để trống.");
        }
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
            std::string msg = "Phòng khám nghỉ lễ vào ngày " + targetDateTime.date().toString("dd/MM/yyyy").toStdString() + " (" + note.toUtf8().toStdString() + "). Vui lòng chọn ngày khác.";
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
    return QStringLiteral("Nghỉ lễ");
}

QString AppointmentController::getHolidayNote(const QDate &date, const std::string &configPath) {
    const auto list = loadHolidays(configPath);
    return getHolidayNoteFromList(date, list);
}

QList<HolidayEntry> AppointmentController::loadHolidays(const std::string &configPath) {
    auto it = s_holidayCache.constFind(configPath);
    if (it != s_holidayCache.constEnd()) {
        return it.value();
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

QList<std::string> AppointmentController::rankSpecialtiesBySymptoms(
    const QList<std::string> &symptomIds,
    const LinkList<Symptom> *symptoms,
    const std::string &fallbackSpId
) {
    QList<SpecScoreEntry> scored;
    if (!symptomIds.isEmpty() && symptoms && symptoms->getHeader()) {
        for (const std::string &sid : symptomIds) {
            if (sid.empty()) continue;
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
    }

    if (scored.size() > 1) {
        libPBL2::qsort(scored, [&fallbackSpId](const SpecScoreEntry &a, const SpecScoreEntry &b) {
            if (a.score != b.score) return a.score > b.score;
            bool aOrig = (!fallbackSpId.empty() && a.spId == fallbackSpId);
            bool bOrig = (!fallbackSpId.empty() && b.spId == fallbackSpId);
            if (aOrig != bOrig) return aOrig;
            return libPBL2::compareNatural(a.spId, b.spId) < 0;
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

QList<Appointment*> AppointmentController::getAffectedAppointmentsForHoliday(
    const LinkList<Appointment> *appointments,
    const QList<HolidayEntry> &currentHolidays,
    const QList<HolidayEntry> &prospectiveHolidays
) {
    QList<Appointment*> affectedList;
    if (!appointments || !appointments->getHeader()) return affectedList;

    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.computeEffectiveStateNow() == 0) {
            Date d = n->info.getDateGet();
            QDate apptDate(d.getYear(), d.getMonth(), d.getDay());
            if (apptDate.isValid()) {
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

void AppointmentController::rollbackInsertedAppointments(
    LinkList<Appointment> *appointments,
    LinkList<SymptomPatient> *symptomPatients,
    LinkList<SymptomPatientDetail> *symptomPatientDetails,
    const QList<std::string> &insertedApptIds,
    const QList<std::string> &insertedSpIds,
    const QList<std::string> &modifiedOldApptIds
) {
    if (appointments && appointments->getHeader() && !modifiedOldApptIds.isEmpty()) {
        auto *n = appointments->getHeader()->next;
        while (n && n->next) {
            if (modifiedOldApptIds.contains(n->info.getId())) {
                n->info.setState(0);
            }
            n = n->next;
        }
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

    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getDoctorId() == doctorId) {
            if (n->info.computeEffectiveStateNow() == 0) {
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
    QList<std::string> *outModifiedOldApptIds
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
    QList<std::string> modifiedOldApptIds;
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
        modifiedOldApptIds.append(oldAppt->getId());

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
                                     insertedApptIds, insertedSpIds, modifiedOldApptIds);
        if (outFailedCount) *outFailedCount = failedCount;
        return 0;
    }

    if (outResultItems) *outResultItems = localResults;
    if (outInsertedApptIds) *outInsertedApptIds = insertedApptIds;
    if (outInsertedSpIds) *outInsertedSpIds = insertedSpIds;
    if (outModifiedOldApptIds) *outModifiedOldApptIds = modifiedOldApptIds;
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
    QList<std::string> *outModifiedOldApptIds
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
            if (outModifiedOldApptIds) outModifiedOldApptIds->append(oldAppt->getId());

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
    int changed = 0;
    auto *n = appointments->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() == 0) {
            if (n->info.computeEffectiveStateNow() == -1) {
                n->info.setState(-1);
                changed++;
            }
        }
        n = n->next;
    }
    return changed;
}
