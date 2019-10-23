// Example code for basic usage.

// include the library
#include <bwabstraction.hpp>
// for cv::imwrite()
#include <opencv2/opencv.hpp>

int main(void)
{
    // create a parameter object. use it to control the behavior of the algorithm.
    bwabstraction::Parameters param;

    // we show here all available options to the algorithm.
    param.scale = 1.0f;
    param.contrastWeight = 0.0f;
    param.inclusionWeight = 1.5f;
    param.backgroundWeight = 250.0f;
    param.neighbourWeight = 50.0f;
    param.consistencyWeight = 50.0f;
    param.featureWeight = 1000.0f;
    param.congruentThreshold = 0.005f;
    param.patchSizeThreshold = 30;
    param.verbose = true;
    param.renderWidth = 1600;
    param.renderHeight = 1200;

    // LoadMVPMatrixFromFile() is a helper function that fills param.mvpMatrix
    // with a text file of 16 real numbers. you can populate param.mvpMatrix any way you like.
    param.LoadMVPMatrixFromFile("camera/Chinese house.camera.txt");

    // create an algorithm object. use it to perform computation.
    bwabstraction::BWAbstraction bwa;

    // load 3D model with parameter object.
    if(!bwa.LoadModel("model/Chinese House.obj", param))
    {
        // load model failed. two possible reasons:
        // 1, cannot open file. 2, mesh quality is bad (face normal not aligned, duplicated vertices, etc...)
        exit(1);
    }

    // create an result object that holds results.
    bwabstraction::Result result;

    // start rendering with result object and parameter object.
    bwa.Render(&result, param);

    // save images returned.
    cv::imwrite("patches.png", result.patchImage);
    cv::imwrite("depthcritical.png", result.depthCriticalLineImage);
    cv::imwrite("sharpedge.png", result.sharpEdgeLineImage);
    cv::imwrite("boundaries.png", result.boundaryImage);
    cv::imwrite("featureline.png", result.featureLineImage);
    cv::imwrite("disttransform.png", result.distanceTransformImage);
    cv::imwrite("consistency.png", result.consistencyImage);
    cv::imwrite("components.png", result.componentImage);
    cv::imwrite("inclusions.png", result.inclusionImage);
    cv::imwrite("bwaResult.png", result.bwaImage);

    #ifdef _MSC_VER
    system("PAUSE");
    #endif

    return 0;
}