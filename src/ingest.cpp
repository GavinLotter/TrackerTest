#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <sqlite3.h>
#include <SQLiteCpp/SQLiteCpp.h>

#include "event.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        puts("usage: ingest <JSON file name>");
        return 1;
    }

    const char *fileName = argv[1];
    std::filesystem::path filePath = fileName;
    if (!std::filesystem::is_regular_file(filePath))
    {
        printf("unable to open file %s\n", fileName);
        return 1;
    }

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        printf("failed to open %s\n", fileName);
        return 1;
    }


    SQLite::Database db("db/events.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    EventStore::Setup(db);

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line))
    {
        EventRecord evRecord;
        lineNumber++;
        try
        {
            evRecord.Parse(line);
            EventStore::Insert(db, evRecord);
        }
        catch (const nlohmann::json::exception& e)
        {
            printf("failed: %s in line number %d\n", e.what(), lineNumber);
            return 1;
        }
    }
    printf("%d events processed\n", lineNumber);

    return 0;
}
