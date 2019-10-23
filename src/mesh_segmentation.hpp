#ifndef MESHSEGMENTATION_H
#define MESHSEGMENTATION_H

#include <vector>
#include <unordered_map>
#include "trimesh.hpp"

namespace bwabstraction
{

/**
 * @brief The MeshSegmentation class implements mesh segmentation algorithms for TriMesh objects.
 * Given a TriMesh object, the MeshSegmentation class provides methods to segment it into submeshes.
 * The result submeshes are returned as a vector of TriMesh objects.
 */
class MeshSegmentation
{

public:

    static std::vector<TriMesh *> ComponentSegmentation(TriMesh *pMesh, OpenMesh::FPropHandleT<unsigned int> fPropComponentID);
    static void FreeSubmeshVector(std::vector<TriMesh *> &pSubmeshes);

private:

    static void AddFace(TriMesh *pMesh, TriMesh *pSubmesh, std::unordered_map<int, int> &pVertexHandleMap, OpenMesh::FaceHandle pFace);

};

} // namespace bwabstraction

#endif // MESHSEGMENTATION_H
