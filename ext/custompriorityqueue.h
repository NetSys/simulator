/*
 * custompriorityqueue.h
 *
 *  Created on: Apr 28, 2015
 *      Author: peter
 */

#ifndef CUSTOMPRIORITYQUEUE_H_
#define CUSTOMPRIORITYQUEUE_H_

#include <vector>
#include "assert.h"

template<typename _Tp, typename _Sequence, typename _Compare>
class CustomPriorityQueue
{
private:
    int best_index;

public:
    _Sequence v;
    _Compare comp;


    CustomPriorityQueue()
    {
        comp = _Compare();
        v = _Sequence();
        best_index = -1;
    }

    bool empty() const
    {
        return v.empty();
    }
    int size() const {
        return v.size();
    }
    void push(const _Tp& x)
    {
        assert(x);
        v.push_back(x);
        best_index = -1;
    }

    void pop()
    {
        assert(v.size() > 0);
        if(best_index < 0)
            this->top();
        if(best_index >= 0){
            v.erase(v.begin() + best_index);
            best_index = -1;
        }
    }

    _Tp top()
    {
        assert(v.size() > 0);
        _Tp best = *v.begin();
        for(int i = 0; i < v.size(); i++)
        {
            if(!comp(v[i],best)){
                best = v[i];
                best_index = i;
            }
        }
        return best;
    }
};


#endif /* CUSTOMPRIORITYQUEUE_H_ */
