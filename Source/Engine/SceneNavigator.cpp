#include "Engine/SceneNavigator.hpp"
#include "Engine/Scene.h"

#include "DetourNavMeshBuilder.h"
//#include "NavMeshPruneTool.h"
//#include "OffMeshConnectionTool.h"
//#include "ConvexVolumeTool.h"
//#include "CrowdTool.h"


SceneNavigator::SceneNavigator(Scene* scene) :
    mScene(scene)
{
    m_navMesh = dtAllocNavMesh();
    generateNavmesh();
}


SceneNavigator::~SceneNavigator()
{
    dtFreeNavMesh(m_navMesh);
}


/*bool SceneNavigator::generateNavmesh()
{
    const float* bmin = m_geom->getNavMeshBoundsMin();
    const float* bmax = m_geom->getNavMeshBoundsMax();
    const float* verts = m_geom->getMesh()->getVerts();
    const int nverts = m_geom->getMesh()->getVertCount();
    const int* tris = m_geom->getMesh()->getTris();
    const int ntris = m_geom->getMesh()->getTriCount();

    //
    // Step 1. Initialize build config.
    //

    // Init build configuration from GUI
    rcConfig config{};
    memset(&config, 0, sizeof(config));
    config.cs = m_cellSize;
    config.ch = m_cellHeight;
    config.walkableSlopeAngle = m_agentMaxSlope;
    config.walkableHeight = (int)ceilf(m_agentHeight / config.ch);
    config.walkableClimb = (int)floorf(m_agentMaxClimb / config.ch);
    config.walkableRadius = (int)ceilf(m_agentRadius / config.cs);
    config.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
    config.maxSimplificationError = m_edgeMaxError;
    config.minRegionArea = (int)rcSqr(m_regionMinSize);		// Note: area = size*size
    config.mergeRegionArea = (int)rcSqr(m_regionMergeSize);	// Note: area = size*size
    config.maxVertsPerPoly = (int)m_vertsPerPoly;
    config.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
    config.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;

    // Set the area where the navigation will be build.
    // Here the bounds of the input mesh are used, but the
    // area could be specified by an user defined box, etc.
    rcVcopy(config.bmin, bmin);
    rcVcopy(config.bmax, bmax);
    rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

    // Reset build times gathering.
    m_ctx->resetTimers();

    // Start the build process.
    m_ctx->startTimer(RC_TIMER_TOTAL);

    //
    // Step 2. Rasterize input polygon soup.
    //

    // Allocate voxel heightfield where we rasterize our input data to.
    m_solid = rcAllocHeightfield();
    if (!m_solid)
    {
        return false;
    }
    if (!rcCreateHeightfield(m_ctx, *m_solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
    {
        return false;
    }

    // Allocate array that can hold triangle area types.
    // If you have multiple meshes you need to process, allocate
    // and array which can hold the max number of triangles you need to process.
    unsigned char* triareas = new unsigned char[ntris];
    if (!triareas)
    {
        return false;
    }

    // Find triangles which are walkable based on their slope and rasterize them.
    // If your input data is multiple meshes, you can transform them here, calculate
    // the are type for each of the meshes and rasterize them.
    memset(m_triareas, 0, ntris*sizeof(unsigned char));
    rcMarkWalkableTriangles(m_ctx, config.walkableSlopeAngle, verts, nverts, tris, ntris, triareas);
    if (!rcRasterizeTriangles(m_ctx, verts, nverts, tris, triareas, ntris, *m_solid, config.walkableClimb))
    {
        return false;
    }

    delete [] triareas;
    triareas = nullptr;

    //
    // Step 3. Filter walkables surfaces.
    //

    // Once all geoemtry is rasterized, we do initial pass of filtering to
    // remove unwanted overhangs caused by the conservative rasterization
    // as well as filter spans where the character cannot possibly stand.
    if (m_filterLowHangingObstacles)
        rcFilterLowHangingWalkableObstacles(m_ctx, config.walkableClimb, *m_solid);
    if (m_filterLedgeSpans)
        rcFilterLedgeSpans(m_ctx, config.walkableHeight, config.walkableClimb, *m_solid);
    if (m_filterWalkableLowHeightSpans)
        rcFilterWalkableLowHeightSpans(m_ctx, config.walkableHeight, *m_solid);


    //
    // Step 4. Partition walkable surface to simple regions.
    //

    // Compact the heightfield so that it is faster to handle from now on.
    // This will result more cache coherent data as well as the neighbours
    // between walkable cells will be calculated.
    m_chf = rcAllocCompactHeightfield();
    if (!m_chf)
    {
        return false;
    }
    if (!rcBuildCompactHeightfield(m_ctx, config.walkableHeight, config.walkableClimb, *m_solid, *m_chf))
    {
        return false;
    }

    if (!m_keepInterResults)
    {
        rcFreeHeightField(m_solid);
        m_solid = 0;
    }

    // Erode the walkable area by agent radius.
    if (!rcErodeWalkableArea(m_ctx, config.walkableRadius, *m_chf))
    {
        return false;
    }

    // (Optional) Mark areas.
    const ConvexVolume* vols = m_geom->getConvexVolumes();
    for (int i  = 0; i < m_geom->getConvexVolumeCount(); ++i)
        rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);


    // Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
    // There are 3 martitioning methods, each with some pros and cons:
    // 1) Watershed partitioning
    //   - the classic Recast partitioning
    //   - creates the nicest tessellation
    //   - usually slowest
    //   - partitions the heightfield into nice regions without holes or overlaps
    //   - the are some corner cases where this method creates produces holes and overlaps
    //      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
    //      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
    //   * generally the best choice if you precompute the nacmesh, use this if you have large open areas
    // 2) Monotone partioning
    //   - fastest
    //   - partitions the heightfield into regions without holes and overlaps (guaranteed)
    //   - creates long thin polygons, which sometimes causes paths with detours
    //   * use this if you want fast navmesh generation
    // 3) Layer partitoining
    //   - quite fast
    //   - partitions the heighfield into non-overlapping regions
    //   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
    //   - produces better triangles than monotone partitioning
    //   - does not have the corner cases of watershed partitioning
    //   - can be slow and create a bit ugly tessellation (still better than monotone)
    //     if you have large open areas with small obstacles (not a problem if you use tiles)
    //   * good choice to use for tiled navmesh with medium and small sized tiles

    //if (m_partitionType == SAMPLE_PARTITION_WATERSHED)
    //{
    // Prepare for region partitioning, by calculating distance field along the walkable surface.
    if (!rcBuildDistanceField(m_ctx, *m_chf))
    {
        return false;
    }

    // Partition the walkable surface into simple regions without holes.
    if (!rcBuildRegions(m_ctx, *m_chf, 0, config.minRegionArea, config.mergeRegionArea))
    {
        return false;
    }
    //}
    else if (m_partitionType == SAMPLE_PARTITION_MONOTONE)
    {
        // Partition the walkable surface into simple regions without holes.
        // Monotone partitioning does not need distancefield.
        if (!rcBuildRegionsMonotone(m_ctx, *m_chf, 0, config.minRegionArea, config.mergeRegionArea))
        {
            return false;
        }
    }
    else // SAMPLE_PARTITION_LAYERS
    {
        // Partition the walkable surface into simple regions without holes.
        if (!rcBuildLayerRegions(m_ctx, *m_chf, 0, config.minRegionArea))
        {
            return false;
        }
    }

    //
    // Step 5. Trace and simplify region contours.
    //

    // Create contours.
    m_cset = rcAllocContourSet();
    if (!m_cset)
    {
        return false;
    }
    if (!rcBuildContours(m_ctx, *m_chf, config.maxSimplificationError, config.maxEdgeLen, *m_cset))
    {
        return false;
    }

    //
    // Step 6. Build polygons mesh from contours.
    //

    // Build polygon navmesh from the contours.
    m_pmesh = rcAllocPolyMesh();
    if (!m_pmesh)
    {
        return false;
    }
    if (!rcBuildPolyMesh(m_ctx, *m_cset, config.maxVertsPerPoly, *m_pmesh))
    {
        return false;
    }

    //
    // Step 7. Create detail mesh which allows to access approximate height on each polygon.
    //

    m_dmesh = rcAllocPolyMeshDetail();
    if (!m_dmesh)
    {
        return false;
    }

    if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, config.detailSampleDist, config.detailSampleMaxError, *m_dmesh))
    {
        return false;
    }

    rcFreeCompactHeightfield(m_chf);
    m_chf = 0;
    rcFreeContourSet(m_cset);
    m_cset = 0;

    // At this point the navigation mesh data is ready, you can access it from m_pmesh.
    // See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

    //
    // (Optional) Step 8. Create Detour data from Recast poly mesh.
    //

    // The GUI may allow more max points per polygon than Detour can handle.
    // Only build the detour navmesh if we do not exceed the limit.
    if (config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
    {
        unsigned char* navData = 0;
        int navDataSize = 0;

        // Update poly flags from areas.
        for (int i = 0; i < m_pmesh->npolys; ++i)
        {
            if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
                m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;

            if (m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
                    m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
                    m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
            {
                m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
            }
            else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER)
            {
                m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
            }
            else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR)
            {
                m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
            }
        }


        dtNavMeshCreateParams params;
        memset(&params, 0, sizeof(params));
        params.verts = m_pmesh->verts;
        params.vertCount = m_pmesh->nverts;
        params.polys = m_pmesh->polys;
        params.polyAreas = m_pmesh->areas;
        params.polyFlags = m_pmesh->flags;
        params.polyCount = m_pmesh->npolys;
        params.nvp = m_pmesh->nvp;
        params.detailMeshes = m_dmesh->meshes;
        params.detailVerts = m_dmesh->verts;
        params.detailVertsCount = m_dmesh->nverts;
        params.detailTris = m_dmesh->tris;
        params.detailTriCount = m_dmesh->ntris;
        params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
        params.offMeshConRad = m_geom->getOffMeshConnectionRads();
        params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
        params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
        params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
        params.offMeshConUserID = m_geom->getOffMeshConnectionId();
        params.offMeshConCount = m_geom->getOffMeshConnectionCount();
        params.walkableHeight = m_agentHeight;
        params.walkableRadius = m_agentRadius;
        params.walkableClimb = m_agentMaxClimb;
        rcVcopy(params.bmin, m_pmesh->bmin);
        rcVcopy(params.bmax, m_pmesh->bmax);
        params.cs = config.cs;
        params.ch = config.ch;
        params.buildBvTree = true;

        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            return false;
        }

        m_navMesh = dtAllocNavMesh();
        if (!m_navMesh)
        {
            dtFree(navData);
            return false;
        }

        dtStatus status;

        status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
        if (dtStatusFailed(status))
        {
            dtFree(navData);
            return false;
        }

        status = m_navQuery->init(m_navMesh, 2048);
        if (dtStatusFailed(status))
        {
            return false;
        }
    }

    m_ctx->stopTimer(RC_TIMER_TOTAL);

    // Show performance stats.
    duLogBuildTimes(*m_ctx, m_ctx->getAccumulatedTime(RC_TIMER_TOTAL));

    m_totalBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;

    if (m_tool)
        m_tool->init(this);
    initToolStates(this);

    return true;
}*/
