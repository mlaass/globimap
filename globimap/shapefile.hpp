#ifndef SHAPEFILE_HPP
#define SHAPEFILE_HPP
#include <string>

/*
    Section 1.2: Shapefile Loading
    ==================================
    This section uses libshp to load the polygons from the shapefile. Note that
    access to the Shapefile data table is implemented, but not used.
*/

#include <shapefil.h>

std::string shapename(size_t type) {
  switch (type) {
  case 0:
    return std::string("NULL");
  case 1:
    return std::string("POINT");
  case 3:
    return std::string("ARC");
  case 5:
    return std::string("POLYGON");
  case 8:
    return std::string("MULTIPOINT");
  default:
    return std::string("undefined");
  }
}
std::string dbftype(size_t type) {
  switch (type) {
  case 0:
    return std::string("FTString");
  case 1:
    return std::string("FTInteger");
  case 2:
    return std::string("FTDouble");
  case 3:
    return std::string("FTLogical");
  default:
    return std::string("FTundefined");
  }
}

void importSHP(std::string filename,
               std::function<void(SHPObject *, int, int)> handler,
               bool verbose = false) {
  SHPHandle hSHP;
  DBFHandle hDBF;
  int nShapeType, i, nEntities;
  SHPObject *shape;

  hSHP = SHPOpen((filename + std::string(".shp")).c_str(), "rb");
  hDBF = DBFOpen((filename + std::string(".dbf")).c_str(), "rb");
  if (hSHP == NULL)
    throw(std::runtime_error("Unable to open Shapefile"));
  if (hDBF == NULL)
    throw(std::runtime_error("Unable to open DBF "));

  SHPGetInfo(hSHP, &nEntities, &nShapeType, NULL, NULL);
  if (verbose)
    std::cout << "Importing file with " << nEntities << " entities"
              << std::endl;

  int nDBFRecords = DBFGetRecordCount(hDBF);
  if (verbose)
    std::cout << "Found a DBF with " << nDBFRecords << " entries" << std::endl;

  if (nEntities != nDBFRecords) {
    std::cout << "Using DBF and SHP pair of different size: " << filename
              << std::endl;
  }

  /*And the associated DBF information*/
  size_t fields = DBFGetFieldCount(hDBF);
  if (verbose) {
    for (size_t i = 0; i < fields; i++) {
      char name[20];
      size_t type = DBFGetFieldInfo(hDBF, i, name, NULL, NULL);
      std::cout << dbftype(type) << ":" << name << "\n";
    }
    std::cout << std::endl << std::endl;
  }

  // File Loading loop
  // coll.resize(nEntities);

  for (i = 0; i < nEntities; i++) {
    shape = SHPReadObject(hSHP, i);
    if (shape == NULL)
      throw(std::runtime_error("unable to read some shape"));
#if SHAPE_VERBOSE == 2
    std::cout << "Found a shape" << shapename(shape->nSHPType) << "("
              << shape->nSHPType << ")" << std::endl;
    std::cout << "ID:" << shape->nShapeId << std::endl;
    std::cout << "numParts: " << shape->nParts << std::endl;
    std::cout << "numVertices:" << shape->nVertices << std::endl;
    std::cout << "numParts" << shape->nParts << std::endl;
#endif
    if (shape->nParts != 0) {
      for (int j = 0; j < shape->nParts; j++) {
        if (verbose)
          std::cout << "Part:" << shape->panPartStart[j] << std::endl;
        int start = shape->panPartStart[j];
        int end = shape->nVertices;
        if (j + 1 < shape->nParts)
          end = shape->panPartStart[j + 1];

        handler(shape, start, end);
      }
    } else {

      handler(shape, 0, shape->nVertices);
    }
    SHPDestroyObject(shape);
    /*This is the place to read all attributes of this entity*/
  }
  SHPClose(hSHP);
  DBFClose(hDBF);
  if (verbose)
    std::cout << "Completed " << nEntities << " entities." << std::endl;
}

#endif