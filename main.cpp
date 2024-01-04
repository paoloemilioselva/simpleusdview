// workaround for a compilation issue 
// https://github.com/microsoft/STL/issues/2335#issuecomment-967306862
#define _STL_CRT_SECURE_INVALID_PARAMETER(expr) _CRT_SECURE_INVALID_PARAMETER(expr)

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <pxr/base/tf/preprocessorUtils.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/imaging/glf/simpleLightingContext.h>
#include <pxr/imaging/glf/drawTarget.h>
#include <pxr/base/tf/notice.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/glf/glContext.h>
#include <pxr/imaging/glf/info.h>
#include <pxr/usd/usdLux/domeLight.h>
#include <pxr/pxr.h>

#include <iostream>
#include <cstdlib>

#include "font.h"

#define WIDTH 1024
#define HEIGHT 768

double lookAtDistance = 6.0;
double yaw = 7.0;
double pitch = 0.0;
double rollX = 0.0;
double rollY = 0.0;
double offsetX = 0.0;
double offsetY = 0.0;
float domeExposure = 1.0f;

int currentDelegate = 0;
int newDelegate = 0;

std::string currentFilename = "";
std::string newFilename = "";

bool animate = true;

int totalCubes = 0;

bool fullscreen = false;

int recentButton = GLFW_GAMEPAD_BUTTON_LAST;

bool showHelp = true;
bool highlight = true;

float eyesHeight = 0.0f;
float focalLength = 30.f;

float positionMultiplier = 5.0f;
float rotationMultiplier = 0.2f;
float heightMultiplier = 2.0f;
float distanceMultiplier = 3.0f;

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    else if (key == GLFW_KEY_UP)
    {
        lookAtDistance -= 0.5;
    }
    else if (key == GLFW_KEY_DOWN)
    {
        lookAtDistance += 0.5;
    }
    else if (key == GLFW_KEY_W)
    {
        yaw -= 0.5;
    }
    else if (key == GLFW_KEY_S)
    {
        yaw += 0.5;
    }
    else if (key == GLFW_KEY_A)
    {
        pitch -= 0.5;
    }
    else if (key == GLFW_KEY_D)
    {
        pitch += 0.5;
    }

    else if (key == GLFW_KEY_6 && action == GLFW_PRESS)
    {
        positionMultiplier += 0.1f;
    }
    else if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        positionMultiplier -= 0.1f;
    }
    else if (key == GLFW_KEY_7 && action == GLFW_PRESS)
    {
        rotationMultiplier += 0.1f;
    }
    else if (key == GLFW_KEY_U && action == GLFW_PRESS)
    {
        rotationMultiplier -= 0.1f;
    }
    else if (key == GLFW_KEY_8 && action == GLFW_PRESS)
    {
        heightMultiplier += 0.1f;
    }
    else if (key == GLFW_KEY_I && action == GLFW_PRESS)
    {
        heightMultiplier -= 0.1f;
    }
    else if (key == GLFW_KEY_9 && action == GLFW_PRESS)
    {
        distanceMultiplier += 0.1f;
    }
    else if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        distanceMultiplier -= 0.1f;
    }

    else if (key == GLFW_KEY_0 && action == GLFW_PRESS)
    {
        newDelegate = 0;
    }
    else if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        newDelegate = 1;
    }
    else if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        newDelegate = 2;
    }
    else if (key == GLFW_KEY_MINUS)
    {
        domeExposure -= 0.1f;
    }
    else if (key == GLFW_KEY_EQUAL)
    {
        domeExposure += 0.1f;
    }
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        animate = !animate;
    }
    else if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        fullscreen = !fullscreen;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        if (fullscreen)
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        else
            glfwSetWindowMonitor(window, NULL, WIDTH / 2.0, HEIGHT / 2.0, WIDTH, HEIGHT, 0);
    }
    else if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        showHelp = !showHelp;
    }
    else if (key == GLFW_KEY_J && action == GLFW_PRESS)
    {
        highlight = !highlight;
    }
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    int i;
    for (i = 0; i < count; i++)
    {
        // only first one, and it replaces current one
        newFilename = std::string(paths[i]);
        break;
    }
}

void AddMeshCube(pxr::UsdStageRefPtr i_stage, pxr::SdfPath& i_path, pxr::GfVec3d& i_pos)
{
    pxr::UsdGeomMesh& cubeMesh = pxr::UsdGeomMesh::Define(i_stage, i_path);
    cubeMesh.CreateOrientationAttr().Set(pxr::UsdGeomTokens->leftHanded);
    cubeMesh.CreateSubdivisionSchemeAttr().Set(pxr::UsdGeomTokens->none);

    pxr::VtArray<pxr::GfVec3f> points;
    points.emplace_back(pxr::GfVec3f(0.5, -0.5, 0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, -0.5, 0.5));
    points.emplace_back(pxr::GfVec3f(0.5, 0.5, 0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, 0.5, 0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, -0.5, -0.5));
    points.emplace_back(pxr::GfVec3f(0.5, -0.5, -0.5));
    points.emplace_back(pxr::GfVec3f(-0.5, 0.5, -0.5));
    points.emplace_back(pxr::GfVec3f(0.5, 0.5, -0.5));
    cubeMesh.CreatePointsAttr().Set(points);
    pxr::VtArray<pxr::GfVec3f> displayColors;
    displayColors.emplace_back(pxr::GfVec3f(1, 0, 1));
    displayColors.emplace_back(pxr::GfVec3f(0, 0, 1));
    displayColors.emplace_back(pxr::GfVec3f(1, 1, 1));
    displayColors.emplace_back(pxr::GfVec3f(0, 1, 1));
    displayColors.emplace_back(pxr::GfVec3f(0, 0, 0));
    displayColors.emplace_back(pxr::GfVec3f(1, 0, 0));
    displayColors.emplace_back(pxr::GfVec3f(0, 1, 0));
    displayColors.emplace_back(pxr::GfVec3f(1, 1, 0));
    pxr::UsdAttribute& displayColorsAttr = cubeMesh.CreateDisplayColorAttr();
    displayColorsAttr.Set(displayColors);
    pxr::UsdGeomPrimvar displayColorsPrimvar(displayColorsAttr);
    displayColorsPrimvar.SetInterpolation(pxr::UsdGeomTokens->vertex);

    pxr::VtArray<int> faceVertexCounts = { 4, 4, 4, 4, 4, 4 };
    cubeMesh.CreateFaceVertexCountsAttr().Set(faceVertexCounts);
    pxr::VtArray<int> faceVertexIndices = { 0, 1, 3, 2, 4, 5, 7, 6, 6, 7, 2, 3, 5, 4, 1, 0, 5, 0, 2, 7, 1, 4, 6, 3 };
    cubeMesh.CreateFaceVertexIndicesAttr().Set(faceVertexIndices);
    
    cubeMesh.AddTranslateOp().Set(i_pos);
}

void AddAreaLight(pxr::UsdStageRefPtr i_stage, pxr::GfMatrix4d& i_matrix)
{
    int i = 1;
    while (i_stage->GetPrimAtPath(pxr::SdfPath("/arealight" + pxr::TfIntToString(i))))
        i++;
    pxr::UsdLuxRectLight& light = pxr::UsdLuxRectLight::Define(i_stage, pxr::SdfPath("/arealight" + pxr::TfIntToString(i)));
    light.CreateExposureAttr().Set(2.0f);
    light.CreateIntensityAttr().Set(2.0f);
    light.CreateWidthAttr().Set(50.0f);
    light.CreateHeightAttr().Set(50.0f);
    auto& xformOp = light.AddXformOp(pxr::UsdGeomXformOp::TypeTransform);
    xformOp.Set(i_matrix.GetInverse());

    //CHAOS_SCENE_INFO("Light '" << pxr::SdfPath("/arealight" + pxr::TfIntToString(i)).GetString() << "' created");
}

void AddDomeLight(pxr::UsdStageRefPtr i_stage)
{
    int i = 1;
    while (i_stage->GetPrimAtPath(pxr::SdfPath("/ibl" + pxr::TfIntToString(i))))
        i++;
    pxr::UsdLuxDomeLight& ibl = pxr::UsdLuxDomeLight::Define(i_stage, pxr::SdfPath("/ibl" + pxr::TfIntToString(i)));
    ibl.CreateTextureFileAttr().Set(pxr::SdfAssetPath("./meadow_2_2k.exr"));
}

int main(int argc, char** argv)
{
	if (!glfwInit())
	{
		std::cout << "Failed initializing glfw" << std::endl;
		return 1;
	}
    glfwSetErrorCallback(error_callback);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "simplecube", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed creating a glfw window with OpenGL, retrying without it" << std::endl;
        return 1;
    }
    glfwSetKeyCallback(window, key_callback);
    glfwSetDropCallback(window, drop_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    pxr::GlfContextCaps::InitInstance();

    std::unique_ptr<class pxr::UsdImagingGLEngine> engine;
    pxr::GfCamera camera;
    pxr::GfMatrix4d cameraTransform;
    pxr::GfVec3d cameraPivot(0,eyesHeight,0);

    pxr::GfMatrix4d viewMatrix;
    pxr::GfMatrix4d projectionMatrix;
    pxr::GfFrustum frustum;

    pxr::UsdImagingGLRenderParams renderParams;

    pxr::UsdStageRefPtr stage = pxr::UsdStage::CreateInMemory();

    // cube-mesh
    for(int cx=0;cx<1;++cx)
        for (int cy = 0; cy < 1; ++cy)
        {
            AddMeshCube(stage, pxr::SdfPath("/myCubeMesh_" + std::to_string(totalCubes)), pxr::GfVec3d(cx * 2.5, 0.0, cy * 2.5));
            totalCubes++;
        }

    pxr::SdfPathVector excludedPaths;
    engine.reset(new pxr::UsdImagingGLEngine(
        stage->GetPseudoRoot().GetPath(), excludedPaths));

    pxr::UsdLuxDomeLight& ibl = pxr::UsdLuxDomeLight::Define(stage, pxr::SdfPath("/myIbl"));
    ibl.CreateTextureFileAttr().Set(pxr::SdfAssetPath("./meadow_2_2k.exr"));

    std::cout << "Available Hydra Delegates:" << std::endl;
#if PXR_VERSION >= 2311
    auto& renderDelegates = engine->GetRendererPlugins();
#else
    auto renderDelegates = engine->GetRendererPlugins();
#endif
    bool enabled = engine->SetRendererPlugin(renderDelegates[0]);

    int frame = 0;

    pxr::GfVec4f clearColor(0.18f, 0.18f, 0.18f, 1.0f);

    pxr::GfVec2f joystickZeroLeft;
    pxr::GfVec2f joystickZeroRight;
    float joystickTriggerLeft = 0.0f;
    float joystickTriggerRight = 0.0f;
    if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
    {
        GLFWgamepadstate state;
        if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
        {
            std::cout << "Joystick/Gamepad found: " << glfwGetGamepadName(GLFW_JOYSTICK_1) << std::endl;
            joystickZeroLeft = pxr::GfVec2f(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
            joystickZeroRight = pxr::GfVec2f(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);
            joystickTriggerLeft = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
            joystickTriggerRight = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
        }
    }

    pxr::SdfPath selectedPrimPath;

    while (!glfwWindowShouldClose(window))
    {
        if(animate)
            frame++;

        glfwMakeContextCurrent(window);

        glfwPollEvents();

        bool delegateSelectionMode = false;
        bool primLocked = false;

        if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
        {
            GLFWgamepadstate state;
            if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
            {
                pxr::GfVec2f stickLeft = pxr::GfVec2f(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
                pxr::GfVec2f stickRight = pxr::GfVec2f(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);

// marco to check if a gamepad button is pressed
#define GAMEPAD_BUTTON_PRESSED(btn) state.buttons[btn] == GLFW_PRESS
// macro to check if a gamepad button has been pressed once
#define GAMEPAD_BUTTON_ONCE(btn) state.buttons[btn] == GLFW_PRESS && recentButton != btn

                if (GAMEPAD_BUTTON_ONCE(GLFW_GAMEPAD_BUTTON_A))
                {
                    //AddMeshCube(stage, pxr::SdfPath("/myCubeMesh_" + std::to_string(totalCubes)), cameraPivot);
                    //totalCubes++;
                    AddAreaLight(stage, viewMatrix);
                }
                if (GAMEPAD_BUTTON_ONCE(GLFW_GAMEPAD_BUTTON_B))
                {
                    AddDomeLight(stage);
                }
                if (GAMEPAD_BUTTON_PRESSED(GLFW_GAMEPAD_BUTTON_X))
                {
                    primLocked = true;
                }
                if (GAMEPAD_BUTTON_PRESSED(GLFW_GAMEPAD_BUTTON_Y))
                {
                    delegateSelectionMode = true;
                }
                if (GAMEPAD_BUTTON_PRESSED(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER))
                {
                    eyesHeight += heightMultiplier;
                    cameraPivot = pxr::GfVec3d(cameraPivot[0], eyesHeight, cameraPivot[2]);
                }
                if (GAMEPAD_BUTTON_PRESSED(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))
                {
                    eyesHeight -= heightMultiplier;
                    cameraPivot = pxr::GfVec3d(cameraPivot[0], eyesHeight, cameraPivot[2]);
                }
                if (GAMEPAD_BUTTON_ONCE(GLFW_GAMEPAD_BUTTON_DPAD_UP))
                {
                    if(delegateSelectionMode)
                        newDelegate = std::max(0, newDelegate - 1);
                }
                if (GAMEPAD_BUTTON_ONCE(GLFW_GAMEPAD_BUTTON_DPAD_DOWN))
                {
                    if(delegateSelectionMode)
                        newDelegate++;
                }

                if (GAMEPAD_BUTTON_PRESSED(GLFW_GAMEPAD_BUTTON_DPAD_LEFT))
                {
                    focalLength -= 0.2f;
                }
                if (GAMEPAD_BUTTON_PRESSED(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT))
                {
                    focalLength += 0.2f;
                }

                // set recent button
                for (int bi = 0; bi < 15; ++bi)
                {
                    recentButton = GLFW_GAMEPAD_BUTTON_LAST;
                    if (bi != GLFW_GAMEPAD_BUTTON_Y && state.buttons[bi] == GLFW_PRESS )
                    {
                        recentButton = bi;
                        break;
                    }
                }
#undef GAMEPAD_BUTTON_PRESSED
#undef GAMEPAD_BUTTON_ONCE

                if ((joystickZeroLeft - stickLeft).GetLength() > 0.08)
                {
                    pxr::GfVec3d camDir = cameraTransform.ExtractTranslation() - cameraPivot;
                    camDir[1] = 0.0;
                    camDir.Normalize();
                    cameraPivot = pxr::GfVec3d(cameraPivot[0] + stickLeft[1] * camDir[0] * positionMultiplier, eyesHeight, cameraPivot[2] + stickLeft[1] * camDir[2] * positionMultiplier);
                    cameraPivot = pxr::GfVec3d(cameraPivot[0] + stickLeft[0] * camDir[2] * positionMultiplier, eyesHeight, cameraPivot[2] + stickLeft[0] * (-camDir[0]) * positionMultiplier);

                    if (primLocked && !selectedPrimPath.IsEmpty())
                    {
                        pxr::UsdPrim& prim = stage->GetPrimAtPath(selectedPrimPath);
                        pxr::GfVec3d translate;
                        pxr::GfVec3f rotate;
                        pxr::GfVec3f scale;
                        pxr::GfVec3f pivot;
                        pxr::UsdGeomXformCommonAPI::RotationOrder rotOrder;
                        pxr::UsdGeomXformCommonAPI(prim).GetXformVectors(&translate, &rotate, &scale, &pivot, &rotOrder, pxr::UsdTimeCode::Default());

                        translate[0] += stickLeft[1] * camDir[0] * positionMultiplier + stickLeft[0] * camDir[2] * positionMultiplier;
                        translate[1] += 0.0;
                        translate[2] += stickLeft[1] * camDir[2] * positionMultiplier + stickLeft[0] * (-camDir[0]) * positionMultiplier;

                        // pxr::UsdGeomImageable(prim).ComputeLocalToWorldTransform(0);
                        auto& sourcePrim = pxr::UsdGeomXform(prim);
                        sourcePrim.ClearXformOpOrder();
                        auto& transformOp = sourcePrim.AddTransformOp();
                        auto m = pxr::GfMatrix4d().SetIdentity();
                        m *= pxr::GfMatrix4d().SetScale(scale);
                        m *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(1, 0, 0), rotate[0]));
                        m *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(0, 1, 0), rotate[1]));
                        m *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(0, 0, 1), rotate[2]));
                        m *= pxr::GfMatrix4d().SetTranslate(translate);
                        transformOp.Set(m);

                    }
                }
                if ((joystickZeroRight - stickRight).GetLength() > 0.08)
                {
                    pitch += stickRight[0] * rotationMultiplier;
                    yaw += stickRight[1] * rotationMultiplier;
                }

                if (abs(joystickTriggerLeft - state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]) > 0.05)
                    lookAtDistance -= (state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1.0) * distanceMultiplier;
                if (abs(joystickTriggerRight - state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]) > 0.05)
                    lookAtDistance += (state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1.0) * distanceMultiplier;
                lookAtDistance = std::max(0.1, lookAtDistance);


            }
        }

        if (currentFilename != newFilename)
        {
            currentFilename = newFilename;
            stage = pxr::UsdStage::Open(currentFilename);
            stage->Load(pxr::SdfPath::AbsoluteRootPath());

            pxr::SdfPathVector excludedPaths;
            engine.reset(new pxr::UsdImagingGLEngine(
                stage->GetPseudoRoot().GetPath(), excludedPaths));
        }

        if (!delegateSelectionMode && newDelegate != currentDelegate)
        {
            currentDelegate = newDelegate;
            engine->SetRendererPlugin(renderDelegates[currentDelegate]);
        }

        // set cube rotation
        //
        //cubeMeshOp.Set(float(frame));

        // get display size (inner display buffer)
        //
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // get full-window size (borders included)
        //
        int window_w, window_h;
        glfwGetWindowSize(window, &window_w, &window_h);

        cameraTransform.SetIdentity();
        cameraTransform *= pxr::GfMatrix4d().SetTranslate(pxr::GfVec3d(-offsetX, -offsetY, 0.0));
        cameraTransform *= pxr::GfMatrix4d().SetTranslate(pxr::GfVec3d(0, 0, lookAtDistance));
        cameraTransform *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(0, 0, 1), -rollX * 5.0));
        cameraTransform *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(1, 0, 0), -yaw * 5.0));
        cameraTransform *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(0, 1, 0), -pitch * 5.0));
        cameraTransform *= pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d(0, 0, 1), -rollY * 5.0));
        cameraTransform *= pxr::GfMatrix4d().SetTranslate(cameraPivot);

        camera.SetTransform(cameraTransform);
        frustum = camera.GetFrustum();
        double fovy = focalLength;
        double znear = 0.5;
        double zfar = 100000.0;
        const double aspectRatio = double(display_w) / double(display_h);
        frustum.SetPerspective(fovy, aspectRatio, znear, zfar);
        projectionMatrix = frustum.ComputeProjectionMatrix();
        viewMatrix = frustum.ComputeViewMatrix();

        //if (!pxr::UsdImagingGLEngine::IsColorCorrectionCapable())
        //    glEnable(GL_FRAMEBUFFER_SRGB_EXT);

        glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // setup viewport for 3d scene render
        glPushMatrix();
        {
            glViewport(0, 0, display_w, display_h);
            pxr::GfVec4d viewport(0, 0, display_w, display_h);

            // scene has lighting
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);

            // Set engine properties and parameters
            engine->SetRenderBufferSize(pxr::GfVec2i(window_w, window_h));
            engine->SetSelectionColor(pxr::GfVec4f(1.0, 1.0, 0.0, 1.0));
            engine->SetCameraState(viewMatrix, projectionMatrix);
            engine->SetRenderViewport(viewport);
            engine->SetEnablePresentation(true);

            // update render params
            renderParams.frame = frame;
            renderParams.enableLighting = true;
            renderParams.enableSceneLights = true;
            renderParams.enableSceneMaterials = true;
            renderParams.showProxy = true;
            renderParams.showRender = false;
            renderParams.showGuides = false;
            renderParams.forceRefresh = false;
            renderParams.highlight = highlight;
            renderParams.enableUsdDrawModes = true;
            renderParams.drawMode = pxr::UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
            renderParams.gammaCorrectColors = true;
            renderParams.clearColor = clearColor;
            renderParams.enableIdRender = false;
            renderParams.enableSampleAlphaToCoverage = false;
            renderParams.complexity = 1.0f;

            // render all paths from root
            engine->Render(stage->GetPseudoRoot(), renderParams);
        }
        glPopMatrix();

        pxr::GfVec2d halfSize(1.0 / display_w, 1.0 / display_h);
        pxr::GfVec2d screenPoint(2.0 * ((display_w/2.0) / display_w) - 1.0, 2.0 * (1.0 - (display_h/2.0) / display_h) - 1.0);

        // Compute pick frustum.
        auto pickFrustum = frustum.ComputeNarrowedFrustum(screenPoint, halfSize);
        auto pickView = pickFrustum.ComputeViewMatrix();
        auto pickProj = pickFrustum.ComputeProjectionMatrix();

        pxr::GfVec3d selectionHitPoint;
        pxr::GfVec3d selectionHitNormal;

        if (!primLocked)
        {
            selectedPrimPath = pxr::SdfPath();
            if (highlight && engine->TestIntersection(
                pickView,
                pickProj,
                stage->GetPseudoRoot(),
                renderParams,
                &selectionHitPoint,
                &selectionHitNormal,
                &selectedPrimPath))
            {
                engine->SetSelected({ selectedPrimPath });
            }
            else
            {
                engine->ClearSelected();
            }
        }

        glClear(GL_DEPTH_BUFFER_BIT);

        // HUD
        glDisable(GL_LIGHTING);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPushMatrix();
        {
            glViewport(0, 0, display_w, display_h);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            
            // draw white cross
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex3f(-0.1, 0.0, 0.0);
            glVertex3f(0.1, 0.0, 0.0);
            glVertex3f(0.0, -0.1, 0.0);
            glVertex3f(0.0, 0.1, 0.0);
            glEnd();
            
            // draw text
            pfResetTop();
            pfPixelSize(2.0f);
            pfDisplaySize(display_w, display_h);

            // show help
            if (showHelp)
            {
                pfText(std::string("            fov = ") + std::to_string(focalLength), false);
                pfText(std::string("eyes-height(cm) = ") + std::to_string(eyesHeight), false);
                pfText(std::string("camera-distance = ") + std::to_string(lookAtDistance), false);
                pfText(std::string(""), false);
                pfText(std::string("Multipliers:"), false);
                pfText(std::string("  position = ") + std::to_string(positionMultiplier), false);
                pfText(std::string("  rotation = ") + std::to_string(rotationMultiplier), false);
                pfText(std::string("    height = ") + std::to_string(heightMultiplier), false);
                pfText(std::string("  distance = ") + std::to_string(distanceMultiplier), false);
                pfText(std::string(""), false);
            }
            // show delegates only if in selection mode
            if (delegateSelectionMode)
            {
                pfText(std::string("Available delegates:"), false);
                for (size_t i = 0; i < renderDelegates.size(); ++i)
                {
                    std::string currline = "[" + std::to_string(i) + "] "
                        + engine->GetRendererDisplayName(renderDelegates[i])
                        + " (" + renderDelegates[i].GetString() + ")";
                    pfText(currline, newDelegate == i);
                }
            }
            else
            {
                std::string currline = engine->GetRendererDisplayName(renderDelegates[currentDelegate])
                    + " (" + renderDelegates[currentDelegate].GetString() + ")";
                pfText(currline, false);
            }
        }
        glPopMatrix();

        // Keep running
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}