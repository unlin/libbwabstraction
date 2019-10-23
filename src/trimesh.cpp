#include "trimesh.hpp"
#include <CGAL/convex_hull_2.h>
#include <CGAL/min_quadrilateral_2.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Min_circle_2.h>
#include <CGAL/Min_circle_2_traits_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/Modifier_base.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <fstream>
#include <sstream>

using namespace bwabstraction;
using namespace std;
using namespace OpenMesh;
using namespace Eigen;

bool TriMesh::LoadOBJ(string file)
{
    if (!OpenMesh::IO::read_mesh(*this, file))
    {
        std::cerr << "read error\n";
        return false;
    }
    Normalize();
    update_normals();
    return true;
}

bool TriMesh::LoadOM(string file)
{
    OpenMesh::IO::Options options;
    options += OpenMesh::IO::Options::Binary;
    options += OpenMesh::IO::Options::LSB;
    options += OpenMesh::IO::Options::VertexNormal;
    options += OpenMesh::IO::Options::FaceNormal;

    if (!OpenMesh::IO::read_mesh(*this, file, options))
    {
        printf("LoadOM() failed.\n");
        return false;
    }

    // although we have saved face normals in .om files, but OpenMesh seems to screw it up...
    // recompute face normals...
    if (!options.check(IO::Options::FaceNormal) && has_face_normals())
    {
        update_face_normals();
    }

    // update vertex normals require face normals
    if (!options.check(IO::Options::VertexNormal) && has_vertex_normals())
    {
        update_vertex_normals();
    }

    return true;
}

void TriMesh::SaveOM(string file)
{
    OpenMesh::IO::Options options;
    options += OpenMesh::IO::Options::Binary;
    options += OpenMesh::IO::Options::LSB;
    options += OpenMesh::IO::Options::VertexNormal;
    options += OpenMesh::IO::Options::FaceNormal;
    //property(fPropGroupId).set_persistent(true);
    if (!OpenMesh::IO::write_mesh(*this, file, options))
    {
        printf("SaveOM() failed.\n");
    }
}

void TriMesh::SaveOBJ(string file)
{
    OpenMesh::IO::Options options;
    options += OpenMesh::IO::Options::VertexNormal;
    if (!OpenMesh::IO::write_mesh(*this, file, options))
    {
        printf("SaveOBJ() failed.\n");
    }
}

bool TriMesh::Load(string filePath)
{
    OpenMesh::IO::Options options;

    if(!OpenMesh::IO::read_mesh(*this, filePath, options))
    {
        return false;
    }

    update_normals();
    return true;
}

void TriMesh::Normalize()
{
    // normalize the model into (-1, -1, -1) ~ (1, 1, 1).
    double min_x = DBL_MAX;
    double max_x = DBL_MIN;
    double min_y = DBL_MAX;
    double max_y = DBL_MIN;
    double min_z = DBL_MAX;
    double max_z = DBL_MIN;

    // for each vertices, find minimum and maximum on each axis.
    for(VertexIter vit = vertices_begin(); vit != vertices_end(); ++vit) {
        Point p = point(*vit);
        min_x = min(p[0], min_x);
        max_x = max(p[0], max_x);
        min_y = min(p[1], min_y);
        max_y = max(p[1], max_y);
        min_z = min(p[2], min_z);
        max_z = max(p[2], max_z);
    }

    // scale and translation values.
    double scale = 2.0 / max(max(max_x - min_x, max_y - min_y), max_z - min_z);
    double x_trans = (max_x + min_x) / -2.0;
    double y_trans = (max_y + min_y) / -2.0;
    double z_trans = (max_z + min_z) / -2.0;

    // normalize the model into (-1, -1, -1) ~ (1, 1, 1).
    for(VertexIter vit = vertices_begin(); vit != vertices_end(); ++vit) {
        Point p = point(*vit);
        p[0] = (p[0] + x_trans) * scale;
        p[1] = (p[1] + y_trans) * scale;
        p[2] = (p[2] + z_trans) * scale;
        set_point(*vit, p);
    }
}

TriMesh::Point TriMesh::ComputeCentroid(void)
{
    Point centroid;
    for(VertexIter vit = vertices_begin(); vit != vertices_end(); ++vit)
    {
        Point p = point(*vit);
        centroid += p;
    }
    centroid /= n_vertices();
    return centroid;
}

void TriMesh::GetMeshData(vector<float> &vertices, vector<float> &normals, vector<unsigned int> &indices)
{
    // clear result vectors.
    vertices.clear();
    normals.clear();
    indices.clear();

    // For each vertex, save its position and normal.
    for(VertexIter it = vertices_begin(); it != vertices_end(); ++it)
    {
        Point p = point(*it);
        vertices.push_back(static_cast<float>(p[0]));
        vertices.push_back(static_cast<float>(p[1]));
        vertices.push_back(static_cast<float>(p[2]));
        Normal n = normal(*it);
        normals.push_back(static_cast<float>(n[0]));
        normals.push_back(static_cast<float>(n[1]));
        normals.push_back(static_cast<float>(n[2]));
    }
    
    // For each face, save three indices.
    for(FaceIter it = faces_begin(); it != faces_end(); ++it)
    {
        for(FaceVertexIter fvit = fv_iter(*it); fvit.is_valid(); ++fvit)
        {
            // OpenMesh uses 0-based index.
            indices.push_back(fvit->idx());
        }
    }
}

void TriMesh::Translate(const Point& t)
{
    for(VertexIter vit = vertices_begin(); vit != vertices_end(); ++vit)
    {
        Point v = point(*vit);
        set_point(*vit, v + t);
    }
}

void TriMesh::RotateNoNormalUpdate(const Point& center, const Point& axis, double radian)
{
    Vector3d v(axis[0], axis[1], axis[2]);
    v.normalize();
    Matrix3d R = AngleAxisd(radian, v).toRotationMatrix();
    RotateNoNormalUpdate(center, R);
}

void TriMesh::RotateNoNormalUpdate(const Point& center, const Matrix3d& R)
{
    for(VertexIter vit = vertices_begin(); vit != vertices_end(); ++vit)
    {
        Point v = point(*vit);
        Vector3d localV = Vector3d(v[0] - center[0], v[1] - center[1], v[2] - center[2]);
        Vector3d rotV = R * localV;
        set_point(*vit, Point(rotV(0) + center[0], rotV(1) + center[1], rotV(2) + center[2]));
    }
}

template<class Key, class Value> bool PairGreater(const pair<Key, Value>& elem1, const pair<Key, Value>& elem2)
{
    return elem1.second > elem2.second;
}

template<class Key, class Value> bool PairSmaller(const pair<Key, Value>& elem1, const pair<Key, Value>& elem2)
{
    return elem1.second < elem2.second;
}

void TriMesh::BuildOBB()
{
    BuildOBBPCA();
    Point size = obb.max - obb.min;
    if(size[0] == 0 || size[1] == 0 || size[2] == 0)
    {
        return;
    }
    for(int i = 0; i < 3; ++i)
    {
        BuildOBBRotatingCalipers();
    }
}

void TriMesh::BuildOBBPCA()
{
    vector<Point> points;
    Point center(0, 0, 0);
    for(VertexIter vit = vertices_begin(); vit != vertices_end(); ++vit)
    {
        Point v = point(*vit);
        points.push_back(v);
        center += v;
    }
    center /= points.size();

    Eigen::Matrix3d tensor;
    tensor << 0, 0, 0, 0, 0, 0, 0, 0, 0;

    for(unsigned int i = 0; i < points.size(); ++i)
    {
        Point v = points[i];
        for( int m=0; m<3; m++ )
        {
            for( int n=0; n<3; n++ )
            {
                tensor( m, n ) += (v[m]-center[m])*(v[n]-center[n]);
            }
        }
    }

    // eigen decomposition
    Eigen::SelfAdjointEigenSolver< Eigen::Matrix3d > eigensolver( tensor );
    Eigen::Vector3d eigenvalue = eigensolver.eigenvalues();
    Eigen::Matrix3d eigenvector = eigensolver.eigenvectors();

    vector< pair<int, double> > eigenValueList;

    for( int i=0; i<3; i++ )
    {
        eigenValueList.push_back( pair<int, double>( i, eigenvalue( i ) ) );
    }

    sort( eigenValueList.begin(), eigenValueList.end(), PairGreater<int, double> );

    int iMax, iMid, iMin;

    iMax = eigenValueList[0].first;
    iMid = eigenValueList[1].first;
    iMin = eigenValueList[2].first;

    obb.origin = center;
    obb.maxAxis = Vec3d( eigenvector( 0, iMax ), eigenvector( 1, iMax ), eigenvector( 2, iMax ) );
    obb.midAxis = Vec3d( eigenvector( 0, iMid ), eigenvector( 1, iMid ), eigenvector( 2, iMid ) );
    obb.minAxis = cross(obb.maxAxis, obb.midAxis);
    obb.max = Vec3d(-DBL_MAX, -DBL_MAX, -DBL_MAX);
    obb.min = Vec3d(DBL_MAX, DBL_MAX, DBL_MAX);
    for(unsigned int i = 0; i < points.size(); ++i)
    {
        obb.Insert_A_Point(points[i]);
    }
    obb.MoveOriginToMiddilePoint();
}

void minRectangle( const vector<Vec2d>& points, vector<Vec2d>& rect )
{
    // build 2D convex hull
    typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
    typedef K::Point_2 Point_2;

    vector<Point_2> pts;
    for( unsigned int i=0; i<points.size(); i++ ) {
        pts.push_back( Point_2( points[i][0], points[i][1] ) );
    }

    vector<Point_2> ch;
    CGAL::convex_hull_2( pts.begin(), pts.end(), std::back_inserter(ch) );

    // compute min rectangle using rotating caliper algorithm [Tou83]
    vector<Point_2> rt;
    CGAL::min_rectangle_2( ch.begin(), ch.end(), std::back_inserter(rt) );

    rect.clear();

    for( unsigned int i=0; i<rt.size(); i++ ) {
        rect.push_back( Vec2d( rt[i].hx(), rt[i].hy() ) );
    }
}

void sortTripleSmaller( const Vec3d& v, Vec3i& vi )
{
    vector<pair<int, double> > pairs;
    for( int i=0; i<3; i++ ) {
        pairs.push_back( pair<int, double>( i, v[i] ) );
    }

    sort( pairs.begin(), pairs.end(), PairSmaller<int, double> );

    for( int i=0; i<3; i++ ) {
        vi[i] = pairs[i].first;
    }
}

void TriMesh::BuildOBBRotatingCalipers()
{
    // find the side with maximal error w.r.t the minimal bounding rectangle from rotating calipers, recompute

    Point dir[3];
    dir[0] = obb.maxAxis;
    dir[1] = obb.midAxis;
    dir[2] = obb.minAxis;

    double side[3];
    double minSide = 1e30;
    int minSideIndex;
    for( int i=0; i<3; i++ ) {
        side[i] = obb.max[i] - obb.min[i];
        if( minSide>side[i] ) {
            minSide = side[i];
            minSideIndex = i;
        }
    }

    if( minSide<1e-3 ) {

        vector<Vec2d> projPts;
        for( VertexIter vit=vertices_begin(); vit!=vertices_end(); ++vit ) {
            Vec3d v = point(*vit );
            Vec2d projV;
            projV[0] = (v-obb.origin) | dir[(minSideIndex+1)%3];
            projV[1] = (v-obb.origin) | dir[(minSideIndex+2)%3];
            projPts.push_back( projV );
        }

        vector<Vec2d> rect;
        minRectangle( projPts, rect );

        Vec3d newDir1 = (rect[1] - rect[0])[0] * dir[(minSideIndex+1)%3] + (rect[1] - rect[0])[1] * dir[(minSideIndex+2)%3];
        newDir1.normalize();
        Vec3d newDir2 = (rect[2] - rect[1])[0] * dir[(minSideIndex+1)%3] + (rect[2] - rect[1])[1] * dir[(minSideIndex+2)%3];
        newDir2.normalize();

        switch ( minSideIndex )
        {
        case 0:
            obb.midAxis = newDir1;
            obb.minAxis = newDir2;
            break;
        case 1:
            obb.minAxis = newDir1;
            obb.maxAxis = newDir2;
            break;
        case 2:
            obb.maxAxis = newDir1;
            obb.midAxis = newDir2;
            break;
        }


        OBB nobb;
        nobb.origin = obb.origin;
        nobb.maxAxis = obb.maxAxis;
        nobb.midAxis = obb.midAxis;
        nobb.minAxis = obb.minAxis;
        for( VertexIter vit=vertices_begin(); vit!=vertices_end(); ++vit ) {
            Vec3d v = point( *vit );
            nobb.Insert_A_Point( v );
        }

        obb = nobb;
        obb.OrderAxis();
        obb.MoveOriginToMiddilePoint();

        return;
    }

    double area[3];
    for( int i=0; i<3; i++ ) {
        area[i] = side[(i+1)%3] * side[(i+2)%3];
    }

    double minRectArea[3];
    vector<vector<Vec2d> > minRects;
    for( int i=0; i<3; i++ ) {

        vector<Vec2d> projPts;
        for( VertexIter vit=vertices_begin(); vit!=vertices_end(); ++vit ) {
            Vec3d v = point( *vit );
            Vec2d projV;
            projV[0] = (v-obb.origin) | dir[(i+1)%3];
            projV[1] = (v-obb.origin) | dir[(i+2)%3];
            projPts.push_back( projV );
        }

        vector<Vec2d> rect;
        minRectangle( projPts, rect );
        minRectArea[i] = (rect[1]-rect[0]).length() * (rect[2]-rect[1]).length();
        minRects.push_back( rect );
    }

    Vec3d minRatioTriple;
    for( int i=0; i<3; i++ )
        minRatioTriple[i] = minRectArea[i] / area[i];

    Vec3i tripleIndices;
    sortTripleSmaller( minRatioTriple, tripleIndices );

    int minIndex = tripleIndices[0];

    Vec3d newDir1 = (minRects[minIndex][1] - minRects[minIndex][0])[0] * dir[(minIndex+1)%3] + (minRects[minIndex][1] - minRects[minIndex][0])[1] * dir[(minIndex+2)%3];
    newDir1.normalize();
    Vec3d newDir2 = (minRects[minIndex][2] - minRects[minIndex][1])[0] * dir[(minIndex+1)%3] + (minRects[minIndex][2] - minRects[minIndex][1])[1] * dir[(minIndex+2)%3];
    newDir2.normalize();

    switch ( minIndex )
    {
    case 0:
        obb.midAxis = newDir1;
        obb.minAxis = newDir2;
        break;
    case 1:
        obb.minAxis = newDir1;
        obb.maxAxis = newDir2;
        break;
    case 2:
        obb.maxAxis = newDir1;
        obb.midAxis = newDir2;
        break;
    }

    OBB nobb;
    nobb.origin = obb.origin;
    nobb.maxAxis = obb.maxAxis;
    nobb.midAxis = obb.midAxis;
    nobb.minAxis = obb.minAxis;
    for( VertexIter vit=vertices_begin(); vit!=vertices_end(); ++vit ) {
        Vec3d v = point( *vit );
        nobb.Insert_A_Point( v );
    }

    obb = nobb;
    obb.OrderAxis();
    obb.MoveOriginToMiddilePoint();
}

void TriMesh::OBB::Insert_A_Point(const Point& p)
{
    Point pLocal;
    pLocal[0] = (p-origin) | maxAxis;
    pLocal[1] = (p-origin) | midAxis;
    pLocal[2] = (p-origin) | minAxis;

    min[0] = ::min(min[0], pLocal[0]);
    min[1] = ::min(min[1], pLocal[1]);
    min[2] = ::min(min[2], pLocal[2]);
    max[0] = ::max(max[0], pLocal[0]);
    max[1] = ::max(max[1], pLocal[1]);
    max[2] = ::max(max[2], pLocal[2]);
}

void TriMesh::OBB::MoveOriginToMiddilePoint()
{
    Point newOriginLocal = (max + min) / 2;
    max -= newOriginLocal;
    min -= newOriginLocal;
    origin = origin + maxAxis * newOriginLocal[0] + midAxis * newOriginLocal[1] + minAxis * newOriginLocal[2];
}

void TriMesh::OBB::OrderAxis()
{
    Vec3d dims;
    for( int i=0; i<3; i++ ) {
        dims[i]= max[i] - min[i];
    }

    Vec3d axes[3];
    axes[0] = maxAxis;
    axes[1] = midAxis;
    axes[2] = minAxis;

    Vec3i sortIds;
    sortTripleSmaller( dims, sortIds );

    maxAxis = axes[sortIds[2]];
    midAxis = axes[sortIds[1]];
    minAxis = axes[sortIds[0]];

    Vec3d tempMin, tempMax;
    tempMin[0] = min[sortIds[2]];
    tempMin[1] = min[sortIds[1]];
    tempMin[2] = min[sortIds[0]];

    tempMax[0] = max[sortIds[2]];
    tempMax[1] = max[sortIds[1]];
    tempMax[2] = max[sortIds[0]];

    min = tempMin;
    max = tempMax;
}

TriMesh::OBB::OBB()
{
    origin = Vec3d( 0, 0, 0 );
    maxAxis = Vec3d( 1, 0, 0 );
    midAxis = Vec3d( 0, 1, 0 );
    minAxis = Vec3d( 0, 0, 1 );
    max = Vec3d(-DBL_MAX, -DBL_MAX, -DBL_MAX);
    min = Vec3d(DBL_MAX, DBL_MAX, DBL_MAX);
}

template<template <typename MeshType, typename RealType=float>class Subdivider>
void TriMesh::Subdivide(int iterations)
{
    Subdivider<TriMesh> subdivider;
    subdivider.attach(*this);
    subdivider(iterations);
    subdivider.detach();
}
