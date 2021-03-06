#ifndef RBF_RBFNODE_H
#define RBF_RBFNODE_H

#include <maya/MArrayDataHandle.h>
#include <maya/MDoubleArray.h>
#include <maya/MPxNode.h>
#include <maya/MQuaternion.h>

#include <Eigen/Dense>
#include <Eigen/Eigenvalues> 

#include <limits>
#include <vector>

using Eigen::MatrixXd;
using Eigen::VectorXd;


class RBFNode : public MPxNode {
 public:
  RBFNode();
  virtual ~RBFNode();
  static void* creator();

  virtual MStatus setDependentsDirty(const MPlug& plug, MPlugArray& affectedPlugs) override;
  virtual MStatus preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode) override;
  virtual MStatus compute(const MPlug& plug, MDataBlock& data) override;
  virtual bool isPassiveOutput(const MPlug & plug) const override;

  static MStatus initialize();
  static MTypeId id;
  static const MString kName;
  static MObject aOutputValues;
  static MObject aOutputRotateX;
  static MObject aOutputRotateY;
  static MObject aOutputRotateZ;
  static MObject aOutputRotate;
  static MObject aInputValues;
  static MObject aInputQuats;
  static MObject aInputRestQuats;
  static MObject aInputValueCount;
  static MObject aInputQuatCount;
  static MObject aOutputValueCount;
  static MObject aOutputQuatCount;
  static MObject aRBFFunction;
  static MObject aRadius;
  static MObject aRegularization;
  static MObject aSamples;
  static MObject aSampleRadius;
  static MObject aSampleInputValues;
  static MObject aSampleInputQuats;
  static MObject aSampleOutputValues;
  static MObject aSampleOutputQuats;

 private:
   static void affects(const MObject& attribute);
   MStatus buildFeatureMatrix(MDataBlock& data, int inputCount, int outputCount, int inputQuatCount,
                             int outputQuatCount, short rbf, double radius, const std::vector<MQuaternion>& inputRestQuats);
  MStatus getDoubleValues(MArrayDataHandle& hArray, int count, VectorXd& values);
  MStatus getQuaternionValues(MArrayDataHandle& hArray, int count,
                              std::vector<MQuaternion>& quaternions);
  MatrixXd pseudoInverse(const MatrixXd& a,
                         double epsilon = std::numeric_limits<double>::epsilon());
  void decomposeSwingTwist(const MQuaternion& q, MQuaternion& swing, MQuaternion& twist);
  void swingTwistDistance(MQuaternion& q1, MQuaternion& q2, double& swingDistance, double& twistDistance);
  double quaternionDistance(MQuaternion& q1, MQuaternion& q2);

  inline double quaternionDot(const MQuaternion& q1, const MQuaternion& q2) {
    double dotValue = (q1.x * q2.x) + (q1.y * q2.y) + (q1.z * q2.z) + (q1.w * q2.w);
    // Clamp any floating point error
    if (dotValue < -1.0) {
      dotValue = -1.0;
    } else if (dotValue > 1.0) {
      dotValue = 1.0;
    }
    return dotValue;
  }

  bool dirty_;
  double distanceNorm_;
  VectorXd sampleRadius_;
  VectorXd featureNorms_;
  MatrixXd featureMatrix_;
  std::vector<std::vector<MQuaternion>> featureQuatMatrix_;
  MatrixXd outputScalarMatrix_;
  std::vector<std::vector<MQuaternion>> outputQuatMatrix_;
  std::vector<MatrixXd> outputQuats_;
  MatrixXd theta_;
};

struct Gaussian {
  Gaussian(const double& radius) {
    static const double kFalloff = 0.4;
    r = radius > 0.0 ? radius : 0.001;
    r *= kFalloff;
  }
  const double operator()(const double& x) const { return exp(-(x * x) / (2.0 * r * r)); }
  double r;
};

struct ThinPlate {
  ThinPlate(const double& radius) { r = radius > 0.0 ? radius : 0.001; }
  const double operator()(const double& x) const {
    double v = x / r;
    return v > 0.0 ? v * v * log(v) : v;
  }
  double r;
};

struct MultiQuadraticBiharmonic {
  MultiQuadraticBiharmonic(const double& radius) : r(radius) {}
  const double operator()(const double& x) const { return sqrt((x * x) + (r * r)); }
  double r;
};

struct InverseMultiQuadraticBiharmonic {
  InverseMultiQuadraticBiharmonic(const double& radius) : r(radius) {}
  const double operator()(const double& x) const { return 1.0 / sqrt((x * x) + (r * r)); }
  double r;
};

struct BeckertWendlandC2Basis {
  BeckertWendlandC2Basis(const double& radius) { r = radius > 0.0 ? radius : 0.001; }
  const double operator()(const double& x) const {
    double v = x / r;
    double first = (1.0 - v > 0.0) ? pow(1.0 - v, 4) : 0.0;
    double second = 4.0 * v + 1.0;
    return first * second;
  }
  double r;
};

template <typename T>
void applyRbf(Eigen::MatrixBase<T>& m, short rbf, double radius) {
  switch (rbf) {
    case 0:
      break;
    case 1:
      m = m.unaryExpr(Gaussian(radius));
      break;
    case 2:
      m = m.unaryExpr(ThinPlate(radius));
      break;
    case 3:
      m = m.unaryExpr(MultiQuadraticBiharmonic(radius));
      break;
    case 4:
      m = m.unaryExpr(InverseMultiQuadraticBiharmonic(radius));
      break;
    case 5:
      m = m.unaryExpr(BeckertWendlandC2Basis(radius));
      break;
  }
}

#endif
