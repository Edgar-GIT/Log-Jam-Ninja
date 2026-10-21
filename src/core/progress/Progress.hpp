// Progress.hpp - what the player has done: which levels were finished, with which time and how many deaths.

#pragma once

#include <expected>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace ljn {

//the best results of one level
struct LevelRecord {
    bool completed{false};

    //seconds of the fastest run and the fewest deaths of any run, only meaningful when completed
    double bestTime{0.0};
    int fewestDeaths{0};

    //how many times the level was finished
    int clears{0};

    [[nodiscard]] bool operator==(const LevelRecord&) const noexcept = default;
};

//the result of one run through a level
struct RunResult {
    double time{0.0};
    int deaths{0};
};

//what a finished run improved
struct Improvement {
    bool firstClear{false};
    bool newBestTime{false};
    bool fewerDeaths{false};
};

//the records of all levels, the key is the level id (the file name without .json)
class Progress {
public:
    //null if the level was never finished
    [[nodiscard]] const LevelRecord* find(std::string_view levelId) const noexcept;
    [[nodiscard]] bool isCompleted(std::string_view levelId) const noexcept;
    [[nodiscard]] std::size_t completedCount() const noexcept;

    //saves the result of a finished run and says what got better
    Improvement recordClear(std::string_view levelId, RunResult result);

    //forgets everything
    void clear() noexcept { m_levels.clear(); }

    [[nodiscard]] const std::map<std::string, LevelRecord, std::less<>>& levels() const noexcept { return m_levels; }

    //used when loading a file, the numbers are made safe (no negative time or deaths)
    void set(std::string levelId, LevelRecord record);

    [[nodiscard]] bool operator==(const Progress&) const noexcept = default;

private:
    std::map<std::string, LevelRecord, std::less<>> m_levels;
};

[[nodiscard]] std::string progressToJson(const Progress& progress);

//wrong entries are skipped, the only error is text that is not json at all
[[nodiscard]] std::expected<Progress, std::string> progressFromJson(const std::string& text);

struct LoadedProgress {
    Progress progress;
    std::string problem;
};

//a file that does not exist is normal (first run)
[[nodiscard]] LoadedProgress loadProgress(const std::filesystem::path& path);
[[nodiscard]] std::expected<void, std::string> saveProgress(const std::filesystem::path& path, const Progress& progress);

}  //namespace ljn
