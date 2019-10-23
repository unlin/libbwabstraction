// Example code for multiple render with different viewing angles.

#include <bwabstraction.hpp>
#include <opencv2/opencv.hpp>

// include glm for mvp computation.
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

int main(void)
{
    bwabstraction::BWAbstraction bwa;
    bwabstraction::Result result;
    bwabstraction::Parameters param;
    param.verbose = true;

    // once a model is loaded, it can be rendered multiple times.
    if(!bwa.LoadModel("model/DualShock3.obj", param))
    {
        exit(1);
    }

    glm::mat4 mvp = 
        glm::perspective(30.0f, 4.0f / 3.0f, 0.1f, 3.0f) *
        glm::lookAt(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotation = glm::rotate(30.0f, glm::vec3(0.0f, 0.0f, 1.0f));

    // render the model in a loop, updating param.mvpMatrix each time.
    for (int i = 0; i < 6; ++i)
    {
        mvp = mvp * rotation;
        memcpy(param.mvpMatrix, glm::value_ptr(mvp), sizeof(float) * 16);
        bwa.Render(&result, param);
        char filename[15];
        sprintf(filename, "bwaImage%d.png", i);
        cv::imwrite(filename, result.bwaImage);
    }

    #ifdef _MSC_VER
    system("PAUSE");
    #endif

    return 0;
}