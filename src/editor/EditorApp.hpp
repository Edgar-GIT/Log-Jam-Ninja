// EditorApp.hpp - the map editor program: owns the window, the map and the camera.

#pragma once

#include "Camera.hpp"
#include "map/Map.hpp"

namespace ljn {

class EditorApp {
public:
    //opens the window and creates an empty map
    EditorApp();

    //closes the window
    ~EditorApp();

    //the window is a single global resource, so the app cannot be copied
    EditorApp(const EditorApp&) = delete;
    EditorApp& operator=(const EditorApp&) = delete;

    //runs the editor until the window is closed
    void run();

private:
    void update(float dt);
    void draw() const;

    Map m_map;
    EditorCamera m_camera;
};

}  //namespace ljn
