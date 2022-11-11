#include "maskApi.h"
using namespace std;
MaskApi::MaskApi(){

}

MaskApi::~MaskApi(){

}

void MaskApi::bbIou( BB dt, BB gt, siz m, siz n, byte2 *iscrowd, double *o ){
  double h, w, i, u, ga, da; siz g, d; int crowd;
  for( g=0; g<n; g++ ) {
    BB G=gt+g*4; ga=G[2]*G[3]; crowd=iscrowd!=NULL && iscrowd[g];
    for( d=0; d<m; d++ ) {
      BB D=dt+d*4; da=D[2]*D[3]; o[g*m+d]=0;
      w=fmin(D[2]+D[0],G[2]+G[0])-fmax(D[0],G[0]); if(w<=0) continue;
      h=fmin(D[3]+D[1],G[3]+G[1])-fmax(D[1],G[1]); if(h<=0) continue;
      i=w*h; u = crowd ? da : da+ga-i; o[g*m+d]=i/u;
    }
  }
}