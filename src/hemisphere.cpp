#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include "hemisphere.h"

/* ---------------------------------------------------------------------------------   */
/* functions to transform 3D point location to pixel coordinate on hemispherical image */
double getAzimuth(double dx, double dy) {
  double a;
  if ((dx > 0 && dy > 0) || (dx < 0 && dy > 0))
    a = atan(dx / dy);
  else if (dx < 0 && dy < 0)
    a = -(M_PI - atan(dx / dy)); // III quadrant
  else if (dx > 0 && dy < 0)
    a = M_PI + atan(dx / dy); // IV quadrant
  else if (dx == 0 && dy < 0)
    a = -M_PI;
  else if (dx > 0 && dy == 0)
    a = M_PI / 2;
  else if (dx < 0 && dy == 0)
    a = -M_PI / 2;
  else
    a = 0;
  return (a);
}

void getPointHemiXY(float * pcp, float * cpos, int rmax, int * px, int * py, double * dxyz) {
  double dx = pcp[0] - cpos[0];
  double dy = pcp[1] - cpos[1];
  double dz = pcp[2] - cpos[2];
  double dxy = sqrt(dx * dx + dy * dy);
  * dxyz = sqrt(dxy * dxy + dz * dz);
  double azimuth = getAzimuth(dx, dy);
  double zenith = atan(dxy / dz);
  double pr = sin(zenith) * rmax;

  * px = (int)(sin(azimuth) * pr + rmax);
  * py = (int)(-cos(azimuth) * pr + rmax); // invert since image topleft pixel is 0,0
  //printf("Point X%f Y%f Z%f\n dx=%f dy=%f dz=%f\ndxzy=%f\n a=%f  z=%f\n pr=%f\n px=%i py=%i\n\n",pcp[0], pcp[1], pcp[2], dx, dy, dz, * dxyz, azimuth, zenith, pr, * px, * py);
}

/* --------------------------- */
/*  Calculate SKY VIEW FACTOR  */

void defineAnnuli(uint8_t * annuliArray, int n,int dim){
// Generate Annuli form pixel data
int rmax=dim/2;
int i,j,a;
float ra;
// Codify annuli (code 1 to n define annuli)
for (a=n;a>0;a--){  	// not the most efficient code but it works
    ra = sin(a* (M_PI/2)/n) * rmax;
    //printf("a=%i pv=%i za=%f ra=%f\n",a,n-a+1, a*(90.0)/n ,ra);
    for (j = 0; j < dim; j++)
      for (i = 0; i < dim; i++)  if (((i - rmax) * (i - rmax) + (j - rmax) * (j - rmax)) <= (ra * ra)) annuliArray[j * dim + i] = n-a+1;//a;
}
// Clear outside (code 0)
for (j = 0; j < dim; j++)
      for (i = 0; i < dim; i++) 
		if (((i - rmax) * (i - rmax) + (j - rmax) * (j - rmax)) > (rmax * rmax)) annuliArray[j * dim + i] = 0;
}


void defineSectors(uint8_t * sectorArray, int n, int dim) {
  // Generate sectors from pixel data
  int i, j;
  int rmax = dim / 2;
  float sectorAngle = (2 * M_PI) / n;
  float azimuth;
  for (j = 0; j < dim; j++)
    for (i = 0; i < dim; i++)
      if (((i - rmax) * (i - rmax) + (j - rmax) * (j - rmax)) > (rmax * rmax)) sectorArray[j * dim + i] = 0;
      else {
        azimuth = getAzimuth((double)(j - rmax), (double) - (i - rmax));
        if (azimuth < 0) azimuth = (M_PI * 2) + azimuth;
        sectorArray[j * dim + i] = (int)(azimuth / sectorAngle) + 1;
      }

}

float skyFraction(uint8_t * dataArray ,uint8_t * annuliArray,int dim, int code){
	// transmission per annulus
	int nsky=0;
	int ntot=0;
	float SF;
	for (int i=0;i<dim*dim;i++) {
		if (annuliArray[i]==code){ 
			ntot++;
			if (dataArray[i] > 0) nsky++; // in Binary array, sky is set 255
		}
	}
	if (nsky==0) SF = 0;
	else SF =(float) nsky/ (float) ntot;
	//printf("annulus %i SF=%f\n",code,SF);
	return (SF);
}

float skyViewFactor(uint8_t * binArray, int dim ,int n){
	// Chapman, Thornes and Bradley (2001)
	//sum over each annulus: sm+= sin(M_PI *((2*i-1)/(2*n))*(nsky/ntot))
	//SVF= M_PI/(2*n) * sm
	uint8_t * annuliArray;
	annuliArray = new uint8_t[dim * dim];
        defineAnnuli(annuliArray,n,dim);
	float sm=0;
	for (int i=1;i<=n;i++) sm += sin(M_PI *((2*i-1)/(2*(float)n))) * skyFraction(binArray,annuliArray,dim, i);
	return (M_PI/(2*(float)n) * sm);
}
	
/* TO DO: ADD LAI calculation */
	
	
// Transmission (sky fraction) of each ring/sector segment
void transmission(uint8_t * dataArray, int dim, uint8_t * annuliArray, int nannuli, uint8_t * sectorArray, int nsectors, float * transmission) {
  int i;
  float countSky[nannuli * nsectors];
  float countTot[nannuli * nsectors];
  for (i = 0; i < nannuli * nsectors; i++) {
    countSky[i] = 0;
    countTot[i] = 0;
  }
  for (i = 0; i < dim * dim; i++) {
    if (annuliArray[i] > 0 && sectorArray[i] > 0) {
      countTot[(annuliArray[i] - 1) * nsectors + (sectorArray[i] - 1)]++;
      if (dataArray[i] > 0) countSky[(annuliArray[i] - 1) * nsectors + (sectorArray[i] - 1)]++;
    }
  }

  for (i = 0; i < nannuli * nsectors; i++) {
    if (countSky[i] == 0) transmission[i] = 0;
    else transmission[i] = (float) countSky[i] / (float) countTot[i];
  }
}
