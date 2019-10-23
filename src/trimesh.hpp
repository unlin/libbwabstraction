#ifndef TRIMESH_H
#define TRIMESH_H

#include "mymesh.hpp"
#include <string>
#include <vector>
#include <Eigen/Eigen>

namespace bwabstraction
{

class TriMesh : public MyTriMesh
{

public:

    bool LoadOBJ(std::string file);
    bool LoadOM(std::string file);
    void SaveOBJ(std::string file);
    void SaveOM(std::string file);
    bool Load(std::string filePath);
    void Normalize();
    Point ComputeCentroid(void);
    void GetMeshData(std::vector<float> &vertices, std::vector<float> &normals, std::vector<unsigned int> &indices);
    template<template <typename MeshType, typename RealType=float>class Subdivider> void Subdivide(int iterations);

    void Translate(const Point& t);
    void RotateNoNormalUpdate(const Point& center, const Point& axis, double radian);
    void RotateNoNormalUpdate(const Point& center, const Eigen::Matrix3d& R);
    void BuildOBB();
    void BuildOBBPCA();
    void BuildOBBRotatingCalipers();

    struct OBB
    {

        Point origin;
        Point max;
        Point min;
        Point maxAxis;
        Point midAxis;
        Point minAxis;

        OBB();
        void Insert_A_Point(const Point& p);
        void MoveOriginToMiddilePoint();
        void OrderAxis();

    } obb;

};

} // namespace bwabstraction

#endif // TRIMESH_H
