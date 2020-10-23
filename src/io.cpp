#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <iostream>
#include <fstream>
#include <tiffio.h>
#include "miniply.h"
#include "io.h"


/* -------------------------------------------------------------------------------- */
/* Import pointcloud from PLY */
// the following 4 functions are borrowed from the miniply example (c) 2019 Vilya Harvey https://github.com/vilya/miniply

bool print_ply_header(const char * filename) {
  miniply::PLYReader reader(filename);
  if (!reader.valid()) {
    fprintf(stderr, "Failed to open %s\n", filename);
    return false;
  }
  printf("format %s %d.%d\n", kFileTypes[int(reader.file_type())], reader.version_major(), reader.version_minor());
  for (uint32_t i = 0, endI = reader.num_elements(); i < endI; i++) {
    const miniply::PLYElement * elem = reader.get_element(i);
    printf("element %s %u\n", elem -> name.c_str(), elem -> count);
    for (const miniply::PLYProperty & prop: elem -> properties) {
      if (prop.countType != miniply::PLYPropertyType::None) {
        printf("property list %s %s %s\n", kPropertyTypes[uint32_t(prop.countType)], kPropertyTypes[uint32_t(prop.type)], prop.name.c_str());
      } else {
        printf("property %s %s\n", kPropertyTypes[uint32_t(prop.type)], prop.name.c_str());
      }
    }
  }
  printf("end_header\n");
  return true;
}


void getExtent(float * pos,uint32_t numVerts){
  double mn[3]={1E+37,1E+37,1E+37};
  double mx[3]={0,0,0};
  for (int i=0;i<numVerts*3;i+=3) { 
    if (pos[i]   < mn[0]) mn[0]= pos[i];
    if (pos[i+1] < mn[1]) mn[1]= pos[i+1];
    if (pos[i+2] < mn[2]) mn[2]= pos[i+2];
    if (mx[0] < pos[i])   mx[0]= pos[i];
    if (mx[1] < pos[i+1]) mx[1]= pos[i+1];
    if (mx[2] < pos[i+2]) mx[2]= pos[i+2];
  }
  printf("minX: %f \tmaxX: %f\n",mn[0],mx[0]);
  printf("minY: %f \tmaxY: %f\n",mn[1],mx[1]);
  printf("minZ: %f \tmaxZ: %f\n",mn[2],mx[2]);
}


/* -------------------------------------------------------------------------------- */
/* output to tiff image */
int tiffout(uint8_t * Rband, uint8_t * Gband, uint8_t * Bband, uint32_t width ,uint32_t height,
  const char * filename) {

  char * newfilename = new char [256];
  strcpy(newfilename, filename);
  strcat(newfilename, ".tif");
  //printf("%s", filename);
  uint32_t tileWidth = 16, tileHeight = 16;
  TIFF * tif = TIFFOpen(newfilename, "w");
  int i, j, x;
  int px, py;
  int spp = 3;
  int bps = 8;
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, spp); //R G B A
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps);
  TIFFSetField(tif, TIFFTAG_TILEWIDTH, tileWidth);
  TIFFSetField(tif, TIFFTAG_TILELENGTH, tileHeight);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  int tilesize = tileWidth * tileHeight * bps / 8 * spp;
  uint8_t * buf = (uint8_t * ) malloc(tilesize);
  //printf("%d\n", tilesize);
  for (j = 0; j < width; j += tileWidth) {
    for (i = 0; i < height; i += tileHeight) {
      for (x = 0; x < tilesize; x += spp) {
        px = j + (x / spp) % tileWidth;
        py = i + (x / spp) / tileWidth;
        buf[x + 0] = Rband[px * height + py];
	buf[x + 1] = Gband[px * height + py];
	buf[x + 2] = Bband[px * height + py];
        //buf[x + 3] = 255; // ALPHA
      }
      if (TIFFWriteTile(tif, buf, j, i, 0, 0) < 0) exit(-1);
    }
  }
  free(buf);
  TIFFClose(tif);
  return 0;
}

/* --------------------------------------------------------   */
/* Colormapping functions to visualize differen data outputs  */
void colorizeArray(float * data, int dimX, int dimY, float min, float max, bool rev ,uint8_t * outR, uint8_t * outG, uint8_t * outB) {
  int i;
  float datamin = 1E+37;
  float datamax = -1E+37;
  int nv;
  if (min < max) { // Use provided min max
    datamin = min;
    datamax = max;
    for (i = 0; i < dimX * dimY; i++) {
      if (!std::isnan(data[i])) {
        if (data[i] < datamin) data[i] = datamin;
        if (data[i] > datamax) data[i] = datamax;
      }
    }
  } else { // Get min max from data
    for (i = 0; i < dimX * dimY; i++) {
      if (!std::isnan(data[i])) {
        if (data[i] < datamin) datamin = data[i];
        if (data[i] > datamax) datamax = data[i];
      }
    }
  }
  printf("visualization data range: %f to %f\n", datamin, datamax);
  float range = datamax - datamin;

  for (i = 0; i < dimX * dimY; i++) {
    if (std::isnan(data[i])){
	outR[i]=0;
	outG[i]=0;
	outB[i]=0;
    }
    else {
    nv = (uint8_t)(((data[i] - datamin) / range) * 254 + 1);
    if (rev) nv= 255 - nv+1;
    outR[i] = (uint8_t) (colormapMagma[nv][0]*255);
    outG[i] = (uint8_t) (colormapMagma[nv][1]*255);
    outB[i] = (uint8_t) (colormapMagma[nv][2]*255);
    }
  }
}



void writeBinaryRaster( void * data, const char * filename, int ncol, int nrow, int nbands, const char * datatype, const char* layout,const char* comment){
  // dataype is one of  U8, U16, S8, S16, F32
int nbytes=1;
if (!strcmp(datatype,"U8") || !strcmp(datatype,"S8") ) nbytes=1;
else if (!strcmp(datatype,"U16") || !strcmp(datatype,"S16") )nbytes=2;
else if (!strcmp(datatype,"F32")) nbytes=4;
  // layout BIP (2x2x3: RGB RGB RGB RGB),  BSQ (2x2x3: RRRR GGGG BBBB) , BIL (2x2x3: RRBBGG RRBBGG)
  char * outfilename = new char[256];
  // write header
  int n;
  char * header = new char[256];
  n=sprintf (header, "NROWS %i\nNCOLS %i\nNBANDS %i\nNDATATYPE %s\nLAYOUT %s\n%s\n",ncol,nrow,nbands, datatype,layout,comment);
  std::ofstream headerfile;
  strcpy(outfilename,filename);
  strcat(outfilename,".HDR");
  headerfile.open (outfilename);
  headerfile << header;
  headerfile.close();
  // write data
  strcpy(outfilename,filename);
  strcat(outfilename,".");
  strcat(outfilename,layout);
  std::ofstream datafile (outfilename, std::ios::out | std::ios::binary);
  datafile.write ((char *) data, ncol*nrow*nbands*nbytes);
  datafile.close();
}
