// Progress.cpp - implementation of the save data.

#include "progress/Progress.hpp"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include "util/JsonFile.hpp"

namespace ljn {
namespace {

using Json = nlohmann::json;

constexpr int kVersion = 1;

//a record with numbers that cannot be negative or not-a-number
LevelRecord cleaned(LevelRecord record) noexcept {
    if (!std::isfinite(record.bestTime) || record.bestTime < 0.0) {
        record.bestTime = 0.0;
    }
    record.fewestDeaths = std::max(record.fewestDeaths, 0);
    record.clears = std::max(record.clears, 0);
    return record;
}

}  //anonymous namespace

const LevelRecord* Progress::find(std::string_view levelId) const noexcept {
    const auto found = m_levels.find(levelId);
    return found != m_levels.end() ? &found->second : nullptr;
}

bool Progress::isCompleted(std::string_view levelId) const noexcept {
    const LevelRecord* record = find(levelId);
    return record != nullptr && record->completed;
}

std::size_t Progress::completedCount() const noexcept {
    return static_cast<std::size_t>(
        std::ranges::count_if(m_levels, [](const auto& entry) { return entry.second.completed; }));
}

Improvement Progress::recordClear(std::string_view levelId, RunResult result) {
    result.time = std::isfinite(result.time) ? std::max(result.time, 0.0) : 0.0;
    result.deaths = std::max(result.deaths, 0);

    auto found = m_levels.find(levelId);
    if (found == m_levels.end()) {
        found = m_levels.emplace(std::string{levelId}, LevelRecord{}).first;
    }
    LevelRecord& record = found->second;

    Improvement improvement;
    if (!record.completed) {
        //the first time: everything is a record
        improvement = {.firstClear = true, .newBestTime = true, .fewerDeaths = true};
        record.completed = true;
        record.bestTime = result.time;
        record.fewestDeaths = result.deaths;
    } else {
        if (result.time < record.bestTime) {
            improvement.newBestTime = true;
            record.bestTime = result.time;
        }
        if (result.deaths < record.fewestDeaths) {
            improvement.fewerDeaths = true;
            record.fewestDeaths = result.deaths;
        }
    }
    ++record.clears;
    return improvement;
}

void Progress::set(std::string levelId, LevelRecord record) {
    m_levels[std::move(levelId)] = cleaned(record);
}

std::string progressToJson(const Progress& progress) {
    Json levels = Json::object();
    for (const auto& [id, record] : progress.levels()) {
        levels[id] = {{"completed", record.completed},
                      {"bestTime", record.bestTime},
                      {"fewestDeaths", record.fewestDeaths},
                      {"clears", record.clears}};
    }
    const Json root = {{"version", kVersion}, {"levels", levels}};
    return root.dump(2) + "\n";
}

std::expected<Progress, std::string> progressFromJson(const std::string& text) {
    const Json root = Json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        return std::unexpected("the save file is not valid json");
    }

    Progress progress;
    const auto levels = root.find("levels");
    if (levels == root.end() || !levels->is_object()) {
        return progress;
    }
    for (const auto& [id, entry] : levels->items()) {
        if (!entry.is_object()) {
            continue;
        }

        //a field with a wrong type keeps its default value
        LevelRecord record;
        if (const auto field = entry.find("completed"); field != entry.end() && field->is_boolean()) {
            record.completed = field->get<bool>();
        }
        if (const auto field = entry.find("bestTime"); field != entry.end() && field->is_number()) {
            record.bestTime = field->get<double>();
        }
        if (const auto field = entry.find("fewestDeaths"); field != entry.end() && field->is_number_integer()) {
            record.fewestDeaths = field->get<int>();
        }
        if (const auto field = entry.find("clears"); field != entry.end() && field->is_number_integer()) {
            record.clears = field->get<int>();
        }
        progress.set(id, record);
    }
    return progress;
}

LoadedProgress loadProgress(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        return {};
    }

    const auto text = readTextFile(path);
    if (!text) {
        return {.progress = {}, .problem = text.error()};
    }
    auto progress = progressFromJson(*text);
    if (!progress) {
        return {.progress = {}, .problem = progress.error()};
    }
    return {.progress = std::move(*progress), .problem = {}};
}

std::expected<void, std::string> saveProgress(const std::filesystem::path& path, const Progress& progress) {
    return writeTextAtomic(path, progressToJson(progress));
}

}  //namespace ljn
