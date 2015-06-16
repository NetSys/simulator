#ifndef STATS_H
#define STATS_H

#include <map>
#include <vector>
class Stats
{
private:
    double sum;
    double sq_sum;
    int count;
    bool get_dist;
    std::map<double, int> distribution;
    int dist_round_precision;
    std::vector<double> raw;

public:
    Stats(bool get_dist = false);
    void input_data(double data);
    void input_data(int);
    void operator+=(const double &data);
    void operator+=(const int &data);
    double avg();
    double size();
    double total();
    double sd();
    void set_precision();
    double get_percentile(double p);
};


#endif
