#ifndef BWABSTRACTION_H
#define BWABSTRACTION_H

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

//#define BWA_MULTITHREAD
#define USE_EMBEDDED_SHADER 1

#include <opencv2/opencv.hpp>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

#ifdef BWA_MULTITHREAD
#define NUM_THREADS 4
#endif

#define MAX_WIDTH 1600
#define MIN_WIDTH 200
#define SCALE_LARGEST_PX 4.5
#define SCALE_SMALLEST_PX 20
#define SCALE_TO_IMAGE_WIDTH(s) (MIN_WIDTH + (MAX_WIDTH - MIN_WIDTH) * (s))
#define SCALE_TO_PIXEL_WIDTH(s) (SCALE_SMALLEST_PX - (SCALE_SMALLEST_PX - SCALE_LARGEST_PX) * ((SCALE_TO_IMAGE_WIDTH(s) - MIN_WIDTH) / (MAX_WIDTH - MIN_WIDTH)))

class GLFWwindow;

typedef struct _INT32DTarget INT32DTarget;
typedef struct _RGBADTarget RGBADTarget;
class StandardShader;

namespace bwabstraction {

class TriMesh;

// bitfield
enum ResultImage
{
    BWA = 1,
    PATCH = 1 << 1,
    DEPTH_CRITICAL = 1 << 2,
    SHARP_EDGE = 1 << 3,
    BOUNDARY = 1 << 4,
    FEATURE_LINE = 1 << 5,
    DISTANCE_TRANSFORM = 1 << 6,
    CONSISTENCY = 1 << 7,
    COMPONENT = 1 << 8,
    INCLUSION = 1 << 9,
    ALL = (1 << 10) - 1,
};

typedef struct _Parameters
{

    float scale;
    float contrastWeight;
    float inclusionWeight;
    float backgroundWeight;
    float neighbourWeight;
    float consistencyWeight;
    float featureWeight;

    float congruentThreshold;
    int patchSizeThreshold;

    bool useHostOpenGL;
    bool verbose;
    int renderWidth;
    int renderHeight;
    int resultImage;

    float mvpMatrix[16];
    float backgroundColor[3];

    _Parameters()
    {
        renderWidth = 1600;
        renderHeight = 1200;

        scale = 1.0f;
        contrastWeight = 0.0f;
        inclusionWeight = 1.5f;
        backgroundWeight = 250.0f;
        neighbourWeight = 50.0f;
        consistencyWeight = 50.0f;
        featureWeight = 1000.0f;

        congruentThreshold = 0.005f;
        patchSizeThreshold = 30;

        useHostOpenGL = false;
        verbose = false;

        resultImage += ResultImage::BWA;

        backgroundColor[0] = backgroundColor[1] = backgroundColor[2] = 1.0f;
    }

    void LoadMVPMatrixFromFile(std::string file);

} Parameters;

typedef struct _Result
{

    cv::Mat bwaImage;
    cv::Mat patchImage;
    cv::Mat depthCriticalLineImage;
    cv::Mat boundaryImage;
    cv::Mat sharpEdgeLineImage;
    cv::Mat featureLineImage;
    cv::Mat consistencyImage;
    cv::Mat componentImage;
    cv::Mat inclusionImage;
    cv::Mat distanceTransformImage;

} Result;

typedef struct BoundingBox_S
{

    int min_x = INT_MAX;
    int min_y = INT_MAX;
    int max_x = -INT_MAX;
    int max_y = -INT_MAX;

    void union_point(int x, int y)
    {
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }

    struct BoundingBox_S intersect_box(struct BoundingBox_S a)
    {
        struct BoundingBox_S b;
        b.min_x = std::max(min_x, a.min_x);
        b.min_y = std::max(min_y, a.min_y);
        b.max_x = std::min(max_x, a.max_x);
        b.max_y = std::min(max_y, a.max_y);
        return b;
    }

    int size()
    {
        return std::max(0, (max_x - min_x)) * std::max(0, (max_y - min_y));
    }

    bool operator==(struct BoundingBox_S bb)
    {
        return min_x == bb.min_x && min_y == bb.min_y && max_x == bb.max_x && max_y == bb.max_y;
    }

} BoundingBox;

typedef struct _Patch
{

    std::vector<std::pair<int, int> > pixels;
    BoundingBox boundingBox;
    int componentID;
    int label;
    int labelOverride;
    int boundaryLength;
    int backgroundBoundaryLength;
    float maxDistanceTransform;
    std::unordered_set<int> neighbourPatches;

    _Patch()
    {
        componentID = label = labelOverride = -1;
        boundaryLength = backgroundBoundaryLength = 0;
        maxDistanceTransform = 0.0f;
    }

} Patch;

typedef struct _Boundary
{
    int patchIDs[2];
    int votes[2];
    std::vector<std::pair<int, int> > pixels[2];
    float avgDepthDiff;
    int label;
    int labelOverride;

    _Boundary(int pid, int pid2)
    {
        patchIDs[0] = pid;
        patchIDs[1] = pid2;
        votes[0] = votes[1] = 0;
        label = labelOverride = -1;
        avgDepthDiff = 0.0f;
    }

} Boundary;

typedef struct _FeatureLine
{
    int patchID;
    std::vector<std::pair<int, int> > pixels;

    _FeatureLine(int pid)
    {
        patchID = pid;
    }
} FeatureLine;

class BWAbstraction
{

public:

    BWAbstraction();
    bool LoadModel(std::string modelFilePath, bwabstraction::Parameters param);
    void Render(Result *result, Parameters param);

private:
    // steps done in LoadModel (only need to be run once for each model)
    void ComputeCongruencies(void);
    void ComputeSharpEdges(void);
    void ComputeSurfaceConnect(void);
    
    // 1st step
    void ComputePatches(void);
    // steps of ComputePatches
    void RenderPatches(void);
    void ListSurfacePixels(void);
    void TriangleSurfaceConnectFloodfill(void); // includes depth critical
    void ComponentFloodfill(void);
    void GreedyMergePixel(void);
    void GreedyMergePatch(void);

    // 2nd step
    void ComputeBoundaries(void); // includes distance transform

    // 3rd step
    void ComputeFeatureLines(void);
    // step of ComputeFeatureLines
    void RenderSharpEdgeLines(void);

    // 4th step
    void ComputeSimilaritySets(void);

    // 5th step
    void ComputeInclusionPairs(void);

    // 6th step
    void BeliefPropagationOptimization(void);
    // terms used in optimization
    float b_term(int c);
    float a_term(int q);
    float d_term(int c);
    float D_term(int q);
    float l_term(int c);
    float lbg_term(int q);
    float scale_contrast_term(int c);
    float scale_line_term(int c);

    // final step
    cv::Mat RenderBWAImage(bwabstraction::Parameters param);

    // other utility functions
    void InitializeGL(void);
    void InitializeGLResources(void);
    void AddPixelToBoundary(int row, int col, int pid2, float depth2);

    GLFWwindow* glWindow;
    bool glInitialized;
    bool glResourceInitialized;
    TriMesh* mesh;
    OpenMesh::FPropHandleT<unsigned int> meshFPropComponentID;
    Parameters param;
    Result* result;
    std::vector<TriMesh*> components;
    std::vector<std::pair<int, int> > congruencies;
    std::vector<float> sharpEdges;
    std::vector<int> surfaceOffset;
    std::vector<int> surfaceConnect;
    std::vector<Patch> patches;
    std::vector<Boundary> boundaries;
    std::vector<FeatureLine> featureLines;
    std::vector<int> nodeLabel;
    int renderCount = 0;

    template<typename T1, typename T2>
    struct pair_hash
    {
        std::size_t operator()(std::pair<T1, T2> const &p) const
        {
            std::size_t seed = 0;
            // Cantor pairing function
            return (p.first + p.second) * (p.first + p.second + 1) / 2 + p.second;
        }
    };

    std::unordered_set<std::pair<int, int>, pair_hash<int, int> > inclusionPairs;
    std::vector<std::vector<int> > similaritySets;

    // map (segA, segB)->boundary ID
    std::unordered_map<std::pair<int, int>, int, pair_hash<int, int> > boundaryMapping;

    std::vector<std::pair<int, int> > surfacePixels;
    std::vector<std::pair<int, int> > surfaceBoundaryPixels;
#ifdef BWA_MULTITHREAD
    std::vector<std::pair<int, int> > surfacePixelBins[NUM_THREADS];
#endif
    std::vector<std::pair<int, int> > strandedPixels;

    cv::Mat markedMap;
    cv::Mat distFieldMapInput;
    cv::Mat sharpEdgeLineMap;
    cv::Mat depthCritLineMap;
    cv::Mat featureMap;
    cv::Mat featureLineMap;
    cv::Mat boundaryMap;
    cv::Mat boundaryLineMap;
    cv::Mat mixedLineMap;
    cv::Mat triangleIDMap;
    cv::Mat patchIDMap;
    cv::Mat depthMap;
    cv::Mat distTransMap;

    INT32DTarget* sharpEdgeLineTarget;
    INT32DTarget* triangleIDTarget;
    INT32DTarget* patchIDTarget;
    INT32DTarget* lineMapTarget;
    RGBADTarget* outputTarget;

    StandardShader* featureLineShader;
    StandardShader* triangleIDShader;
    StandardShader* hairlineShader;

    unsigned int labelSSBO;

}; // class BWAbstraction

} // namespace bwabstraction

#endif // BWABSTRACTION_H