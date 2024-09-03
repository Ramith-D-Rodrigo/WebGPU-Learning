// Include the C++ wrapper instead of the raw header(s)
#define WEBGPU_CPP_IMPLEMENTATION

#include "utils.h"
//#include "build/SceneObject.h"
//#include "build/Model.h"
#include "Application.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720


int main() {
    Application app;

    if (!app.Initialize(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        return 1;
    }
#ifdef __EMSCRIPTEN__
    auto emScriptCallback = [](void* arg) {
        Application* app = static_cast<Application*>(arg); //get the application

        app->MainLoop(); //run the main loop
        };

    emscripten_set_main_loop_arg(emScriptCallback, &app, 0, 1);
#else // NOT __EMSCRIPTEN__
    while (app.IsRunning()) {
        app.MainLoop();
    }
#endif // NOT __EMSCRIPTEN__

    app.Terminate();

    return 0;
}