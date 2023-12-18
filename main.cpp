// workaround for a compilation issue 
// https://github.com/microsoft/STL/issues/2335#issuecomment-967306862
#define _STL_CRT_SECURE_INVALID_PARAMETER(expr) _CRT_SECURE_INVALID_PARAMETER(expr)

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <pxr/base/tf/preprocessorUtils.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
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

#define WIDTH 1024
#define HEIGHT 768

double lookAtDistance = 0.0;
double yaw = 0.0;
double pitch = 0.0;
double rollX = 0.0;
double rollY = 0.0;
double offsetX = 0.0;
double offsetY = 0.0;
float domeExposure = 1.0f;

int currentDelegate = 0;
int newDelegate = 0;

bool animate = false;

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
}

int main(int argc, char** argv)
{
	if (!glfwInit())
	{
		std::cout << "Failed initializing glfw" << std::endl;
		return 1;
	}
    glfwSetErrorCallback(error_callback);


    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_SAMPLES, 1);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "simplecube", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed creating a glfw window with OpenGL, retrying without it" << std::endl;
        return 1;
    }
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    //gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // Enable vsync

    pxr::GlfContextCaps::InitInstance();

    std::unique_ptr<class pxr::UsdImagingGLEngine> engine;
    pxr::GfCamera camera;
    pxr::GfMatrix4d cameraTransform;
    pxr::GfVec3d cameraPivot(0,0,0);

    pxr::GfMatrix4d viewMatrix;
    pxr::GfMatrix4d projectionMatrix;
    pxr::GfFrustum frustum;

    pxr::UsdImagingGLRenderParams renderParams;

    pxr::UsdStageRefPtr stage;
    //stage = pxr::UsdStage::Open("C:/Users/paolo/Desktop/usdassets/Kitchen_set/Kitchen_set.usd");
    stage = pxr::UsdStage::Open("C:\\Users\\paolo\\Desktop\\solaris\\rubbertoys.usda");
    stage->Load(pxr::SdfPath::AbsoluteRootPath());

#if PXR_VERSION >= 2311
    pxr::UsdLuxDomeLight& domeLight = pxr::UsdLuxDomeLight::Define(stage, pxr::SdfPath("/myDomeLight"));
#else
    pxr::UsdLuxDomeLight domeLight = pxr::UsdLuxDomeLight::Define(stage, pxr::SdfPath("/myDomeLight"));
#endif

#if PXR_VERSION >= 2311
    pxr::UsdGeomCube& cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/myCube"));
#else
    pxr::UsdGeomCube cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/myCube"));
#endif
    cube.CreateSizeAttr().Set(1.0);
    auto cubeOp = cube.AddRotateYOp();

    pxr::SdfPathVector excludedPaths;
    engine.reset(new pxr::UsdImagingGLEngine(
        stage->GetPseudoRoot().GetPath(), excludedPaths));

    std::cout << "Available Hydra Delegates:" << std::endl;
#if PXR_VERSION >= 2311
    auto& renderDelegates = engine->GetRendererPlugins();
#else
    auto renderDelegates = engine->GetRendererPlugins();
#endif
    for (size_t i = 0; i < renderDelegates.size(); ++i)
    {
        std::cout << "[" << (i) << "] "
            << engine->GetRendererDisplayName(renderDelegates[i])
            << " (" << renderDelegates[i] << ")" << std::endl;
    }
    bool enabled = engine->SetRendererPlugin(renderDelegates[0]);

    int frame = 0;

    while (!glfwWindowShouldClose(window))
    {
        if(animate)
            frame++;

        glfwMakeContextCurrent(window);

        glfwPollEvents();

        if (newDelegate != currentDelegate)
        {
            currentDelegate = newDelegate;
            engine->SetRendererPlugin(renderDelegates[currentDelegate]);
        }

        domeLight.CreateIntensityAttr().Set(domeExposure);
        domeLight.CreateExposureAttr().Set(domeExposure);
        cubeOp.Set(float(frame));

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
        double fovy = 30.0;
        double znear = 0.1;
        double zfar = 10000.0;
        const double aspectRatio = double(display_w) / double(display_h);
        frustum.SetPerspective(fovy, aspectRatio, znear, zfar);
        projectionMatrix = frustum.ComputeProjectionMatrix();
        viewMatrix = frustum.ComputeViewMatrix();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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
            renderParams.frame = 1;
            renderParams.enableLighting = true;
            renderParams.enableSceneLights = true;
            renderParams.enableSceneMaterials = false;
            renderParams.showProxy = true;
            renderParams.showRender = false;
            renderParams.showGuides = false;
            renderParams.forceRefresh = false;
            renderParams.highlight = true;
            renderParams.enableUsdDrawModes = true;
            renderParams.drawMode = pxr::UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
            renderParams.gammaCorrectColors = false;
            renderParams.clearColor = pxr::GfVec4f(0.1f, 0.1f, 0.1f, 1.0f);
            renderParams.enableIdRender = false;
            renderParams.enableSampleAlphaToCoverage = false;
            // From Pixar's doc, complexity values are 
            // low=1.0, medium=1.1, high=1.2, veryhigh=1.3
            renderParams.complexity = 1.0f;

            // render all paths from root
            engine->Render(stage->GetPseudoRoot(), renderParams);
        }
        glPopMatrix();

        // Keep running
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}