#ifndef DATABASE_H
#define DATABASE_H
#include <string>
#include <vector>

struct Song
{
    int id;
    std::string title;
    std::string artist;
    std::string filepath;
    std::string format;
    double duration;
    double sizeKB;
    std::string addedAt;
    int playCount;
};

struct Playlist
{
    int id;
    std::string name;
};

bool db_init(const std::string &path = "music.db");

int db_addPlaylist(const std::string &name);

int db_addSong(const std::string &title,
               const std::string &artist,
               const std::string &filepath,
               const std::string &format,
               double duration,
               double sizeKB);

bool db_linkSongToPlaylist(int playlist_id, int song_id);

std::vector<Playlist> db_loadPlaylists();

std::vector<Song> db_loadSongsForPlaylist(int playlist_id);

bool db_removeSong(int song_id);

bool db_removePlaylist(int playlist_id);

bool db_unlinkSongFromPlaylist(int playlist_id, int song_id);

void db_close();

#endif 