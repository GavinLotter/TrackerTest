# Take Home Assignment: Telematics Ingestion and Reporting (C++/SQL, Cross Platform)
This is a solution to the technical vetting assignment given to me by Shanique Jooste at OfferZen on behalf of Tracker.

## Build Instructions
This solution has been implemented using GCC and CMake.  These can be installed using [MSYS2](https://www.msys2.org/).
To build the solution, first build the environment using
```bash
cmake -B build -G "Ninja" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc
```
This step should cause CMake to fetch the SQLite, nlohmann/json and SQLiteCpp dependencies.

The binaries are then built with
```bash
cmake --build build
```
This will place the two binaries ```ingest``` and ```report``` in the build directory.

## Ingest instructions
The ```ingest``` application reads JSON files and populates a table in ```db/events.db3```.  Although ```ingest``` will create the
database, the directory 'db' will need to be created before running ```ingest```.

```ingest``` is run in the project directory (not the build directory where it is placed).  It takes a single JSON file name as a
parameter.  For example:
```bash
ingest samples/set1.json
```
Where set1.json is a file in the samples directory

## Report instructions
The ```report``` application takes a report name as a parameter and any further parameters required by the specific report.  It
expects the database in ```db/events.db3``` to already exist.

Examples:
```bash
report location
```
```bash
report distance 20240101
```
```bash
report top5speed
```

All results are written in CSV format, and ```report``` returns an exit code to the shell, allowing scripts to automate it.