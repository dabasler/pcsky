
#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <tiffio.h>

#include "miniply.h"

#include "io.h"
#include "hemisphere.h"



int main(int argc, char ** argv) {
int i,j;
if (argc==1){
printf("run as tests [argumnet] with one of the following atguments:\n");
printf("-all\t\n");
printf("-sector \n\ttest sector definition. parameters  [dim] [n]\n");
printf("-annulus\n\ttest annulus definition. parameters [dim] [n]\n");
return 0;
}

if (!strcmp(argv[1],"-sector") || !strcmp(argv[1],"-all")){
	 int i ,j;
	// TESTING SectorArray
	  int dim =512;
	  int n =12;
	  uint8_t * sectorArray;
	  sectorArray = new uint8_t[dim * dim];
	  defineSectors(sectorArray,n,dim);
	  for (i=0;i<dim*dim;i++)sectorArray[i]=sectorArray[i]*10;
	  tiffout((uint8_t * ) sectorArray, (uint8_t * )sectorArray, (uint8_t * ) sectorArray, dim, "sectors.tif");
}
if (!strcmp(argv[1],"-annulus") || !strcmp(argv[1],"-all")){

	// TESTING annuliArray
	  int dim =512;
	  int n =36;
	  uint8_t * annuliArray;
	  annuliArray = new uint8_t[dim * dim];
	  defineAnnuli(annuliArray,n,dim);
	  for (i=0;i<dim*dim;i++)annuliArray[i]=annuliArray[i]*5;
	  tiffout((uint8_t * ) annuliArray, (uint8_t * ) annuliArray, (uint8_t * ) annuliArray, dim, "annuli.tif");
}


if (!strcmp(argv[1],"-sectorannulus") || !strcmp(argv[1],"-all")){
	 int i ,j;
	// TESTING SectorArray
	  int dim =512;
	  int ns =12;
	  uint8_t * sectorArray;
	  sectorArray = new uint8_t[dim * dim];
	  defineSectors(sectorArray,ns,dim);
	  int na =36;
	  uint8_t * annuliArray;
	  annuliArray = new uint8_t[dim * dim];
	  defineAnnuli(annuliArray,na,dim);
	  for (i=0;i<dim*dim;i++) sectorArray[i]=sectorArray[i]*10;
	  for (i=0;i<dim*dim;i++)annuliArray[i]=annuliArray[i]*5;
	  tiffout((uint8_t * ) annuliArray, (uint8_t * )sectorArray, (uint8_t * ) sectorArray, dim, "sectorsannulus.tif");
}





return 0;
}



	/*Test SFV
	printf ("SVF = %f \n ", skyViewFactor(binArray,dim ,36));
	for (j = 0; j < dim; j++)for (i = 0; i < dim; i++) if (i>dim/2) binArray[j * dim + i] = 0;
	tiffout((uint8_t * ) binArray, (uint8_t * ) binArray, (uint8_t * ) binArray, dim, "TEST.tif");
	printf ("SVF = %f \n ", skyViewFactor(binArray,dim ,36));
	for (j = 0; j < dim; j++)for (i = 0; i < dim; i++) if (j>dim/2) binArray[j * dim + i] = 0;
	tiffout((uint8_t * ) binArray, (uint8_t * ) binArray, (uint8_t * ) binArray, dim, "TEST2.tif");
	printf ("SVF = %f \n", skyViewFactor(binArray,dim ,36));
	return;
	*/

