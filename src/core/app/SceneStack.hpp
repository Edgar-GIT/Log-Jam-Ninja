// SceneStack.hpp - which screen of the game is shown. the top one is active, the ones below wait (pause on top of the level).

#pragma once

#include <cstddef>
#include <vector>

namespace ljn {

enum class Scene {
    Title,
    LevelSelect,
    Settings,
    Controls,
    Playing,
    Paused,
    LevelComplete,
};

class SceneStack {
public:
    //starts with one scene, so there is always something to show
    explicit SceneStack(Scene first) : m_scenes{first} {}

    [[nodiscard]] Scene current() const noexcept { return m_scenes.back(); }
    [[nodiscard]] std::size_t depth() const noexcept { return m_scenes.size(); }

    //true if the scene is anywhere in the stack (the level is still there under the pause menu)
    [[nodiscard]] bool contains(Scene scene) const noexcept {
        for (const Scene s : m_scenes) {
            if (s == scene) {
                return true;
            }
        }
        return false;
    }

    //the scene below the top one, or the same one if there is only one (used to know where "back" goes)
    [[nodiscard]] Scene below() const noexcept {
        return m_scenes.size() > 1 ? m_scenes[m_scenes.size() - 2] : m_scenes.back();
    }

    //puts a scene on top
    void push(Scene scene) { m_scenes.push_back(scene); }

    //removes the top scene, the first one never leaves
    void pop() noexcept {
        if (m_scenes.size() > 1) {
            m_scenes.pop_back();
        }
    }

    //changes the top scene for another one
    void replace(Scene scene) noexcept { m_scenes.back() = scene; }

    //throws everything away and starts again with one scene
    void reset(Scene first) { m_scenes.assign(1, first); }

private:
    std::vector<Scene> m_scenes;
};

}  //namespace ljn
