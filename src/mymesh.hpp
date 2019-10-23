#ifndef MYMESH_H
#define MYMESH_H

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>

namespace bwabstraction
{

/**
 * @brief The MyTraits struct defines attributes used by OpenMesh.
 * For OpenMesh 6.
 * Please read http://www.openmesh.org/Documentation/OpenMesh-Doc-Latest/a00020.html
 */
struct MyTraits
{

    // Basic type and percision used by OpenMesh.
    typedef OpenMesh::Vec3d     Point;
    typedef OpenMesh::Vec3d     Normal;
    typedef double              TexCoord1D;
    typedef OpenMesh::Vec2d     TexCoord2D;
    typedef OpenMesh::Vec3d     TexCoord3D;
    typedef short               TextureIndex;
    typedef OpenMesh::Vec3uc    Color;

    // These are macros.
    // Define user-defined data here.
    VertexTraits    {};
    HalfedgeTraits  {};
    EdgeTraits      {};
    FaceTraits      {};

    // These are macros too.
    // Request standard OpenMesh data here.
    VertexAttributes    ( OpenMesh::Attributes::Normal );
    HalfedgeAttributes  ( OpenMesh::Attributes::PrevHalfedge );
    EdgeAttributes      ( 0 );
    FaceAttributes      ( OpenMesh::Attributes::Normal | OpenMesh::Attributes::Status );

};

// Define basic mesh type.
typedef OpenMesh::TriMesh_ArrayKernelT<MyTraits> MyTriMesh;
typedef OpenMesh::PolyMesh_ArrayKernelT<MyTraits> MyPolyMesh;

} // namespace bwabstraction

#endif // MYMESH_H

