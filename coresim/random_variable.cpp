#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <deque>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#include "random_variable.h"

using namespace std;

/* Uniform Random Variable
*/
UniformRandomVariable::UniformRandomVariable() {
  min_ = 0.0;
  max_ = 1.0;
}

UniformRandomVariable::UniformRandomVariable(double min, double max) {
  min_ = min;
  max_ = max;
}

double UniformRandomVariable::value() { // never return 0
  //double unif0_1 = (1.0 * rand() + 1.0) / (RAND_MAX* 1.0 + 1.0);
  double unif0_1 = (1.0 * rand()) / (RAND_MAX * 1.0);
  return min_ + (max_ - min_) * unif0_1;
}



/* Exponential Random Variable
 */
ExponentialRandomVariable::ExponentialRandomVariable(double avg) {
  avg_ = avg;
  urv = UniformRandomVariable();

}

double ExponentialRandomVariable::value() {
  return -1.0 * avg_ * log(urv.value());
}




/* Empirical Random Variable with random interpolation
 * Ported from NS2
 */
EmpiricalRandomVariable::EmpiricalRandomVariable(std::string filename, bool smooth) {
  this->smooth = smooth;
  minCDF_ = 0;
  maxCDF_ = 1;
  maxEntry_ = 65536;
  table_ = new CDFentry[maxEntry_];
  if(filename != "")
      loadCDF(filename);
}

double EmpiricalRandomVariable::value() {
  if (numEntry_ <= 0)
    return 0;
  double u = (1.0 * rand()) / RAND_MAX;
  int mid = lookup(u);
  if (mid && u < table_[mid].cdf_)
    return interpolate(u, table_[mid-1].cdf_, table_[mid-1].val_,
           table_[mid].cdf_, table_[mid].val_);
  return table_[mid].val_;
}

double EmpiricalRandomVariable::interpolate(double x, double x1, double y1,
                                            double x2, double y2) {
  double value = y1 + (x - x1) * (y2 - y1) / (x2 - x1);
  return value;
}

int EmpiricalRandomVariable::lookup(double u) {
  // always return an index whose value is >= u
  int lo, hi, mid;
  if (u <= table_[0].cdf_)
    return 0;
  for (lo=1, hi=numEntry_-1;  lo < hi; ) {
    mid = (lo + hi) / 2;
    if (u > table_[mid].cdf_)
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo;
}

int EmpiricalRandomVariable::loadCDF(std::string filename) {
  assert(false);
  std::string line;
  std::ifstream myfile(filename);
  assert(myfile.good());
  numEntry_ = 0;
  double prev_cd = 0;
  int prev_sz = 1;
  double w_sum = 0;
  while (std::getline(myfile, line))
  {
    std::istringstream is(line);
    is >> table_[numEntry_].val_;
    is >> table_[numEntry_].cdf_;
    is >> table_[numEntry_].cdf_;

    double freq = table_[numEntry_].cdf_ - prev_cd;
    double flow_sz = this->smooth?(table_[numEntry_].val_ + prev_sz)/2.0:table_[numEntry_].val_;
    assert(freq >= 0);
    w_sum += freq * flow_sz;
    prev_cd = table_[numEntry_].cdf_;
    prev_sz = table_[numEntry_].val_;
    numEntry_ ++;
  }
  this->mean_flow_size = w_sum * 1460.0;
  //std::cout << "Mean flow size derived from CDF file:" << this->mean_flow_size << " smooth = " << this->smooth << "\n";
  //std::cout << "Number of lines in text file: " << numEntry_ << "\n";
  if (myfile.is_open()) {
    myfile.close();
  }
  return numEntry_;
}


EmpiricalBytesRandomVariable::EmpiricalBytesRandomVariable(std::string filename, bool smooth) : EmpiricalRandomVariable("", smooth) {
  if(filename != "")
      loadCDF(filename);
}

int EmpiricalBytesRandomVariable::loadCDF(std::string filename) {
  std::string line;
  std::ifstream myfile(filename);
  assert(myfile.good());
  numEntry_ = 0;
  double prev_cd = 0;
  int prev_sz = 1;
  double w_sum = 0;
  this->sizeWithHeader = 0.0;
  while (std::getline(myfile, line)) {
    std::istringstream is(line);
    is >> table_[numEntry_].val_;
    is >> table_[numEntry_].cdf_;
    is >> table_[numEntry_].cdf_;

    double freq = table_[numEntry_].cdf_ - prev_cd;
    double flow_sz = this->smooth?(table_[numEntry_].val_ + prev_sz)/2.0:table_[numEntry_].val_;
    assert(freq >= 0);
    double num_pkts = std::ceil(flow_sz / 1460);
    double tot = 40 * num_pkts + flow_sz;
    sizeWithHeader += freq * tot;
    w_sum += freq * flow_sz;
    prev_cd = table_[numEntry_].cdf_;
    prev_sz = table_[numEntry_].val_;
    numEntry_ ++;
  }
  this->mean_flow_size = w_sum;
  if (myfile.is_open()) {
    myfile.close();
  }
  return numEntry_;
}


NAryRandomVariable::NAryRandomVariable(std::string filename)
  : EmpiricalRandomVariable(filename) {
  loadSizes(filename);
}

int NAryRandomVariable::loadSizes(std::string filename) {
  std::string line;
  std::ifstream myfile(filename);
  double temp;
  while (std::getline(myfile, line)) {
    std::istringstream is(line);
    is >> temp;
    this->flowSizes.push_back(temp);
  }
  return this->flowSizes.size();
}

double NAryRandomVariable::value() {
  return this->flowSizes[rand() % this->flowSizes.size()];
}

CDFRandomVariable::CDFRandomVariable(std::string filename)
 : EmpiricalRandomVariable(filename, false) {}

double CDFRandomVariable::value() {
  double val = static_cast <double> (rand()) / static_cast <double> (RAND_MAX);
//  std::cout << "randval " << val << " ";
  for (int i = 0; i < numEntry_; i++) {
  //  std::cout << " cdf " << table_[i].cdf_ << " " << table_[i].val_ << " ";
    if (val <= table_[i].cdf_) {
    //  std::cout << " found\n";
      return table_[i].val_;
    }
  }
//  std::cout << " notfound " << table_[numEntry_-1].cdf_ << " " << table_[numEntry_-1].val_ << "\n";
  return table_[numEntry_-1].val_;
}


ConstantVariable::ConstantVariable(double value) : EmpiricalRandomVariable("", false)
{
    this->v = value;
}

double ConstantVariable::value()
{
    return v;
}

GaussianRandomVariable::GaussianRandomVariable(double avg, double std) {
  this->avg = avg;
  this->std = std;

  distribution = new std::normal_distribution<double>(avg,std);
}

double GaussianRandomVariable::value() {
  return (*distribution)(generator);
}
