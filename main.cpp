// workaround for a compilation issue 
// https://github.com/microsoft/STL/issues/2335#issuecomment-967306862
#define _STL_CRT_SECURE_INVALID_PARAMETER(expr) _CRT_SECURE_INVALID_PARAMETER(expr)

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <pxr/base/tf/preprocessorUtils.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/mesh.h>
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

bool animate = true;

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

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "simplecube", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed creating a glfw window with OpenGL, retrying without it" << std::endl;
        return 1;
    }
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    pxr::GlfContextCaps::InitInstance();

    std::unique_ptr<class pxr::UsdImagingGLEngine> engine;
    pxr::GfCamera camera;
    pxr::GfMatrix4d cameraTransform;
    pxr::GfVec3d cameraPivot(0,0,0);

    pxr::GfMatrix4d viewMatrix;
    pxr::GfMatrix4d projectionMatrix;
    pxr::GfFrustum frustum;

    pxr::UsdImagingGLRenderParams renderParams;

    pxr::UsdStageRefPtr stage = pxr::UsdStage::CreateInMemory();

    // cube-mesh
    pxr::UsdGeomMesh& cubeMesh = pxr::UsdGeomMesh::Define( stage, pxr::SdfPath("/myCubeMesh"));
    cubeMesh.CreateOrientationAttr().Set( pxr::UsdGeomTokens->leftHanded );
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
    displayColors.emplace_back(pxr::GfVec3f(1,0,1));
    displayColors.emplace_back(pxr::GfVec3f(0,0,1));
    displayColors.emplace_back(pxr::GfVec3f(1,1,1));
    displayColors.emplace_back(pxr::GfVec3f(0,1,1));
    displayColors.emplace_back(pxr::GfVec3f(0,0,0));
    displayColors.emplace_back(pxr::GfVec3f(1,0,0));
    displayColors.emplace_back(pxr::GfVec3f(0,1,0));
    displayColors.emplace_back(pxr::GfVec3f(1,1,0));
    pxr::UsdAttribute& displayColorsAttr = cubeMesh.CreateDisplayColorAttr();
    displayColorsAttr.Set(displayColors);
    pxr::UsdGeomPrimvar displayColorsPrimvar(displayColorsAttr);
    displayColorsPrimvar.SetInterpolation( pxr::UsdGeomTokens->vertex );

    pxr::VtArray<int> faceVertexCounts = { 4, 4, 4, 4, 4, 4 };
    cubeMesh.CreateFaceVertexCountsAttr().Set(faceVertexCounts);
    pxr::VtArray<int> faceVertexIndices = { 0, 1, 3, 2, 4, 5, 7, 6, 6, 7, 2, 3, 5, 4, 1, 0, 5, 0, 2, 7, 1, 4, 6, 3 };
    cubeMesh.CreateFaceVertexIndicesAttr().Set(faceVertexIndices);
    auto cubeMeshOp = cubeMesh.AddRotateYOp();

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

    pxr::GfVec4f clearColor(0.18f, 0.18f, 0.18f, 1.0f);

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

        // set cube rotation
        //
        cubeMeshOp.Set(float(frame));

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
            renderParams.frame = 1;
            renderParams.enableLighting = false;
            renderParams.enableSceneLights = false;
            renderParams.enableSceneMaterials = false;
            renderParams.showProxy = true;
            renderParams.showRender = false;
            renderParams.showGuides = false;
            renderParams.forceRefresh = false;
            renderParams.highlight = true;
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

        // Keep running
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}