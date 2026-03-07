#include "database.h"
#include "sqlite3.h" 
#include <iostream>
#include <ctime>

static sqlite3 *db = nullptr;

bool db_init(const std::string &path)
{
    int rc = sqlite3_open(path.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    char *errMsg = nullptr;
    rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to enable foreign keys: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    const char *sql_create_tables = R"(
        CREATE TABLE IF NOT EXISTS playlists (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE
        );

        CREATE TABLE IF NOT EXISTS songs (
            id INTEGER PRIMARY KEY,
            title TEXT,
            artist TEXT,
            filepath TEXT UNIQUE,
            format TEXT,
            duration REAL,
            size_kb REAL,
            added_at TEXT,
            play_count INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS playlist_songs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            playlist_id INTEGER,
            song_id INTEGER,
            FOREIGN KEY(playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,
            FOREIGN KEY(song_id) REFERENCES songs(id) ON DELETE CASCADE
        );
    )";

    // Reuse errMsg variable
    rc = sqlite3_exec(db, sql_create_tables, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

int db_addPlaylist(const std::string &name)
{
    const char *sql = "INSERT OR IGNORE INTO playlists (name) VALUES (?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    // Check if inserted or ignored
    if (sqlite3_changes(db) == 0)
    {
        // Already exists, retrieve ID
        const char *sql_select = "SELECT id FROM playlists WHERE name = ?;";
        sqlite3_stmt *stmt_select;

        if (sqlite3_prepare_v2(db, sql_select, -1, &stmt_select, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt_select, 1, name.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt_select) == SQLITE_ROW)
            {
                int id = sqlite3_column_int(stmt_select, 0);
                sqlite3_finalize(stmt_select);
                return id;
            }
            sqlite3_finalize(stmt_select);
        }
        return -1;
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

int db_addSong(const std::string &title,
               const std::string &artist,
               const std::string &filepath,
               const std::string &format,
               double duration,
               double  sizeKB)
{


    // Get current timestamp
    time_t now = time(nullptr);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    const char *sql = R"(
        INSERT OR IGNORE INTO songs (title, artist, filepath, format, duration, size_kb, added_at, play_count)
        VALUES (?, ?, ?, ?, ?, ?, ?, 0);
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artist.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, filepath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, format.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, duration);
    sqlite3_bind_double(stmt, 6, sizeKB);
    sqlite3_bind_text(stmt, 7, timestamp, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    // Check if inserted or ignored
    if (sqlite3_changes(db) == 0)
    {
        // Already exists, retrieve ID
        const char *sql_select = "SELECT id FROM songs WHERE filepath = ?;";
        sqlite3_stmt *stmt_select;

        if (sqlite3_prepare_v2(db, sql_select, -1, &stmt_select, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt_select, 1, filepath.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt_select) == SQLITE_ROW)
            {
                int id = sqlite3_column_int(stmt_select, 0);
                sqlite3_finalize(stmt_select);
                return id;
            }
            sqlite3_finalize(stmt_select);
        }
        return -1;
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

bool db_linkSongToPlaylist(int playlist_id, int song_id)
{
    const char *sql = "INSERT OR IGNORE INTO playlist_songs (playlist_id, song_id) VALUES (?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, playlist_id);
    sqlite3_bind_int(stmt, 2, song_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<Playlist> db_loadPlaylists()
{
    std::vector<Playlist> playlists;
    const char *sql = "SELECT id, name FROM playlists ORDER BY id;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return playlists;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Playlist p;
        p.id = sqlite3_column_int(stmt, 0);
        p.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        playlists.push_back(p);
    }

    sqlite3_finalize(stmt);
    return playlists;
}

std::vector<Song> db_loadSongsForPlaylist(int playlist_id)
{
    std::vector<Song> songs;
    const char *sql = R"(
        SELECT s.id, s.title, s.artist, s.filepath, s.format, s.duration, s.size_kb, s.added_at, s.play_count
        FROM songs s
        JOIN playlist_songs ps ON s.id = ps.song_id
        WHERE ps.playlist_id = ?
        ORDER BY ps.id;
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return songs;
    }

    sqlite3_bind_int(stmt, 1, playlist_id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Song s;
        s.id = sqlite3_column_int(stmt, 0);
        s.title = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        s.artist = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        s.filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        s.format = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        s.duration = sqlite3_column_double(stmt, 5);
        s.sizeKB = sqlite3_column_double(stmt, 6);
        s.addedAt = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
        s.playCount = sqlite3_column_int(stmt, 8);
        songs.push_back(s);
    }

    sqlite3_finalize(stmt);
    return songs;
}

bool db_removeSong(int song_id)
{
    const char *sql = "DELETE FROM songs WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, song_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool db_removePlaylist(int playlist_id)
{
    const char *sql = "DELETE FROM playlists WHERE id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, playlist_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool db_unlinkSongFromPlaylist(int playlist_id, int song_id)
{
    const char *sql = "DELETE FROM playlist_songs WHERE playlist_id = ? AND song_id = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, playlist_id);
    sqlite3_bind_int(stmt, 2, song_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        // Check if this song is in any other playlist
        const char *check_sql = "SELECT COUNT(*) FROM playlist_songs WHERE song_id = ?;";
        sqlite3_stmt *check_stmt;

        if (sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(check_stmt, 1, song_id);
            if (sqlite3_step(check_stmt) == SQLITE_ROW)
            {
                int count = sqlite3_column_int(check_stmt, 0);
                sqlite3_finalize(check_stmt);

                // If song is not in any other playlist, delete it completely
                if (count == 0)
                {
                    std::cout << "Song not in any other playlist, removing from database..." << std::endl;
                    db_removeSong(song_id);
                }
            }
            else
            {
                sqlite3_finalize(check_stmt);
            }
        }
    }

    return rc == SQLITE_DONE;
}

void db_close()
{
    if (db)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}