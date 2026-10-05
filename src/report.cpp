#include <stdio.h>
#include <sqlite3.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <functional>
#include <math.h>
#include <chrono>
#include <algorithm>


const double MPStoKMPH = 3.6;                           // m/s to km/h
const unsigned long EarthRadius = 6371000;              // Equatorial radius.  Polar radius: 6366197
const unsigned int MillisecondsPerDay = 86400000;

// Return the normalised distance between two points on a sphere
double SphericalDistance(double lat1, double lon1, double lat2, double lon2)
{
    double t = cos(lat1) * cos(lon1) * cos(lat2) * cos(lon2) +
               cos(lat1) * sin(lon1) * cos(lat2) * sin(lon2) +
               sin(lat1) * sin(lat2);
    return acos(t);
}


// Extracts the year, month, day from a date string in the format YYYYMMDD
bool ParseSimpleDate(const std::string& dateStr, int64_t& ms)
{
    if (dateStr.length() != 8) return false;
    for (const char c : dateStr) if (!isdigit(c)) return false;

    const unsigned int dateNum = atol(dateStr.c_str());
    std::chrono::year_month_day ymd{std::chrono::year{(int)dateNum / 10000},
                                    std::chrono::month{(dateNum / 100) % 100},
                                    std::chrono::day{dateNum % 100}};
    if (!ymd.ok()) return false;

    const std::chrono::sys_days days(ymd);
    const std::chrono::sys_seconds seconds(days);
    ms = 1000 * duration_cast<std::chrono::seconds>(seconds.time_since_epoch()).count();

    return true;
}



// Accumulates device speed and distance from a query of the form "SELECT device_id, ts_ms, lat, lon FROM events"
// The query must be sorted by device_id, ts_ms
struct AvgSpeed
{
    std::string device_id;
    double distance, avgSpeed;
};
void AccumulateSpeed(SQLite::Statement& query, std::vector<AvgSpeed>& speedList)
{
    // Accumulate the distance and time over each individual device_id
    std::string prevDevice = "";
    double prevLat=0, prevLon=0, accDistance=0;
    int64_t prevTime=0, accDuration = 0;
    while (query.executeStep())
    {
        if (prevDevice != (const char*)query.getColumn("device_id"))
        {
            // New device.  Store the accumulated results
            if (prevDevice != "" && accDuration > 0) speedList.emplace_back(prevDevice, accDistance, accDistance / accDuration);

            // Prepare accumulation for the new device
            prevDevice = (const char*)query.getColumn("device_id");
            prevTime = (int64_t)query.getColumn("ts_ms");
            prevLat = (double)query.getColumn("lat");
            prevLon = (double)query.getColumn("lon");
            accDistance = 0;
            accDuration = 0;
        }
        else
        {
            // We have the next event for the current device, so we can determine distance and time since the previous event
            const int64_t curTime = (int64_t)query.getColumn("ts_ms");
            const double curLat = (double)query.getColumn("lat");
            const double curLon = (double)query.getColumn("lon");
            accDistance += SphericalDistance(prevLat, prevLon, curLat, curLon) * EarthRadius;
            if (accDistance > 0) accDuration += curTime - prevTime; // Don't accumulate the time if there's been no movement

            prevTime = curTime;
            prevLat = curLat;
            prevLon = curLon;
        }
    }
    // Store the last device speed and distance
    if (prevDevice != "" && accDuration > 0) speedList.emplace_back(prevDevice, accDistance, accDistance / accDuration);
}




void Report_LastKnownLocation(SQLite::Database& db, __attribute__((unused)) const std::string& parameters)
{
    SQLite::Statement query(db,
                            "SELECT events.device_id, datetime(ts_ms/1000, 'unixepoch', 'localtime') DT, events.lat, events.lon FROM events"
                            " INNER JOIN (SELECT device_id, MAX(ts_ms) AS max_ts FROM events GROUP BY device_ID) lastTimestamp"
                            "    ON events.device_id = lastTimestamp.device_id"
                            "   AND events.ts_ms = lastTimestamp.max_ts");
    while (query.executeStep())
    {
        printf("%s,%s,%.6f,%.6f\n",
               (const char*)query.getColumn("device_id"), (const char*)query.getColumn("DT"),
               (double)query.getColumn("lat"), (double)query.getColumn("lon"));
    }
}


void Report_DistancePerDevice(SQLite::Database& db, const std::string& parameters)
{
    int64_t timestamp;
    if (!ParseSimpleDate(parameters, timestamp))
    {
        puts("date must be valid and in the format YYYYMMDD, e.g. 20260102");
        return;
    }

    SQLite::Statement query(db,
                            "SELECT device_id, ts_ms, lat, lon FROM events"
                            " WHERE ts_ms >= ? AND ts_ms < ?"
                            " ORDER BY device_id, ts_ms");
    query.bind(1, timestamp);
    query.bind(2, timestamp + MillisecondsPerDay);        // Add one day's worth of ms

    std::vector<AvgSpeed> speedList;
    AccumulateSpeed(query, speedList);
    for (AvgSpeed& avgSpeed : speedList) printf("%s,%.1f\n", avgSpeed.device_id.c_str(), avgSpeed.distance);
}


void Report_Top5AvgSpeed(SQLite::Database& db, __attribute__((unused)) const std::string& parameters)
{
    // Query for all records for the last 24 hours - from 24 hours ago till now()
    std::chrono::time_point tpNow = std::chrono::utc_clock::now();
    int64_t timestamp = duration_cast<std::chrono::seconds>(tpNow.time_since_epoch()).count() * 1000;

    SQLite::Statement query(db,
                            "SELECT device_id, ts_ms, lat, lon FROM events"
                            " WHERE ts_ms >= ?"
                            " ORDER BY device_id, ts_ms");
    query.bind(1, timestamp - MillisecondsPerDay);

    // Accumulate the distance and time over each individual device_id
    std::vector<AvgSpeed> speedList;
    AccumulateSpeed(query, speedList);

    // Now to sort the speed list, descending, and display the top 5, converting from m/s to km/h
    std::ranges::sort(speedList, [](AvgSpeed& a, AvgSpeed& b) { return a.avgSpeed < b.avgSpeed; });
    const int rowCount = std::min(5, (int)speedList.size());
    for (int index = 0; index < rowCount; index++) printf("%s,%.1f\n", speedList[index].device_id.c_str(), speedList[index].avgSpeed * MPStoKMPH);
}


void Report_DumpAll(SQLite::Database& db, __attribute__((unused)) const std::string& parameters)
{
    SQLite::Statement query(db, "SELECT * from events");
    while (query.executeStep())
    {
        printf("%s,%s,%lld,%.6f,%.6f,%.1f,%.1f\n",
               (const char*)query.getColumn("device_id"), (const char*)query.getColumn("event_id"),
               (int64_t)query.getColumn("ts_ms"), (double)query.getColumn("lat"), (double)query.getColumn("lon"),
               (double)query.getColumn("speed_kph"), (double)query.getColumn("heading_deg"));
    }
}


struct ReportDescriptor
{
    const char* name;
    const char* description;
    const std::function<void(SQLite::Database&, const std::string&)> run;
} reports[] =
{
    {"location", "Last known location of all devices", Report_LastKnownLocation},
    {"distance", "Distance travelled per device (requires date as YYYYMMDD)", Report_DistancePerDevice},
    {"top5speed", "Top 5 average device speeds", Report_Top5AvgSpeed},
    {"dumpall", "Display all records", Report_DumpAll},
};



void ShowUsage()
{
    puts("usage: report <report name> <report parameters>");
    for (ReportDescriptor& report : reports) printf("   %s - %s\n", report.name, report.description);
}


int main(int argc, char** argv)
{
    if (argc < 2)
    {
        ShowUsage();
        return 1;
    }
    const std::string reportName = argv[1];
    const std::string reportParameters = argc > 2 ? argv[2] : "";

    bool found = false;
    for (const ReportDescriptor& report : reports) if (reportName == report.name)
    {
        SQLite::Database db("db/events.db3", SQLite::OPEN_READONLY);
        report.run(db, reportParameters);
        found = true;
        break;
    }
    if (!found)
    {
        ShowUsage();
        return 1;
    }

    return 0;
}
