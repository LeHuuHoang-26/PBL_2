#include "StatisticController.h"
#include "libPBL2.h"
#include "AppExceptions.h"
#include <memory>
#include <utility>

namespace {

struct DoctorLookupEntry {
    std::string id;
    std::string spId;
    double fee = 0.0;
};

struct DoctorLookupTable {
    std::unique_ptr<DoctorLookupEntry[]> entries;
    int count = 0;

    void build(const LinkList<Doctor> *docs) {
        count = 0;
        entries.reset();
        if (!docs || !docs->getHeader()) return;
        int cap = docs->getCount();
        if (cap <= 0) return;
        try {
            entries = std::make_unique<DoctorLookupEntry[]>(cap);
        } catch (const std::bad_alloc &) {
            throw MemoryAllocationException("Không đủ bộ nhớ để cấp phát bảng tra cứu bác sĩ.");
        }
        auto *dn = docs->getHeader()->next;
        while (dn && dn->next && count < cap) {
            if (!dn->info.isDefault()) {
                entries[count].id = dn->info.getID();
                entries[count].spId = dn->info.getSpId();
                entries[count].fee = dn->info.getFee();
                count++;
            }
            dn = dn->next;
        }
        if (count > 1) {
            libPBL2::qsort(entries.get(), 0, count - 1, [](const DoctorLookupEntry &a, const DoctorLookupEntry &b) {
                return a.id < b.id;
            });
        }
    }

    const DoctorLookupEntry *find(const std::string &id) const {
        int lo = 0, hi = count - 1;
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            if (entries[mid].id == id) return &entries[mid];
            if (entries[mid].id < id) lo = mid + 1;
            else hi = mid - 1;
        }
        return nullptr;
    }
};

struct SpPatientEntry {
    std::string spId;
    std::string patientId;
};

struct SpPatientTable {
    std::unique_ptr<SpPatientEntry[]> entries;
    int count = 0;

    void build(const LinkList<SymptomPatient> *sps) {
        count = 0;
        entries.reset();
        if (!sps || !sps->getHeader()) return;
        int cap = sps->getCount();
        if (cap <= 0) return;
        try {
            entries = std::make_unique<SpPatientEntry[]>(cap);
        } catch (const std::bad_alloc &) {
            throw MemoryAllocationException("Không đủ bộ nhớ để cấp phát bảng tra cứu hồ sơ triệu chứng.");
        }
        auto *n = sps->getHeader()->next;
        while (n && n->next && count < cap) {
            if (!n->info.isDefault()) {
                entries[count].spId = n->info.getSP_id();
                entries[count].patientId = n->info.getP_id();
                count++;
            }
            n = n->next;
        }
        if (count > 1) {
            libPBL2::qsort(entries.get(), 0, count - 1, [](const SpPatientEntry &a, const SpPatientEntry &b) {
                return a.spId < b.spId;
            });
        }
    }

    const std::string *findPatientId(const std::string &spId) const {
        int lo = 0, hi = count - 1;
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            if (entries[mid].spId == spId) return &entries[mid].patientId;
            if (entries[mid].spId < spId) lo = mid + 1;
            else hi = mid - 1;
        }
        return nullptr;
    }
};

bool isDateInRange(const Date &ad, int sVal, int eVal) {
    if (ad.getYear() < 1 || ad.getMonth() < 1 || ad.getMonth() > 12 || ad.getDay() < 1) return false;
    int aVal = ad.getYear() * 10000 + ad.getMonth() * 100 + ad.getDay();
    return aVal >= sVal && aVal <= eVal;
}

long long toJulianDay(int d, int m, int y) {
    int a = (14 - m) / 12;
    long long yy = static_cast<long long>(y) + 4800 - a;
    int mm = m + 12 * a - 3;
    return d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
}

void accumulateBucket(
    TimeSeriesItem &bucket,
    const Appointment &appt,
    const DoctorLookupTable &docTable,
    int curDay, int curMonth, int curYear, int curHour, int curMin, int curSec
) {
    int st = appt.getState();
    if (st == 1) {
        bucket.completedCount++;
        if (appt.hasPaidFee()) {
            bucket.revenue += appt.getPaidFee();
        } else if (const auto *doc = docTable.find(appt.getDoctorId())) {
            bucket.revenue += doc->fee;
        }
    } else if (st == -2) {
        bucket.rescheduledCount++;
    } else {
        int eff = appt.computeEffectiveState(curDay, curMonth, curYear, curHour, curMin, curSec);
        if (eff == -1) {
            bucket.delayedCount++;
        }
    }
}

}

int StatisticController::patientCount(const LinkList<Patient> *items) {
    if (!items || !items->getHeader()) return 0;
    int c = 0;
    auto *n = items->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() != -1) c++;
        n = n->next;
    }
    return c;
}

int StatisticController::doctorCount(const LinkList<Doctor> *items) {
    if (!items || !items->getHeader()) return 0;
    int c = 0;
    auto *n = items->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() != -1) c++;
        n = n->next;
    }
    return c;
}

int StatisticController::appointmentCount(const LinkList<Appointment> *items) {
    if (!items || !items->getHeader()) return 0;
    int c = 0;
    auto *n = items->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() != -2) c++;
        n = n->next;
    }
    return c;
}

int StatisticController::pendingAppointmentCount(const LinkList<Appointment> *items) {
    if (!items || !items->getHeader()) return 0;
    int c = 0;
    auto *n = items->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.computeEffectiveStateNow() == 0) {
            c++;
        }
        n = n->next;
    }
    return c;
}

void StatisticController::appointmentStatusCounts(
    const LinkList<Appointment> *appts,
    int &completed, int &pending, int &delayed, int &rescheduled,
    int currentDay, int currentMonth, int currentYear, int currentHour, int currentMinute,
    int currentSecond,
    int startDay, int startMonth, int startYear,
    int endDay, int endMonth, int endYear
) {
    completed = 0;
    pending = 0;
    delayed = 0;
    rescheduled = 0;
    if (!appts || !appts->getHeader()) return;

    bool hasDateFilter = (startDay > 0 && startMonth > 0 && startYear > 0 &&
                          endDay > 0 && endMonth > 0 && endYear > 0);
    int sVal = startYear * 10000 + startMonth * 100 + startDay;
    int eVal = endYear * 10000 + endMonth * 100 + endDay;

    auto *n = appts->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault()) {
            if (hasDateFilter && !isDateInRange(n->info.getDateGet(), sVal, eVal)) {
                n = n->next;
                continue;
            }
            int st = n->info.computeEffectiveState(currentDay, currentMonth, currentYear, currentHour, currentMinute, currentSecond);
            switch (st) {
                case 1: completed++; break;
                case 0: pending++; break;
                case -1: delayed++; break;
                case -2: rescheduled++; break;
                default: break;
            }
        }
        n = n->next;
    }
}

void StatisticController::patientGenderCounts(const LinkList<Patient> *patients, int &male, int &female) {
    male = 0;
    female = 0;
    if (!patients || !patients->getHeader()) return;

    auto *n = patients->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() != -1) {
            if (n->info.getGender()) male++;
            else female++;
        }
        n = n->next;
    }
}

void StatisticController::patientAgeDistribution(const LinkList<Patient> *patients, int &under18, int &from18to45, int &from46to60, int &over60) {
    under18 = 0;
    from18to45 = 0;
    from46to60 = 0;
    over60 = 0;
    if (!patients || !patients->getHeader()) return;

    auto *n = patients->getHeader()->next;
    while (n && n->next) {
        if (!n->info.isDefault() && n->info.getState() != -1) {
            int age = n->info.getAge();
            if (age < 18) under18++;
            else if (age <= 45) from18to45++;
            else if (age <= 60) from46to60++;
            else over60++;
        }
        n = n->next;
    }
}

void StatisticController::patientDemographicsInRange(
    const LinkList<Patient> *patients,
    const LinkList<Appointment> *appts,
    const LinkList<SymptomPatient> *symptomPatients,
    int startDay, int startMonth, int startYear,
    int endDay, int endMonth, int endYear,
    int &male, int &female,
    int &under18, int &from18to45, int &from46to60, int &over60
) {
    male = female = 0;
    under18 = from18to45 = from46to60 = over60 = 0;
    if (!patients || !patients->getHeader()) return;

    bool hasDateFilter = (startDay > 0 && startMonth > 0 && startYear > 0 &&
                          endDay > 0 && endMonth > 0 && endYear > 0);
    if (!hasDateFilter || !appts || !appts->getHeader() || !symptomPatients || !symptomPatients->getHeader()) {
        patientGenderCounts(patients, male, female);
        patientAgeDistribution(patients, under18, from18to45, from46to60, over60);
        return;
    }

    int sVal = startYear * 10000 + startMonth * 100 + startDay;
    int eVal = endYear * 10000 + endMonth * 100 + endDay;

    SpPatientTable spTable;
    spTable.build(symptomPatients);
    if (spTable.count <= 0) return;

    int cap = appts->getCount();
    if (cap <= 0) return;
    std::unique_ptr<std::string[]> matchedIds;
    try {
        matchedIds = std::make_unique<std::string[]>(cap);
    } catch (const std::bad_alloc &) {
        throw MemoryAllocationException("Không đủ bộ nhớ để cấp phát mảng lọc bệnh nhân.");
    }
    int matchedCount = 0;

    auto *an = appts->getHeader()->next;
    while (an && an->next && matchedCount < cap) {
        if (!an->info.isDefault() && an->info.getState() != -2 && isDateInRange(an->info.getDateGet(), sVal, eVal)) {
            if (const std::string *pid = spTable.findPatientId(an->info.getSymptomPatientId())) {
                if (!pid->empty()) {
                    matchedIds[matchedCount++] = *pid;
                }
            }
        }
        an = an->next;
    }
    if (matchedCount == 0) return;

    if (matchedCount > 1) {
        libPBL2::qsort(matchedIds.get(), 0, matchedCount - 1, [](const std::string &a, const std::string &b) {
            return a < b;
        });
    }
    int uniqueCount = 0;
    for (int i = 0; i < matchedCount; ++i) {
        if (uniqueCount == 0 || matchedIds[i] != matchedIds[uniqueCount - 1]) {
            if (uniqueCount != i) matchedIds[uniqueCount] = std::move(matchedIds[i]);
            uniqueCount++;
        }
    }

    auto *pn = patients->getHeader()->next;
    while (pn && pn->next) {
        if (!pn->info.isDefault() && pn->info.getState() != -1) {
            const std::string pid = pn->info.getID();
            int lo = 0, hi = uniqueCount - 1;
            bool found = false;
            while (lo <= hi) {
                int mid = lo + (hi - lo) / 2;
                if (matchedIds[mid] == pid) { found = true; break; }
                if (matchedIds[mid] < pid) lo = mid + 1;
                else hi = mid - 1;
            }
            if (found) {
                if (pn->info.getGender()) male++;
                else female++;
                int age = pn->info.getAge();
                if (age < 18) under18++;
                else if (age <= 45) from18to45++;
                else if (age <= 60) from46to60++;
                else over60++;
            }
        }
        pn = pn->next;
    }
}

int StatisticController::getTopDoctors(
    const LinkList<Appointment> *appts,
    const LinkList<Doctor> *docs,
    DoctorLeaderboardItem outList[],
    int maxCount,
    int startDay, int startMonth, int startYear,
    int endDay, int endMonth, int endYear
) {
    if (!docs || !docs->getHeader() || !outList || maxCount <= 0) return 0;

    int totalDocs = docs->getCount();
    if (totalDocs <= 0) return 0;

    std::unique_ptr<DoctorLeaderboardItem[]> allItems;
    try {
        allItems = std::make_unique<DoctorLeaderboardItem[]>(totalDocs);
    } catch (const std::bad_alloc &) {
        throw MemoryAllocationException("Không đủ bộ nhớ để cấp phát mảng thống kê bác sĩ.");
    }

    int validDoctors = 0;
    auto *dn = docs->getHeader()->next;
    while (dn && dn->next && validDoctors < totalDocs) {
        if (!dn->info.isDefault() && dn->info.getState() != -1) {
            allItems[validDoctors].doctorId = dn->info.getID();
            allItems[validDoctors].doctorName = dn->info.getfullName();
            allItems[validDoctors].specialtyId = dn->info.getSpId();
            allItems[validDoctors].roomId = dn->info.getRoomId();
            allItems[validDoctors].rating = dn->info.getRating();
            allItems[validDoctors].appointmentCount = 0;
            validDoctors++;
        }
        dn = dn->next;
    }

    if (validDoctors > 1) {
        libPBL2::qsort(allItems.get(), 0, validDoctors - 1, [](const DoctorLeaderboardItem &a, const DoctorLeaderboardItem &b) {
            return a.doctorId < b.doctorId;
        });
    }

    bool hasDateFilter = (startDay > 0 && startMonth > 0 && startYear > 0 &&
                          endDay > 0 && endMonth > 0 && endYear > 0);
    int sVal = startYear * 10000 + startMonth * 100 + startDay;
    int eVal = endYear * 10000 + endMonth * 100 + endDay;

    if (appts && appts->getHeader()) {
        auto *an = appts->getHeader()->next;
        while (an && an->next) {
            if (!an->info.isDefault() && an->info.getState() == 1) {
                if (hasDateFilter && !isDateInRange(an->info.getDateGet(), sVal, eVal)) {
                    an = an->next;
                    continue;
                }
                const std::string did = an->info.getDoctorId();
                int lo = 0, hi = validDoctors - 1;
                while (lo <= hi) {
                    int mid = lo + (hi - lo) / 2;
                    if (allItems[mid].doctorId == did) {
                        allItems[mid].appointmentCount++;
                        break;
                    }
                    if (allItems[mid].doctorId < did) lo = mid + 1;
                    else hi = mid - 1;
                }
            }
            an = an->next;
        }
    }

    int positiveCount = 0;
    for (int i = 0; i < validDoctors; ++i) {
        if (allItems[i].appointmentCount >= 1) {
            if (positiveCount != i) allItems[positiveCount] = std::move(allItems[i]);
            positiveCount++;
        }
    }
    validDoctors = positiveCount;

    if (validDoctors > 1) {
        libPBL2::qsort(allItems.get(), 0, validDoctors - 1, [](const DoctorLeaderboardItem &a, const DoctorLeaderboardItem &b) {
            if (a.appointmentCount != b.appointmentCount) return a.appointmentCount > b.appointmentCount;
            if (a.rating != b.rating) return a.rating > b.rating;
            return libPBL2::compareNatural(a.doctorId, b.doctorId) < 0;
        });
    }

    int copyCount = (validDoctors < maxCount) ? validDoctors : maxCount;
    for (int i = 0; i < copyCount; ++i) {
        outList[i] = allItems[i];
    }
    return copyCount;
}

int StatisticController::getSpecialtyDemand(
    const LinkList<Appointment> *appts,
    const LinkList<Doctor> *docs,
    SpecialtyDemandItem outList[],
    int maxCount,
    int startDay, int startMonth, int startYear,
    int endDay, int endMonth, int endYear
) {
    if (!appts || !appts->getHeader() || !docs || !docs->getHeader() || !outList || maxCount <= 0) return 0;

    DoctorLookupTable docTable;
    docTable.build(docs);
    if (docTable.count <= 0) return 0;

    std::unique_ptr<SpecialtyDemandItem[]> items;
    try {
        items = std::make_unique<SpecialtyDemandItem[]>(docTable.count);
    } catch (const std::bad_alloc &) {
        throw MemoryAllocationException("Không đủ bộ nhớ để cấp phát mảng nhu cầu chuyên khoa.");
    }
    int uniqueCount = 0;

    bool hasDateFilter = (startDay > 0 && startMonth > 0 && startYear > 0 &&
                          endDay > 0 && endMonth > 0 && endYear > 0);
    int sVal = startYear * 10000 + startMonth * 100 + startDay;
    int eVal = endYear * 10000 + endMonth * 100 + endDay;

    auto *an = appts->getHeader()->next;
    while (an && an->next) {
        if (!an->info.isDefault() && an->info.getState() != -2) {
            if (hasDateFilter && !isDateInRange(an->info.getDateGet(), sVal, eVal)) {
                an = an->next;
                continue;
            }
            if (const auto *doc = docTable.find(an->info.getDoctorId())) {
                int foundIdx = -1;
                for (int i = 0; i < uniqueCount; ++i) {
                    if (items[i].spId == doc->spId) {
                        foundIdx = i;
                        break;
                    }
                }
                if (foundIdx >= 0) {
                    items[foundIdx].count++;
                } else if (uniqueCount < docTable.count) {
                    items[uniqueCount].spId = doc->spId;
                    items[uniqueCount].count = 1;
                    uniqueCount++;
                }
            }
        }
        an = an->next;
    }

    if (uniqueCount > 1) {
        libPBL2::qsort(items.get(), 0, uniqueCount - 1, [](const SpecialtyDemandItem &a, const SpecialtyDemandItem &b) {
            if (a.count != b.count) return a.count > b.count;
            return libPBL2::compareNatural(a.spId, b.spId) < 0;
        });
    }

    int copyCount = (uniqueCount < maxCount) ? uniqueCount : maxCount;
    for (int i = 0; i < copyCount; ++i) {
        outList[i] = items[i];
    }
    return copyCount;
}

int StatisticController::getTimeSeriesDaily(
    const LinkList<Appointment> *appts,
    const LinkList<Doctor> *docs,
    int startDay, int startMonth, int startYear,
    int endDay, int endMonth, int endYear,
    int curDay, int curMonth, int curYear, int curHour, int curMin,
    TimeSeriesItem outItems[], int maxItems,
    int curSec
) {
    if (!outItems || maxItems <= 0) return 0;
    if (startMonth < 1 || startMonth > 12 || endMonth < 1 || endMonth > 12 || startYear < 1 || endYear < 1) return 0;
    auto isLeap = [](int y) {
        return y > 0 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
    };
    auto daysInMonth = [&](int m, int y) {
        if (m == 2) return isLeap(y) ? 29 : 28;
        if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
        return 31;
    };
    if (startDay < 1 || startDay > daysInMonth(startMonth, startYear) ||
        endDay < 1 || endDay > daysInMonth(endMonth, endYear)) return 0;

    long long startJdn = toJulianDay(startDay, startMonth, startYear);
    long long endJdn = toJulianDay(endDay, endMonth, endYear);
    if (startJdn > endJdn) return 0;

    int count = 0;
    int cDay = startDay, cMonth = startMonth, cYear = startYear;

    while (count < maxItems) {
        outItems[count].day = cDay;
        outItems[count].month = cMonth;
        outItems[count].year = cYear;
        std::string dStr = (cDay < 10 ? "0" : "") + std::to_string(cDay);
        std::string mStr = (cMonth < 10 ? "0" : "") + std::to_string(cMonth);
        outItems[count].label = dStr + "/" + mStr;
        outItems[count].revenue = 0.0;
        outItems[count].completedCount = 0;
        outItems[count].delayedCount = 0;
        outItems[count].rescheduledCount = 0;
        count++;

        if (cDay == endDay && cMonth == endMonth && cYear == endYear) {
            break;
        }

        cDay++;
        if (cDay > daysInMonth(cMonth, cYear)) {
            cDay = 1;
            cMonth++;
            if (cMonth > 12) {
                cMonth = 1;
                cYear++;
            }
        }
    }

    if (!appts || !appts->getHeader() || count == 0) return count;

    DoctorLookupTable docTable;
    docTable.build(docs);
    auto *an = appts->getHeader()->next;
    while (an && an->next) {
        if (!an->info.isDefault()) {
            Date ad = an->info.getDateGet();
            if (ad.getYear() >= 1 && ad.getMonth() >= 1 && ad.getMonth() <= 12 && ad.getDay() >= 1) {
                long long offset = toJulianDay(ad.getDay(), ad.getMonth(), ad.getYear()) - startJdn;
                if (offset >= 0 && offset < count) {
                    accumulateBucket(outItems[offset], an->info, docTable, curDay, curMonth, curYear, curHour, curMin, curSec);
                }
            }
        }
        an = an->next;
    }

    return count;
}

int StatisticController::getTimeSeriesMonthlyRange(
    const LinkList<Appointment> *appts,
    const LinkList<Doctor> *docs,
    int startDay, int startMonth, int startYear,
    int endDay, int endMonth, int endYear,
    int curDay, int curMonth, int curYear, int curHour, int curMin,
    TimeSeriesItem outItems[], int maxItems,
    int curSec
) {
    if (!outItems || maxItems <= 0) return 0;
    if (startMonth < 1 || startMonth > 12 || endMonth < 1 || endMonth > 12 ||
        startYear < 1 || endYear < 1 || startDay < 1 || endDay < 1) return 0;
    int sVal = startYear * 10000 + startMonth * 100 + startDay;
    int eVal = endYear * 10000 + endMonth * 100 + endDay;
    if (sVal > eVal) return 0;

    int startMonthKey = startYear * 12 + startMonth;
    int endMonthKey = endYear * 12 + endMonth;

    int count = 0;
    int cMonth = startMonth, cYear = startYear;

    while (count < maxItems && (cYear * 12 + cMonth) <= endMonthKey) {
        outItems[count].day = 1;
        outItems[count].month = cMonth;
        outItems[count].year = cYear;
        outItems[count].label = "T" + std::to_string(cMonth);
        if (startYear != endYear) {
            std::string yStr = std::to_string(cYear);
            outItems[count].label += "/" + (yStr.size() >= 2 ? yStr.substr(yStr.size() - 2) : yStr);
        }
        outItems[count].revenue = 0.0;
        outItems[count].completedCount = 0;
        outItems[count].delayedCount = 0;
        outItems[count].rescheduledCount = 0;
        count++;

        if (cMonth == endMonth && cYear == endYear) break;

        cMonth++;
        if (cMonth > 12) {
            cMonth = 1;
            cYear++;
        }
    }

    if (!appts || !appts->getHeader() || count == 0) return count;

    DoctorLookupTable docTable;
    docTable.build(docs);
    auto *an = appts->getHeader()->next;
    while (an && an->next) {
        if (!an->info.isDefault()) {
            Date ad = an->info.getDateGet();
            if (isDateInRange(ad, sVal, eVal)) {
                int idx = (ad.getYear() * 12 + ad.getMonth()) - startMonthKey;
                if (idx >= 0 && idx < count) {
                    accumulateBucket(outItems[idx], an->info, docTable, curDay, curMonth, curYear, curHour, curMin, curSec);
                }
            }
        }
        an = an->next;
    }

    return count;
}
