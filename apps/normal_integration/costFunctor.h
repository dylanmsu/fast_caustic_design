#ifndef COSTFUNCTOR_H
#define COSTFUNCTOR_H

#include "ceres/ceres.h"
#include "glog/logging.h"

#define EINT_WEIGHT 0.01
#define EBAR_WEIGHT 0.0
#define EDIR_WEIGHT 5.0
#define EREG_WEIGHT 1.0

/******************************************************/
/*************** Ceres-Helper-Methods *****************/
/******************************************************/

using namespace std;

typedef unsigned int uint;

template<typename T> void cross(T* v1, T* v2, T* result);
template<typename T> T angle(T* v1, T* v2);
template<typename T> void normalize(T* v);
template<typename T> T evaluateInt(const T* const vertex, const T** neighbors, uint nNeighbors, const vector<int> & neighborMap, const std::vector<double> &desiredNormal, T* result);
template<typename T> T evaluateInt2(const T* const vertex, const T** neighbors, uint nNeighbors, const vector<int> & neighborMap, const std::vector<double> &desiredNormal, T* result);
template<typename T> void calcVertexNormal(const T* vertex, std::vector<T> &result, const T** neighbors, const std::vector<int>& neighborMap);
//template<typename T> T evaluateReg(const T** const allVertices, const float* L, uint nVertices);

/******************************************************/
/*************** Ceres-Helper-Methods *****************/
/******************************************************/

// For EInt
template<typename T> void cross(T* v1, T* v2, T* result)
{
    result[0] = T(v1[1]*v2[2] - v1[2]*v2[1]);
    result[1] = T(v1[2]*v2[0] - v1[0]*v2[2]);
    result[2] = T(v1[0]*v2[1] - v1[1]*v2[0]);
}

template<typename T> T angle(T* v1, T* v2)
{
    T dot = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    T angle = ceres::acos(dot);
    return angle;
}

template<typename T> std::vector<T> refract_T(
    const std::vector<T> &surfaceNormal,
    const std::vector<T>& rayDirection,
    T n1,  // Index of refraction of the initial medium
    T n2   // Index of refraction of the second medium
) {
    // Calculate the ratio of indices of refraction
    T nRatio = n1 / n2;

    // Calculate the dot product of surfaceNormal and rayDirection
    T dotProduct = surfaceNormal[0] * rayDirection[0] +
                        surfaceNormal[1] * rayDirection[1] +
                        surfaceNormal[2] * rayDirection[2];

    // Determine the cosine of the incident angle
    T cosThetaI = -dotProduct;  // Cosine of the angle between the ray and the normal

    // Calculate sin^2(thetaT) using Snell's Law
    T sin2ThetaT = nRatio * nRatio * (T(1.0) - cosThetaI * cosThetaI);

    // Compute cos(thetaT) for the refracted angle
    T cosThetaT = ceres::sqrt(T(1.0) - sin2ThetaT);

    // Calculate the refracted ray direction
    std::vector<T> refractedRay(3);
    for (int i = 0; i < 3; ++i) {
        refractedRay[i] = nRatio * rayDirection[i] + 
                          (nRatio * cosThetaI - cosThetaT) * surfaceNormal[i];
    }

    return refractedRay;
}

template<typename T> void normalize(T* v)
{
    T sum = T(0);
    for (uint i=0; i<3; i++)
    {
        sum += v[i] * v[i];
    }

    sum = ceres::sqrt(sum);

    //if (sum > T(10e-12)) {
        for (uint i=0; i<3; i++)
        {
            v[i] = v[i]/sum;
        }
    //}
}

template<typename T> T evaluateInt(const T* const vertex, const T** neighbors, uint nNeighbors, const vector<int> & neighborMap, const std::vector<double> &desiredNormal, T* result)
{
    // -- vertex normal
    std::vector<T> vertexNormal(3);
    calcVertexNormal(vertex, vertexNormal, neighbors, neighborMap);

    // evaluation
    T x = vertexNormal[0] - T(desiredNormal[0]);
    T y = vertexNormal[1] - T(desiredNormal[1]);
    T z = vertexNormal[2] - T(desiredNormal[2]);

    result[0] = x*T(EINT_WEIGHT);
    result[1] = y*T(EINT_WEIGHT);
    result[2] = z*T(EINT_WEIGHT);

    return T(0);
}

template<typename T> T evaluateInt2(const T* const vertex, const T** neighbors, uint nNeighbors, const vector<int> & neighborMap, const std::vector<double> &desiredNormal, T* result)
{
    // -- vertex normal
    std::vector<T> vertexNormal(3);
    calcVertexNormal(vertex, vertexNormal, neighbors, neighborMap);

    std::vector<T> incidentLight(3);
    incidentLight[0] = T(0.0);
    incidentLight[1] = T(0.0);
    incidentLight[2] = T(-1.0);

    T r = T(1.49);

    vertexNormal[0] *= T(-1.0);
    vertexNormal[1] *= T(-1.0);
    vertexNormal[2] *= T(-1.0);

    std::vector<T> refracted = refract_T(vertexNormal, incidentLight, r, T(1.0));

    refracted[0] /= -(refracted[2] + vertex[2]);
    refracted[1] /= -(refracted[2] + vertex[2]);
    refracted[2] /= -(refracted[2] + vertex[2]);

    refracted[0] += vertex[0];
    refracted[1] += vertex[1];
    refracted[2] += vertex[2];

    // evaluation
    T x = refracted[0] - T(desiredNormal[0]);
    T y = refracted[1] - T(desiredNormal[1]);
    T z = refracted[2] - T(desiredNormal[2]);

    result[0] = x*T(EINT_WEIGHT);
    result[1] = y*T(EINT_WEIGHT);
    result[2] = z*T(EINT_WEIGHT);

    return T(0);
}

template<typename T>
void calcVertexNormal(const T* vertex, std::vector<T> &result, const T** neighbors, const std::vector<int>& neighborMap) {
    result[0] = T(0);
    result[1] = T(0);
    result[2] = T(0);

    for (uint i = 0; i < neighborMap.size() / 2; i++) {
        T faceNormal[3] = {T(0), T(0), T(0)};
        T edge1Res[3], edge2Res[3];
        
        // Calculate edge vectors
        for (uint j = 0; j < 3; j++) {
            edge1Res[j] = neighbors[neighborMap[i * 2]][j] - vertex[j];
            edge2Res[j] = neighbors[neighborMap[i * 2 + 1]][j] - vertex[j];
        }

        // Normalize if non-zero magnitude
        normalize(edge1Res);
        normalize(edge2Res);

        // Calculate incident angle between edge vectors
        T incidentAngle = angle(edge1Res, edge2Res);

        // Calculate and normalize the cross product for the face normal
        cross(edge1Res, edge2Res, faceNormal);

        // Accumulate weighted normal
        for (uint j = 0; j < 3; j++) {
            result[j] += -faceNormal[j] * incidentAngle;
        }
    }

    // Normalize the accumulated vertex normal
    normalize(result.data());
}

// For EReg
template<typename T> 
void evaluateReg(const T** const allVertices, uint nVertices, 
                 std::vector<double> &laplacian, 
                 std::vector<int> &neighbors, T* res) 
{
    double laplacian_sum = 0.0;

    // Compute sum of Laplacian weights
    for (uint i = 0; i < laplacian.size(); i++) {
        laplacian_sum += laplacian[i];
    }

    // Clear the result
    for (uint i = 0; i < 3; i++) {
        res[i] = T(0);
    }

    // Apply Laplacian smoothing with density compensation
    for (uint i = 0; i < nVertices; i++) {
        if (i == 0) {
            res[2] += T(-laplacian_sum) * allVertices[i][2] * T(EREG_WEIGHT);
        } else {
            res[2] += T(laplacian[i - 1]) * allVertices[i][2] * T(EREG_WEIGHT);
        }
    }
}



/**********************************************/
/************** Cost-Functors *****************/
/**********************************************/

/********* EInt *********/

class CostFunctorEint8Neighbors{
public:
    CostFunctorEint8Neighbors(std::vector<double> &desiredNormal, vector<int> & neighborMap): desiredNormal(desiredNormal), neighborMap(neighborMap)
    {}

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          const T* const neighbor5,
                                          const T* const neighbor6,
                                          const T* const neighbor7,
                                          const T* const neighbor8,
                                          T* residual) const
    {

        // -- preparation
        const T* neighbors[8];
        neighbors[0] = neighbor1;
        neighbors[1] = neighbor2;
        neighbors[2] = neighbor3;
        neighbors[3] = neighbor4;
        neighbors[4] = neighbor5;
        neighbors[5] = neighbor6;
        neighbors[6] = neighbor7;
        neighbors[7] = neighbor8;


        evaluateInt(vertex, neighbors, 8, neighborMap, desiredNormal, residual);

        return true;
    }

private:
    std::vector<double> desiredNormal;
    vector<int> neighborMap;

};


class CostFunctorEint7Neighbors{
public:
    CostFunctorEint7Neighbors(std::vector<double> &desiredNormal, vector<int> & neighborMap): desiredNormal(desiredNormal), neighborMap(neighborMap)
    {}

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          const T* const neighbor5,
                                          const T* const neighbor6,
                                          const T* const neighbor7,
                                          T* residual) const
    {

        // -- preparation
        const T* neighbors[7];
        neighbors[0] = neighbor1;
        neighbors[1] = neighbor2;
        neighbors[2] = neighbor3;
        neighbors[3] = neighbor4;
        neighbors[4] = neighbor5;
        neighbors[5] = neighbor6;
        neighbors[6] = neighbor7;


        evaluateInt(vertex, neighbors, 7, neighborMap, desiredNormal, residual);

        return true;
    }

private:
    std::vector<double> desiredNormal;
    vector<int> neighborMap;

};


class CostFunctorEint6Neighbors{
public:
    CostFunctorEint6Neighbors(std::vector<double> &desiredNormal, vector<int> & neighborMap): desiredNormal(desiredNormal), neighborMap(neighborMap)
    {}

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          const T* const neighbor5,
                                          const T* const neighbor6,
                                          T* residual) const
    {

        // -- preparation
        const T* neighbors[6];
        neighbors[0] = neighbor1;
        neighbors[1] = neighbor2;
        neighbors[2] = neighbor3;
        neighbors[3] = neighbor4;
        neighbors[4] = neighbor5;
        neighbors[5] = neighbor6;


        evaluateInt(vertex, neighbors, 6, neighborMap, desiredNormal, residual);

        return true;
    }

private:
    std::vector<double> desiredNormal;
    vector<int> neighborMap;

};

class CostFunctorEint5Neighbors{
public:
    CostFunctorEint5Neighbors(std::vector<double> &desiredNormal, vector<int> & neighborMap): desiredNormal(desiredNormal), neighborMap(neighborMap)
    {}

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          const T* const neighbor5,
                                          T* residual) const
    {

        // -- preparation
        const T* neighbors[5];
        neighbors[0] = neighbor1;
        neighbors[1] = neighbor2;
        neighbors[2] = neighbor3;
        neighbors[3] = neighbor4;
        neighbors[4] = neighbor5;


        evaluateInt(vertex, neighbors, 5, neighborMap, desiredNormal, residual);

        return true;
    }

private:
    std::vector<double> desiredNormal;
    vector<int> neighborMap;

};


class CostFunctorEint4Neighbors{
public:
    CostFunctorEint4Neighbors(std::vector<double> &desiredNormal, vector<int> & neighborMap): desiredNormal(desiredNormal), neighborMap(neighborMap)
    {}

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          T* residual) const
    {

        // -- preparation
        const T* neighbors[4];
        neighbors[0] = neighbor1;
        neighbors[1] = neighbor2;
        neighbors[2] = neighbor3;
        neighbors[3] = neighbor4;


        evaluateInt(vertex, neighbors, 4, neighborMap, desiredNormal, residual);

        return true;
    }

private:
    std::vector<double> desiredNormal;
    vector<int> neighborMap;

};


class CostFunctorEint3Neighbors{
public:
    CostFunctorEint3Neighbors(std::vector<double> &desiredNormal, vector<int> & neighborMap): desiredNormal(desiredNormal), neighborMap(neighborMap)
    {}

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          T* residual) const
    {

        // -- preparation
        const T* neighbors[3];
        neighbors[0] = neighbor1;
        neighbors[1] = neighbor2;
        neighbors[2] = neighbor3;


        evaluateInt(vertex, neighbors, 3, neighborMap, desiredNormal, residual);

        return true;
    }

private:
    std::vector<double> desiredNormal;
    vector<int> neighborMap;

};


class CostFunctorEint2Neighbors{
public:
    CostFunctorEint2Neighbors(std::vector<double> &desiredNormal, vector<int> & neighborMap): desiredNormal(desiredNormal), neighborMap(neighborMap)
    {}

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          T* residual) const
    {

        // -- preparation
        const T* neighbors[2];
        neighbors[0] = neighbor1;
        neighbors[1] = neighbor2;

        evaluateInt(vertex, neighbors, 2, neighborMap, desiredNormal, residual);

        return true;
    }

private:
    std::vector<double> desiredNormal;
    vector<int> neighborMap;

};

/********* EReg *********/

class CostFunctorEreg8Neighbors{
public:
    CostFunctorEreg8Neighbors(std::vector<double> &laplacian, vector<int> &neighbors) : laplacian(laplacian), neighbors(neighbors)
    {

    }

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          const T* const neighbor5,
                                          const T* const neighbor6,
                                          const T* const neighbor7,
                                          const T* const neighbor8,
                                          T* residual) const
    {

        const T* allVertices[9];
        allVertices[0] = vertex;
        allVertices[1] = neighbor1;
        allVertices[2] = neighbor2;
        allVertices[3] = neighbor3;
        allVertices[4] = neighbor4;
        allVertices[5] = neighbor5;
        allVertices[6] = neighbor6;
        allVertices[7] = neighbor7;
        allVertices[8] = neighbor8;


        evaluateReg(allVertices, 9, laplacian, neighbors, residual);

        return true;
    }
private:
    std::vector<double> &laplacian;
    vector<int> &neighbors;
};

class CostFunctorEreg7Neighbors{
public:
    CostFunctorEreg7Neighbors(std::vector<double> &laplacian, vector<int> &neighbors) : laplacian(laplacian), neighbors(neighbors)
    {

    }

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          const T* const neighbor5,
                                          const T* const neighbor6,
                                          const T* const neighbor7,
                                          T* residual) const
    {

        const T* allVertices[8];
        allVertices[0] = vertex;
        allVertices[1] = neighbor1;
        allVertices[2] = neighbor2;
        allVertices[3] = neighbor3;
        allVertices[4] = neighbor4;
        allVertices[5] = neighbor5;
        allVertices[6] = neighbor6;
        allVertices[7] = neighbor7;


        evaluateReg(allVertices, 8, laplacian, neighbors, residual);

        return true;
    }
private:
    std::vector<double> &laplacian;
    vector<int> &neighbors;
};

class CostFunctorEreg6Neighbors{
public:
    CostFunctorEreg6Neighbors(std::vector<double> &laplacian, vector<int> &neighbors) : laplacian(laplacian), neighbors(neighbors)
    {

    }

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          const T* const neighbor5,
                                          const T* const neighbor6,
                                          T* residual) const
    {

        const T* allVertices[7];
        allVertices[0] = vertex;
        allVertices[1] = neighbor1;
        allVertices[2] = neighbor2;
        allVertices[3] = neighbor3;
        allVertices[4] = neighbor4;
        allVertices[5] = neighbor5;
        allVertices[6] = neighbor6;


        evaluateReg(allVertices, 7, laplacian, neighbors, residual);

        return true;
    }
private:
    std::vector<double> &laplacian;
    vector<int> &neighbors;
};

class CostFunctorEreg5Neighbors{
public:
    CostFunctorEreg5Neighbors(std::vector<double> &laplacian, vector<int> &neighbors) : laplacian(laplacian), neighbors(neighbors)
    {

    }

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          const T* const neighbor5,
                                          T* residual) const
    {

        const T* allVertices[6];
        allVertices[0] = vertex;
        allVertices[1] = neighbor1;
        allVertices[2] = neighbor2;
        allVertices[3] = neighbor3;
        allVertices[4] = neighbor4;
        allVertices[5] = neighbor5;


        evaluateReg(allVertices, 6, laplacian, neighbors, residual);

        return true;
    }
private:
    std::vector<double> &laplacian;
    vector<int> &neighbors;
};

class CostFunctorEreg4Neighbors{
public:
    CostFunctorEreg4Neighbors(std::vector<double> &laplacian, vector<int> &neighbors) : laplacian(laplacian), neighbors(neighbors)
    {

    }

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          const T* const neighbor4,
                                          T* residual) const
    {

        const T* allVertices[5];
        allVertices[0] = vertex;
        allVertices[1] = neighbor1;
        allVertices[2] = neighbor2;
        allVertices[3] = neighbor3;
        allVertices[4] = neighbor4;


        evaluateReg(allVertices, 5, laplacian, neighbors, residual);

        return true;
    }
private:
    std::vector<double> &laplacian;
    vector<int> &neighbors;
};


class CostFunctorEreg3Neighbors{
public:
    CostFunctorEreg3Neighbors(std::vector<double> &laplacian, vector<int> &neighbors) : laplacian(laplacian), neighbors(neighbors)
    {

    }

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          const T* const neighbor3,
                                          T* residual) const
    {

        const T* allVertices[4];
        allVertices[0] = vertex;
        allVertices[1] = neighbor1;
        allVertices[2] = neighbor2;
        allVertices[3] = neighbor3;


        evaluateReg(allVertices, 4, laplacian, neighbors, residual);

        return true;
    }
private:
    std::vector<double> &laplacian;
    vector<int> &neighbors;
};


class CostFunctorEreg2Neighbors{
public:
    CostFunctorEreg2Neighbors(std::vector<double> &laplacian, vector<int> &neighbors) : laplacian(laplacian), neighbors(neighbors)
    {

    }

    template <typename T> bool operator()(const T* const vertex,
                                          const T* const neighbor1,
                                          const T* const neighbor2,
                                          T* residual) const
    {

        const T* allVertices[3];
        allVertices[0] = vertex;
        allVertices[1] = neighbor1;
        allVertices[2] = neighbor2;


        evaluateReg(allVertices, 3, laplacian, neighbors, residual);

        return true;
    }
private:
    std::vector<double> &laplacian;
    vector<int> &neighbors;
};

/********* EBar *********/

/*class CostFunctorEbar2 {
public:
    CostFunctorEbar2 (std::vector<double>* receiverPosition): receiverPosition(receiverPosition)
    {}

    template <typename T> bool operator()(const T* const vertex, T* e) const{

        T dth = T(EBAR_DETH);

        T distance = vertex[0] - T(receiverPosition->x);
        e[0] = T(EBAR_WEIGHT) * ceres::fmax(T(0), - ceres::log( (T(1)-distance) + dth) );

        return true;
    }

private:
    std::vector<double>* receiverPosition;

};*/



/********* EDir *********/

class CostFunctorEdir2{
public:
    CostFunctorEdir2(std::vector<double> * s, float weightMultiplicator): source(s), weightMultiplicator(weightMultiplicator) {
    }

    template <typename T> bool operator()(const T* const vertex, T* e) const{
        // x - proj(..) will only return the difference in y- and z-coordinates. we may ignore the rest
        T x = vertex[0] - T((*source)[0]);
        T y = vertex[1] - T((*source)[1]);
        e[0] = T(EDIR_WEIGHT*weightMultiplicator) * x;
        e[1] = T(EDIR_WEIGHT*weightMultiplicator) * y;
        e[2] = T(0);
        return true;
    }

private:
    std::vector<double> * source;
    float weightMultiplicator;
};


#endif // COSTFUNCTOR_H
