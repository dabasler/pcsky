#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <tiffio.h>

#include "miniply.h"

#include "io.h"
#include "hemisphere.h"



/* -----------------------------------------------------------------   */
/* Actual transfomation routine: Points to Hemispherical data arrays   */
void hemispherical_transform(const char * filename, float * cameraPositon, int rmax,
  const char * outfilepattern,
    const char * modestring) {
  printf("Extracting points\n\n");
  miniply::PLYReader reader(filename);
  if (!reader.valid()) {
    printf("ERROR could not open file %s", filename);
    return;
  }
  //VARIABLES
  int i, j;
  uint32_t propIdxs[3];
  uint32_t numVerts;
  float * pos;
  uint8_t * col;
  int px, py;
  double dxyz;
  short dim = 2 * rmax;
  bool hascolor = false;
  char * fname = new char[256];
  // initialize output data arrays
  uint8_t * binArray; // Binary Image (0/255)
  binArray = new uint8_t[dim * dim];
  uint16_t * countArray;
  countArray = new uint16_t[dim * dim];
  float * minDistanceArray;
  minDistanceArray = new float[dim * dim];
  uint8_t outR[dim][dim];
  uint8_t outG[dim][dim];
  uint8_t outB[dim][dim];
  float * areaArray;
  areaArray = new float[dim * dim];
  float pixArea;
  float pointArea=0.01;

  for (j = 0; j < dim; j++) // y
    for (i = 0; i < dim; i++) { //x
      countArray[j * dim + i] = 0;
      minDistanceArray[j * dim + i] = NAN;
      if (((i - rmax) * (i - rmax) + (j - rmax) * (j - rmax)) <= (rmax * rmax)) {
        binArray[j * dim + i] = 255;
	areaArray[j * dim + i] = 0;
        outR[j][i] = 230;
        outG[j][i] = 230;
        outB[j][i] = 240;
      } else {
	areaArray[j * dim + i] = NAN;
        binArray[j * dim + i] = 0;
        outR[j][i] = 0;
        outG[j][i] = 0;
        outB[j][i] = 0;
      }
    }

  // READ POINT CLOUD 
  if (reader.has_element()) {
    // Get POINTS
    if (reader.element_is(miniply::kPLYVertexElement) && reader.load_element() && reader.find_pos(propIdxs)) {
      numVerts = reader.num_rows();
      printf("num vert.: %i\n", numVerts);
      pos = new float[numVerts * 3];
      reader.extract_properties(propIdxs, 3, miniply::PLYPropertyType::Float, pos);
      
      getExtent(pos,numVerts);
      //int l=0;
      //for (i=0;i<numVerts*3;i+=3) if (!pos[i]==0 && !pos[i+1]==0 && !pos[i+2]==0) l++; //printf("%f %f %f\n",pos[i],pos[i+1],pos[i+2]);
      //printf("NON ZERO: %i\n", l);

      //Get color //reader.find_color(propIdxs) // looks for r g b
      if (reader.find_properties(propIdxs, 3, "red", "green", "blue")) {
        col = new uint8_t[numVerts * 3];
        reader.extract_properties(propIdxs, 3, miniply::PLYPropertyType::UChar, col);
        hascolor = true;
      }
      // EXTRACT hemisherical point location for current cameraPositon
      for (i = 0; i < numVerts * 3; i += 3) {
        if (pos[i + 2] >= cameraPositon[2]) { // Keep only points above camera
          getPointHemiXY( & pos[i], cameraPositon, rmax, & px, & py, & dxyz);
          countArray[py * dim + px] = countArray[py * dim + px] + 1;
	  pixArea= 2*tan(M_PI/(4*rmax))*dxyz; // pointsize/(Pixelsize at distance)
	  pixArea=pixArea*pixArea;
	  areaArray[py * dim + px] = areaArray[py * dim + px] + pointArea/pixArea;
          if (dxyz < minDistanceArray[py * dim + px] || std::isnan(minDistanceArray[py * dim + px])) {
            minDistanceArray[py * dim + px] = dxyz;
            binArray[py * dim + px] = 0;
            if (strchr(modestring, 'v') && hascolor) { // only create the color image when color is actually present and visual output active
              outR[py][px] = col[i];
              outG[py][px] = col[i + 1];
              outB[py][px] = col[i + 2];
            }
          }
        }
      }

      printf ("SVF = %f \n", skyViewFactor(binArray,dim ,36));

     // WRITE OUTPUT IMAGES
      if (strchr(modestring, 'v') && hascolor) {
	sprintf(fname,"%s_vis",outfilepattern);
        tiffout((uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB, dim, dim, fname);
      }
	  
      if (strchr(modestring, 'b')) {
	sprintf(fname,"%s_bin",outfilepattern);
        tiffout((uint8_t * ) binArray, (uint8_t * ) binArray, (uint8_t * ) binArray, dim, dim, fname);
	writeBinaryRaster((void *) binArray, fname, dim,dim,1,"U8", "BSQ","# Data created by pcsky software https://github.com/dabasler/pcsky");
      }

      if (strchr(modestring, 'a')) { // Reusing binArray for area_bin output
	    for ( i=0;i<dim*dim;i++) {
			if (std::isnan(areaArray[i])){
				binArray[i]= 0;
			}else{
				if (areaArray[i]>0.5) binArray[i]= 0;
				else binArray[i]= 255;
			}
		}
	sprintf(fname,"%s_areabin",outfilepattern);
        tiffout((uint8_t * ) binArray, (uint8_t * ) binArray, (uint8_t * ) binArray, dim, dim, fname);
      }

      if (strchr(modestring, 'd')) {
	sprintf(fname,"%s_dist",outfilepattern);
	writeBinaryRaster((void *) minDistanceArray, fname, dim,dim,1,"F32", "BSQ","# Data created by pcsky software https://github.com/dabasler/pcsky");
        colorizeArray_float(minDistanceArray, dim, dim, 0, 60, true, (uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB);
        tiffout((uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB, dim, dim, fname);
      }

      if (strchr(modestring, 'c')) {
	sprintf(fname,"%s_count",outfilepattern);
        colorizeArray_uint16(countArray, dim, dim, 0,500,0,false, (uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB);
        tiffout((uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB, dim, dim, fname);
      }

    }
  }
}



/* -------------------------------------------------------------   */
/* Main function, parsing commandline arguments and checking input
ToDo:
--quiet flag, ouny output data
--info flag print header and extent only
--mode: limit output to certain images/data only 
--limits specify custom limits for minmax visulization
*/
int main(int argc, char ** argv) {

  // Parse command line arguments
  if (argc < 4) {
    printf("\n Missing commandline arguments.\n use %s [pointcloud.ply] c=0,0,0 r=512 m=[mode] [outputfilepattern]\n", argv[0]);
    return (0);
  }
  int i, j;
  char * infilenameBuffer = new char[256];
  char * outfilenameBuffer = new char[256];
  char * modestring = new char[20];
  char * stringbuffer = new char[64];
  float cameraPositon[3];
  int radius;
  char * token;
  bool reqparameter[4] = {
    false,
    false,
    false,
    false
  };
  strcpy(modestring, "vbdca");
  for (i = 0; i < argc; i++) {
    if (has_extension(argv[i], "ply")) {
      strcpy(infilenameBuffer, argv[i]);
      reqparameter[0] = true;
    } else {
      strcpy(stringbuffer, argv[i]);
      stringbuffer[2] = '\0';
      if (!strcmp(stringbuffer, "r=")) {
        strcpy(stringbuffer, argv[i] + 2); // RADIUS
        radius = atoi(stringbuffer);
        reqparameter[1] = true;
      } else if (!strcmp(stringbuffer, "c=")) {
        strcpy(stringbuffer, argv[i] + 2); // CAMERA
        j = 0;
        token = strtok(stringbuffer, ",");
        while (token != NULL && j < 4) {
          cameraPositon[j] = atof(token);
          token = strtok(NULL, ",");
          j++;
        }
        reqparameter[2] = true;
      }
      /*	else if (!strcmp(stringbuffer,"--")) {
      			strcpy(stringbuffer,argv[i]+1);// mode
      			if (!strcmp(stringbuffer,"all"))  strcat(modestring,"vbdc");
      			if (!strcmp(stringbuffer,"vis"))  strcat(modestring,"v");
      			if (!strcmp(stringbuffer,"bin"))  strcat(modestring,"b");
      			if (!strcmp(stringbuffer,"dist")) strcat(modestring,"d");
      			if (!strcmp(stringbuffer,"bin"))  strcat(modestring,"c");
      		} */
      else {
        strcpy(outfilenameBuffer, argv[i]);
        reqparameter[3] = true;
      }
    }
  }
  printf("\n**********************\n SVF from UAV Point Cloud \n**********************\n\n");
  printf("PARAMETERS\n");
  printf("input:  %s\n", infilenameBuffer);
  printf("output: %s_\n", outfilenameBuffer);
  printf("camera:  %f %f %f\n", cameraPositon[0], cameraPositon[1], cameraPositon[2]);
  printf("radius: %i\n", radius);

  if (!reqparameter[0]) {
    fprintf(stderr, "No input files provided.\n");
    return EXIT_SUCCESS;
  } else if ((reqparameter[0] + reqparameter[1] + reqparameter[2] + reqparameter[3]) == 4) {
    printf("**********************\n\n");
    printf("FILE:%s\n", infilenameBuffer);
    print_ply_header(infilenameBuffer);
    printf("**********************\n\n");
    hemispherical_transform(infilenameBuffer, cameraPositon, radius, outfilenameBuffer, modestring);
    return 1;
  }
}
