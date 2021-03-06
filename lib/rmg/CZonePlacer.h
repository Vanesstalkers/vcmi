/*
 * CZonePlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CMapGenerator.h"
#include "float3.h"
#include "../int3.h"

class CZoneGraph;
class CMap;
class CRandomGenerator;
class CRmgTemplateZone;
class CMapGenerator;

typedef std::vector<std::pair<TRmgTemplateZoneId, std::shared_ptr<CRmgTemplateZone>>> TZoneVector;
typedef std::map <TRmgTemplateZoneId, std::shared_ptr<CRmgTemplateZone>> TZoneMap;
typedef std::map <std::shared_ptr<CRmgTemplateZone>, float3> TForceVector;
typedef std::map <std::shared_ptr<CRmgTemplateZone>, float> TDistanceVector;

class CZonePlacer
{
public:
	explicit CZonePlacer(CMapGenerator * gen);
	int3 cords (const float3 f) const;
	float metric (const int3 &a, const int3 &b) const;
	float getDistance(float distance) const; //additional scaling without 0 divison
	~CZonePlacer();

	void placeZones(CRandomGenerator * rand);
	void assignZones();
	
private:
	void prepareZones(TZoneMap &zones, TZoneVector &zonesVector, const bool underground, CRandomGenerator * rand);
	void attractConnectedZones(TZoneMap &zones, TForceVector &forces, TDistanceVector &distances);
	void separateOverlappingZones(TZoneMap &zones, TForceVector &forces, TDistanceVector &overlaps);
	void moveOneZone(TZoneMap &zones, TForceVector &totalForces, TDistanceVector &distances, TDistanceVector &overlaps);

private:
	int width;
	int height;
	//metric coefiicients
	float scaleX;
	float scaleY;
	float mapSize;

	float gravityConstant;
	float stiffnessConstant;
    //float a1, b1, c1, a2, b2, c2;
	//CMap * map;
	//std::unique_ptr<CZoneGraph> graph;
	CMapGenerator * gen;
};
