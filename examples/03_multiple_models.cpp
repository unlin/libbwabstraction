// Example code for rendering multiple models.

#include <bwabstraction.hpp>
#include <opencv2/opencv.hpp>

int main(void)
{
    bwabstraction::BWAbstraction bwa;
    bwabstraction::Result result;
    bwabstraction::Parameters param;
    param.verbose = true;

    std::string models[6] = {
        "model/Y4701_abacus.obj",
        "model/Y1704_PIANO.obj",
        "model/Y5709_Joystick.obj",
        "model/Y6379_Sparrow.obj",
        "model/Y9085_fax_machine.obj",
        "model/Y9336_microscope.obj"
    };

    std::string cameras[6] = {
        "camera/Y4701_abacus.camera.txt",
        "camera/Y1704_PIANO.camera.txt",
        "camera/Y5709_Joystick.camera.txt",
        "camera/Y6379_Sparrow.camera.txt",
        "camera/Y9085_fax_machine.camera.txt",
        "camera/Y9336_microscope.camera.txt"
    };

    // load new model in each iteration and render it.
    for (int i = 0; i < 6; ++i)
    {
        if (!bwa.LoadModel(models[i], param))
        {
            continue;
        }
        param.LoadMVPMatrixFromFile(cameras[i].c_str());
        bwa.Render(&result, param);
        char filename[15];
        sprintf(filename, "bwaImage%d.png", i + 6);
        cv::imwrite(filename, result.bwaImage);
    }

    #ifdef _MSC_VER
    system("PAUSE");
    #endif

    return 0;
}