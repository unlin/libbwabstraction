#include "bwabstraction.hpp"
#include "mesh_segmentation.hpp"
#include <vector>
#include <utility>
#include <algorithm>
#include <queue>
#include <cmath>
#include <fstream>
#include <Eigen/Eigen>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glutils.hpp"
#include "trimesh.hpp"
#include "rendertarget.hpp"
#include "standardshader.hpp"
#include "timer.hpp"

#if USE_EMBEDDED_SHADER
#include "shaders/shaders.hpp"
#endif

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// undef max/min macros defined in minwindef.h.
// these macros will break the codes in opengm.
#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif

#ifdef _MSC_VER

// disable C4267 and C4244 (conversion possible loss of data warning) for OpenGM modules
#pragma warning(push)
#pragma warning(disable:4267)
#pragma warning(disable:4244)
#include <opengm/graphicalmodel/space/simplediscretespace.hxx>
#include <opengm/operations/adder.hxx>
#include <opengm/graphicalmodel/graphicalmodel.hxx>
#include <opengm/inference/messagepassing/messagepassing.hxx>
#pragma warning(pop) 

#else

#include <opengm/graphicalmodel/space/simplediscretespace.hxx>
#include <opengm/operations/adder.hxx>
#include <opengm/graphicalmodel/graphicalmodel.hxx>
#include <opengm/inference/messagepassing/messagepassing.hxx>

#endif

#define PIXEL(mat, type, row, col) (*(mat.ptr<type>(row)+col))

using namespace bwabstraction;
using namespace cv;
using namespace std;
using namespace Eigen;
using namespace opengm;

void GetDistinctColor(int id, unsigned char rgb[])
{
    float hue = static_cast<float>(id % 36 * 10);
    float value = (id % 6 * 20 + 155) / 255.0f;
    float saturation = 1.0f;

    int hi = static_cast<int>(hue) / 60 % 6;
    float f = hue / 60.0f - hi;
    float p = value * (1 - saturation);
    float q = value * (1 - f * saturation);
    float t = value * (1 - (1 - f) * saturation);

    switch(hi)
    {
    case 0:
        rgb[0] = static_cast<int>(value * 255.0f);
        rgb[1] = static_cast<int>(t * 255.0f);
        rgb[2] = static_cast<int>(p * 255.0f);
        break;
    case 1:
        rgb[0] = static_cast<int>(q * 255.0f);
        rgb[1] = static_cast<int>(value * 255.0f);
        rgb[2] = static_cast<int>(p * 255.0f);
        break;
    case 2:
        rgb[0] = static_cast<int>(p * 255.0f);
        rgb[1] = static_cast<int>(value * 255.0f);
        rgb[2] = static_cast<int>(t * 255.0f);
        break;
    case 3:
        rgb[0] = static_cast<int>(p * 255.0f);
        rgb[1] = static_cast<int>(q * 255.0f);
        rgb[2] = static_cast<int>(value * 255.0f);
        break;
    case 4:
        rgb[0] = static_cast<int>(t * 255.0f);
        rgb[1] = static_cast<int>(p * 255.0f);
        rgb[2] = static_cast<int>(value * 255.0f);
        break;
    case 5:
        rgb[0] = static_cast<int>(value * 255.0f);
        rgb[1] = static_cast<int>(p * 255.0f);
        rgb[2] = static_cast<int>(q * 255.0f);
        break;
    }
}

unsigned char RGBToGray(unsigned char rgb[])
{
    return static_cast<unsigned char>(rgb[0] * 0.2126f + rgb[1] * 0.7152f + rgb[2] * 0.0722f);
}

bool cvCheckPointRange(int row, int col, Mat &mat)
{
    return row >= 0 && col >= 0 && row < mat.rows && col < mat.cols;
}

void _Parameters::LoadMVPMatrixFromFile(string file)
    {
        std::ifstream in;
        in.open(file, std::ifstream::in);
        if (in.is_open())
        {
            for (int i = 0; i < 16; ++i)
            {
                in >> mvpMatrix[i];
            }
            in.close();
        }
    }

StandardVertexAttribute CreateVertexAttrFromMesh(TriMesh *mesh)
{
    vector<float> vertices;
    vector<float> normals;
    vector<float> empty;
    vector<unsigned int> indices;
    mesh->GetMeshData(vertices, normals, indices);
    StandardVertexAttribute attr;
    attr.Create();
    attr.BufferData(vertices, normals, empty, empty, indices);
    return attr;
}

StandardVertexAttribute CreateVertexAttrFromLines(vector<float> &lines)
{
    // every 6 float form a (x1, y1, z1) <-> (x2, y2, z2) line segment
    vector<float> empty;
    vector<unsigned int> emptyIndices;
    StandardVertexAttribute attr;
    attr.Create();
    attr.BufferData(lines, empty, empty, empty, emptyIndices);
    return attr;
}

BWAbstraction::BWAbstraction() :
    mesh(NULL),
    glInitialized(false),
    glResourceInitialized(false)
{
    mesh = new TriMesh();
}

bool BWAbstraction::LoadModel(string modelFilePath, bwabstraction::Parameters param)
{
    this->param = param;
    ::Timer timer;
    if (this->param.verbose)
    {
        timer.Start();
    }

    if (!mesh->LoadOBJ(modelFilePath))
    {
        cout << "LoadOBJ() failed." << endl;
        return false;
    }

    renderCount = 0;
    mesh->Normalize();
    mesh->add_property(meshFPropComponentID);
    components = MeshSegmentation::ComponentSegmentation(mesh, meshFPropComponentID);
    ComputeCongruencies();
    ComputeSharpEdges();
    ComputeSurfaceConnect();
    if (this->param.verbose)
    {
        timer.Update();
        cout << "Precompute time: " << timer.DeltaTime() << " seconds" << endl;
    }

    return true;
}

void BWAbstraction::ComputeSharpEdges()
{
    // use sharp edges as feature lines.
    const float angleThreshold = 80.0f;
    sharpEdges.clear();
    for(TriMesh::EdgeIter eit = mesh->edges_begin(); eit != mesh->edges_end(); eit++)
    {
        TriMesh::FaceHandle f1 = mesh->face_handle(mesh->halfedge_handle(*eit, 0));
        TriMesh::FaceHandle f2 = mesh->face_handle(mesh->halfedge_handle(*eit, 1));
        if(!f1.is_valid() || !f2.is_valid())
        {
            continue;
        }
        TriMesh::Normal n1 = mesh->normal(f1).normalized();
        TriMesh::Normal n2 = mesh->normal(f2).normalized();
        if(dot(n1, n2) <= cos(angleThreshold / 180.0 * M_PI))
        {
            TriMesh::HalfedgeHandle he = mesh->halfedge_handle(*eit, 0);
            TriMesh::VertexHandle v1 = mesh->from_vertex_handle(he);
            TriMesh::VertexHandle v2 = mesh->to_vertex_handle(he);
            TriMesh::Point p1 = mesh->point(v1);
            TriMesh::Point p2 = mesh->point(v2);
            sharpEdges.push_back((float)p1[0]);
            sharpEdges.push_back((float)p1[1]);
            sharpEdges.push_back((float)p1[2]);
            sharpEdges.push_back((float)p2[0]);
            sharpEdges.push_back((float)p2[1]);
            sharpEdges.push_back((float)p2[2]);
        }
    }
}

void BWAbstraction::ComputeFeatureLines()
{
    this->featureLines.clear();
    RenderSharpEdgeLines();

#ifdef BWA_MULTITHREAD
    queue<pair<int, int> > featureLinePixelQueues[NUM_THREADS];
    #pragma omp parallel for
    for (int bin = 0; bin < NUM_THREADS; ++bin)
    {
        for (auto iter = surfacePixelBins[bin].begin(); iter != surfacePixelBins[bin].end(); ++iter)
        {
            int row = iter->first;
            int col = iter->second;
            bool isSharpEdge = PIXEL(this->sharpEdgeLineMap, int, row, col) == 1;
            bool isDepthCrit = PIXEL(this->depthCritLineMap, char, row, col) == 1;
            bool isBoundary = PIXEL(this->boundaryMap, char, row, col) == 1;
            if ((isSharpEdge || isDepthCrit) && !isBoundary)
            {
                PIXEL(this->featureMap, char, row, col) = 1;
                featureLinePixelQueues[bin].push(pair<int, int>(row, col));
            }
        }
    }
    queue<pair<int, int> > featureLinePixelQueue;
    for (int bin = 0; bin < NUM_THREADS; ++bin)
    {
        while (!featureLinePixelQueues[bin].empty())
        {
            featureLinePixelQueue.push(featureLinePixelQueues[bin].front());
            featureLinePixelQueues[bin].pop();
        }
    }
#else
    queue<pair<int, int> > featureLinePixelQueue;
    for (auto iter = surfacePixels.begin(); iter != surfacePixels.end(); ++iter)
    {
        int row = iter->first;
        int col = iter->second;
        bool isSharpEdge = PIXEL(this->sharpEdgeLineMap, int, row, col) == 1;
        bool isDepthCrit = PIXEL(this->depthCritLineMap, char, row, col) == 1;
        bool isBoundary = PIXEL(this->boundaryMap, char, row, col) == 1;
        if ((isSharpEdge || isDepthCrit) && !isBoundary)
        {
            PIXEL(this->featureMap, char, row, col) = 1;
            featureLinePixelQueue.push(pair<int, int>(row, col));
        }
    }
#endif

    static const int neighbors[] =
    {
        -1, 0,
        1, 0,
        0, -1,
        0, 1,
        -1, -1,
        1, -1,
        -1, 1,
        1, 1
    };

    while (!featureLinePixelQueue.empty())
    {
        int startRow = featureLinePixelQueue.front().first;
        int startCol = featureLinePixelQueue.front().second;
        featureLinePixelQueue.pop();

        if (PIXEL(markedMap, char, startRow, startCol) == 1)
        {
            continue;
        }

        queue<pair<int, int> > pixelQueue;
        pixelQueue.push(pair<int, int>(startRow, startCol));
        PIXEL(markedMap, char, startRow, startCol) = 1;
        int patchID = PIXEL(this->patchIDMap, int, startRow, startCol);
        FeatureLine newLine(patchID);
        newLine.pixels.push_back(pair<int, int>(startRow, startCol));
        while (!pixelQueue.empty())
        {
            int row = pixelQueue.front().first;
            int col = pixelQueue.front().second;
            pixelQueue.pop();

            for (int i = 0; i < 8; ++i)
            {
                int nrow = row + neighbors[i * 2];
                int ncol = col + neighbors[i * 2 + 1];

                if (!cvCheckPointRange(nrow, ncol, this->triangleIDMap))
                {
                    continue;
                }

                if (PIXEL(markedMap, char, nrow, ncol) == 1 || PIXEL(this->featureMap, char, nrow, ncol) == -1 ||
                    PIXEL(this->patchIDMap, int, nrow, ncol) != patchID)
                {
                    continue;
                }

                pixelQueue.push(pair<int, int>(nrow, ncol));
                newLine.pixels.push_back(pair<int, int>(nrow, ncol));
                PIXEL(markedMap, char, nrow, ncol) = 1;
            }
        }
        this->featureLines.push_back(newLine);
    }
}

void BWAbstraction::RenderSharpEdgeLines()
{
    sharpEdgeLineTarget->bindClear(-1);
    // use depth map from triangleIDTarget instead of copying the texture
    // sharpEdgeLineTarget->updateDepth(this->depthMap);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, triangleIDTarget->depth_buffer, 0);
    featureLineShader->Bind();
    glUniformMatrix4fv(
        glGetUniformLocation(featureLineShader->program, "mvp"),
        1, GL_FALSE, param.mvpMatrix);
    StandardVertexAttribute attr = CreateVertexAttrFromLines(this->sharpEdges);
    glEnable(GL_DEPTH_TEST);
    featureLineShader->Draw(GL_LINES, attr);
    glDisable(GL_DEPTH_TEST);
    this->sharpEdgeLineMap = sharpEdgeLineTarget->readback();
    // unbind the depth buffer
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sharpEdgeLineTarget->depth_buffer, 0);

    attr.Destroy();
    /*
    glm::mat4 mvp;
    memcpy(glm::value_ptr(mvp), this->param.mvpMatrix, sizeof(float) * 16);
    // for each line segment
    for (int i = 0; i < this->sharpEdges.size(); i += 6)
    {
        glm::vec4 p1(sharpEdges[i + 0], sharpEdges[i + 1], sharpEdges[i + 2], 1.0f);
        glm::vec4 p2(sharpEdges[i + 3], sharpEdges[i + 4], sharpEdges[i + 5], 1.0f);
        p1 = mvp * p1;
        p2 = mvp * p2;
        p1 /= p1.w;
        p2 /= p2.w;
        float p1x = (p1.x * 0.5f + 0.5f) * this->param.renderWidth;
        float p1y = (-p1.y * 0.5f + 0.5f) * this->param.renderHeight;
        float p2x = (p2.x * 0.5f + 0.5f) * this->param.renderWidth;
        float p2y = (-p2.y * 0.5f + 0.5f) * this->param.renderHeight;
        float p1z = p1.z * 0.5f + 0.5f;
        float p2z = p2.z * 0.5f + 0.5f;
        float xdiff = p2x - p1x;
        float ydiff = p2y - p1y;
        float zdiff = p2z - p1z;
        if (fabs(xdiff) > fabs(ydiff))
        {
            float xmin, xmax;
            if (p1x < p2x)
            {
                xmin = p1x;
                xmax = p2x;
            }
            else
            {
                xmin = p2x;
                xmax = p1x;
            }
            float yslope = ydiff / xdiff;
            float zslope = zdiff / xdiff;
            for (float x = xmin; x < xmax; x += 1.0f)
            {
                float y = p1y + yslope * (x - p1x);
                float z = p1z + zslope * (x - p1x);
                int ix = static_cast<int>(floor(x));
                int iy = static_cast<int>(floor(y));
                if (ix >= 0 && ix < this->param.renderWidth && iy >= 0 && iy < this->param.renderHeight)
                {
                    if (z <= PIXEL(this->depthMap, float, iy, ix))
                    {
                        PIXEL(this->sharpEdgeLineMap, int, iy, ix) = 1;
                    }
                }
            }
        }
        else
        {
            float ymin, ymax;
            if (p1y < p2y)
            {
                ymin = p1y;
                ymax = p2y;
            }
            else
            {
                ymin = p2y;
                ymax = p1y;
            }
            float xslope = xdiff / ydiff;
            float zslope = zdiff / ydiff;
            for (float y = ymin; y < ymax; y += 1.0f)
            {
                float x = p1x + xslope * (y - p1y);
                float z = p1z + zslope * (y - p1y);
                int ix = static_cast<int>(floor(x));
                int iy = static_cast<int>(floor(y));
                if (ix >= 0 && ix < this->param.renderWidth && iy >= 0 && iy < this->param.renderHeight)
                {
                    if (z <= PIXEL(this->depthMap, float, iy, ix))
                    {
                        PIXEL(this->sharpEdgeLineMap, int, iy, ix) = 1;
                    }
                }
            }
        }
    }
    */
}

void BWAbstraction::ComputePatches()
{
    patches.clear();
    surfacePixels.reserve(this->param.renderWidth * this->param.renderHeight);
    surfacePixels.clear();
    surfaceBoundaryPixels.clear();
#ifdef BWA_MULTITHREAD
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        surfacePixelBins[i].reserve(this->param.renderWidth * this->param.renderHeight / NUM_THREADS);
        surfacePixelBins[i].clear();
    }
#endif
    strandedPixels.clear();

    RenderPatches();
    ListSurfacePixels();
    TriangleSurfaceConnectFloodfill();
    ComponentFloodfill();
    GreedyMergePixel();
    GreedyMergePatch();
}

void BWAbstraction::TriangleSurfaceConnectFloodfill()
{
    static const int neighbors[] =
    {
        -1, 0,
        1, 0,
        0, -1,
        0, 1
    };

    queue<cv::Point> neighborQueue;
    for(auto iter = surfacePixels.begin(); iter != surfacePixels.end(); ++iter)
    {
        int row = iter->first;
        int col = iter->second;
        if(PIXEL(patchIDMap, int, row, col) != -1)
        {
            continue;
        }

        // add new patch
        int patchID = static_cast<int>(patches.size());
        neighborQueue.push(cv::Point(row, col));
        PIXEL(patchIDMap, int, row, col) = patchID;
        Patch newPatch;
        int fid = PIXEL(triangleIDMap, int, row, col);
        newPatch.componentID = mesh->property(meshFPropComponentID, TriMesh::FaceHandle(fid)) - 1; // 1-based index in property
        while(!neighborQueue.empty())
        {
            cv::Point p = neighborQueue.front();
            neighborQueue.pop();
            newPatch.pixels.push_back(pair<int, int>(p.x, p.y));
            int id = PIXEL(triangleIDMap, int, p.x, p.y);
            for(int n = 0; n < 4; ++n)
            {
                int nrow = p.x + neighbors[2 * n];
                int ncol = p.y + neighbors[2 * n + 1];

                // is (nrow, ncol) out of range or being a background?
                if(!cvCheckPointRange(nrow, ncol, triangleIDMap) || PIXEL(triangleIDMap, int, nrow, ncol) == -1)
                {
                    continue;
                }

                // is (nrow, ncol) labelled?
                if(PIXEL(patchIDMap, int, nrow, ncol) != -1)
                {
                    continue;
                }
                int id2 = PIXEL(triangleIDMap, int, nrow, ncol);
                int start = surfaceOffset[id];
                int end = surfaceOffset[id + 1];
                bool found = false;
                for(int k = start; k < end; ++k)
                {
                    if(surfaceConnect[k] == id2)
                    {
                        found = true;
                        break;
                    }
                }
                if(found)
                {
                    neighborQueue.push(cv::Point(nrow, ncol));
                    PIXEL(this->patchIDMap, int, nrow, ncol) = patchID;
                }
                else
                {
                    PIXEL(this->depthCritLineMap, char, p.x, p.y) = 1;
                }
            }
        }

        // is this newPatch large enough?
        if(newPatch.pixels.size() >= param.patchSizeThreshold)
        {
            patches.push_back(newPatch);
        }
        else
        {
            // this patch is too small. add its pixels to strandedPixels. (label = -2)
            strandedPixels.push_back(newPatch.pixels[0]);
            for(auto pixel = newPatch.pixels.begin(); pixel != newPatch.pixels.end(); ++pixel)
            {
                int row = pixel->first;
                int col = pixel->second;
                PIXEL(patchIDMap, int, row, col) = -2;
            }
        }
    }
}

void BWAbstraction::ComponentFloodfill()
{
    static const int neighbors[] =
    {
        -1, 0,
        1, 0,
        0, -1,
        0, 1
    };

    vector<pair<int, int> > newStrandedPixels;
    queue<cv::Point> neighborQueue;
    for(auto iter = strandedPixels.begin(); iter != strandedPixels.end(); ++iter)
    {
        int row = iter->first;
        int col = iter->second;
        if(patchIDMap.at<int>(row, col) != -2)
        {
            continue;
        }

        // add new patch
        neighborQueue.push(cv::Point(row, col));
        Patch newPatch;
        int patchID = static_cast<int>(patches.size());
        patchIDMap.at<int>(row, col) = patchID;
        int fid = triangleIDMap.at<int>(row, col);
        newPatch.componentID = mesh->property(meshFPropComponentID, TriMesh::FaceHandle(fid)) - 1; // 1-based index in property
        while(!neighborQueue.empty())
        {
            cv::Point p = neighborQueue.front();
            neighborQueue.pop();
            newPatch.pixels.push_back(pair<int, int>(p.x, p.y));
            for(int n = 0; n < 4; ++n)
            {
                int nrow = p.x + neighbors[2 * n];
                int ncol = p.y + neighbors[2 * n + 1];

                // is (nrow, ncol) out of range or being a background pixel?
                if(!cvCheckPointRange(nrow, ncol, triangleIDMap) || triangleIDMap.at<int>(nrow, ncol) == -1)
                {
                    continue;
                }

                // is (nrow, ncol) labelled?
                if(patchIDMap.at<int>(nrow, ncol) != -2)
                {
                    continue;
                }
                
                int fid = triangleIDMap.at<int>(nrow, ncol);
                int cid2 = mesh->property(meshFPropComponentID, TriMesh::FaceHandle(fid)) - 1; // 1-based index in property
                
                // does (nrow, ncol) have the same component as (row, col)?
                if(newPatch.componentID == cid2)
                {
                    neighborQueue.push(cv::Point(nrow, ncol));
                    patchIDMap.at<int>(nrow, ncol) = patchID;
                }
            }
        }

        // is this newPatch large enough?
        if(newPatch.pixels.size() >= param.patchSizeThreshold)
        {
            patches.push_back(newPatch);
        }
        else
        {
            // this patch is too small. add its pixels to strandedPixels. (label = -3)
            for(auto pixel = newPatch.pixels.begin(); pixel != newPatch.pixels.end(); ++pixel)
            {
                int row = pixel->first;
                int col = pixel->second;
                newStrandedPixels.push_back(pair<int, int>(row, col));
                patchIDMap.at<int>(row, col) = -3;
            }
        }
    }
    strandedPixels = newStrandedPixels;
}

void BWAbstraction::GreedyMergePixel()
{
    static const int neighbors[] =
    {
        -1, 0,
        1, 0,
        0, -1,
        0, 1
    };

    while(!strandedPixels.empty())
    {
        vector<pair<int, int> > newStrandedPixels;
        for(auto iter = strandedPixels.begin(); iter != strandedPixels.end(); ++iter)
        {
            int row = iter->first;
            int col = iter->second;

            int largestPatchID= -1;
            int largestPatchSize = 0;

            for(int n = 0; n < 4; ++n)
            {
                int nrow = row + neighbors[2 * n];
                int ncol = col + neighbors[2 * n + 1];

                if(!cvCheckPointRange(nrow, ncol, triangleIDMap) || triangleIDMap.at<int>(nrow, ncol) == -1)
                {
                    continue;
                }

                int pid = patchIDMap.at<int>(nrow, ncol);
                if(pid < 0)
                {
                    continue;
                }
                if(patches[pid].pixels.size() > largestPatchSize)
                {
                    largestPatchSize = static_cast<int>(patches[pid].pixels.size());
                    largestPatchID = pid;
                }
            }

            if(largestPatchID != -1)
            {
                patchIDMap.at<int>(row, col) = largestPatchID;
                patches[largestPatchID].pixels.push_back(*iter);
            }
            else
            {
                newStrandedPixels.push_back(*iter);
            }
        }
        if(strandedPixels.size() == newStrandedPixels.size())
        {
            break;
        }
        strandedPixels = newStrandedPixels;
    }
}

void BWAbstraction::GreedyMergePatch()
{
    // TODO
}

void BWAbstraction::RenderPatches()
{
    triangleIDTarget->bindClear(-1);

    triangleIDShader->Bind();
    glUniformMatrix4fv(
        glGetUniformLocation(triangleIDShader->program, "mvp"),
        1, GL_FALSE, param.mvpMatrix);

    StandardVertexAttribute attr = CreateVertexAttrFromMesh(mesh);
    glEnable(GL_DEPTH_TEST);
    triangleIDShader->Draw(GL_TRIANGLES, attr);
    glDisable(GL_DEPTH_TEST);
    triangleIDMap = triangleIDTarget->readback();

    triangleIDTarget->bindClear(-1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL); // offset depth for RenderFeatureLine()
    glPolygonOffset(5.0f, 5.0f);
    triangleIDShader->Draw(GL_TRIANGLES, attr);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_DEPTH_TEST);
    depthMap = triangleIDTarget->readbackDepth();

    attr.Destroy();
}

void BWAbstraction::AddPixelToBoundary(int row, int col, int pid2, float depth2)
{
    int pid = patchIDMap.at<int>(row, col);
    float depth = depthMap.at<float>(row, col);

    if(pid > pid2)
    {
        swap(pid, pid2);
        swap(depth, depth2);
    }

    pair<int, int> key(pid, pid2);

    if(boundaryMapping.find(key) == boundaryMapping.end())
    {
        boundaryMapping[key] = static_cast<int>(boundaries.size());
        Boundary newBoundary(pid, pid2);
        boundaries.push_back(newBoundary);
    }

    int bid = boundaryMapping[key];

    if(key.first == patchIDMap.at<int>(row, col))
    {
        boundaries[bid].pixels[0].push_back(pair<int, int>(row, col));
    }
    else
    {
        boundaries[bid].pixels[1].push_back(pair<int, int>(row, col));
    }

    if(depth > depth2)
    {
        ++boundaries[bid].votes[0];
    }
    else
    {
        ++boundaries[bid].votes[1];
    }

    boundaries[bid].avgDepthDiff += fabs(depth - depth2);
}

void BWAbstraction::ComputeBoundaries()
{
    boundaries.clear();
    boundaryMapping.clear();

    static const int neighbors[] =
    {
        -1, 0,
        1, 0,
        0, -1,
        0, 1
    };

    vector<int> contactPids;

    for(int pid = 0; pid < patches.size(); ++pid)
    {
        Patch &patch = patches[pid];
        for(auto pixel = patch.pixels.begin(); pixel != patch.pixels.end(); ++pixel)
        {
            bool hasBackgroundContact = false;
            bool isBoundary = false;
            contactPids.clear();
            int row = pixel->first;
            int col = pixel->second;

            for(int n = 0; n < 4; ++n)
            {
                int nrow = row + neighbors[2 * n];
                int ncol = col + neighbors[2 * n + 1];
                if(!cvCheckPointRange(nrow, ncol, patchIDMap))
                {
                    continue;
                }

                int pid2 = patchIDMap.at<int>(nrow, ncol);
                if(pid2 < 0)
                {
                    isBoundary = true;
                    hasBackgroundContact = true;
                }
                else if(pid != pid2)
                {
                    isBoundary = true;
                    if(find(contactPids.begin(), contactPids.end(), pid2) == contactPids.end())
                    {
                        contactPids.push_back(pid2);
                        float depth2 = depthMap.at<float>(nrow, ncol);
                        AddPixelToBoundary(row, col, pid2, depth2);
                        this->boundaryMap.at<char>(row, col) = 1;
                    }
                }
            }

            if(isBoundary)
            {
                ++patch.boundaryLength;
                distFieldMapInput.at<uchar>(row, col) = 0;
                patch.boundingBox.union_point(row, col);
            }

            if(hasBackgroundContact)
            {
                ++patch.backgroundBoundaryLength;
            }
        }
    }

#if CV_MAJOR_VERSION > 2
    distanceTransform(distFieldMapInput, this->distTransMap, DIST_L2, DIST_MASK_PRECISE);
#else
    distanceTransform(distFieldMapInput, this->distTransMap, CV_DIST_L2, CV_DIST_MASK_PRECISE);
#endif

    for(auto iter = surfacePixels.begin(); iter != surfacePixels.end(); ++iter)
    {
        int row = iter->first;
        int col = iter->second;
        int id = patchIDMap.at<int>(row, col);
        // there are -3s in the surfacePixel area, because there are still strandedPixels (GreedyMergePixel is not implemented)
        if (id >= 0) {
            patches[id].maxDistanceTransform = max(patches[id].maxDistanceTransform, this->distTransMap.at<float>(row, col));
        }
    }

    for(auto bit = boundaries.begin(); bit != boundaries.end(); ++bit)
    {
        int pid = bit->patchIDs[0];
        int pid2 = bit->patchIDs[1];

        patches[pid].neighbourPatches.insert(pid2);
        patches[pid2].neighbourPatches.insert(pid);
        bit->avgDepthDiff /= bit->pixels[0].size() + bit->pixels[1].size();
    }

}

void BWAbstraction::InitializeGL()
{
    if(glInitialized || param.useHostOpenGL)
    {
        return;
    }

    if(param.verbose)
    {
        cout << "Initializing OpenGL..." << endl;
    }

    if (!glfwInit())
    {
        return;
    }

    // try to create OpenGL 4.5 context
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glWindow = glfwCreateWindow(1, 1, "GLFW", NULL, NULL);
    if (!glWindow)
    {
        // fall back to 4.1
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glWindow = glfwCreateWindow(1, 1, "GLFW", NULL, NULL);
        if (!glWindow)
        {
            glfwTerminate();
            return;
        }
    }

    glfwMakeContextCurrent(glWindow);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        fprintf(stdout, "Error: %s\n", glewGetErrorString(err));
    }

    if (param.verbose)
    {
        fprintf(stdout, "Using GLEW %s\n", glewGetString(GLEW_VERSION));
        glPrintContextInfo();
    }

    glInitialized = true;
}

void BWAbstraction::InitializeGLResources()
{
    if (glResourceInitialized)
    {
        return;
    }

    try
    {
        featureLineShader = new StandardShader();
#if USE_EMBEDDED_SHADER
        featureLineShader->Create(
            featureline_vs_glsl,
            featureline_fs_glsl);
#else
        featureLineShader->CreateFromFile(
            "shader/featureline.vs.glsl",
            "shader/featureline.fs.glsl");
#endif
        triangleIDShader = new StandardShader();
#if USE_EMBEDDED_SHADER
        triangleIDShader->Create(
            triangleid_vs_glsl,
            triangleid_fs_glsl);
#else
        triangleIDShader->CreateFromFile(
            "shader/triangleid.vs.glsl",
            "shader/triangleid.fs.glsl");
#endif
        if (GLEW_ARB_compute_shader && GLEW_VERSION_4_3)
        {
            hairlineShader = new StandardShader();
#if USE_EMBEDDED_SHADER
            hairlineShader->CreateCompute(bake_hairline_cs_glsl);
#else
            hairlineShader->CreateComputeFromFile("shader/bake_hairline.cs.glsl");
#endif
        }
    }
    catch (std::invalid_argument ex)
    {
        cout << ex.what() << endl;
        cout << "Cannot continue. Exiting." << flush << endl;
        exit(1);
    }

    sharpEdgeLineTarget = new INT32DTarget();
    triangleIDTarget = new INT32DTarget();
    patchIDTarget = new INT32DTarget();
    lineMapTarget = new INT32DTarget();
    outputTarget = new RGBADTarget();

    if (GLEW_ARB_compute_shader && GLEW_VERSION_4_3)
    {
        glGenBuffers(1, &labelSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, labelSSBO);
    }

    sharpEdgeLineTarget->init();
    triangleIDTarget->init();
    patchIDTarget->init();
    lineMapTarget->init();
    outputTarget->init();

    glResourceInitialized = true;
}

Mat BWAbstraction::RenderBWAImage(bwabstraction::Parameters param)
{
    int radius = static_cast<int>(round(SCALE_TO_PIXEL_WIDTH(this->param.scale)));
    int radius2 = radius / 2;

    if (GLEW_ARB_compute_shader && GLEW_VERSION_4_3)
    {
        // draw boundary line 1px
#ifdef BWA_MULTITHREAD
        vector<Boundary> boundaryOMP[NUM_THREADS];
        for (auto bit = boundaries.begin(); bit != boundaries.end(); ++bit)
        {
            static int bin = 0;
            boundaryOMP[bin].push_back(*bit);
            bin = (bin + 1) % NUM_THREADS;
        }

#pragma omp parallel for
        for (int bin = 0; bin < NUM_THREADS; ++bin)
        {
            for (auto bit = boundaryOMP[bin].begin(); bit != boundaryOMP[bin].end(); ++bit)
            {
                if (bit->label == 0)
                {
                    continue;
                }
                int pid = bit->votes[0] > bit->votes[1] ? bit->patchIDs[0] : bit->patchIDs[1];
                vector<pair<int, int> > &pixels = bit->votes[0] > bit->votes[1] ? bit->pixels[0] : bit->pixels[1];
                for (auto it = pixels.begin(); it != pixels.end(); ++it)
                {
                    mixedLineMap.at<int>(it->first, it->second) = pid;
                }
            }
        }
#else
        for (auto bit = boundaries.begin(); bit != boundaries.end(); ++bit)
        {
            if (bit->label == 0)
            {
                continue;
            }
            int pid = bit->votes[0] > bit->votes[1] ? bit->patchIDs[0] : bit->patchIDs[1];
            vector<pair<int, int> > &pixels = bit->votes[0] > bit->votes[1] ? bit->pixels[0] : bit->pixels[1];
            for (auto it = pixels.begin(); it != pixels.end(); ++it)
            {
                mixedLineMap.at<int>(it->first, it->second) = pid;
            }
        }
#endif

        // draw feature line 1 px
#ifdef BWA_MULTITHREAD
        vector<FeatureLine> featureLineOMP[NUM_THREADS];
        for (auto bit = featureLines.begin(); bit != featureLines.end(); ++bit)
        {
            static int bin = 0;
            featureLineOMP[bin].push_back(*bit);
            bin = (bin + 1) % NUM_THREADS;
        }

#pragma omp parallel for
        for (int bin = 0; bin < NUM_THREADS; ++bin)
        {
            for (auto bit = featureLineOMP[bin].begin(); bit != featureLineOMP[bin].end(); ++bit)
            {
                if (bit->pixels.size() < radius * (this->param.featureWeight / 100.0f) ||
                    this->patches[bit->patchID].maxDistanceTransform < radius)
                {
                    continue;
                }
                for (auto it = bit->pixels.begin(); it != bit->pixels.end(); ++it)
                {
                    mixedLineMap.at<int>(it->first, it->second) = -patchIDMap.at<int>(it->first, it->second) - 1;
                }
            }
        }
#else
        for (auto bit = this->featureLines.begin(); bit != this->featureLines.end(); ++bit)
        {
            if (bit->pixels.size() < radius * (this->param.featureWeight / 100.0f) ||
                this->patches[bit->patchID].maxDistanceTransform < radius)
            {
                continue;
            }
            for (auto it = bit->pixels.begin(); it != bit->pixels.end(); ++it)
            {
                mixedLineMap.at<int>(it->first, it->second) = -patchIDMap.at<int>(it->first, it->second) - 1;
            }
        }
#endif

        // mixed line map content:
        // clear value: -2147483647 (background value)
        // boundary: +pid (positive patch id)
        // feature: -pid-1 (negative patch id minus one)

        // draw lines with radius (GPU)
        patchIDTarget->update(this->patchIDMap);
        lineMapTarget->update(mixedLineMap);
        glClearColor(param.backgroundColor[0], param.backgroundColor[1], param.backgroundColor[2], 1.0f);
        outputTarget->bindClear();

        glNamedBufferData(labelSSBO, nodeLabel.size() * sizeof(int), nodeLabel.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, labelSSBO);

        hairlineShader->Bind();
        glUniform1i(0, radius);
        // output texture 0
        glBindImageTexture(0, outputTarget->color_id_buffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        // input texture 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lineMapTarget->color_id_buffer);
        // inpute texture 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, patchIDTarget->color_id_buffer);
        glDispatchCompute(this->param.renderWidth / 8, this->param.renderHeight / 8, 1);

        Mat output8uc4 = outputTarget->readback();
        Mat output = Mat(param.renderHeight, param.renderWidth, CV_8UC1);
        const int from_to[] = { 0,0 };
        mixChannels(output8uc4, output, from_to, 1);
        return output;
    }
    else
    {
        Mat bwaImage = Mat(param.renderHeight, param.renderWidth, CV_8UC1, Scalar(255));

        // draw boundary line 1px
#ifdef BWA_MULTITHREAD
        vector<Boundary> boundaryOMP[NUM_THREADS];
        for (auto bit = boundaries.begin(); bit != boundaries.end(); ++bit)
        {
            static int bin = 0;
            boundaryOMP[bin].push_back(*bit);
            bin = (bin + 1) % NUM_THREADS;
        }

#pragma omp parallel for
        for (int bin = 0; bin < NUM_THREADS; ++bin)
        {
            for (auto bit = boundaryOMP[bin].begin(); bit != boundaryOMP[bin].end(); ++bit)
            {
                if (bit->label == 0)
                {
                    continue;
                }
                int pid = bit->votes[0] > bit->votes[1] ? bit->patchIDs[0] : bit->patchIDs[1];
                vector<pair<int, int> > &pixels = bit->votes[0] > bit->votes[1] ? bit->pixels[0] : bit->pixels[1];
                for (auto it = pixels.begin(); it != pixels.end(); ++it)
                {
                    boundaryLineMap.at<int>(it->first, it->second) = pid;
                }
            }
        }
#else
        for (auto bit = boundaries.begin(); bit != boundaries.end(); ++bit)
        {
            if (bit->label == 0)
            {
                continue;
            }
            int pid = bit->votes[0] > bit->votes[1] ? bit->patchIDs[0] : bit->patchIDs[1];
            vector<pair<int, int> > &pixels = bit->votes[0] > bit->votes[1] ? bit->pixels[0] : bit->pixels[1];
            for (auto it = pixels.begin(); it != pixels.end(); ++it)
            {
                boundaryLineMap.at<int>(it->first, it->second) = pid;
            }
        }
#endif

        // draw feature line 1 px
#ifdef BWA_MULTITHREAD
        vector<FeatureLine> featureLineOMP[NUM_THREADS];
        for (auto bit = featureLines.begin(); bit != featureLines.end(); ++bit)
        {
            static int bin = 0;
            featureLineOMP[bin].push_back(*bit);
            bin = (bin + 1) % NUM_THREADS;
        }

#pragma omp parallel for
        for (int bin = 0; bin < NUM_THREADS; ++bin)
        {
            for (auto bit = featureLineOMP[bin].begin(); bit != featureLineOMP[bin].end(); ++bit)
            {
                if (bit->pixels.size() < radius * (this->param.featureWeight / 100.0f) ||
                    this->patches[bit->patchID].maxDistanceTransform < radius)
                {
                    continue;
                }
                for (auto it = bit->pixels.begin(); it != bit->pixels.end(); ++it)
                {
                    featureLineMap.at<int>(it->first, it->second) = patchIDMap.at<int>(it->first, it->second);
                }
            }
        }
#else
        for (auto bit = this->featureLines.begin(); bit != this->featureLines.end(); ++bit)
        {
            if (bit->pixels.size() < radius * (this->param.featureWeight / 100.0f) ||
                this->patches[bit->patchID].maxDistanceTransform < radius)
            {
                continue;
            }
            for (auto it = bit->pixels.begin(); it != bit->pixels.end(); ++it)
            {
                featureLineMap.at<int>(it->first, it->second) = patchIDMap.at<int>(it->first, it->second);
            }
        }
#endif

        // draw lines with radius (CPU)
#ifdef BWA_MULTITHREAD
        vector<Patch> patchOMP[NUM_THREADS];
        for (auto bit = patches.begin(); bit != patches.end(); ++bit)
        {
            static int bin = 0;
            patchOMP[bin].push_back(*bit);
            bin = (bin + 1) % NUM_THREADS;
        }

#pragma omp parallel for
        for (int bin = 0; bin < NUM_THREADS; ++bin)
        {
            for (auto pit = patchOMP[bin].begin(); pit != patchOMP[bin].end(); ++pit)
            {
                int color = pit->label == 1 ? 255 : 0;
                int negColor = pit->label == 1 ? 0 : 255;
                for (auto it = pit->pixels.begin(); it != pit->pixels.end(); ++it)
                {
                    bwaImage.at<uchar>(it->first, it->second) = color;

                    int row = it->first;
                    int col = it->second;
                    int patchID = this->patchIDMap.at<int>(row, col);

                    bool found = false;
                    for (int r = -radius; r <= radius; ++r)
                    {
                        for (int c = -radius; c <= radius; ++c)
                        {
                            if (r * r + c * c > radius * radius)
                            {
                                continue;
                            }
                            int nrow = row + r;
                            int ncol = col + c;
                            if (!cvCheckPointRange(nrow, ncol, this->triangleIDMap))
                            {
                                continue;
                            }
                            if (boundaryLineMap.at<int>(nrow, ncol) == patchID)
                            {
                                bwaImage.at<uchar>(it->first, it->second) = negColor;
                                goto END_BWAPIXEL;
                            }
                            if (r * r + c * c > radius2 * radius2)
                            {
                                continue;
                            }
                            if (featureLineMap.at<int>(nrow, ncol) == patchID)
                            {
                                bwaImage.at<uchar>(it->first, it->second) = negColor;
                                goto END_BWAPIXEL;
                            }
                        }
                    }
                END_BWAPIXEL:
                    ;
                }
            }
        }
#else
        for (auto pit = patches.begin(); pit != patches.end(); ++pit)
        {
            int color = pit->label == 1 ? 255 : 0;
            int negColor = pit->label == 1 ? 0 : 255;
            for (auto it = pit->pixels.begin(); it != pit->pixels.end(); ++it)
            {
                bwaImage.at<uchar>(it->first, it->second) = color;

                int row = it->first;
                int col = it->second;
                int patchID = this->patchIDMap.at<int>(row, col);

                bool found = false;
                for (int r = -radius; r <= radius; ++r)
                {
                    for (int c = -radius; c <= radius; ++c)
                    {
                        if (r * r + c * c > radius * radius)
                        {
                            continue;
                        }
                        int nrow = row + r;
                        int ncol = col + c;
                        if (!cvCheckPointRange(nrow, ncol, this->triangleIDMap))
                        {
                            continue;
                        }
                        if (boundaryLineMap.at<int>(nrow, ncol) == patchID)
                        {
                            bwaImage.at<uchar>(it->first, it->second) = negColor;
                            goto END_BWAPIXEL;
                        }
                        if (r * r + c * c > radius2 * radius2)
                        {
                            continue;
                        }
                        if (featureLineMap.at<int>(nrow, ncol) == patchID)
                        {
                            bwaImage.at<uchar>(it->first, it->second) = negColor;
                            goto END_BWAPIXEL;
                            }
                        }
                    }
            END_BWAPIXEL:
                ;
                }
            }
#endif

        return bwaImage;
    }
}

void BWAbstraction::Render(Result *result, bwabstraction::Parameters param)
{
    this->param = param;
    this->result = result;
    ++renderCount;

    ::Timer timer;
    if (this->param.verbose)
    {
        timer.Start();
    }

    InitializeGL();
    if (!glInitialized && !param.useHostOpenGL)
    {
        cout << "OpenGL initialization failed. Stopping." << endl;
        return;
    }
    InitializeGLResources();

    if (this->param.renderWidth != this->triangleIDMap.cols || this->param.renderHeight != this->triangleIDMap.rows)
    {
        this->featureMap = Mat(param.renderHeight, param.renderWidth, CV_8SC1, Scalar(-1));
        this->markedMap = Mat(param.renderHeight, param.renderWidth, CV_8SC1, Scalar(0));
        this->patchIDMap = Mat(param.renderHeight, param.renderWidth, CV_32SC1, Scalar(-1));
        this->depthCritLineMap = Mat(param.renderHeight, param.renderWidth, CV_8SC1, Scalar(-1));
        this->distFieldMapInput = Mat(param.renderHeight, param.renderWidth, CV_8UC1, Scalar(255));
        this->boundaryMap = Mat(param.renderHeight, param.renderWidth, CV_8SC1, Scalar(-1));
        this->sharpEdgeLineMap = Mat(param.renderHeight, param.renderWidth, CV_32SC1, Scalar(-1));
        if (GLEW_ARB_compute_shader && GLEW_VERSION_4_3)
        {
            this->mixedLineMap = Mat(param.renderHeight, param.renderWidth, CV_32SC1, Scalar(-2147483647));
        }
        else
        {
            this->boundaryLineMap = Mat(param.renderHeight, param.renderWidth, CV_32SC1, Scalar(-1));
            this->featureLineMap = Mat(param.renderHeight, param.renderWidth, CV_32SC1, Scalar(-1));
        }

        sharpEdgeLineTarget->resize(param.renderWidth, param.renderHeight);
        triangleIDTarget->resize(param.renderWidth, param.renderHeight);
        patchIDTarget->resize(this->param.renderWidth, this->param.renderHeight);
        lineMapTarget->resize(this->param.renderWidth, this->param.renderHeight);
        outputTarget->resize(this->param.renderWidth, this->param.renderHeight);
    }
    else
    {
        this->featureMap = Scalar(-1);
        this->markedMap = Scalar(0);
        this->patchIDMap = Scalar(-1);
        this->depthCritLineMap = Scalar(-1);
        this->distFieldMapInput = Scalar(255);
        this->boundaryMap = Scalar(-1);
        this->sharpEdgeLineMap = Scalar(-1);
        if (GLEW_ARB_compute_shader && GLEW_VERSION_4_3)
        {
            this->mixedLineMap = Scalar(-2147483647);
        }
        else
        {
            this->boundaryLineMap = Scalar(-1);
            this->featureLineMap = Scalar(-1);
        }
    }

    ComputePatches();
    ComputeBoundaries();
    ComputeFeatureLines();
    ComputeSimilaritySets();
    ComputeInclusionPairs();
    BeliefPropagationOptimization();
    result->bwaImage = RenderBWAImage(param);

    if (this->param.verbose)
    {
        timer.Update();
        cout << "Model Render Count #" << renderCount << endl;
        cout << "Frame time: " << timer.DeltaTime() << " seconds" << endl;

        cout << "Patches: " << patches.size() << endl;
        cout << "Boundaries: " << boundaries.size() << endl;
        cout << "Components: " << components.size() << endl;
        cout << "Similarity Sets: " << similaritySets.size() << endl;
        cout << "Inclusion Pairs: " << inclusionPairs.size() << endl;
        cout << "--" << endl;
    }

    Scalar backgroundColor = Scalar(
        static_cast<int>(255.0f * param.backgroundColor[0]),
        static_cast<int>(255.0f * param.backgroundColor[1]),
        static_cast<int>(255.0f * param.backgroundColor[2]));

    if (this->param.resultImage & ResultImage::CONSISTENCY)
    {
        result->consistencyImage = Mat(param.renderHeight, param.renderWidth, CV_8UC3, backgroundColor);
        for (int sid = 0; sid < similaritySets.size(); ++sid)
        {
            unsigned char rgb[3];
            GetDistinctColor(sid, rgb);
            for (auto it = similaritySets[sid].begin(); it != similaritySets[sid].end(); ++it)
            {
                for (auto pixel = patches[*it].pixels.begin(); pixel != patches[*it].pixels.end(); ++pixel)
                {
                    result->consistencyImage.at<Vec3b>(pixel->first, pixel->second) = Vec3b(rgb[0], rgb[1], rgb[2]);
                }
            }
        }
    }

    if (this->param.resultImage & ResultImage::PATCH)
    {
        result->patchImage = Mat(param.renderHeight, param.renderWidth, CV_8UC3, backgroundColor);
        int pid = 0;
        for (auto pit = patches.begin(); pit != patches.end(); ++pit)
        {
            unsigned char rgb[3];
            GetDistinctColor(pid, rgb);
            for (auto it = pit->pixels.begin(); it != pit->pixels.end(); ++it)
            {
                result->patchImage.at<Vec3b>(it->first, it->second) = Vec3b(rgb[0], rgb[1], rgb[2]);
            }
            ++pid;
        }
    }

    if (this->param.resultImage & ResultImage::BOUNDARY)
    {
        result->boundaryImage = Mat(param.renderHeight, param.renderWidth, CV_8UC3, backgroundColor);
        int bid = 0;
        for (auto bit = boundaries.begin(); bit != boundaries.end(); ++bit)
        {
            unsigned char rgb[3];
            int pid = bit->votes[0] > bit->votes[1] ? bit->patchIDs[0] : bit->patchIDs[1];
            GetDistinctColor(pid, rgb);
            vector<pair<int, int> > &pixels = bit->votes[0] > bit->votes[1] ? bit->pixels[0] : bit->pixels[1];
            for (auto it = pixels.begin(); it != pixels.end(); ++it)
            {
                result->boundaryImage.at<Vec3b>(it->first, it->second) = Vec3b(rgb[0], rgb[1], rgb[2]);
            }
            ++bid;
        }
    }

    if (this->param.resultImage & ResultImage::DEPTH_CRITICAL)
    {
        result->depthCriticalLineImage = Mat(param.renderHeight, param.renderWidth, CV_8UC3, backgroundColor);
        for (int row = 0; row < param.renderHeight; ++row)
        {
            for (int col = 0; col < param.renderWidth; ++col)
            {
                if (this->depthCritLineMap.at<char>(row, col) == 1)
                {
                    result->depthCriticalLineImage.at<Vec3b>(row, col) = Vec3b(0, 0, 0);
                }
            }
        }
    }

    if (this->param.resultImage & ResultImage::SHARP_EDGE)
    {
        result->sharpEdgeLineImage = Mat(param.renderHeight, param.renderWidth, CV_8UC3, backgroundColor);
        for (int row = 0; row < param.renderHeight; ++row)
        {
            for (int col = 0; col < param.renderWidth; ++col)
            {
                if (this->sharpEdgeLineMap.at<int>(row, col) == 1)
                {
                    result->sharpEdgeLineImage.at<Vec3b>(row, col) = Vec3b(0, 0, 0);
                }
            }
        }
    }

    if (this->param.resultImage & ResultImage::FEATURE_LINE)
    {
        result->featureLineImage = Mat(param.renderHeight, param.renderWidth, CV_8UC3, backgroundColor);
        int fid = 0;
        for (auto fit = featureLines.begin(); fit != featureLines.end(); ++fit)
        {
            unsigned char rgb[3];
            GetDistinctColor(fid, rgb);
            for (auto it = fit->pixels.begin(); it != fit->pixels.end(); ++it)
            {
                result->featureLineImage.at<Vec3b>(it->first, it->second) = Vec3b(rgb[0], rgb[1], rgb[2]);
            }
            ++fid;
        }
    }

    if (this->param.resultImage & ResultImage::DISTANCE_TRANSFORM)
    {
        result->distanceTransformImage = Mat(param.renderHeight, param.renderWidth, CV_8UC3, backgroundColor);
        for (int row = 0; row < param.renderHeight; ++row)
        {
            for (int col = 0; col < param.renderWidth; ++col)
            {
                int val = static_cast<int>(min(1.0f, this->distTransMap.at<float>(row, col) * 0.01f) * 255.0f);
                result->distanceTransformImage.at<Vec3b>(row, col) = Vec3b(val, val, val);
            }
        }
    }

    if (this->param.resultImage & ResultImage::COMPONENT)
    {
        result->componentImage = Mat(param.renderHeight, param.renderWidth, CV_8UC3, backgroundColor);
        for (auto pit = surfacePixels.begin(); pit != surfacePixels.end(); ++pit)
        {
            int row = pit->first;
            int col = pit->second;
            int faceID = triangleIDMap.at<int>(row, col);
            int componentID = mesh->property(meshFPropComponentID, TriMesh::FaceHandle(faceID));
            unsigned char rgb[3];
            GetDistinctColor(componentID, rgb);
            result->componentImage.at<Vec3b>(row, col) = Vec3b(rgb[0], rgb[1], rgb[2]);
        }
    }

    if (this->param.resultImage & ResultImage::INCLUSION)
    {
        result->inclusionImage = Mat(param.renderHeight, param.renderWidth, CV_8UC3, backgroundColor);
        int pid = 0;
        for(auto pit = patches.begin(); pit != patches.end(); ++pit)
        {
            unsigned char rgb[3];
            GetDistinctColor(pid, rgb);
            int gray = RGBToGray(rgb);
            for(auto it = pit->pixels.begin(); it != pit->pixels.end(); ++it)
            {
                result->inclusionImage.at<Vec3b>(it->first, it->second) = Vec3b(gray, gray, gray);
            }
            ++pid;
        }
        for(auto iit = inclusionPairs.begin(); iit != inclusionPairs.end(); ++iit)
        {
            int patchA = iit->first;
            int patchB = iit->second;
            unsigned char rgb[3];
            GetDistinctColor(patchB, rgb);
            for(auto it = patches[patchB].pixels.begin(); it != patches[patchB].pixels.end(); ++it)
            {
                result->inclusionImage.at<Vec3b>(it->first, it->second) = Vec3b(rgb[0], rgb[1], rgb[2]);
            }
        }
    }
}

void BWAbstraction::ListSurfacePixels()
{
    for (int row = 0; row < triangleIDMap.rows; ++row)
    {
        int* row_ptr = triangleIDMap.ptr<int>(row);
        for (int col = 0; col < triangleIDMap.cols; ++col)
        {
            if(*(row_ptr + col) != -1)
            {
                surfacePixels.push_back(pair<int, int>(row, col));
#ifdef BWA_MULTITHREAD
                static int bin = 0;
                surfacePixelBins[bin].push_back(pair<int, int>(row, col));
                bin = (bin + 1) % NUM_THREADS;
#endif
            }
        }
    }
}

void BWAbstraction::ComputeSurfaceConnect()
{
    // TODO: possible speedup by using only face-to-face adjacent faces
    surfaceConnect.clear();
    surfaceOffset.clear();
    std::vector<int> faceIds;
    for(TriMesh::FaceIter it = mesh->faces_begin(); it != mesh->faces_end(); ++it)
    {
        surfaceOffset.push_back(static_cast<int>(surfaceConnect.size()));
        faceIds.clear();
        for(TriMesh::FaceVertexIter fvit = mesh->fv_iter(*it); fvit.is_valid(); ++fvit)
        {
            for(TriMesh::VertexFaceIter vfit = mesh->vf_iter(*fvit); vfit.is_valid(); ++vfit)
            {
                if (find(faceIds.begin(), faceIds.end(), vfit->idx()) == faceIds.end())
                {
                    faceIds.push_back(vfit->idx());
                }
            }
        }
        for(auto fit = faceIds.begin(); fit != faceIds.end(); ++fit)
        {
            surfaceConnect.push_back(*fit);
        }
    }
    surfaceOffset.push_back(static_cast<int>(surfaceConnect.size()));
}

void BWAbstraction::ComputeCongruencies()
{

    // the model is normalized into (-1, -1, -1) ~ (1, 1, 1).
    // allowing 0.5% difference
    const double sizeTolerance = sqrt(2 * 2 * 3) * param.congruentThreshold;
    vector<pair<int, int> > task_list;
    for(int m = 0; m < components.size(); ++m)
    {
        components[m]->BuildOBB();
    }
    // generate task list
    // filter out those whose bounding size difference is too large
    for(int m = 0; m < components.size(); ++m)
    {
        for(int n = m + 1; n < components.size(); ++n)
        {
            TriMesh* m1 = components[m];
            TriMesh* m2 = components[n];
            TriMesh::Point refSize = m1->obb.max - m1->obb.min;
            TriMesh::Point objSize = m2->obb.max - m2->obb.min;
            if((refSize-objSize).length() > sizeTolerance)
            {
                continue;
            }
            task_list.push_back(pair<int, int>(m, n));
        }
    }
#ifdef BWA_MULTITHREAD
    // process each task in parallel with omp parallel for
    const int n_tasks = static_cast<int>(task_list.size());
    int n_similarities[NUM_THREADS] = { 0 };
    bool* similarity_table = new bool[n_tasks]();
    #pragma omp parallel for
    for (int x = 0; x < NUM_THREADS; ++x)
    {
        for (int y = x; y < n_tasks; y += NUM_THREADS)
        {
#else
    const int n_tasks = task_list.size();
    int n_similarities[1] = { 0 };
    bool* similarity_table = new bool[n_tasks]();
    for (int x = 0; x < 1; ++x)
    {
        for (int y = x; y < n_tasks; y += 1)
        {
#endif
            int m = task_list[y].first;
            int n = task_list[y].second;
            TriMesh* m1 = components[m];
            TriMesh* m2 = components[n];

            TriMesh::Point refOrigin = m1->obb.origin;
            TriMesh::Point refSize = m1->obb.max - m1->obb.min;
            TriMesh::Point refAxes[3];
            refAxes[0] = m1->obb.maxAxis;
            refAxes[1] = m1->obb.midAxis;
            refAxes[2] = m1->obb.minAxis;

            TriMesh::Point objOrigin = m2->obb.origin;
            TriMesh::Point objAxes[3];
            objAxes[0] = m2->obb.maxAxis;
            objAxes[1] = m2->obb.midAxis;
            objAxes[2] = m2->obb.minAxis;

            double shapeTolerance = refSize.length() * 0.03;

            // align w.r.t. reference obb axis
            Matrix3d refBasisMat = Matrix3d::Zero();
            Matrix3d objBasisMat = Matrix3d::Zero();
            for(int i = 0; i < 3; ++i)
            {
                for(int j = 0; j < 3; ++j)
                {
                    refBasisMat(i, j) = refAxes[j][i];
                    objBasisMat(i, j) = objAxes[j][i];
                }
            }
            Matrix3d R = refBasisMat * objBasisMat.inverse();
            // translate object mesh to reference mesh so that origin matches
            TriMesh m2copy = *m2;
            m2copy.Translate(refOrigin - objOrigin);
            m2copy.RotateNoNormalUpdate(refOrigin, R);

            // try different orientations, compute ROOT MEAN SQUARED DISTANCE
            for(int i = 0; i < 4; ++i)
            {
                TriMesh pTrialMesh = m2copy;
                if(i != 0)
                {
                    pTrialMesh.RotateNoNormalUpdate(refOrigin, refAxes[i - 1], M_PI);
                }
                double RMSD = 0;
                for(TriMesh::VertexIter vit1 = pTrialMesh.vertices_begin(); vit1 != pTrialMesh.vertices_end(); ++vit1)
                {
                    TriMesh::Point v = pTrialMesh.point(*vit1);
                    double minDis2 = DBL_MAX;
                    for(TriMesh::VertexIter vit2 = m1->vertices_begin(); vit2 != m1->vertices_end(); ++vit2)
                    {
                        TriMesh::Point refV = m1->point(*vit2);
                        double dis = (v-refV).length();
                        double dis2 = dis * dis;
                        minDis2 = min(minDis2, dis2);
                    }
                    RMSD += minDis2;
                }
                RMSD /= pTrialMesh.n_vertices();
                RMSD = sqrt(RMSD);
                if(RMSD < shapeTolerance)
                {
                    ++n_similarities[x];
                    similarity_table[y] = true;
                    break;
                }
            }
        }
    }

    congruencies.clear();
    for(int x = 0; x < n_tasks; ++x)
    {
        if(similarity_table[x])
        {
            int m = task_list[x].first;
            int n = task_list[x].second;
            congruencies.push_back(pair<int, int>(m, n));
        }
    }

    delete[] similarity_table;
}

void BWAbstraction::ComputeInclusionPairs()
{
    inclusionPairs.clear();
    for(int pid = 0; pid < patches.size(); ++pid)
    {
        if(patches[pid].backgroundBoundaryLength > 0)
        {
            continue;
        }
        int includeCount = 0;
        int includedCount = 0;
        int includerID = -1;
        BoundingBox &bb1 = patches[pid].boundingBox;
        for(auto neighbourID = patches[pid].neighbourPatches.begin(); neighbourID != patches[pid].neighbourPatches.end(); ++neighbourID)
        {
            BoundingBox &bb2 = patches[*neighbourID].boundingBox;
            if(bb2.size() > bb1.size() && bb2.intersect_box(bb1) == bb1)
            {
                includerID = *neighbourID;
                ++includedCount;
            }
            else if(bb1.size() > bb2.size() && bb1.intersect_box(bb2) == bb2)
            {
                ++includeCount;
            }
        }
        if(includedCount == 1 && (includeCount + includedCount) == patches[pid].neighbourPatches.size())
        {
            inclusionPairs.insert(pair<int, int>(includerID, pid));
        }
    }
}

void BWAbstraction::ComputeSimilaritySets()
{
    similaritySets.clear();
    const int numComponents = static_cast<int>(components.size());
    vector<vector<int> > adjacencyList;
    vector<vector<int> > patchList;
    vector<bool> visited;
    adjacencyList.resize(numComponents);
    patchList.resize(numComponents);
    visited.resize(numComponents, false);

    for(auto cit = congruencies.begin(); cit != congruencies.end(); ++cit)
    {
        adjacencyList[cit->first].push_back(cit->second);
        adjacencyList[cit->second].push_back(cit->first);
    }

    for(int pid = 0; pid < patches.size(); ++pid)
    {
        patchList[patches[pid].componentID].push_back(pid);
    }

    for(int cid = 0; cid < numComponents; ++cid)
    {
        if(visited[cid])
        {
            continue;
        }
        queue<int> componentQueue;
        vector<int> similarComponents;
        componentQueue.push(cid);
        while(!componentQueue.empty())
        {
            int id = componentQueue.front();
            componentQueue.pop();
            if(visited[id])
            {
                continue;
            }
            visited[id] = true;
            similarComponents.push_back(id);
            for(auto it = adjacencyList[id].begin(); it != adjacencyList[id].end(); ++it)
            {
                componentQueue.push(*it);
            }
        }

        vector<int> similarPatches;
        for(auto it = similarComponents.begin(); it != similarComponents.end(); ++it)
        {
            similarPatches.insert(similarPatches.end(), patchList[*it].begin(), patchList[*it].end());
        }
        
        if(similarPatches.size() > 0)
        {
            similaritySets.push_back(similarPatches);
        }
    }
}

float BWAbstraction::b_term(int c)
{
    int boundary_length = static_cast<int>((boundaries[c].pixels[0].size() + boundaries[c].pixels[1].size()) * 0.5f);
    return static_cast<float>(boundary_length) / static_cast<float>(param.renderWidth);
}

float BWAbstraction::a_term(int q)
{
    return static_cast<float>(patches[q].pixels.size()) / static_cast<float>(param.renderWidth * param.renderHeight);
}

float BWAbstraction::d_term(int c)
{
    return boundaries[c].avgDepthDiff;
}

float BWAbstraction::D_term(int q)
{
    return patches[q].maxDistanceTransform;
}

float BWAbstraction::l_term(int c)
{
    int pid = boundaries[c].patchIDs[0];
    int pid2 = boundaries[c].patchIDs[1];
    return b_term(c) * min(a_term(pid), a_term(pid2));
}

float BWAbstraction::lbg_term(int q)
{
    float normalizedLength = static_cast<float>(patches[q].backgroundBoundaryLength) / static_cast<float>(param.renderWidth);
    return normalizedLength * a_term(q);
}

float BWAbstraction::scale_contrast_term(int c)
{
    float D1 = D_term(boundaries[c].patchIDs[0]);
    float D2 = D_term(boundaries[c].patchIDs[1]);
    float B = 2.0f * min(D1, D2);
    return max(0.0f, 1.0f - param.scale / B);
}

float BWAbstraction::scale_line_term(int c)
{
    float D1 = D_term(boundaries[c].patchIDs[0]);
    float D2 = D_term(boundaries[c].patchIDs[1]);
    float B = 1.0f * min(D1, D2);
    return max(0.0f, 1.0f - param.scale / B);
}

void BWAbstraction::BeliefPropagationOptimization()
{
    size_t numNodes = patches.size() + boundaries.size();
    nodeLabel.clear();
    nodeLabel.resize(numNodes);
    size_t numLabels = 2;

    typedef SimpleDiscreteSpace<int, int> Space;
    typedef float PrecisionT;
    typedef GraphicalModel<PrecisionT, Adder, ExplicitFunction<PrecisionT>, Space> Model;
    typedef BeliefPropagationUpdateRules<Model, Maximizer> UpdateRules;
    typedef MessagePassing<Model, Maximizer, UpdateRules, MaxDistance> BeliefPropogation;
    const size_t maxIterations = 40;
    const PrecisionT convergenceBound = static_cast<PrecisionT>(1e-20);
    const PrecisionT damping = 0.0;

    Space space(static_cast<int>(numNodes), static_cast<int>(numLabels));
    Model model(space);

    // similarity term (SMOOTH)
    for(auto it = similaritySets.begin(); it != similaritySets.end(); ++it)
    {
        for(auto it2 = it->begin(); it2 != it->end(); ++it2)
        {
            for(auto it3 = it2 + 1; it3 != it->end(); ++it3)
            {
                pair<int, int> p = *it2 >= *it3 ? pair<int, int>(*it3, *it2) : pair<int, int>(*it2, *it3);
                const size_t shape[] = { numLabels, numLabels };
                ExplicitFunction<PrecisionT> f(shape, shape + 2, 1.0);
                float m_a = min(a_term(p.first), a_term(p.second)) * param.consistencyWeight;
                f(0, 0) = 1 * m_a;
                f(1, 0) = -1 * m_a;
                f(0, 1) = -1 * m_a;
                f(1, 1) = 1 * m_a;
                Model::FunctionIdentifier fid = model.addFunction(f);
                const int variableIndices[] = { p.first, p.second };
                model.addFactor(fid, variableIndices, variableIndices + 2);
            }
        }
    }

    // background term (DATA)
    for(int i = 0; i < patches.size(); ++i)
    {
        const size_t shape[] = { numLabels };
        ExplicitFunction<PrecisionT> f(shape, shape + 1);
        float l = lbg_term(i) * param.backgroundWeight;
        f(0) = 1 * l;
        f(1) = 0 * l;
        Model::FunctionIdentifier fid = model.addFunction(f);
        const int variableIndices[] = { i };
        model.addFactor(fid, variableIndices, variableIndices + 1);
    }

    // neighbor term
    float maxAvgDepthDiff = 0;
    for(int c = 0; c < boundaries.size(); ++c)
    {
        maxAvgDepthDiff = max(maxAvgDepthDiff, boundaries[c].avgDepthDiff);
    }
    for(int c = 0; c < boundaries.size(); ++c)
    {
        pair<int, int> p = pair<int, int>(boundaries[c].patchIDs[0], boundaries[c].patchIDs[1]);
        int q1 = p.first;
        int q2 = p.second;

        const size_t shape[] = { numLabels, numLabels, numLabels };
        ExplicitFunction<PrecisionT> f(shape, shape + 3, 1.0);
        float lt = l_term(c);
        float dt = d_term(c) / maxAvgDepthDiff;
        bool hasInclusionRelation =
            inclusionPairs.find(pair<int, int>(q1, q2)) != inclusionPairs.end() ||
            inclusionPairs.find(pair<int, int>(q2, q1)) != inclusionPairs.end();
        float include = hasInclusionRelation ? param.inclusionWeight : 1.0f;
        // merge
        f(0, 0, 0) =
            f(1, 1, 0) = 0.00000001f * param.neighbourWeight;
        // split contrast
        f(1, 0, 0) =
            f(0, 1, 0) = scale_contrast_term(c) * lt * (1 - dt) * include * param.neighbourWeight;
        // split line
        f(0, 0, 1) =
            f(1, 1, 1) = scale_line_term(c) * lt * max(0.01f, (dt + param.contrastWeight)) * param.neighbourWeight;
        // invalid case
        f(1, 0, 1) =
            f(0, 1, 1) = 0;
        Model::FunctionIdentifier fid = model.addFunction(f);
        const int variableIndices[] = { q1, q2, c + static_cast<int>(patches.size()) };
        model.addFactor(fid, variableIndices, variableIndices + 3);
    }

    /*
    // force term (DATA)
    for(int i = 0; i < l->segmentLabelOverride.size(); ++i)
    {
        int labelOverride = l->segmentLabelOverride[i];
        if(labelOverride == -1)
        {
            continue;
        }
        const size_t shape[] = { numLabels };
        ExplicitFunction<PrecisionT> f(shape, shape + 1);
        if(labelOverride == 0)
        {
            f(0) = 10000;
            f(1) = 0;
        }
        else
        {
            f(0) = 0;
            f(1) = 10000;
        }
        Model::FunctionIdentifier fid = model.addFunction(f);
        const int variableIndices[] = { i };
        model.addFactor(fid, variableIndices, variableIndices + 1);
    }
    */

    BeliefPropogation::Parameter parameter(maxIterations, convergenceBound, damping);
    BeliefPropogation bp(model, parameter);
    bp.infer();
    bp.arg(nodeLabel);
    for(int i = 0; i < patches.size(); ++i)
    {
        patches[i].label = nodeLabel[i];
    }
    for(int i = 0; i < boundaries.size(); ++i)
    {
        boundaries[i].label = nodeLabel[i + patches.size()];
    }
}