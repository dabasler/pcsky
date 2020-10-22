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
  char * buffer = new char[256];
  // initialize output data arrays
  uint8_t * binArray; // Binary Image (0/255)
  binArray = new uint8_t[dim * dim];
  float * countArray;
  countArray = new float[dim * dim];
  float * minDistanceArray;
  minDistanceArray = new float[dim * dim];
  uint8_t outR[dim][dim];
  uint8_t outG[dim][dim];
  uint8_t outB[dim][dim];
  float * areaArray;
  areaArray = new float[dim * dim];
  float pixArea;
  float pointArea=0.01;

  for (j = 0; j < dim; j++)
    for (i = 0; i < dim; i++) {
      countArray[j * dim + i] = 0;
      minDistanceArray[j * dim + i] = NAN;
      if (((i - rmax) * (i - rmax) + (j - rmax) * (j - rmax)) <= (rmax * rmax)) {
        binArray[j * dim + i] = 255;
		areaArray[j * dim + i] = 0;
        outR[i][j] = 230;
        outG[i][j] = 230;
        outB[i][j] = 240;
      } else {
        countArray[j * dim + i] = NAN;
		areaArray[j * dim + i] = NAN;
        binArray[j * dim + i] = 0;
        outR[i][j] = 0;
        outG[i][j] = 0;
        outB[i][j] = 0;
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
          countArray[px * dim + py] = countArray[px * dim + py] + 1;
	  pixArea= 2*tan(M_PI/(4*rmax))*dxyz; // pointsize/(Pixelsize at distance)
	  pixArea=pixArea*pixArea;
	  areaArray[px * dim + py] = areaArray[px * dim + py] + pointArea/pixArea;
          if (dxyz < minDistanceArray[px * dim + py] || std::isnan(minDistanceArray[px * dim + py])) {
            minDistanceArray[px * dim + py] = dxyz;
            binArray[px * dim + py] = 0;
            if (strchr(modestring, 'v') && hascolor) { // only create the color image when color is actually present and visual output active
              outR[px][py] = col[i];
              outG[px][py] = col[i + 1];
              outB[px][py] = col[i + 2];
            }
          }
        }
      }

      printf ("SVF = %f \n", skyViewFactor(binArray,dim ,36));

     // WRITE OUTPUT IMAGES
      if (strchr(modestring, 'v') && hascolor) {
        strcpy(buffer, outfilepattern);
        strcat(buffer, "_vis.tif");
        tiffout((uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB, dim, buffer);
      }
	  
      if (strchr(modestring, 'b')) {
        strcpy(buffer, outfilepattern);
        strcat(buffer, "_bin.tif");
        tiffout((uint8_t * ) binArray, (uint8_t * ) binArray, (uint8_t * ) binArray, dim, buffer);
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
        strcpy(buffer, outfilepattern);
        strcat(buffer, "_areabin.tif");
        tiffout((uint8_t * ) binArray, (uint8_t * ) binArray, (uint8_t * ) binArray, dim, buffer);
      }

      if (strchr(modestring, 'd')) {
        colorizeArray(minDistanceArray, dim, 0.5, 50, (uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB);
        strcpy(buffer, outfilepattern);
        strcat(buffer, "_dist.tif");
        tiffout((uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB, dim, buffer);
      }

      if (strchr(modestring, 'c')) {
        colorizeArray(countArray, dim, 1, 500, (uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB);
        strcpy(buffer, outfilepattern);
        strcat(buffer, "_count.tif");
        tiffout((uint8_t * ) outR, (uint8_t * ) outG, (uint8_t * ) outB, dim, buffer);
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
  char * buffer = new char[64];
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
      strcpy(buffer, argv[i]);
      buffer[2] = '\0';
      if (!strcmp(buffer, "r=")) {
        strcpy(buffer, argv[i] + 2); // RADIUS
        radius = atoi(buffer);
        reqparameter[1] = true;
      } else if (!strcmp(buffer, "c=")) {
        strcpy(buffer, argv[i] + 2); // CAMERA
        j = 0;
        token = strtok(buffer, ",");
        while (token != NULL && j < 4) {
          cameraPositon[j] = atof(token);
          token = strtok(NULL, ",");
          j++;
        }
        reqparameter[2] = true;
      }
      /*	else if (!strcmp(buffer,"--")) {
      			strcpy(buffer,argv[i]+1);// mode
      			if (!strcmp(buffer,"all"))  strcat(modestring,"vbdc");
      			if (!strcmp(buffer,"vis"))  strcat(modestring,"v");
      			if (!strcmp(buffer,"bin"))  strcat(modestring,"b");
      			if (!strcmp(buffer,"dist")) strcat(modestring,"d");
      			if (!strcmp(buffer,"bin"))  strcat(modestring,"c");
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
  printf("output: %s*.tif\n", outfilenameBuffer);
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
