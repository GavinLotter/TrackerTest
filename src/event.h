#include <sqlite3.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <nlohmann/json.hpp>

class EventRecord
{
public:
    std::string device_id;
    std::string event_id;
    int64_t ts_ms;
    double lat, lon;
    double speed_kph;
    double heading_deg;

public:
    void Parse(const std::string_view str)
    {
        nlohmann::json record = nlohmann::json::parse(str);
        device_id = record["device_id"].get<std::string>();
        event_id = record["event_id"].get<std::string>();
        ts_ms = record["ts_ms"].get<int64_t>();
        lat = record["lat"].get<double>();
        lon = record["lon"].get<double>();
        speed_kph = record["speed_kph"].get<double>();
        heading_deg = record["heading_deg"].get<double>();
    }

};


class EventStore
{
public:
    static void Setup(SQLite::Database &db)
    {
        db.exec("CREATE TABLE IF NOT EXISTS"
                " events(device_id TEXT, event_id TEXT, ts_ms NUM,"
                " lat REAL, lon REAL, speed_kph REAL, heading_deg REAL,"
                " UNIQUE(device_id, event_id) ON CONFLICT IGNORE)");
    }
    static void Insert(SQLite::Database &db, const EventRecord &record)
    {
        SQLite::Statement query(db, "INSERT INTO events VALUES(?, ?, ?, ?, ?, ?, ?)");
        query.bind(1, record.device_id);
        query.bind(2, record.event_id);
        query.bind(3, record.ts_ms);
        query.bind(4, record.lat);
        query.bind(5, record.lon);
        query.bind(6, record.speed_kph);
        query.bind(7, record.heading_deg);
        query.exec();

    }

    static void Load(SQLite::Statement &query, EventRecord &record)
    {
        record.device_id = (const char*)query.getColumn(0);
        record.event_id = (const char*)query.getColumn(1);
        record.ts_ms = query.getColumn(2);
        record.lat = query.getColumn(3);
        record.lon = query.getColumn(4);
        record.speed_kph = query.getColumn(5);
        record.heading_deg = query.getColumn(6);
    }
};
