#include "mesh_segmentation.hpp"
#include <queue>

using namespace bwabstraction;
using namespace std;
using namespace OpenMesh;

/**
 * @brief MeshSegmentation::AddFace
 * Static function. Add a face from an original mesh to a submesh.
 * @param pMesh The original mesh.
 * @param pSubmesh A submesh of pMesh.
 * @param pVertexHandleMap A hashmap mapping from original mesh vertex index to submesh vertex index. Used to prevent adding redundant vertex.
 * @param pFace The face handle of the face in the original mesh that is to be added to the submesh.
 */
void MeshSegmentation::AddFace(TriMesh *pMesh, TriMesh *pSubmesh, unordered_map<int, int> &pVertexHandleMap, FaceHandle pFace)
{
    vector<VertexHandle> faceVertexHandles;
    for(TriMesh::FaceVertexIter fvit = pMesh->fv_iter(pFace); fvit.is_valid(); ++fvit)
    {
        VertexHandle vh;
        if(pVertexHandleMap.count(fvit->idx()) == 0)
        {
            vh = pSubmesh->add_vertex(pMesh->point(*fvit));
            pVertexHandleMap[fvit->idx()] = vh.idx();
        }
        else
        {
            vh = pSubmesh->vertex_handle(pVertexHandleMap[fvit->idx()]);
        }
        faceVertexHandles.push_back(vh);
    }
    pSubmesh->add_face(faceVertexHandles);
}

/**
 * @brief MeshSegmentation::ComponentSegmentation
 * Segment a mesh by connectivity only.
 * @param pMesh The mesh that is to be segmented.
 * @return An array of submeshes.
 */
vector<TriMesh *> MeshSegmentation::ComponentSegmentation(TriMesh *pMesh, FPropHandleT<unsigned int> fPropComponentID)
{

    // Label each face by region growing, and create a submesh in the process.
    vector<TriMesh *> submeshes;
    unordered_map<int, int> vertexHandleMap;
    unsigned int segId = 0;
    for(TriMesh::FaceIter fit = pMesh->faces_begin(); fit != pMesh->faces_end(); ++fit)
    {
        // Have this face been labeled yet? 0 is invalid segment id.
        if(pMesh->property(fPropComponentID, *fit) != 0)
        {
            continue;
        }
        segId++;

        // Region growing by BFS, and create a submesh in the process.
        TriMesh *submesh = new TriMesh;
        vertexHandleMap.clear();
        queue<FaceHandle> q;
        q.push(*fit);
        AddFace(pMesh, submesh, vertexHandleMap, *fit);
        pMesh->property(fPropComponentID, *fit) = segId;
        while(!q.empty())
        {
            FaceHandle f = q.front();
            q.pop();
            for(TriMesh::FaceHalfedgeIter fhit = pMesh->fh_iter(f); fhit.is_valid(); ++fhit)
            {
                FaceHandle f2 = pMesh->opposite_face_handle(*fhit);
                if(f2.is_valid() && pMesh->property(fPropComponentID, f2) == 0)
                {
                    q.push(f2);
                    AddFace(pMesh, submesh, vertexHandleMap, f2);
                    pMesh->property(fPropComponentID, f2) = segId;
                }
            }
        }
        submesh->update_normals();
        submeshes.push_back(submesh);
    }

    return submeshes;
}

/**
 * @brief MeshSegmentation::FreeSubmeshVector
 * Free an array of submeshes.
 * @param pSubmeshes The array of submeshes that is to be freed.
 */
void MeshSegmentation::FreeSubmeshVector(vector<TriMesh *> &pSubmeshes)
{
    for(vector<TriMesh *>::iterator it = pSubmeshes.begin(); it != pSubmeshes.end(); ++it)
    {
        delete *it;
    }
    pSubmeshes.clear();
}
