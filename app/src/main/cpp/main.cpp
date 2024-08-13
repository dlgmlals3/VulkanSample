#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

#include "header/VkRenderer.h"
#include "header/AndroidOut.h"

extern "C" {

#include <game-activity/native_app_glue/android_native_app_glue.c>
#include <time.h>

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd(android_app *pApp, int32_t cmd) {
    switch (cmd) {

        case APP_CMD_INIT_WINDOW:
            pApp->userData = new VkRenderer(pApp->window, pApp->activity->assetManager);
            break;
        case APP_CMD_TERM_WINDOW:
            if (pApp->userData) {
                delete static_cast<VkRenderer *>(pApp->userData);
                pApp->userData = nullptr;
            }
            break;
        default:
            break;
    }
}

/*!
 * Enable the motion events you want to handle; not handled events are
 * passed back to OS for further processing. For this example case,
 * only pointer and joystick devices are enabled.
 *
 * @param motionEvent the newly arrived GameActivityMotionEvent.
 * @return true if the event is from a pointer or joystick device,
 *         false for all other input devices.
 */
bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
    // Register an event handler for Android events
    pApp->onAppCmd = handle_cmd;

    // Set input event filters (set it to NULL if the app wants to process all inputs).
    // Note that for key inputs, this example uses the default default_key_filter()
    // implemented in android_native_app_glue.c.
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    // This sets up a typical game/event loop. It will run until the app is destroyed.
    int events;
    int angle = 0;
    android_poll_source *pSource;
    do {
        // Process all pending events before running game logic.
        if (ALooper_pollAll(0, nullptr, &events, (void **) &pSource) >= 0) {
            if (pSource) {
                pSource->process(pApp, pSource);
            }
        }

        // Check if any user data is associated. This is assigned in handle_cmd
        if (pApp->userData) {
            glm::mat4 firstModel(1.0f);
            glm::mat4 secondModel(1.0f);

            firstModel = glm::translate(firstModel, glm::vec3(-1.0f, 0.0f, -1.0f));
            firstModel = glm::rotate(firstModel, glm::radians((float)angle), glm::vec3(0.0f, 0.0f, 1.0f));
            secondModel = glm::translate(secondModel, glm::vec3(1.0f, 0.0f, -3.0f));
            secondModel = glm::rotate(secondModel, glm::radians((float)-angle * 10), glm::vec3(0.0f, 0.0f, 1.0f));
            //static_cast<VkRenderer *>(pApp->userData)->UpdateModel(0, firstModel);
            //static_cast<VkRenderer *>(pApp->userData)->UpdateModel(1, secondModel);

            static_cast<VkRenderer *>(pApp->userData)->Draw();
            angle %= 360;
            angle++;
        }
    } while (!pApp->destroyRequested);
}
}