#pragma once
#include "ITerrainGraphicsAPIAdaptor.h"

class TerrainOpenGLAdaptor : public ITerrainGraphicsAPIAdaptor
{
public:


	// Inherited via ITerrainGraphicsAPIAdaptor
	virtual void UploadNewlyPolygonizedToGPU(PolygonizeWorkerThreadData* data) override;

};