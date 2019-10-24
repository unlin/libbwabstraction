#include <bwabstraction.hpp>
#include <opencv2/opencv.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <cstdlib>
#include <boost/timer.hpp>
#include <iostream>

using namespace std;

void benchmark_one_model_one_render()
{
    boost::timer t;

    bwabstraction::Parameters param;
    bwabstraction::BWAbstraction bwa;
    bwabstraction::Result result;
    param.LoadMVPMatrixFromFile("camera/Chinese House.camera.txt");
    bwa.LoadModel("model/Chinese House.obj", param);
    
    bwa.Render(&result, param);

    cout << "benchmark_one_model_one_render: " << t.elapsed() << endl;
}

void benchmark_one_model_multiple_render()
{
    boost::timer t;

    bwabstraction::Parameters param;
    bwabstraction::BWAbstraction bwa;
    bwabstraction::Result result;
    param.LoadMVPMatrixFromFile("camera/Chinese House.camera.txt");
    bwa.LoadModel("model/Chinese House.obj", param);

    glm::mat4 mvp;
    for (int i = 0; i < 16; ++i) glm::value_ptr(mvp)[i] = param.mvpMatrix[i];
    glm::mat4 rotation = glm::rotate(30.0f, glm::vec3(0.0f, 0.0f, 1.0f));

    for (int i = 0; i < 10; ++i)
    {
        mvp = mvp * rotation;
        memcpy(param.mvpMatrix, glm::value_ptr(mvp), sizeof(float) * 16);
        bwa.Render(&result, param);

        //char name[50];
        //sprintf(name, "bench2-%d.png", i);
        //cv::imwrite(name, result.bwaImage);
    }

    cout << "benchmark_one_model_multiple_render: " << t.elapsed() << endl;
}

void benchmark_multiple_model_one_render()
{
    boost::timer t;

    std::string models[10] = {
        "model/Y4701_abacus.obj",
        "model/Y1704_PIANO.obj",
        "model/Y5709_Joystick.obj",
        "model/Y6379_Sparrow.obj",
        "model/Y9085_fax_machine.obj",
        "model/Y9336_microscope.obj",
        "model/Y60_bethoven.obj",
        "model/Y10399_White House.obj",
        "model/Y3664_gazebo.obj",
        "model/Y6002_OUTLET.obj",
    };

    std::string cameras[10] = {
        "camera/Y4701_abacus.camera.txt",
        "camera/Y1704_PIANO.camera.txt",
        "camera/Y5709_Joystick.camera.txt",
        "camera/Y6379_Sparrow.camera.txt",
        "camera/Y9085_fax_machine.camera.txt",
        "camera/Y9336_microscope.camera.txt",
        "camera/Y60_bethoven.camera.txt",
        "camera/Y10399_White House.camera.txt",
        "camera/Y3664_gazebo.camera.txt",
        "camera/Y6002_OUTLET.camera.txt",
    };

    bwabstraction::Parameters param;
    bwabstraction::BWAbstraction bwa;
    bwabstraction::Result result;

    for (int i = 0; i < 10; ++i)
    {
        param.LoadMVPMatrixFromFile(cameras[i]);
        bwa.LoadModel(models[i], param);
        bwa.Render(&result, param);
    }

    cout << "benchmark_multiple_model_one_render: " << t.elapsed() << endl;
}

void benchmark_multiple_model_multiple_render()
{
    boost::timer t;

    std::string models[10] = {
        "model/Y4701_abacus.obj",
        "model/Y1704_PIANO.obj",
        "model/Y5709_Joystick.obj",
        "model/Y6379_Sparrow.obj",
        "model/Y9085_fax_machine.obj",
        "model/Y9336_microscope.obj",
        "model/Y60_bethoven.obj",
        "model/Y10399_White House.obj",
        "model/Y3664_gazebo.obj",
        "model/Y6002_OUTLET.obj",
    };

    std::string cameras[10] = {
        "camera/Y4701_abacus.camera.txt",
        "camera/Y1704_PIANO.camera.txt",
        "camera/Y5709_Joystick.camera.txt",
        "camera/Y6379_Sparrow.camera.txt",
        "camera/Y9085_fax_machine.camera.txt",
        "camera/Y9336_microscope.camera.txt",
        "camera/Y60_bethoven.camera.txt",
        "camera/Y10399_White House.camera.txt",
        "camera/Y3664_gazebo.camera.txt",
        "camera/Y6002_OUTLET.camera.txt",
    };

    bwabstraction::Parameters param;
    bwabstraction::BWAbstraction bwa;
    bwabstraction::Result result;

    for (int i = 0; i < 10; ++i)
    {
        param.LoadMVPMatrixFromFile(cameras[i]);
        bwa.LoadModel(models[i], param);

        glm::mat4 mvp;
        for (int i = 0; i < 16; ++i) glm::value_ptr(mvp)[i] = param.mvpMatrix[i];
        glm::mat4 rotation = glm::rotate(30.0f, glm::vec3(0.0f, 0.0f, 1.0f));

        for (int j = 0; j < 10; ++j)
        {
            mvp = mvp * rotation;
            memcpy(param.mvpMatrix, glm::value_ptr(mvp), sizeof(float) * 16);
            bwa.Render(&result, param);

            //char name[50];
            //sprintf(name, "bench4-%d-%d.png", i, j);
            //cv::imwrite(name, result.bwaImage);
        }
        
    }

    cout << "benchmark_multiple_model_multiple_render: " << t.elapsed() << endl;
}

int main(void)
{
    benchmark_one_model_one_render();
    benchmark_one_model_multiple_render();
    benchmark_multiple_model_one_render();
    benchmark_multiple_model_multiple_render();

#ifdef _MSC_VER
    system("PAUSE");
#endif

    return 0;
}