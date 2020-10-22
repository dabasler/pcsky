#ifndef HEMISPHERE_H_
#define HEMISPHERE_H_

#include <cmath>

double getAzimuth(double dx, double dy);
void getPointHemiXY(float * pcp, float * cpos, int rmax, int * px, int * py, double * dxyz);

void defineAnnuli(uint8_t * annuliArray, int n,int dim);
void defineSectors(uint8_t * sectorArray, int n,int dim);

float skyFraction(uint8_t * dataArray ,uint8_t * annuliArray,int dim, int code);
float skyViewFactor(uint8_t * binArray, int dim ,int n);

#endif

