// progress_test.cpp - checks the save data: the records of each level and the file they live in.

#include <print>
#include "check.hpp"
#include "progress/Progress.hpp"

using namespace ljn;

namespace {

//a folder for the temporary files of the test
std::filesystem::path tempFolder() {
    return std::filesystem::temp_directory_path() / "ljn_progress_test";
}

//a new progress has no levels
static void testEmpty() {
    const Progress progress;

    CHECK(progress.levels().empty());
    CHECK(progress.completedCount() == 0);
    CHECK(!progress.isCompleted("level_01"));
    CHECK(progress.find("level_01") == nullptr);
}

//the first time a level is finished everything is a record
static void testFirstClear() {
    Progress progress;

    const Improvement improvement = progress.recordClear("level_01", {.time = 42.5, .deaths = 3});
    CHECK(improvement.firstClear);
    CHECK(improvement.newBestTime);
    CHECK(improvement.fewerDeaths);
    CHECK(progress.isCompleted("level_01"));

    const LevelRecord* record = progress.find("level_01");
    CHECK(record != nullptr);
    CHECK(record->bestTime == 42.5);
    CHECK(record->fewestDeaths == 3);
    CHECK(record->clears == 1);
}

//a slower run does not break the record
static void testSlowerRun() {
    Progress progress;
    progress.recordClear("level_01", {.time = 30.0, .deaths = 1});

    const Improvement improvement = progress.recordClear("level_01", {.time = 60.0, .deaths = 5});
    CHECK(!improvement.firstClear);
    CHECK(!improvement.newBestTime);
    CHECK(!improvement.fewerDeaths);

    const LevelRecord* record = progress.find("level_01");
    CHECK(record->bestTime == 30.0);
    CHECK(record->fewestDeaths == 1);
    CHECK(record->clears == 2);
}

//a faster run improves the time
static void testFasterRun() {
    Progress progress;
    progress.recordClear("level_01", {.time = 60.0, .deaths = 5});

    const Improvement improvement = progress.recordClear("level_01", {.time = 45.0, .deaths = 5});
    CHECK(!improvement.firstClear);
    CHECK(improvement.newBestTime);
    CHECK(!improvement.fewerDeaths);
    CHECK(progress.find("level_01")->bestTime == 45.0);
}

//a run with fewer deaths improves that
static void testFewerDeaths() {
    Progress progress;
    progress.recordClear("level_01", {.time = 60.0, .deaths = 5});

    const Improvement improvement = progress.recordClear("level_01", {.time = 60.0, .deaths = 2});
    CHECK(!improvement.newBestTime);
    CHECK(improvement.fewerDeaths);
    CHECK(progress.find("level_01")->fewestDeaths == 2);
}

//a run can be better in both
static void testBetterInBoth() {
    Progress progress;
    progress.recordClear("level_01", {.time = 60.0, .deaths = 5});

    const Improvement improvement = progress.recordClear("level_01", {.time = 30.0, .deaths = 0});
    CHECK(improvement.newBestTime);
    CHECK(improvement.fewerDeaths);
}

//the records survive a trip through json
static void testJsonRoundTrip() {
    Progress progress;
    progress.recordClear("level_01", {.time = 25.0, .deaths = 1});
    progress.recordClear("level_02", {.time = 70.5, .deaths = 4});

    const auto loaded = progressFromJson(progressToJson(progress));
    CHECK(loaded.has_value());
    CHECK(loaded->completedCount() == 2);
    CHECK(loaded->isCompleted("level_01"));
    CHECK(loaded->find("level_02")->bestTime == 70.5);
    CHECK(loaded->find("level_02")->clears == 1);
}

//text that is not json is an error
static void testBrokenJson() {
    const auto loaded = progressFromJson("not json at all");
    CHECK(!loaded.has_value());
}

//an entry with a wrong type keeps its default values
static void testWrongTypes() {
    const auto loaded = progressFromJson("{\"levels\": {\"level_01\": {\"completed\": \"yes\", \"bestTime\": 10.0}}}");
    CHECK(loaded.has_value());
    CHECK(loaded->find("level_01") != nullptr);
    CHECK(!loaded->find("level_01")->completed);
    CHECK(loaded->find("level_01")->bestTime == 10.0);
}

//a file that does not exist gives an empty progress and no problem
static void testMissingFile() {
    const LoadedProgress loaded = loadProgress(tempFolder() / "does_not_exist.json");
    CHECK(loaded.problem.empty());
    CHECK(loaded.progress.levels().empty());
}

//the progress is saved and loaded back
static void testSaveAndLoad() {
    const std::filesystem::path path = tempFolder() / "progress.json";
    std::error_code error;
    std::filesystem::remove_all(tempFolder(), error);

    Progress progress;
    progress.recordClear("level_01", {.time = 12.5, .deaths = 0});
    CHECK(saveProgress(path, progress).has_value());
    CHECK(std::filesystem::exists(path));

    const LoadedProgress loaded = loadProgress(path);
    CHECK(loaded.problem.empty());
    CHECK(loaded.progress.isCompleted("level_01"));
    CHECK(loaded.progress.find("level_01")->bestTime == 12.5);

    std::filesystem::remove_all(tempFolder(), error);
}

//the numbers that come from a file are made safe
static void testSafeNumbers() {
    Progress progress;
    progress.set("level_01", {.completed = true, .bestTime = -5.0, .fewestDeaths = -2, .clears = -1});

    const LevelRecord* record = progress.find("level_01");
    CHECK(record->bestTime == 0.0);
    CHECK(record->fewestDeaths == 0);
    CHECK(record->clears == 0);
}

//forgetting everything
static void testClear() {
    Progress progress;
    progress.recordClear("level_01", {.time = 10.0, .deaths = 0});
    progress.clear();

    CHECK(progress.levels().empty());
    CHECK(progress.completedCount() == 0);
}

}  //anonymous namespace

//runs every test, any failed CHECK ends the program with exit code 1
int main() {
    testEmpty();
    testFirstClear();
    testSlowerRun();
    testFasterRun();
    testFewerDeaths();
    testBetterInBoth();
    testJsonRoundTrip();
    testBrokenJson();
    testWrongTypes();
    testMissingFile();
    testSaveAndLoad();
    testSafeNumbers();
    testClear();
    std::println("all progress tests passed");
}
