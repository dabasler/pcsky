#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <tiffio.h>

#include "miniply.h"

#include "io.h"

/* -----------------------------------------------------------------   */
/* Rasterize pointcloud by flattening one dimension */
void rasterize_pointcloud(const char * filename, float * bbox, double pixsize,  const char * outfilepattern,    const char * modestring) {
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
  
  //bbox Xmin,  Xmax,  Ymin,  Ymax, Zmin, Zmax
  //       0      1     2      3     4     5
  int dim[2];
  dim[0] = (int) ceil((bbox[1]-bbox[0])/pixsize);
  dim[1] = (int) ceil((bbox[3]-bbox[2])/pixsize);
  printf("Output: %i x %i pixels\n",dim[0],dim[1]);
  if (dim[0]>5000 ||dim[1]>5000 ){
    printf("TOO BIG\n");
    return;
  }
  bool hascolor = false;
  char * fname = new char[256];
  int ix,iy,iz;
  // initialize output data arrays
  // counts,dem and colors
  uint16_t * countArray;
  countArray = new uint16_t[dim[0] * dim[1]];
  float * elevationArray;
  elevationArray = new float[dim[1] * dim[0]];
  uint8_t * outR;
  outR= new   uint8_t [dim[1]*dim[0]];
  uint8_t * outG;
  outG= new   uint8_t [dim[1]*dim[0]];
  uint8_t * outB;
  outB= new   uint8_t [dim[1]*dim[0]];

  for (i = 0; i < dim[1]*dim[0]; i++) { 
    countArray[i] = 0;
    elevationArray[i] = NAN;
    outR[i] = 0;
    outG[i] = 0;
    outB[i] = 0;
  }

  // READ POINT CLOUD 
  if (reader.has_element()) {
    // Get POINTS
    if (reader.element_is(miniply::kPLYVertexElement) && reader.load_element() && reader.find_pos(propIdxs)) {
      numVerts = reader.num_rows();
      printf("num vert.: %i\n", numVerts);
      pos = new float[numVerts * 3];
      reader.extract_properties(propIdxs, 3, miniply::PLYPropertyType::Float, pos);   

      //Get color //reader.find_color(propIdxs) // looks for r g b
      if (reader.find_properties(propIdxs, 3, "red", "green", "blue")) {
        col = new uint8_t[numVerts * 3];
        reader.extract_properties(propIdxs, 3, miniply::PLYPropertyType::UChar, col);
        hascolor = true;
      }
      //getExtent(pos,numVerts);
      // Place point to pixel arrat
      for (i = 0; i < numVerts * 3; i += 3) {
	ix=i;
	iy=i+1;
	iz=i+2;
	if (pos[ix] >= bbox[0] && pos[ix] <= bbox[1] && pos[iy] >= bbox[2] && pos[iy] <= bbox[3]  &&  pos[iz] >= bbox[4] && pos[iz] <= bbox[5]) {   
	  px = (int) ((pos[ix]-bbox[0])/ pixsize);
	  py = (int) -((pos[iy]-bbox[3])/ pixsize); // images are filled from top
          countArray[py * dim[0] + px] = countArray[py * dim[0] + px] + 1;

           //printf("%0.1f/%0.1f->%f/%f ->%i/ %i: %i-%i-%i \n",pos[ix],pos[iy],pos[ix]-bbox[0],pos[iy]-bbox[2],px, py,col[i],col[i+1],col[i+2]);
           if (elevationArray[py * dim[0] + px]< pos[iz] || std::isnan(elevationArray[py * dim[0] + px])) { // Get highest point

	     elevationArray[py * dim[0] + px]=pos[iz];
          if (strchr(modestring, 'v') && hascolor) { // only create the color image when color is actually present and visual output active
              outR[py * dim[0] + px] = col[i];
              outG[py * dim[0] + px] = col[i + 1];
              outB[py * dim[0] + px] = col[i + 2];	  
           }


          }
        }
      }

	char* commentfname=new char [256];
	sprintf(commentfname,"XDIM %f\nYDIM %f\nULXMAP %f\nULYMAP %f\n# Data created from '%s' by pcsky software https://github.com/dabasler/pcsky\n",pixsize,pixsize,bbox[0],bbox[3],filename);

printf("VIS");
     // WRITE OUTPUT IMAGES
     // if (strchr(modestring, 'v') && hascolor) {
	sprintf(fname,"%s_vis",outfilepattern);
	printf("Output: %i x %i pixels\n",dim[0],dim[1]);
        tiffout(outR, outG, outB, dim[0], dim[1] , fname);
	//printf("->%s\n",fname);
      //}

printf("DEM");
      if (strchr(modestring, 'd')) {
        colorizeArray_float(elevationArray, dim[0],dim[1], 400,600 ,false, outR, outG, outB);
	sprintf(fname,"%s_dem",outfilepattern);
        tiffout(outR, outG, outB, dim[0], dim[1] , fname);
      }

printf("COUNT");
      if (strchr(modestring, 'c')) {
	sprintf(fname,"%s_count",outfilepattern);
	writeBinaryRaster((void *) countArray,fname, dim[0],dim[1],1,"U16", "BSQ",commentfname);
        colorizeArray_uint16(countArray, dim[0],dim[1], 0,1000, 0 ,false, outR,outG,outB);
        tiffout(outR, outG, outB, dim[0], dim[1] , fname);
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


// TIFF TEST
int width = 400;
int height= 200;
uint8_t * Array;
Array = new uint8_t[width * height];
int cv=0;
for (int i=0;i<width * height;i++){
	Array [i]=cv;
	cv++;
	if (cv>=100) cv=0;
}
for (int x=0;x<width;x++){for (int y=0;y<height;y++) printf("%03i ", Array[x*height+y]); printf("\n\n\n");}
tiffout(Array, Array, Array,width,height, "TEST.tif");
return 0;
*/
int main(int argc, char ** argv) {
/*
// TIFF TEST
int width = 255;
int height= 500;
uint8_t * Array;
Array = new uint8_t[width * height];
int cv=0;
for (int i=0;i<width * height;i++){
	Array [i]=cv;
	cv++;
	if (cv>=255) cv=0;
}
//for (int y=0;y<height;y++){ for (int x=0;x<width;x++)printf("%03i ", Array[y*width+x]); printf("\n\n\n");}
tiffout(Array, Array, Array,width,height, "TEST.tif");
return 0;
*/


  // Parse command line arguments
  if (argc < 4) {
    printf("\n Missing commandline arguments.\n use %s [pointcloud.ply] b=0,0,0,0,0,0 c=10 m=[mode] [outputfilepattern]\n", argv[0]);
    return (0);
  }
  int i, j;
  char * infilenamefname = new char[256];
  char * outfilenamefname = new char[256];
  char * modestring = new char[20];
  char * fname = new char[64];
  float bbox[6];
  float csize;
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
      strcpy(infilenamefname, argv[i]);
      reqparameter[0] = true;
    } else {
      strcpy(fname, argv[i]);
      fname[2] = '\0';
      if (!strcmp(fname, "c=")) {
        strcpy(fname, argv[i] + 2); // RADIUS
        csize = atof(fname);
        reqparameter[1] = true;
      } else if (!strcmp(fname, "b=")) {
        strcpy(fname, argv[i] + 2); // CAMERA
        j = 0;
        token = strtok(fname, ",");
        while (token != NULL && j < 7) {
          bbox[j] = atof(token);
          token = strtok(NULL, ",");
          j++;
        }
        reqparameter[2] = true;
      }
      /*	else if (!strcmp(fname,"--")) {
      			strcpy(fname,argv[i]+1);// mode
      			if (!strcmp(fname,"all"))  strcat(modestring,"vbdc");
      			if (!strcmp(fname,"vis"))  strcat(modestring,"v");
      			if (!strcmp(fname,"bin"))  strcat(modestring,"b");
      			if (!strcmp(fname,"dist")) strcat(modestring,"d");
      			if (!strcmp(fname,"bin"))  strcat(modestring,"c");
      		} */
      else {
        strcpy(outfilenamefname, argv[i]);
        reqparameter[3] = true;
      }
    }
  }
  printf("\n**********************\n Rasterize Point Cloud \n**********************\n\n");
  printf("PARAMETERS\n");
  printf("input:  %s\n", infilenamefname);
  printf("output: %s*.tif\n", outfilenamefname);
  printf("bbox:  X%f %f Y%f %f Z%f %f\n", bbox[0],bbox[1],bbox[2],bbox[3],bbox[4],bbox[5]);
  printf("cellsize: %f\n", csize);

  if (!reqparameter[0]) {
    fprintf(stderr, "No input files provided.\n");
    return EXIT_SUCCESS;
  } else if ((reqparameter[0] + reqparameter[1] + reqparameter[2] + reqparameter[3]) == 4) {
    printf("**********************\n\n");
    printf("FILE:%s\n", infilenamefname);
    print_ply_header(infilenamefname);
    printf("**********************\n\n");
    rasterize_pointcloud(infilenamefname, bbox, csize, outfilenamefname, modestring);
    return 1;
  }
}
