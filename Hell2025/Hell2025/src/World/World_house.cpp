#include "World.h"
#include "AssetManagement/AssetManager.h"
#include "Core/JSON.h"
#include "Renderer/RenderDataManager.h"
#include "World/HouseManager.h"

namespace World {

    MeshBuffer g_houseMeshBuffer;

    void AddHouse(HouseCreateInfo houseCreateInfo, SpawnOffset spawnOffset) {
        for (DoorCreateInfo& createInfo : houseCreateInfo.doors) {
            AddDoor(createInfo, spawnOffset);
        }
        for (PlaneCreateInfo& createInfo : houseCreateInfo.planes) {
            AddHousePlane(createInfo, spawnOffset);
        }
        for (LightCreateInfo& createInfo : houseCreateInfo.lights) {
            AddLight(createInfo, spawnOffset);
        }
        for (PianoCreateInfo& createInfo : houseCreateInfo.pianos) {
            AddPiano(createInfo, spawnOffset);
        }
        for (WindowCreateInfo& createInfo : houseCreateInfo.windows) {
            AddWindow(createInfo, spawnOffset);
        }

        // Update the clipping cubes, so the walls have correct cut outs for doors/windows
        UpdateClippingCubes();
        
        for (WallCreateInfo& createInfo : houseCreateInfo.walls) {
            AddWall(createInfo, spawnOffset);
        }

        UpdateHouseMeshBuffer();
    }

    void SaveHouse() {

        std::string filename = "TestHouse.json";

        HouseCreateInfo houseCreateInfo;

        for (Door& door : GetDoors()) {
            houseCreateInfo.doors.emplace_back(door.GetCreateInfo());
        }
        for (Light& light : GetLights()) {
            houseCreateInfo.lights.emplace_back(light.GetCreateInfo());
        }
        for (Piano& piano : GetPianos()) {
            houseCreateInfo.pianos.emplace_back(piano.GetCreateInfo());
        }
        for (Plane& plane: GetPlanes()) {
            houseCreateInfo.planes.emplace_back(plane.GetCreateInfo());
        }
        for (Window& window : GetWindows()) {
            houseCreateInfo.windows.emplace_back(window.GetCreateInfo());
        }
        for (Wall& wall: GetWalls()) {
            houseCreateInfo.walls.emplace_back(wall.GetCreateInfo());
        }

        JSON::SaveHouse("res/houses/" + filename, houseCreateInfo);
    }


    void UpdateHouseMeshBuffer() {
        g_houseMeshBuffer.Reset();

        for (Wall& wall : GetWalls()) {
            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                uint32_t meshIndex = g_houseMeshBuffer.AddMesh(wallSegment.GetVertices(), wallSegment.GetIndices());
                wallSegment.SetMeshIndex(meshIndex);
            }
        }
        for (Plane& housePlane : GetPlanes()) {
            uint32_t meshIndex = g_houseMeshBuffer.AddMesh(housePlane.GetVertices(), housePlane.GetIndices());
            housePlane.SetMeshIndex(meshIndex);
        }

        g_houseMeshBuffer.UpdateBuffers();
    }

    MeshBuffer& GetHouseMeshBuffer() {
        return g_houseMeshBuffer;
    }

    Mesh* GetHouseMeshByIndex(uint32_t meshIndex) {
        return g_houseMeshBuffer.GetMeshByIndex(meshIndex);
    }
}