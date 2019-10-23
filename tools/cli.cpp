// Command line interface program code.

#include <bwabstraction.hpp>
#include <opencv2/opencv.hpp>
#include <glm/vec3.hpp>   // glm::vec3
#include <glm/vec4.hpp>   // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <cxxopts.hpp>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    // create cxxopts instance
    cxxopts::Options options(argv[0], " - command line interface for BWAbstraction");

    // add positional argument help text
    options
        .positional_help("input.obj camera.txt output.png")
        .show_positional_help();

    options.add_options()
        ("h,help", "Optional. Print help.")
        ("input", "Positional. Required. 3D model file to be rendered.", cxxopts::value<string>())
        ("camera", "Positional. Required. Camera settings expressed by a 4x4 matrix.", cxxopts::value<string>())
        ("output", "Positional. Required. Output image file name.", cxxopts::value<string>())
        ("scale", "Optional. Scale factor.", cxxopts::value<float>())
        ("contrastWeight", "Optional. Weight of contrast term.", cxxopts::value<float>())
        ("inclusionWeight", "Optional. Weight of inclusion term.", cxxopts::value<float>())
        ("backgroundWeight", "Optional. Weight of background term.", cxxopts::value<float>())
        ("neighbourWeight", "Optional. Weight of neighbour term.", cxxopts::value<float>())
        ("consistencyWeight", "Optional. Weight of consistency term.", cxxopts::value<float>())
        ("featureWeight", "Optional. Weight of feature term.", cxxopts::value<float>())
        ("congruentThreshold", "Optional. Threshold of congruency between mesh components.", cxxopts::value<float>())
        ("patchSizeThreshold", "Optional. Threshold of minimal patch size in pixels.", cxxopts::value<int>())
        ("v,verbose", "Optional. Verbose mode.", cxxopts::value<bool>()->implicit_value("true"))
        ("renderWidth", "Optional. Render width in pixels.", cxxopts::value<int>())
        ("renderHeight", "Optional. Render height in pixels.", cxxopts::value<int>());

    // designate "input", "camera" and "output" as positional arguments
    options.parse_positional({"input", "camera", "output"});

    // parse argument, get result object args
    try
    {
        auto args = options.parse(argc, argv);

        // if h/help option is used, print help and exit
        if (args.count("h"))
        {
            cout << options.help({ "" }) << endl;
            exit(0);
        }

        // initialize BWAbstraction algorithm parameters, basing on user argument
        bwabstraction::Parameters param;
        if (args.count("scale"))
        {
            param.scale = args["scale"].as<float>();
        }
        if (args.count("contrastWeight"))
        {
            param.contrastWeight = args["contrastWeight"].as<float>();
        }
        if (args.count("inclusionWeight"))
        {
            param.inclusionWeight = args["inclusionWeight"].as<float>();
        }
        if (args.count("backgroundWeight"))
        {
            param.backgroundWeight = args["backgroundWeight"].as<float>();
        }
        if (args.count("neighbourWeight"))
        {
            param.neighbourWeight = args["neighbourWeight"].as<float>();
        }
        if (args.count("consistencyWeight"))
        {
            param.consistencyWeight = args["consistencyWeight"].as<float>();
        }
        if (args.count("featureWeight"))
        {
            param.featureWeight = args["featureWeight"].as<float>();
        }
        if (args.count("congruentThreshold"))
        {
            param.congruentThreshold = args["congruentThreshold"].as<float>();
        }
        if (args.count("patchSizeThreshold"))
        {
            param.patchSizeThreshold = args["patchSizeThreshold"].as<int>();
        }
        if (args.count("verbose"))
        {
            param.verbose = args["verbose"].as<bool>();
        }
        if (args.count("renderWidth"))
        {
            param.renderWidth = args["renderWidth"].as<int>();
        }
        if (args.count("renderHeight"))
        {
            param.renderHeight = args["renderHeight"].as<int>();
        }

        // load camera file and input file, run the algorithm, then save result image as output file
        param.LoadMVPMatrixFromFile(args["camera"].as<string>());
        bwabstraction::BWAbstraction bwa;
        bwa.LoadModel(args["input"].as<string>(), param);
        bwabstraction::Result result;
        bwa.Render(&result, param);
        cv::imwrite(args["output"].as<string>(), result.bwaImage);
    }
    catch (cxxopts::OptionException ex)
    {
        // if the user missed any positional argument, an OptionException is thrown
        cout << ex.what() << endl;
        exit(1);
    }

    return 0;
}