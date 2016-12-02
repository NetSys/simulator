#ifndef RANDOM_VARIABLE_H
#define RANDOM_VARIABLE_H

#include <vector>
#include <random>

class RandomVariable {
public:
    virtual double value() = 0;
};

class UniformRandomVariable : public RandomVariable {
public:
  virtual double value();
  UniformRandomVariable();
  UniformRandomVariable(double min, double max);
  double min_;
  double max_;
};


class ExponentialRandomVariable : public RandomVariable {
public:
  virtual double value();
  ExponentialRandomVariable(double avg);
  double avg_;
  UniformRandomVariable urv;
};



struct CDFentry {
  double cdf_;
  double val_;
};


// READ VALUE IN PACKETS!!
class EmpiricalRandomVariable : public RandomVariable {
public:
  virtual double value();
  double interpolate(double u, double x1, double y1, double x2, double y2);

  EmpiricalRandomVariable(std::string filename, bool smooth = true);
  int loadCDF(std::string filename);

  double mean_flow_size;

  bool smooth;

protected:
  int lookup(double u);

  double minCDF_;		// min value of the CDF (default to 0)
  double maxCDF_;		// max value of the CDF (default to 1)
  int numEntry_;		// number of entries in the CDF table
  int maxEntry_;		// size of the CDF table (mem allocation)
  CDFentry* table_;	// CDF table of (val_, cdf_)
};

// READ VALUE IN BYTES
class EmpiricalBytesRandomVariable : public EmpiricalRandomVariable {
public:
  EmpiricalBytesRandomVariable(std::string filename, bool smooth = true);
  int loadCDF(std::string filename);
  
  double sizeWithHeader;
};

/*
Inherits from EmpricialRandomVariable since they both do flow sizes
This one doesn't interpolate and does uniform/bimodal/trimodal/etc
distributions.
Nice thing about inheriting is you don't have to rewrite
FlowCreationForInitializationEvent to accept this.
*/
class NAryRandomVariable : public EmpiricalRandomVariable {
public:
  virtual double value();
  NAryRandomVariable(std::string filename);
  int loadSizes(std::string filename);

protected:
  std::vector<double> flowSizes;
};

class CDFRandomVariable : public EmpiricalRandomVariable {
public:
  CDFRandomVariable(std::string filename);
  virtual double value();
};

class ConstantVariable : public EmpiricalRandomVariable {
public:
    double v;
    ConstantVariable(double value);
    double value();
};

class GaussianRandomVariable{
  public:
  double value();
  GaussianRandomVariable(double avg, double std);
  double avg;
  double std;

  std::default_random_engine generator;
  std::normal_distribution<double> *distribution;
};

#endif
