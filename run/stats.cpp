#include "stats.h"
#include <cmath>
#include <algorithm>
#include "assert.h"

Stats::Stats(bool get_dist)
{
    this->sum = 0;
    this->sq_sum = 0;
    this->count = 0;
    this->get_dist = get_dist;
}


void Stats::input_data(double data){
    sum += data;
    sq_sum += data * data;
    count++;
    raw.push_back(data);
}

void Stats::input_data(int data){
    input_data((double)data);
}

void Stats::operator+=(const double &data){
    input_data(data);
}
void Stats::operator+=(const int &data)
{
    input_data(data);
}


double Stats::avg(){
    return sum/count;
}

double Stats::size(){
    return count;
}

double Stats::sd(){
    return sqrt(sq_sum/count - avg() * avg());
}

double Stats::total(){
    return sum;
}

void Stats::set_precision()
{

}

double Stats::get_percentile(double p){
    if(raw.size() == 0)
        return -1;
    std::sort(raw.begin(), raw.end());
    int loc = (int)(p*raw.size());
    assert(loc >= 0 && loc < raw.size());
    return raw[loc];
}

