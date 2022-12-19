// Copyright (c) 2011-2020 <>< Charles Lohr - Under the MIT/x11 or NewBSD
// License you choose.
//  NO WARRANTY! NO GUARANTEE OF SUPPORT! USE AT YOUR OWN RISK
//  extern 
extern "C" {

#include <android_native_app_glue.h>

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd(android_app *pApp, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // A new window is created, associate a renderer with it. You may replace this with a
            // "game" class if that suits your needs. Remember to change all instances of userData
            // if you change the class here as a reinterpret_cast is dangerous this in the
            // android_main function and the APP_CMD_TERM_WINDOW handler case.
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being destroyed. Use this to clean up your userData to avoid leaking
            // resources.
            //
            // We have to check if userData is assigned just in case this comes in really quickly
            if (pApp->userData) {
                //
            }
            break;
        default:
            break;
    }
}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
    // // Can be removed, useful to ensure your code is running
    // aout << "Welcome to android_main" << std::endl;

    // register an event handler for Android events
    pApp->onAppCmd = handle_cmd;

    // This sets up a typical game/event loop. It will run until the app is destroyed.
    int events;
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

            // We know that our user data is a Renderer, so reinterpret cast it. If you change your
            // user data remember to change it here
        }
    } while (!pApp->destroyRequested);
}
}
// extern "C" {
// #include <GLES3/gl3.h>
// #include <android/sensor.h>
// #include <android_native_app_glue.h>
// #include <asset_manager.h>
// #include <asset_manager_jni.h>
// #include <math.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
//
// #include "CNFGAndroid.h"
// #include "os_generic.h"
//
// #define CNFG_IMPLEMENTATION
// #include "CNFG.h"
//
// ALooper* l;
//
// unsigned frames = 0;
// unsigned long iframeno = 0;
//
// void AndroidDisplayKeyboard(int pShow);
//
// int lastbuttonx = 0;
// int lastbuttony = 0;
// int lastmotionx = 0;
// int lastmotiony = 0;
// int lastbid = 0;
// int lastmask = 0;
// int lastkey, lastkeydown;
//
// static int keyboard_up;
//
// void HandleKey(int keycode, int bDown) {
//   lastkey = keycode;
//   lastkeydown = bDown;
//   if (keycode == 10 && !bDown) {
//     keyboard_up = 0;
//     AndroidDisplayKeyboard(keyboard_up);
//   }
//
//   if (keycode == 4) {
//     AndroidSendToBack(1);
//   }  // Handle Physical Back Button.
// }
//
// void HandleButton(int x, int y, int button, int bDown) {
//   lastbid = button;
//   lastbuttonx = x;
//   lastbuttony = y;
//
//   if (bDown) {
//     keyboard_up = !keyboard_up;
//     AndroidDisplayKeyboard(keyboard_up);
//   }
// }
//
