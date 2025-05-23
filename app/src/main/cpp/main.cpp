
//////////////////////////////////////////////////////////////////
// Pixel Game Engine Mobile Release 2.2.X,                      //
// John Galvin aka Johnngy63: 10-Jan-2025                       //
// New Support for 3D iOS sensors not supported yet      //
// Please report all bugs to https://discord.com/invite/WhwHUMV //
// Or on Github: https://github.com/Johnnyg63					//
//////////////////////////////////////////////////////////////////

// Set up headers for the different platforms
// #define  __ANDROID__
#if defined (__ANDROID__)

#include "pch.h"
#include <malloc.h>

#endif

#if defined (__APPLE__)

#include "ios_native_app_glue.h"

#endif

//#define STBI_NO_SIMD // Removes SIMD Support
// SIMD greatly improves the speed of your game
#if defined(__arm__)||(__aarch64__)

// Use Advance SIMD NEON when loading images for STB Default is SSE2 (x86)
#define STBI_NEON

#endif

#define OLC_PGE_APPLICATION
#define OLC_IMAGE_STB
#define OLC_ENABLE_EXPERIMENTAL
#include "utilities/olcUTIL_Hardware3D.h"
#include "olcPixelGameEngine_Mobile.h"

#define OLC_PGEX_MINIAUDIO
#include "olcPGEX_MiniAudio.h"  // Checkout https://github.com/Moros1138/olcPGEX_MiniAudio Thanks Moros1138

#include <fstream> // Used for saving the save state to a file

/// <summary>
/// To ensure proper cross platform support keep the class name as PGE_Mobile
/// This will ensure the iOS can launch the engine correctly
/// If you change it make the required changes in GameViewController.mm in the iOS app to suit
/// </summary>
class PGE_Mobile : public olc::PixelGameEngine {

public:

    PGE_Mobile() {
        sAppName = "TEST3DMountains";
    }

    olc::mf4d matWorld;
    olc::mf4d matView;
    olc::mf4d matProject;
    olc::utils::hw3d::mesh meshMountain;
    olc::Renderable gfx1;

    olc::vf3d vf3Up = {0.0f, 1.0f, 0.0f};           // vf3d up direction
    olc::vf3d vf3Camera = {0.0f, 0.0f, 0.0f};       // vf3d camera direction
    olc::vf3d vf3LookDir = { 0.0f, 0.0f, 1.0f };    // vf3d look direction
    olc::vf3d vf3Forward = {0.0f, 0.0f, 0.0f};      // vf3d Forward direction

    float fYaw = 0.0f;		// FPS Camera rotation in XZ plane
    float fTheta = 0.0f;	// Spins World transform


    /* Vectors */
    std::vector<std::string> vecMessages;
    /* END Vectors*/

    int nFrameCount = 0;
    float nStep = 20.0f;

    /* Sprites */
    olc::Sprite* sprTouchTester = nullptr;
    olc::Sprite* sprOLCPGEMobLogo = nullptr;
    olc::Sprite* sprLandScape = nullptr;
    /* END Sprites*/

    /* Decals */
    olc::Decal* decTouchTester = nullptr;
    olc::Decal* decOLCPGEMobLogo = nullptr;
    olc::Decal* decLandScape = nullptr;
    /* End Decals */


    /* Sensors */
    std::vector<olc::SensorInformation> vecSensorInfos;
    /*End Sensors*/

    // To keep track of our sample ID
    // Ensure that all sound IDs are set to -1 stop memory leak when Android/iOS takes
    // the app out of focus
    int32_t song1 = -1;

    // For demonstration controls, with sensible default values
    float pan = 0.0f;
    float pitch = 1.0f;
    float seek = 0.0f;
    float volume = 1.0f;

    // The instance of the audio engine, no fancy config required.
    olc::MiniAudio ma;


    // Manage Touch points
    olc::vi2d centreScreenPos;
    olc::vi2d leftCenterScreenPos;
    olc::vi2d rightCenterScreenPos;


public:
    //Example Save State Struct and Vector for when your app is paused
    struct MySaveState {
        std::string key;
        int value = 0;
    };

    std::vector<MySaveState> vecLastState;

    std::string sampleAFullPath; // Holds the full path to sampleA.wav

public:
    bool OnUserCreate() override {

        float fAspect = float(GetScreenSize().y) / float(GetScreenSize().x);
        float S = 1.0f / (tan(3.14159f * 0.25f));
        float f = 1000.0f;
        float n = 0.1f;

        matProject(0, 0) = fAspect; matProject(0, 1) = 0.0f; matProject(0, 2) = 0.0f;	              matProject(0, 3) = 0.0f;
        matProject(1, 0) = 0.0f;    matProject(1, 1) = 1;    matProject(1, 2) = 0.0f;                 matProject(1, 3) = 0.0f;
        matProject(2, 0) = 0.0f;    matProject(2, 1) = 0.0f; matProject(2, 2) = -(f / (f - n));       matProject(2, 3) = -1.0f;
        matProject(3, 0) = 0.0f;    matProject(3, 1) = 0.0f; matProject(3, 2) = -((f * n) / (f - n)); matProject(3, 3) = 0.0f;

        matWorld.identity();
        matView.identity();

        /*
         * Loading object (blender. mp3 etc) files is different on the PGE Mobile to PGE
         * Loading of images (bmp, png etc) is the exact same as PGE
         *      I don't make the runs, Google and Apple do.
         * The example below shows how to do it
         * */

        // PGE 2.0 Code:
        // auto t = olc::utils::hw3d::LoadObj("./assets/teapot.obj");
        // I know simple right, however mobile devices are different

        // 1: We need to get the path to where your phone OS as placed it your app
        std::string strObjectFileFullPath = (std::string)app_GetExternalAppStorage()  + "/objectfiles/mountains.obj";
        // NOTE: for iOS use:

        // 2: Now we need to extract the file from compress storage to usable store
        olc::rcode fileRes = olc::filehandler->ExtractFileFromAssets("objectfiles/mountains.obj", strObjectFileFullPath);
        // Note for iOS use:

        // 3 Use the olc::rcode to check if everything worked
        switch (fileRes) {

            case olc::rcode::NO_FILE:
            case olc::rcode::FAIL:
            { break; }
            case olc::rcode::OK:
            {
                auto t = olc::utils::hw3d::LoadObj(strObjectFileFullPath);
                if (t.has_value())
                {
                    meshMountain = *t;
                } else
                {
                    int pause = 0; // TODO: Remove. We have an issue
                }
            }
        }

        Clear(olc::BLUE);

        sprTouchTester = new olc::Sprite("images/north_south_east_west_logo.png");
        decTouchTester = new olc::Decal(sprTouchTester);

        sprOLCPGEMobLogo = new olc::Sprite("images/olcpgemobilelogo.png");
        decOLCPGEMobLogo = new olc::Decal(sprOLCPGEMobLogo);

        sprLandScape = new olc::Sprite("images/MountainTest1.jpg");
        decLandScape = new olc::Decal(sprLandScape);

        // Manage Touch points
        centreScreenPos = GetScreenSize();
        centreScreenPos.x = centreScreenPos.x / 2;
        centreScreenPos.y = centreScreenPos.y / 2;

        leftCenterScreenPos = GetScreenSize();
        leftCenterScreenPos.x = leftCenterScreenPos.x / 100 * 25; //TODO remove magic numbers
        leftCenterScreenPos.y = leftCenterScreenPos.y / 2;

        rightCenterScreenPos = GetScreenSize();
        rightCenterScreenPos.x = rightCenterScreenPos.x / 100 * 75;
        rightCenterScreenPos.y = rightCenterScreenPos.y / 2;


        return true;
    }

    // <summary>
    /// Draws a Target Pointer at the center position of Center Point
    /// </summary>
    /// <param name="vCenterPoint">Center Position of the target</param>
    /// <param name="nLineLength">Length of lines</param>
    /// <param name="nCircleRadius">Center Circle radius</param>
    void DrawTargetPointer(const olc::vi2d& vCenterPoint, int32_t nLineLength, int32_t nCircleRadius, olc::Pixel p = olc::WHITE)
    {
        /*
                        |
                        |
                    ----O----
                        |
                        |


        */
        FillCircle(vCenterPoint, nCircleRadius, p);
        DrawLine(vCenterPoint, { vCenterPoint.x, vCenterPoint.y + nLineLength }, p);
        DrawLine(vCenterPoint, { vCenterPoint.x, vCenterPoint.y - nLineLength }, p);
        DrawLine(vCenterPoint, { vCenterPoint.x + nLineLength, vCenterPoint.y }, p);
        DrawLine(vCenterPoint, { vCenterPoint.x - nLineLength, vCenterPoint.y }, p);

    }

    float fThetaX = 0;
    float fThetaY = 0;
    float fThetaZ = 0;
    //float fThetaY = 0;

    float fLightTime = 0.0f;

    bool OnUserUpdate(float fElapsedTime) override {

        SetDrawTarget(nullptr);

        Clear(olc::BLUE);

        nFrameCount = (int32_t)GetFPS();

        std::string sLineBreak = "-------------------------";

        std::string sMessage = "OneLoneCoder.com";
        vecMessages.push_back(sMessage);

        sMessage = "PGE Mobile Release 2.2.X";
        vecMessages.push_back(sMessage);

        sMessage = "Now With 3D Support";
        vecMessages.push_back(sMessage);

        sMessage = "NOTE: Android FPS = CPU FPS, iOS = GPU FPS";
        vecMessages.push_back(sMessage);

        sMessage = sAppName + " - FPS: " + std::to_string(nFrameCount);
        vecMessages.push_back(sMessage);

        sMessage = "Sound Copyright: https://pixabay.com/users/shidenbeatsmusic-25676252/";
        vecMessages.push_back(sMessage);

        sMessage = "---";
        vecMessages.push_back(sMessage);


        // Get the default touch point
        // This is always Index 0 and first touch point
        olc::vi2d defaultTouchPos = GetTouchPos();
        std::string defaultTouch = "Default Touch 0:  X: " + std::to_string(defaultTouchPos.x) + " Y: " + std::to_string(defaultTouchPos.y);
        vecMessages.push_back(defaultTouch);

        // New code:
        olc::vf3d  vf3Target = {0,0,1};

        olc::mf4d mMovement, mYaw, mOffset, mCollision;

        mMovement.translate(vf3Camera);        // first we move to the new location
        mOffset.translate(0.0, -10.0, 0.0);     // Add our offset
        mMovement.rotateY(fTheta);             // Rotate the camera left/right
        matWorld = mMovement * mOffset;        // Get our new view point
        //TODO: Add mCollision
        //mCollision.translate(0.0f, 0.0f, 0.0f);
        //matWorld = mMovement * mOffset * mCollision;

        mYaw.rotateX(fYaw);                       // Second rotate camera Up/Down
        vf3LookDir = mYaw * vf3Target;            // Get our new direction
        vf3Target = vf3Camera + vf3LookDir;     // Set our target

        matView.pointAt(vf3Camera, vf3Target, vf3Up);   // Point at our Target

        ClearBuffer(olc::CYAN, true);


        HW3D_Projection(matProject.m);

        // Lighting
        for (size_t i = 0; i < meshMountain.pos.size(); i += 3)
        {
            const auto& p0 = meshMountain.pos[i + 0];
            const auto& p1 = meshMountain.pos[i + 1];
            const auto& p2 = meshMountain.pos[i + 2];

            olc::vf3d vCross = olc::vf3d(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]).cross(olc::vf3d(p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2])).norm();

            olc::vf3d vLight = olc::vf3d(1.0f, 1.0f, 1.0f).norm();

            float illum = std::clamp(vCross.dot(vLight), 0.0f, 1.0f) * 0.6f + 0.4f;
            meshMountain.col[i + 0] = olc::PixelF(illum, illum, illum, 1.0f);
            meshMountain.col[i + 1] = olc::PixelF(illum, illum, illum, 1.0f);
            meshMountain.col[i + 2] = olc::PixelF(illum, illum, illum, 1.0f);
        }


        HW3D_DrawLine((matView * matWorld).m, { 0.0f, 0.0f, 0.0f }, { 100.0f, 100.0f, 100.0f }, olc::RED);

        HW3D_DrawLineBox((matView * matWorld).m, { 0.0f, 0.0f, 0.0f }, { 10.0f, 10.0f, 10.0f }, olc::YELLOW);


        HW3D_DrawObject((matView * matWorld).m, decLandScape, meshMountain.layout, meshMountain.pos, meshMountain.uv, meshMountain.col);

        // Make sure we have not botched 2D Decals
        //DrawDecal(GetMousePos(), gfx1.Decal());

        // Draw Touch point positions
        DrawTargetPointer(centreScreenPos, 50, 10);
        DrawTargetPointer(leftCenterScreenPos, 50, 10, olc::GREEN);
        DrawTargetPointer(rightCenterScreenPos, 50, 10, olc::RED);

        // End new code

        // Touch 1 handles forward and backwards only
        if (GetTouch(1).bHeld)
        {
            DrawLine(rightCenterScreenPos, GetTouchPos(1), olc::RED, 0xF0F0F0F0);
            DrawTargetPointer(GetTouchPos(1), 50, 10, olc::RED);


            // Moving Forward
            if ((float)GetTouchY(1) < (((float)rightCenterScreenPos.y /100)*70))
            {
                vf3Camera.z += 8.0f * fElapsedTime;
            }

            // Moving Backward
            if ((float)GetTouchY(1) > (((float)rightCenterScreenPos.y /100)*130))
            {
                vf3Camera.z -= 8.0f * fElapsedTime;
            }

            // Moving Left (Strife)
            if ((float)GetTouchX(1) > (((float)rightCenterScreenPos.x / 100)*110) )
            {
                vf3Camera.x -= 8.0f * fElapsedTime;
            }

            // Moving Right (Strife)
            if ((float)GetTouchX(1) < (((float)rightCenterScreenPos.x / 100)*90))
            {
                vf3Camera.x+= 8.0f * fElapsedTime;
            }

            // Moving UP
            // TODO: Add code so that only when movement is > 20% execute
            if (GetTouchY(1) < rightCenterScreenPos.y)
            {
                //vf3Camera.y -= 0.5f * fElapsedTime;
            }

            // Moving Down
            if (GetTouchY(1) > rightCenterScreenPos.y)
            {
                //vf3Camera.y += 0.5f * fElapsedTime;
            }

        }


        // Touch zeros (single touch) handles Camera look direction
        if (GetTouch(0).bHeld)
        {
            DrawLine(leftCenterScreenPos, GetTouchPos(), olc::GREEN, 0xF0F0F0F0);
            DrawTargetPointer(GetTouchPos(), 50, 10, olc::GREEN);

            // We know the Right Center point we need to compare our positions
            // Looking Right
            if ((float)GetTouchX(0) > (((float)leftCenterScreenPos.x / 100)*130) )
            {
                fTheta -= 1.0f * fElapsedTime;


            }

            // Looking Left
            if ((float)GetTouchX(0) < (((float)leftCenterScreenPos.x / 100)*70))
            {
                fTheta += 1.0f * fElapsedTime;


            }

            // Looking Up
            if ((float)GetTouchY(0) < (((float)leftCenterScreenPos.y / 100)*70))
            {
                fYaw -= 0.5f * fElapsedTime;
                if(fYaw < -1.0f) fYaw = -1.0f;

            }

            // Looking Down
            if ((float)GetTouchY(0) > (((float)leftCenterScreenPos.y / 100)*130))
            {
                fYaw += 0.5f * fElapsedTime;
                if(fYaw > 1.0f) fYaw = 1.0f;


            }

        } else
        {
            // Move the camera back to centre, stops the dizzies!
            if(fYaw > -0.01f && fYaw < 0.01f)
            {
                fYaw =0.0f;
            }
            if(fYaw >= 0.01)
            {
                fYaw -= 0.5f * fElapsedTime;
            }
            if(fYaw <= -0.01)
            {
                fYaw += 0.5f * fElapsedTime;
            }

        }




        nStep = 10.0f;
        for (auto& s : vecMessages)
        {
            DrawStringDecal({20.0f, nStep}, s);
            //DrawString(20, nStep, s);
            nStep += 10.0f;
        }
        vecMessages.clear();

        // Draw Logo
        DrawDecal({ 5.0f, (float)ScreenHeight() - 100 }, decOLCPGEMobLogo, { 0.5f, 0.5f });


        return true;
    }

    bool OnUserDestroy() override {
        return true;
    }

    void OnSaveStateRequested() override
    {
        // Fires when the OS is about to put your game into pause mode
        // You have, at best 30 Seconds before your game will be fully shutdown
        // It depends on why the OS is pausing your game tho, Phone call, etc
        // It is best to save a simple Struct of your settings, i.e. current level, player position etc
        // NOTE: The OS can terminate all of your data, pointers, sprites, layers can be freed
        // Therefore do not save sprites, pointers etc

        // Example 1: vector
        vecLastState.clear();
        vecLastState.push_back({ "MouseX", 55 });
        vecLastState.push_back({ "MouseY", 25 });
        vecLastState.push_back({ "GameLevel", 5 });

#if defined(__ANDROID__)
        // You can save files in the android Internal app storage
        const char* internalPath = app_GetInternalAppStorage(); //Android protected storage
#endif
#if defined(__APPLE__)
        // For iOS the internal app storage is read only, therefore we use External App Storage
        const char* internalPath = app_GetExternalAppStorage(); // iOS protected storage AKA /Library
#endif

        std::string dataPath(internalPath);

        // internalDataPath points directly to the files/ directory
        std::string lastStateFile = dataPath + "/lastStateFile.bin";

        std::ofstream file(lastStateFile, std::ios::out | std::ios::binary);

        if (file)
        {
            float fVecSize = vecLastState.size();
            file.write((char*)&fVecSize, sizeof(long));
            for (auto& vSS : vecLastState)
            {
                file.write((char*)&vSS, sizeof(MySaveState));
            }

            file.close();
        }


    }

    void OnRestoreStateRequested() override
    {
        // This will fire every time your game launches
        // OnUserCreate will be fired again as the OS may have terminated all your data

#if defined(__ANDROID__)
        // You can save files in the android Internal app storage
        const char* internalPath = app_GetInternalAppStorage(); //Android protected storage
#endif
#if defined(__APPLE__)
        // For iOS the internal app storage is read only, therefore we use External App Storage
        const char* internalPath = app_GetExternalAppStorage(); // iOS protected storage AKA /Library
#endif

        std::string dataPath(internalPath);
        std::string lastStateFile = dataPath + "/lastStateFile.bin";

        vecLastState.clear();

        std::ifstream file(lastStateFile, std::ios::in | std::ios::binary);

        MySaveState saveState;

        if (file)
        {
            float fVecSize = 0.0f;
            file.read((char*)&fVecSize, sizeof(long));
            for (long i = 0; i < fVecSize; i++)
            {
                file.read((char*)&saveState, sizeof(MySaveState));
                vecLastState.push_back(saveState);
            }

            file.close();
            // Note this is a temp file, we must delete it
            std::remove(lastStateFile.c_str());

        }


    }


    void OnLowMemoryWarning() override
    {
        /// NOTE: Fires when the OS is about to close your app due low memory availability
        /// Use this method to clean up any resources to reduce your memory usage
        /// If you can reduce your memory usage enough the OS will automatically cancel the application termination event

    }
};


/**
* This is the main entry point of a native application that is using
* android_native_app_glue.  It runs in its own thread, with its own
* event loop for receiving input events and doing other things.
* This is now what drives the engine, the thread is controlled from the OS
*/
void android_main(struct android_app* initialstate) {

    /*
        initalstate allows you to make some more edits
        to your app before the PGE Engine starts
        Recommended just to leave it at its defaults
        but change it at your own risk
        to access the Android/iOS directly in your code
        android_app* pMyAndroid = this->pOsEngine.app;;

    */

    PGE_Mobile demo;

    /*
        Note it is best to use HD(1280, 720, ? X ? pixel, Fullscreen = true) the engine can scale this best for all screen sizes,
        without affecting performance... well it will have a very small affect, it will depend on your pixel size
        Note: cohesion is currently not working
    */
    demo.Construct(1280, 720, 2, 2, true, false, false);

    demo.Start(); // Lets get the party started


}

#if defined(__APPLE__)

/*
* The is the calling point from the iOS Objective C, called during the startup of your application
* Use the objects definded in IOSNativeApp to pass data to the Objective C
* By Default you must at minmum pass the game construct vars, pIOSNatvieApp->SetPGEConstruct
*
* iOS runs in its own threads, with its own
* event loop for receiving input events and doing other things.
* This is now what drives the engine, the thread is controlled from the OS
*/
int ios_main(IOSNativeApp* pIOSNatvieApp)
{
    // The iOS will instance your app differnetly to how Android does it
    // In the iOS it will automatically create the required classes and pointers
    // to get the PGE up and running successfull.

    // IMPORTANT: You must set your class name to PGE_Mobile (see above) always for iOS
    // Don't worry it will not conflict with any other apps that use the same base class name of PGE_Mobile
    // I got your back

    // Finally just like the Android you can access any avialble OS options using pIOSNatvieApp
    // Please note options will NOT be the same across both platforms
    // It is best to use the build in functions for File handling, Mouse/Touch events, Key events, Joypad etc

    //
    // To access the iOS directly in your code
    // auto* pMyApple = this->pOsEngine.app;
    //

    /*
        Note it is best to use HD(1280, 720, ? X ? pixel, Fullscreen = true) the engine can scale this best for all screen sizes,
        without affecting performance... well it will have a very small affect, it will depend on your pixel size
        Note: cohesion is currently not working
        Note: It is best to set maintain_aspect_ratio to false, Fullscreen to true and use the olcPGEX_TransformView.h to manage your world-view
        in short iOS does not want to play nice, the screen ratios and renta displays make maintaining a full screen with aspect radio a pain to manage
    */
    pIOSNatvieApp->SetPGEConstruct(1280, 720, 2, 2, true, true, false);


    // We now need to return SUCCESS or FAILURE to get the party stated!!!!
    return EXIT_SUCCESS;
}

#endif
