#include <iostream>
using namespace std;
#include <SFML/Audio.hpp>
#include <cmath>
#include <conio.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#define byte win_byte_override
#include "database.h"
#include <windows.h>

namespace fs = std::filesystem;

// ==========================================
//              Helper Functions
// ==========================================
double getFileSize(const string &path)
{
    try
    {
        if (fs::exists(path))
        {
            return static_cast<double>(fs::file_size(path)) / 1024.0; // KB
        }
    }
    catch (...)
    {
    }
    return 0;
}

string getFileExtension(const string &path)
{
    size_t pos = path.find_last_of('.');
    if (pos != string::npos)
    {
        return path.substr(pos + 1);
    }
    return "unknown";
}
//  function to clean file paths
string cleanPath(const string &str)
{
    if (str.empty())
        return str;

    string result = str;

    // Remove leading quotes and spaces
    while (!result.empty() && (result.front() == '"' || result.front() == '\'' ||
                               result.front() == ' '))
    {
        result.erase(0, 1);
    }

    // Remove trailing quotes and spaces
    while (!result.empty() && (result.back() == '"' || result.back() == '\'' ||
                               result.back() == ' '))
    {
        result.pop_back();
    }

    return result;
}

// ==========================================
//                Audio Class
// ==========================================
class Audio
{
private:
    string name;
    string type;
    string path;
    string artist;
    int duration;
    sf::Music music;
    int db_id; // Database ID

public:
    // Default constructor
    Audio() : name(""), type(""), path(""), artist(""), duration(0), db_id(-1) {}

    Audio(string name, string type, string path, string artist, int duration,
          int db_id = -1)
    {
        this->name = name;
        this->type = type;
        this->path = path;
        this->artist = artist;
        this->duration = duration;
        this->db_id = db_id;
    }

    Audio(const Audio &other)
    {
        name = other.name;
        type = other.type;
        path = other.path;
        artist = other.artist;
        duration = other.duration;
        db_id = other.db_id;
    }
    string getName() const { return name; }
    string getType() const { return type; }
    string getPath() const { return path; }
    string getArtist() const { return artist; }
    int getDuration() const { return duration; }
    int getDbId() const { return db_id; }
    void setDbId(int id) { db_id = id; }

    void play()
    {
        if (music.getStatus() == sf::Music::Status::Paused)
        {
            music.play();
            cout << "Playing: " << name << " - " << artist << endl;
            return;
        }

        if (!path.empty())
        {
            if (!music.openFromFile(path)) // open audio file from path and prepare it
            {
                cerr << "\n========================================" << endl;
                cerr << "ERROR: Could not load audio file!" << endl;
                cerr << "File: " << path << endl;
                cerr << "Tips:" << endl;
                cerr << "  - Make sure file exists" << endl;
                cerr << "  - Supported formats: .ogg, .wav" << endl;
                cerr << "  - Try using forward slashes: C:/folder/file.wav" << endl;
                cerr << "========================================" << endl;
                return;
            }
        }

        music.play();
        cout << "Playing: " << name << " - " << artist << endl;
    }

    void pause()
    {
        if (music.getStatus() == sf::Music::Status::Playing)
        {
            music.pause();
            cout << "Paused at: " << music.getPlayingOffset().asSeconds() << "s"
                 << endl;
        }
    }

    void resume()
    {
        if (music.getStatus() == sf::Music::Status::Paused)
        {
            music.play();
            cout << "Resumed from: " << music.getPlayingOffset().asSeconds() << "s"
                 << endl;
        }
    }

    void stop()
    {
        if (music.getStatus() != sf::Music::Status::Stopped)
        {
            music.stop();
            cout << "Stopped: " << name << endl;
        }
    }

    sf::Music::Status getStatus() const { return music.getStatus(); }

    void setLoop(bool enable) { music.setLooping(enable); }
    bool isLoop() const { return music.isLooping(); }

    void setVolume(float vol)
    {
        if (vol < 0.f)
            vol = 0.f;
        if (vol > 100.f)
            vol = 100.f;
        music.setVolume(vol);
    }

    float getVolume() const { return music.getVolume(); }

    void changeVolume(float delta)
    {
        float v = music.getVolume() + delta;
        if (v < 0.f)
            v = 0.f;
        if (v > 100.f)
            v = 100.f;
        v = std::round(v);
        music.setVolume(v);
    }
};

// ==========================================
//               Node
// ==========================================
struct AudioNode
{
    Audio data;
    AudioNode *next;
    AudioNode *prev;

    AudioNode(const Audio &audio) : data(audio), next(nullptr), prev(nullptr) {}
};

// ==========================================
//               Audio List
// ==========================================
class AudioList
{
private:
    AudioNode *head;
    AudioNode *tail;
    AudioNode *current;
    int size;

public:
    AudioList() : head(nullptr), tail(nullptr), current(nullptr), size(0) {}

    ~AudioList()
    {
        AudioNode *temp = head;
        while (temp)
        {
            AudioNode *next = temp->next;
            delete temp;
            temp = next;
        }
    }

    void addAudio(const Audio &audio)
    {
        AudioNode *newnode = new AudioNode(audio);

        if (head == nullptr)
        {
            head = tail = newnode;
        }
        else
        {
            tail->next = newnode;
            newnode->prev = tail;
            tail = newnode;
        }
        size++;
        cout << "Added: " << audio.getName() << endl;
    }

    void removeAudio(string name)
    {
        AudioNode *temp = head;

        while (temp != nullptr)
        {
            if (temp->data.getName() == name)
            {

                if (temp->prev != nullptr)
                {
                    temp->prev->next = temp->next;
                }

                if (temp->next != nullptr)
                {
                    temp->next->prev = temp->prev;
                }

                if (temp == head)
                {
                    head = temp->next;
                }

                if (temp == tail)
                {
                    tail = temp->prev;
                }

                if (temp == current)
                {
                    temp->data.stop();
                    current = nullptr;
                }

                delete temp;
                size--;

                cout << "Removed: " << name << endl;
                return;
            }

            temp = temp->next;
        }

        cout << "Song not found: " << name << endl;
    }

    AudioNode *searchAudio(string name)
    {
        AudioNode *temp = head;

        while (temp != nullptr)
        {
            if (temp->data.getName() == name)
            {
                return temp;
            }
            temp = temp->next;
        }

        return nullptr;
    }

    void displayList()
    {
        if (head == nullptr)
        {
            cout << "Playlist is empty" << endl;
            return;
        }

        AudioNode *temp = head;
        int index = 1;

        cout << "PLAYLIST" << endl;

        while (temp != nullptr)
        {
            cout << index++ << ". " << temp->data.getName();
            cout << " - " << temp->data.getArtist();
            cout << " (" << temp->data.getDuration() << "s)";

            if (temp == current)
            {
                cout << " [NOW PLAYING]";
            }

            cout << endl;
            temp = temp->next;
        }
        cout << endl;
    }

    void playNext()
    {
        if (head == nullptr)
        {
            cout << "Playlist is empty!" << endl;
            return;
        }

        if (current == nullptr)
        {
            current = head;
        }
        else
        {
            if (current->next != nullptr)
            {
                current->data.stop();
                current = current->next;
            }
            else
            {
                cout << "This is the last track! Looping back to first." << endl;
                current->data.stop();
                current = head;
            }
        }

        current->data.play();
        cout << " Next: " << current->data.getName() << endl;
    }

    void playPrevious()
    {
        if (head == nullptr)
        {
            cout << "Playlist is empty!" << endl;
            return;
        }

        if (current == nullptr)
        {
            current = tail;
        }
        else
        {

            if (current->prev != nullptr)
            {
                current->data.stop();
                current = current->prev;
            }
            else
            {
                cout << "This is the first track, going to last." << endl;
                current->data.stop();
                current = tail;
            }
        }

        current->data.play();
        cout << "Previous: " << current->data.getName() << endl;
    }

    void pause()
    {
        if (current)
        {
            current->data.pause();
        }
        else
        {
            cout << "No track is playing!" << endl;
        }
    }

    void resume()
    {
        if (current)
        {
            current->data.resume();
        }
        else
        {
            cout << "No track to resume!" << endl;
        }
    }

    void stop()
    {
        if (current)
        {
            current->data.stop();
            current = nullptr;
        }
    }

    void playWithControls()
    {
        if (head == nullptr)
        {
            cout << "No track to play!" << endl;
            return;
        }

        if (current == nullptr)
            current = head;

        current->data.play();

        if (current->data.getStatus() == sf::Music::Status::Stopped)
        {
            cout << "ERROR: Could not play track: " << current->data.getName()
                 << endl;
            cout << "File path: " << current->data.getPath() << endl;
            cout << "Press any key to return to menu..." << endl;
            _getch();
            return;
        }

        cout << "\n─── Controls: ← prev │ → next │ ↑ pause │ ↓ resume │ x restart "
                "│ o loop │ + vol up │ - vol down │ q quit ───\n"
             << endl;

        bool exitMode = false;
        bool loopOne = false;

        while (!exitMode)
        {
            if (current && current->data.getStatus() == sf::Music::Status::Stopped &&
                !loopOne)
            {
                AudioNode *startNode = current;
                bool foundPlayable = false;

                do
                {
                    if (current->next)
                        current = current->next;
                    else
                    {
                        // Reached end of playlist
                        cout << "\n═══ End of Playlist ═══" << endl;
                        cout << "Press any key to return to menu..." << endl;
                        exitMode = true;
                        break;
                    }

                    current->data.play();

                    if (current->data.getStatus() != sf::Music::Status::Stopped)
                    {
                        foundPlayable = true;
                        cout << "\nNow playing: " << current->data.getName() << endl;
                        break;
                    }

                    cout << "Skipping unplayable track: " << current->data.getName()
                         << endl;

                } while (current != startNode && !exitMode);

                if (!foundPlayable && !exitMode)
                {
                    cout << "\nERROR: No more playable tracks. Exiting..." << endl;
                    exitMode = true;
                }
            }

            if (current && current->data.getStatus() == sf::Music::Status::Stopped &&loopOne)
            {
                current->data.stop();
                current->data.play();
                if (current->data.getStatus() == sf::Music::Status::Stopped)
                {
                    cout << "\nERROR: Could not loop track. Exiting..." << endl;
                    exitMode = true;
                }
            }

            if (_kbhit())
            {
                int ch = _getch();

                // Check for arrow keys (they send 224 first, then direction code)
                if (ch == 224 || ch == 0)
                {
                    int code = _getch();
                    if (code == 75)
                    {
                        cout << "[Key] Left arrow -> Previous" << endl;
                        playPrevious();
                    }
                    else if (code == 77)
                    {
                        cout << "[Key] Right arrow -> Next" << endl;
                        playNext();
                    }
                    else if (code == 72)
                    {
                        cout << "[Key] Up arrow -> Pause" << endl;
                        pause();
                    }
                    else if (code == 80)
                    {
                        cout << "[Key] Down arrow -> Resume" << endl;
                        resume();
                    }
                }
                else
                {
                    char key = tolower(ch);

                    if (key == 'q')
                    {
                        cout << "[Key] Q -> Quit playback" << endl;
                        stop();
                        exitMode = true;
                    }
                    else if (key == 'p')
                    {
                        cout << "[Key] P -> Pause" << endl;
                        pause();
                    }
                    else if (key == 'r')
                    {
                        cout << "[Key] R -> Resume" << endl;
                        resume();
                    }
                    else if (key == 'x')
                    {
                        cout << "[Key] X -> Restart current track" << endl;
                        if (current)
                        {
                            current->data.stop();
                            current->data.play();
                            cout << "Restarted: " << current->data.getName() << endl;
                        }
                        else
                            cout << "No track to restart!" << endl;
                    }
                    else if (key == 'o')
                    {
                        loopOne = !loopOne;
                        if (current)
                            current->data.setLoop(loopOne);
                        cout << "[Key] O -> Loop mode "
                             << (loopOne ? "ENABLED " : "DISABLED ") << endl;
                    }
                    else if (ch == '+' || ch == '=')
                    {
                        if (current)
                        {
                            current->data.changeVolume(5);
                            cout << "[Key] + -> Volume: " << (int)current->data.getVolume()
                                 << "%" << endl;
                        }
                    }
                    else if (ch == '-' || ch == '_')
                    {
                        if (current)
                        {
                            current->data.changeVolume(-5);
                            cout << "[Key] - -> Volume: " << (int)current->data.getVolume()
                                 << "%" << endl;
                        }
                    }
                }
            }

            sf::sleep(sf::milliseconds(100));
        }

        cout << "\nExiting playback controls.\n";
    }

    AudioNode *getCurrent() const { return current; }
    int getSize() const { return size; }
    bool isEmpty() const { return head == nullptr; }
    AudioNode *getHead() const { return head; }
};

// ==========================================
//             Playlist Node
// ==========================================
struct PlaylistNode
{
    string playlistName;
    int db_id;
    AudioList tracks;
    PlaylistNode *next;
    PlaylistNode *prev;

    PlaylistNode(string name, int db_id = -1)
        : playlistName(name), db_id(db_id), next(nullptr), prev(nullptr) {}
};

// ==========================================
//            Playlist Manager
// ==========================================

class PlaylistManager
{
private:
    PlaylistNode *head;
    PlaylistNode *tail;
    PlaylistNode *currentPlaylist;

public:
    PlaylistManager() : head(nullptr), tail(nullptr), currentPlaylist(nullptr) {}

    ~PlaylistManager()
    {
        PlaylistNode *temp = head;
        while (temp)
        {
            PlaylistNode *next = temp->next;
            delete temp;
            temp = next;
        }
    }

    void createPlaylist(string name)
    {
        PlaylistNode *check = head;
        while (check)
        {
            if (check->playlistName == name)
            {
                cout << "Playlist " << name << " already exists" << endl;
                return;
            }
            check = check->next;
        }

        // Add to database
        int playlist_id = db_addPlaylist(name);
        if (playlist_id < 0)
        {
            cout << "Failed to create playlist in database" << endl;
            return;
        }

        PlaylistNode *newPlaylist = new PlaylistNode(name, playlist_id);

        if (head == nullptr)
        {
            head = tail = newPlaylist;
            currentPlaylist = newPlaylist;
        }
        else
        {
            tail->next = newPlaylist;
            newPlaylist->prev = tail;
            tail = newPlaylist;
        }

        cout << "Playlist " << name
             << " created successfully (DB ID: " << playlist_id << ")" << endl;
    }

    void deletePlaylist(string name)
    {
        PlaylistNode *temp = head;

        while (temp != nullptr)
        {
            if (temp->playlistName == name)
            {
                // Remove from database
                if (temp->db_id >= 0)
                {
                    db_removePlaylist(temp->db_id);
                }

                if (temp == currentPlaylist)
                {
                    if (temp->next != nullptr)
                        currentPlaylist = temp->next;
                    else if (temp->prev != nullptr)
                        currentPlaylist = temp->prev;
                    else
                        currentPlaylist = nullptr;
                }

                if (temp->prev != nullptr)
                    temp->prev->next = temp->next;

                if (temp->next != nullptr)
                    temp->next->prev = temp->prev;

                if (temp == head)
                    head = temp->next;

                if (temp == tail)
                    tail = temp->prev;

                delete temp;
                cout << "Playlist " << name << " deleted" << endl;
                return;
            }
            temp = temp->next;
        }

        cout << "Playlist " << name << " not found" << endl;
    }

    void selectPlaylist(string name)
    {
        PlaylistNode *temp = head;

        while (temp != nullptr)
        {
            if (temp->playlistName == name)
            {
                currentPlaylist = temp;
                cout << "Selected playlist: " << name << endl;
                return;
            }
            temp = temp->next;
        }

        cout << "Playlist " << name << " not found!" << endl;
    }

    void displayAllPlaylists()
    {
        if (head == nullptr)
        {
            cout << "No playlists available!" << endl;
            return;
        }

        PlaylistNode *temp = head;
        int index = 1;

        cout << " ALL PLAYLISTS " << endl;

        while (temp != nullptr)
        {
            cout << index++ << ". " << temp->playlistName;
            cout << " (" << temp->tracks.getSize() << " tracks)";

            if (temp == currentPlaylist)
            {
                cout << " [CURRENT]";
            }

            cout << endl;
            temp = temp->next;
        }
    }

    void addTrackToCurrent(const Audio &audio)
    {
        if (currentPlaylist == nullptr)
        {
            cout << "No playlist selected! Create or select a playlist first."
                 << endl;
            return;
        }

        // Test if file can be loaded before adding to database
        sf::Music testMusic;
        if (!testMusic.openFromFile(audio.getPath()))
        {
            cerr << "\n========================================" << endl;
            cerr << "ERROR: Cannot add track - file not accessible!" << endl;
            cerr << "File: " << audio.getPath() << endl;
            cerr << "\nPossible issues:" << endl;
            cerr << "  1. File doesn't exist at this path" << endl;
            cerr << "  2. File format not supported (use .ogg or .flac)" << endl;
            cerr << "  3. WAV file encoding not compatible" << endl;
            cerr << "\nRecommendation: Convert to .ogg format" << endl;
            cerr << "========================================\n"
                 << endl;
            return;
        }
        // Extract metadata
        double sizeKB = getFileSize(audio.getPath());
        string format = getFileExtension(audio.getPath());

        // Add to database
        int song_id =
            db_addSong(audio.getName(), audio.getArtist(), audio.getPath(), format,
                       static_cast<double>(audio.getDuration()), sizeKB);

        if (song_id < 0)
        {
            cout << "Failed to add song to database" << endl;
            return;
        }

        // Link to playlist
        if (!db_linkSongToPlaylist(currentPlaylist->db_id, song_id))
        {
            cout << "Failed to link song to playlist" << endl;
            return;
        }

        // Create new Audio with db_id
        Audio newAudio(audio.getName(), audio.getType(), audio.getPath(),
                       audio.getArtist(), audio.getDuration(), song_id);

        currentPlaylist->tracks.addAudio(newAudio);
        cout << " Track added successfully to database with ID: " << song_id
             << endl;
    }

    void removeTrackFromCurrent(string trackName)
    {
        if (currentPlaylist == nullptr)
        {
            cout << "No playlist selected" << endl;
            return;
        }

        // Find the song in memory to get its db_id
        AudioNode *node = currentPlaylist->tracks.searchAudio(trackName);
        if (node && node->data.getDbId() >= 0)
        {
            // Unlink from playlist in DB
            db_unlinkSongFromPlaylist(currentPlaylist->db_id, node->data.getDbId());
        }

        currentPlaylist->tracks.removeAudio(trackName);
    }

    void playCurrentPlaylist()
    {
        if (currentPlaylist == nullptr)
        {
            cout << "No playlist selected" << endl;
            return;
        }

        if (currentPlaylist->tracks.isEmpty())
        {
            cout << "Playlist is empty!" << endl;
            return;
        }

        cout << "Playing playlist: " << currentPlaylist->playlistName << endl;
        currentPlaylist->tracks.playWithControls();
    }

    void displayCurrentPlaylist()
    {
        if (currentPlaylist == nullptr)
        {
            cout << "No playlist selected!" << endl;
            return;
        }

        cout << "\nCurrent Playlist: " << currentPlaylist->playlistName << endl;
        currentPlaylist->tracks.displayList();
    }

    void playNext()
    {
        if (currentPlaylist == nullptr)
        {
            cout << "No playlist selected!" << endl;
            return;
        }
        currentPlaylist->tracks.playNext();
    }

    void playPrevious()
    {
        if (currentPlaylist == nullptr)
        {
            cout << "No playlist selected!" << endl;
            return;
        }
        currentPlaylist->tracks.playPrevious();
    }

    string getCurrentPlaylistName() const
    {
        if (currentPlaylist)
            return currentPlaylist->playlistName;
        return "None";
    }
    

    // ==========================================
    //    Load playlists from database
    // ==========================================
    void loadFromDatabase()
    {
        vector<Playlist> playlists = db_loadPlaylists();

        for (const auto &pl : playlists)
        {
            PlaylistNode *newPlaylist = new PlaylistNode(pl.name, pl.id);

            if (head == nullptr)
            {
                head = tail = newPlaylist;
                currentPlaylist = newPlaylist;
            }
            else
            {
                tail->next = newPlaylist;
                newPlaylist->prev = tail;
                tail = newPlaylist;
            }

            // Load songs for this playlist
            vector<Song> songs = db_loadSongsForPlaylist(pl.id);
            for (const auto &song : songs)
            {
                Audio audio(song.title, song.format, song.filepath, song.artist,
                            static_cast<int>(song.duration), song.id);
                newPlaylist->tracks.addAudio(audio);
            }

            cout << "Loaded playlist: " << pl.name << " with " << songs.size()
                 << " tracks" << endl;
        }
    }
    // ==================================================
    // 9. Save playlist to file (simple version)
    // ==================================================
    void savePlaylistToFile()
    {
        if (currentPlaylist == nullptr)
        {
            cout << "No playlist selected!" << endl;
            return;
        }

        string filename;
        cout << "\nEnter filename to save: ";
        getline(cin, filename);

        ofstream out(filename);
        if (!out.is_open())
        {
            cout << "Error opening file!" << endl;
            return;
        }

        // Write playlist name in first line
        out << currentPlaylist->playlistName << "\n";

        // Write tracks (name|artist|path|type|duration)
        AudioNode *temp = currentPlaylist->tracks.getHead();
        while (temp != nullptr)
        {
            out << temp->data.getName() << "|"
                << temp->data.getArtist() << "|"
                << temp->data.getPath() << "|"
                << temp->data.getType() << "|"
                << temp->data.getDuration() << "\n";

            temp = temp->next;
        }

        out.close();
        cout << "Playlist saved successfully.\n";
    }

    void loadPlaylistFromFile()
    {
        if (currentPlaylist == nullptr)
        {
            cout << "No playlist selected!" << endl;
            return;
        }

        string filename;
        cout << "Enter filename to load: ";
        getline(cin, filename);

        ifstream in(filename);
        if (!in.is_open())
        {
            cout << "File not found!" << endl;
            return;
        }

        string line;

        // skip first line (playlist name)
        getline(in, line);

        // read tracks ONLY
        while (getline(in, line))
        {
            if (line.empty())
                continue;

            stringstream ss(line);
            string name, artist, path, type, durationStr;

            getline(ss, name, '|');
            getline(ss, artist, '|');
            getline(ss, path, '|');
            getline(ss, type, '|');
            getline(ss, durationStr, '|');

            int duration = stoi(durationStr);

            currentPlaylist->tracks.addAudio(
                Audio(name, type, path, artist, duration, -1));
        }

        in.close();
        cout << "Loaded." << endl;
    }
};

void displayMenu()
{
    cout << R"(
═══════════════════════════════════════════════════════════════════════════════
│                                                                             │
│                     Audio Playlist Manager - Main Menu                      │
│                                                                             │
═══════════════════════════════════════════════════════════════════════════════
│                                                                             │
│  1.  Create a new playlist                                                  │
│  2.  Delete an existing playlist                                            │
│  3.  Select a playlist to work with                                         │
│  4.  View all available playlists                                           │
│  5.  Display tracks in current playlist                                     │
│  6.  Add a track to current playlist                                        │
│  7.  Remove a track from current playlist                                   │
│  8.  Play current playlist with interactive controls                        │
│       Controls during playback:                                             │
│       • Left Arrow  (←) : Skip to previous track                            │
│       • Right Arrow (→) : Skip to next track                                │
│       • Up Arrow    (↑) : Pause playback                                    │
│       • Down Arrow  (↓) : Resume playback                                   │
│       • X key           : Restart current track                             │
│       • O key           : Toggle loop mode for current track                │
│       • + key           : Increase volume                                   │
│       • - key           : Decrease volume                                   │
│       • Q key           : Exit playback and return to menu                  │
│  9.  Save playlist to file                                                  │
│  10. Load playlist from file                                                │
│  11. Search for a track by name in current playlist                         │
│  12. Exit application                                                       │
│                                                                             │
═══════════════════════════════════════════════════════════════════════════════
)";
    cout << "\nYour choice: ";
}

int main()
{
    // Set console to UTF-8 encoding for proper Unicode display
   SetConsoleOutputCP(CP_UTF8);
   SetConsoleCP(CP_UTF8);

    // Initialize database
    if (!db_init("music.db"))
    {
        cerr << "Failed to initialize database. Exiting." << endl;
        return 1;
    }
    cout << "Database initialized successfully." << endl;

    PlaylistManager manager;

    // Load existing data from database
    cout << "\nLoading playlists from database..." << endl;
    manager.loadFromDatabase();
    cout << "Data loaded successfully.\n"<< endl;

    int choice;
    string playlistName, trackName, trackPath, artistName, trackType;
    int trackDuration;

    while (true)
    {
        displayMenu();
        cin >> choice;
        cin.ignore();

        switch (choice)
        {
        case 1:
            cout << "\nEnter new playlist name: ";
            getline(cin, playlistName);
            manager.createPlaylist(playlistName);
            break;

        case 2:
            cout << "\nEnter playlist name to delete: ";
            getline(cin, playlistName);
            manager.deletePlaylist(playlistName);
            break;

        case 3:
            cout << "\nEnter playlist name to select: ";
            getline(cin, playlistName);
            manager.selectPlaylist(playlistName);
            break;

        case 4:
            cout << "\n";
            manager.displayAllPlaylists();
            break;

        case 5:
            cout << "\n";
            manager.displayCurrentPlaylist();
            break;

        case 6: // Add track
            cout << "\n── Add New Track ──\n";
            cout << "Track name: ";
            getline(cin, trackName);
            cout << "Artist name: ";
            getline(cin, artistName);
            cout << "File path: ";
            getline(cin, trackPath);

            // Clean the path (remove quotes)
            trackPath = cleanPath(trackPath);
            cout << "Cleaned path: " << trackPath << endl; // للتأكد

            cout << "Track type (e.g., Music, Podcast): ";
            getline(cin, trackType);
            cout << "Duration in seconds: ";
            cin >> trackDuration;
            cin.ignore();

            {
                Audio newTrack(trackName, trackType, trackPath, artistName,
                               trackDuration);
                manager.addTrackToCurrent(newTrack);
            }
            break;
        case 7:
            cout << "\nEnter track name to remove: ";
            getline(cin, trackName);
            manager.removeTrackFromCurrent(trackName);
            break;

        case 8:
            cout << "\n";
            manager.playCurrentPlaylist();
            break;

        case 9: // Save playlist to file
            manager.savePlaylistToFile();
            break;

        case 10: // Load playlist from file
            manager.loadPlaylistFromFile();
            break;

        case 11:
            cout << "\nEnter track name to search: ";
            getline(cin, trackName);
            if (manager.getCurrentPlaylistName() == "None")
            {
                cout << "\n No playlist selected!\n";
            }
            else
            {
                manager.displayCurrentPlaylist();
                cout << "\n Search for: " << trackName << endl;
                cout << "Check the list above manually.\n";
            }
            break;

        case 12:
            cout << "\nClosing database and exiting...\n";
            db_close();
            cout << "Thank you for using Audio Playlist Manager. Goodbye!\n";
            return 0;

        default:
            cout << "\nInvalid choice! Please select a number between 1 and 12.\n";
            break;
        }

        cout << "\nPress any key to continue...";
        _getch();
    }

    return 0;
}
/*
sound1 name : 
sound1 path : "C:\Users\Mohamed Ashraf\Music\Quarn\abdelbaset.ogg"

*/