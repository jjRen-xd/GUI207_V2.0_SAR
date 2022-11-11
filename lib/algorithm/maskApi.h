#ifndef MASKAPI_H
#define MASKAPI_H

#include <cmath>

typedef unsigned long siz;
typedef unsigned char byte2;
typedef double* BB;
typedef unsigned int uint;


class MaskApi{
public:
    MaskApi();
    ~MaskApi();
    void bbIou( BB dt, BB gt, siz m, siz n, byte2 *iscrowd, double *o );
};

#endif
