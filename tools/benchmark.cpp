#include <bwabstraction.hpp>
#include <opencv2/opencv.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <fstream>
#include <cstdlib>
#include <boost/timer.hpp>
#include <iostream>

using namespace std;

glm::mat4 LoadCameraMVP(const char* file)
{
    std::ifstream in;
    in.open(file, std::ifstream::in);
    if (in.is_open())
    {
        glm::mat4 mvp;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                float f;
                in >> f;
                mvp[i][j] = f;
            }
        }
        in.close();
        return mvp;
    }
    else
    {
        return glm::mat4();
    }
}

void benchmark_one_model_one_render()
{
    boost::timer t;

    bwabstraction::Parameters param;
    param.scale = 1.0f;
    param.verbose = false;
    param.renderWidth = 1600;
    param.renderHeight = 1200;

    glm::mat4 mvp = LoadCameraMVP("camera/Chinese House.camera.txt");
    memcpy(param.mvpMatrix, glm::value_ptr(mvp), sizeof(float) * 16);
    bwabstraction::BWAbstraction bwa;
    bwa.LoadModel("model/Chinese House.obj", param);
    bwabstraction::Result result;
    bwa.Render(&result, param);

    cout << "benchmark_one_model_one_render: " << t.elapsed() << endl;
}

void benchmark_one_model_multiple_render()
{
    boost::timer t;

    bwabstraction::Parameters param;
    param.scale = 1.0f;
    param.verbose = false;
    param.renderWidth = 1600;
    param.renderHeight = 1200;

    glm::mat4 mvp = LoadCameraMVP("camera/Chinese House.camera.txt");
    glm::mat4 rotation = glm::rotate(60.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    memcpy(param.mvpMatrix, glm::value_ptr(mvp), sizeof(float) * 16);
    bwabstraction::BWAbstraction bwa;
    bwa.LoadModel("model/Chinese House.obj", param);
    bwabstraction::Result result;

    for (int i = 0; i < 10; ++i)
    {
        mvp = mvp * rotation;
        memcpy(param.mvpMatrix, glm::value_ptr(mvp), sizeof(float) * 16);
        bwa.Render(&result, param);
    }

    cout << "benchmark_one_model_multiple_render: " << t.elapsed() << endl;
}

void benchmark_multiple_model_one_render()
{
    bwabstraction::Parameters param;
    param.scale = 1.0f;
    param.verbose = false;
    param.renderWidth = 1600;
    param.renderHeight = 1200;
}

void benchmark_multiple_model_multiple_render()
{
    bwabstraction::Parameters param;
    param.scale = 1.0f;
    param.verbose = false;
    param.renderWidth = 1600;
    param.renderHeight = 1200;
}

int main(void)
{
    //benchmark_one_model_one_render();
    benchmark_one_model_multiple_render();
    system("PAUSE");
    return 0;
}