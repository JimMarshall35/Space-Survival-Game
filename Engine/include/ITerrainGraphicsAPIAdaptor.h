#pragma once

struct PolygonizeWorkerThreadData;

class ITerrainGraphicsAPIAdaptor
{
public:
	virtual void UploadNewlyPolygonizedToGPU(PolygonizeWorkerThreadData* data) = 0;
};