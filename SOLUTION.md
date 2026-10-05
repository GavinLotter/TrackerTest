# Take Home Assignment: Telematics Ingestion and Reporting (C++/SQL, Cross Platform)
This is a solution to the technical vetting assignment given to me by Shanique Jooste at OfferZen on behalf of Tracker.

## Implementation and Tooling Decisions
Although there are alternatives, I chose to use GCC and CMake as the build system for the project.  Intended to be cross-platform,
to work on Windows and Linux, using freely available, open-source software was obvious.  I also chose to use C++20 to gain access
to later range algorithms and date/time handling.  Given the ingest/report requirements, it seemed natural to build two seperate
executables, one to load JSON files into a database, and another to do the reporting.

Tools and libraries:
- [MSYS2](https://www.msys2.org/) was used to install the development tools; GCC, CMake and mingw-w64.
- [nlohmann/json](https://json.nlohmann.me/) is used for parsing JSON files.
- [SQLite](https://sqlite.org/) is used to implement a basic database.
- [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) is a thin RAII wrapper around SQLite.

## Database Schema
The limited JSON associated with the requirements document suggests only a single table is needed for ingestion, although there
are certainly other relations involving devices identified by ```device_id```.  So, SQLite is used as little more than a table
manager and query processor.

The simplicity of the source data leaves several assumptions to be made, mostly resolved by considering longevity and scale.
These are the assumptions I have made:
- device_id is globally unique
- event_id is not globally unique, but is unique inside device_id
- event_id increases within device_id and can be used to order events
- the device_id data suggest a limit of 1,000 items per device type, and are not representative patterns for all device_ids
- the event_id data suggest a limit of 10,000 events per device, and are not representative patterns for all event_ids

The sole table used in this solution is built using this SQL:
```SQL
CREATE TABLE IF NOT EXISTS
    events(device_id TEXT, event_id TEXT, ts_ms NUM, lat REAL, lon REAL, speed_kph REAL, heading_deg REAL,
           UNIQUE(device_id, event_id) ON CONFLICT IGNORE)
```
Because of the assumption that event_id is unique inside device_id, a primary key composed of device_id and event_id could be used
as the table's primary key, however SQLite does not support composite keys.  The UNIQUE constraint on the device_id/event_id pair
is used to enforce row identity, although no indices are created.  This constraint greatly simplifies application bode by
preventing duplicate event insertion.

## Geospatial Assumptions
The requirements document indicates that high-precision geo-spatial measurements are not required.  Indeed, the sample JSON
provided has english co-ordinates which would require a projection suited to England, rather than a local Mercator projection.
From spherical trigonometry, a formula to determine the angle subtended by an arc formed by two points can be coded as:
```C++
double t = cos(lat1) * cos(lon1) * cos(lat2) * cos(lon2) +
            cos(lat1) * sin(lon1) * cos(lat2) * sin(lon2) +
            sin(lat1) * sin(lat2);
double angle = acos(t);
double distance = angle * 6371000;
```
In this code, the radius of the Earth at the points is assumed to be 6,371,000m.  This is inconsistent with the actual shape of
the Earth, and will introduce inaccuracy into distance measurements.

## Performance and Space Considerations
The requirements document suggests a very low ingestion rate, a burst of ~100 events per second.  This low rate indicates makes
me disinclined to use multi-threading.  A single thread is more than capable of maintaining throughput, even with the database IO.
Indeed, performance is not limited by processing, but by file and database IO.  Separating these would be an easy way to enhance
performance.  Moreover, the events are already batched inside a file, so there is no requirement to handle any hardware events.

If a daily ingestion of 20,000 events is expected, then the ```events``` table is going to fill quite quickly, 1,000,000 records
expected over 50 days.  This will require long-term database maintenance which is beyond the scope of this solution.  It also
means that query performance may suffer when the ```ts_ms``` field is not indexed.

## Reports
All three reports from the requirements document have been implemented.  All reports are presented in CSV format, intended to ease
integration with other software.  An assumption has been made for reports involving time periods: the length of a day is taken to
be exactly 86400 seconds, leap seconds are ignored.

### Location
A report to recover the last known location of each device.  
```SQL
SELECT events.device_id, datetime(ts_ms/1000, 'unixepoch', 'localtime') DT, events.lat, events.lon
  FROM events
 INNER JOIN (SELECT device_id, MAX(ts_ms) AS max_ts FROM events GROUP BY device_ID) lastTimestamp
    ON events.device_id = lastTimestamp.device_id
   AND events.ts_ms = lastTimestamp.max_ts
```

### Distance travelled per device
This report lists all devices and the distance that they travelled on a given date.  The SQL is simple, but the distance travelled
is accumulated by the report application.
```SQL
SELECT device_id, ts_ms, lat, lon
  FROM events
 WHERE ts_ms >= <start timestamp> AND ts_ms < <end timestamp>
 ORDER BY device_id, ts_ms
```
The timestamp parameters bound the date of interest.

### Top 5 highest average speeds over the last 24 hours
The average speed of all devices is calculated over the 24-hour period ending at report run time.  Processing is very similar to
the Distance report, and the accumulation code is shared by both.  This report just sorts the results of the accumulation and
lists the five highest average speeds.
```SQL                           
SELECT device_id, ts_ms, lat, lon
  FROM events
 WHERE ts_ms >= <start timestamp>
 ORDER BY device_id, ts_ms
``` 
The start timestamp is 24 hours before current time.

If there is no movement between two consecutive events, no change in lat/long, then the time between the events is not accumulated
in the time spent - time is ignored when the device is standing still.

## Notes on C++ styling
It should be noticed that I use C-style casts, instead of the modern C++ template-styled casts.  My reasoning is simply that I
consider the modern casts to be unwieldy and harder to read, with scant benefit.  The only cast that I recognise the benefit in is
```dynamic_cast<>```, and ```duration_cast<>``` is required by std::chrono functions.

I've used ```printf()``` and ```puts()``` instead of the C++ console IO streams.  My reasoning is that formatting is much easier
with the old C functions than the verbose and unwieldy console stream.  Unfortunately, the even-more-modern std::print methods
are only available with C++23.

## AI Declaration
The suggested 4-hour completion time for this solution strongly suggests that use of AI tools is expected.  While I did use AI to
field specific questions about specific tools, I did not use any AI in the creation of any of the code or documentation in this
assignment.  However, as I have no experience with Azure, I used Gemini to make a template for ```azure-pipelines.yml```.
- Gavin Lotter 2026-10-06
