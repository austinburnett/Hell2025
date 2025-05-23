#include "RenderDataManager.h"
#include "AssetManagement/AssetManager.h"
#include "HellDefines.h"
#include "BackEnd/BackEnd.h"
#include "Camera/Frustum.h"
#include "Core/Game.h"
#include "Config/Config.h"
#include "Editor/Editor.h"
#include "Input/Input.h"
#include "Renderer/Renderer.h"
#include "Viewport/ViewportManager.h"
#include <span>
#include <unordered_map>

#include "Timer.hpp"

// Get me out of here
#include "World/World.h"
// Get me out of here

namespace RenderDataManager {
    DrawCommandsSet g_drawCommandsSet;
    FlashLightShadowMapDrawInfo g_flashLightShadowMapDrawInfo;
    RendererData g_rendererData;
    std::vector<AnimatedGameObject*> g_animatedGameObjectsToSkin;
    std::vector<GPULight> g_gpuLightsHighRes;

    std::vector<HouseRenderItem> g_houseRenderItems;
    std::vector<HouseRenderItem> g_houseOutlineRenderItems;
    std::vector<RenderItem> g_decalRenderItems;
    std::vector<RenderItem> g_renderItems;
    std::vector<RenderItem> g_outlineRenderItems;
    std::vector<RenderItem> g_shadowMapRenderItems;

    std::vector<RenderItem> g_instanceData;
    std::vector<ViewportData> g_viewportData;
    uint32_t g_baseSkinnedVertex;

    void UpdateViewportFrustums();
    void UpdateViewportData();
    void UpdateRendererData();
    void UpdateDrawCommandsSet();
    //void CreateDrawCommands(DrawCommands& drawCommands, std::vector<RenderItem>& renderItems);
    void CreateDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Frustum& frustum, int viewportIndex);
    void CreateDrawCommandsSkinned(DrawCommands& drawCommands, std::vector<RenderItem>& renderItems);
    void CreateMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int playerIndex, int instanceOffset);
    void CreateMultiDrawIndirectCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int playerIndex, int instanceOffset);
    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, uint32_t faceIndex, GPULight& gpuLight);

    int EncodeBaseInstance(int playerIndex, int instanceOffset);
    void DecodeBaseInstance(int baseInstance, int& playerIndex, int& instanceOffset);
   
    void BeginFrame() {
        g_animatedGameObjectsToSkin.clear();
        g_decalRenderItems.clear();
        g_houseOutlineRenderItems.clear();
        g_houseRenderItems.clear();
        g_renderItems.clear();
        g_outlineRenderItems.clear();

        g_gpuLightsHighRes.clear();
    }

    void Update() {
        UpdateViewportData();
        UpdateViewportFrustums();
        UpdateRendererData();
        UpdateDrawCommandsSet();   
    }

    void UpdateViewportData() {
        const Resolutions& resolutions = Config::GetResolutions();
        g_viewportData.resize(4);
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;


            glm::mat4 viewMatrix = glm::mat4(1);
            if (Editor::IsOpen()) {
                viewMatrix = Editor::GetViewportViewMatrix(i);
                g_viewportData[0].orthoSize = Editor::GetEditorOrthoSize(i);
                g_viewportData[0].isOrtho = true;
            }
            else {
                viewMatrix = Game::GetLocalPlayerCameraByIndex(i)->GetViewMatrix();
                g_viewportData[0].orthoSize = 0.0f;
                g_viewportData[0].isOrtho = false;
            }

            g_viewportData[i].projection = viewport->GetProjectionMatrix();
            g_viewportData[i].inverseProjection = glm::inverse(g_viewportData[i].projection);
            g_viewportData[i].view = viewMatrix;
            g_viewportData[i].inverseView = glm::inverse(g_viewportData[i].view);
            g_viewportData[i].projectionView = g_viewportData[i].projection * g_viewportData[i].view;
            g_viewportData[i].inverseProjectionView = glm::inverse(g_viewportData[i].projectionView);
            g_viewportData[i].skyboxProjectionView = viewport->GetPerpsectiveMatrix() * g_viewportData[i].view;
            g_viewportData[i].width = resolutions.gBuffer.x * viewport->GetSize().x;
            g_viewportData[i].height = resolutions.gBuffer.y * viewport->GetSize().y;
            g_viewportData[i].xOffset = resolutions.gBuffer.x * viewport->GetPosition().x;
            g_viewportData[i].yOffset = resolutions.gBuffer.y * viewport->GetPosition().y;
            g_viewportData[i].posX = viewport->GetPosition().x;
            g_viewportData[i].posY = viewport->GetPosition().y;
            g_viewportData[i].sizeX = viewport->GetSize().x;
            g_viewportData[i].sizeY = viewport->GetSize().y;

            viewport->GetFrustum().Update(g_viewportData[i].projectionView);

            g_viewportData[i].frustumPlane0 = viewport->GetFrustum().GetPlane(0);
            g_viewportData[i].frustumPlane1 = viewport->GetFrustum().GetPlane(1);
            g_viewportData[i].frustumPlane2 = viewport->GetFrustum().GetPlane(2);
            g_viewportData[i].frustumPlane3 = viewport->GetFrustum().GetPlane(3);
            g_viewportData[i].frustumPlane4 = viewport->GetFrustum().GetPlane(4);
            g_viewportData[i].frustumPlane5 = viewport->GetFrustum().GetPlane(5);

            // Flashlight
            if (Editor::IsOpen()) {
                g_viewportData[i].flashlightModifer = 0;
                g_viewportData[i].flashlightProjectionView = glm::mat4(1);
                g_viewportData[i].flashlightDir = glm::vec4(0.0f);
                g_viewportData[i].flashlightPosition = glm::vec4(0.0f);
            }
            else {
                Player* player = Game::GetLocalPlayerByIndex(i);
                g_viewportData[i].flashlightProjectionView = player->GetFlashlightProjectionView();
                g_viewportData[i].flashlightDir = glm::vec4(player->GetFlashlightDirection(), 0.0f);
                g_viewportData[i].flashlightPosition = glm::vec4(player->GetFlashlightPosition(), 0.0f);
                g_viewportData[i].flashlightModifer = player->GetFlashLightModifer();
            }
        }
    }

    void UpdateViewportFrustums() {
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                viewport->GetFrustum().Update(g_viewportData[i].projectionView);
            }
        }
    }

    //void UpdateGPULightData() {
    //    int maxLights = 10;
    //
    //    g_gpuLightData.clear();
    //    g_gpuLightData.resize(maxLights);
    //
    //    // Wipe any old light data
    //    for (int i = 0; i < g_gpuLightData.size(); i++) {
    //        GPULight& gpuLight = g_gpuLightData[i];
    //        gpuLight.posX = 0.0f;
    //        gpuLight.posY = 0.0f;
    //        gpuLight.posZ = 0.0f;
    //        gpuLight.colorR = 0.0f;
    //        gpuLight.colorG = 0.0f;
    //        gpuLight.colorB = 0.0f;
    //        gpuLight.radius = 0.0f;
    //        gpuLight.strength = 0.0f;
    //    }
    //
    //    // Populate with new light data
    //    for (int i = 0; i < World::GetLights().size() && i < maxLights; i++) {
    //        Light& light = World::GetLights()[i];
    //        GPULight& gpuLight = g_gpuLightData[i];
    //        gpuLight.posX = light.GetPosition().x;
    //        gpuLight.posY = light.GetPosition().y;
    //        gpuLight.posZ = light.GetPosition().z;
    //        gpuLight.colorR = light.GetColor().r;
    //        gpuLight.colorG = light.GetColor().g;
    //        gpuLight.colorB = light.GetColor().b;
    //        gpuLight.radius = light.GetRadius();
    //        gpuLight.strength = light.GetStrength();
    //    }
    //}

    void UpdateRendererData() {
        const RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        const Resolutions& resolutions = Config::GetResolutions();
        g_rendererData.nearPlane = NEAR_PLANE;
        g_rendererData.farPlane = FAR_PLANE;
        g_rendererData.gBufferWidth = resolutions.gBuffer.x;
        g_rendererData.gBufferHeight = resolutions.gBuffer.y;
        g_rendererData.hairBufferWidth = resolutions.hair.x;
        g_rendererData.hairBufferHeight = resolutions.hair.y;
        g_rendererData.splitscreenMode = (int)Game::GetSplitscreenMode();
        g_rendererData.time = Game::GetTotalTime();
        g_rendererData.rendererOverrideState = (int)rendererSettings.rendererOverrideState;
        g_rendererData.normalizedMouseX = Util::MapRange(Input::GetMouseX(), 0, BackEnd::GetCurrentWindowWidth(), 0.0f, 1.0f);
        g_rendererData.normalizedMouseY = Util::MapRange(Input::GetMouseY(), 0, BackEnd::GetCurrentWindowHeight(), 0.0f, 1.0f);
    }

    void SortRenderItems(std::vector<RenderItem>& renderItems) {
        std::sort(renderItems.begin(), renderItems.end(), [](const RenderItem& a, const RenderItem& b) {
            return a.meshIndex < b.meshIndex;
        });
    }

    void UpdateDrawCommandsSet() {

        /*
        if (Input::KeyDown(HELL_KEY_T)) {

            int iterations = 100;

            {
                Timer("UpdateRenderItemAABB yours)");
                for (int i = 0; i < iterations; i++) {
                    for (RenderItem& renderItem : g_renderItems) {
                        Util::UpdateRenderItemAABBFastB(renderItem);
                    }
                }
            }
            {
                Timer("UpdateRenderItemAABB mine vec4()");
                for (int i = 0; i < iterations; i++) {
                    for (RenderItem& renderItem : g_renderItems) {
                        Util::UpdateRenderItemAABBFastA(renderItem);
                    }
                }
            }
            {
                Timer("UpdateRenderItemAABB mine vec3()");
                for (int i = 0; i < iterations; i++) {
                    for (RenderItem& renderItem : g_renderItems) {
                        Util::UpdateRenderItemAABB(renderItem);
                    }
                }
            }
        }*/

        g_instanceData.clear();
        auto& set = g_drawCommandsSet;

        std::vector<RenderItem> renderItems;
        renderItems.insert(renderItems.end(), g_renderItems.begin(), g_renderItems.end());
        renderItems.insert(renderItems.end(), World::GetRenderItems().begin(), World::GetRenderItems().end());

        // Clear any commands from last frame
        for (int i = 0; i < 4; i++) {
            set.geometry[i].clear();
            set.geometryBlended[i].clear();
            set.geometryAlphaDiscarded[i].clear();
            set.hairTopLayer[i].clear();
            set.hairBottomLayer[i].clear();
            g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i].clear();
            g_flashLightShadowMapDrawInfo.heightMapChunkIndices[i].clear();
            g_flashLightShadowMapDrawInfo.houseMeshRenderItems[i].clear();
        }

        std::vector<RenderItem>& renderItemsBlended = World::GetRenderItemsBlended();
        std::vector<RenderItem>& geometryAlphaDiscarded = World::GetRenderItemsAlphaDiscarded();
        std::vector<RenderItem>& hairTopLayer = World::GetRenderItemsHairTopLayer();
        std::vector<RenderItem>& hairBottomLayer = World::GetRenderItemsHairBottomLayer();

        SortRenderItems(renderItems);
        SortRenderItems(renderItemsBlended);
        SortRenderItems(geometryAlphaDiscarded);
        SortRenderItems(hairTopLayer);
        SortRenderItems(hairBottomLayer);

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Frustum& frustum = viewport->GetFrustum();
            CreateDrawCommands(set.geometry[i], renderItems, frustum, i);
            CreateDrawCommands(set.geometryBlended[i], renderItemsBlended, frustum, i);
            CreateDrawCommands(set.geometryAlphaDiscarded[i], geometryAlphaDiscarded, frustum, i);
            CreateDrawCommands(set.hairTopLayer[i], hairTopLayer, frustum, i);
            CreateDrawCommands(set.hairBottomLayer[i], hairBottomLayer, frustum, i);
        }

        CreateDrawCommandsSkinned(set.skinnedGeometry, World::GetSkinnedRenderItems());

        for (int i = 0; i < g_gpuLightsHighRes.size(); i++) {
            GPULight& gpuLight = g_gpuLightsHighRes[i];
            for (uint32_t j = 0; j < 6; j++) {
                CreateShadowCubeMapMultiDrawIndirectCommands(set.shadowMapHiRes[i][j], j, gpuLight);
            }
        }

        // Flashlight stuff
        for (int i = 0; i < Game::GetLocalPlayerCount(); i++) {
            Player* player = Game::GetLocalPlayerByIndex(i);
            if (!player) continue;

            Frustum flashLightFrustum = player->GetFlashlightFrustum();

            // Build multidraw commands for regular geometry
            CreateDrawCommands(g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i], renderItems, flashLightFrustum, i);

            // Frustum cull the heightmap chunks
            std::vector<HeightMapChunk>& chunks = World::GetHeightMapChunks();
            for (int i = 0; i < chunks.size(); i++) {
                HeightMapChunk& chunk = chunks[i];
                if (flashLightFrustum.IntersectsAABBFast(AABB(chunk.aabbMin, chunk.aabbMax))) {
                    g_flashLightShadowMapDrawInfo.heightMapChunkIndices->push_back(i);
                }
            }

            // Frustum cull the house mesh
            g_flashLightShadowMapDrawInfo.houseMeshRenderItems->reserve(g_houseRenderItems.size());
            for (int i = 0; i < g_houseRenderItems.size(); i++) {
                HouseRenderItem& renderItem = g_houseRenderItems[i];
                if (flashLightFrustum.IntersectsAABBFast(renderItem)) {
                    g_flashLightShadowMapDrawInfo.houseMeshRenderItems->push_back(renderItem);
                }
            }
        }
    }

    void CreateDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Frustum& frustum, int viewportIndex) {
        // Store the instance offset for this list of commands
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data
        for (const RenderItem& renderItem : renderItems) {
            if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == viewportIndex) continue;
            if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

            // Frustum cull it
            if (frustum.IntersectsAABBFast(renderItem)) {
                g_instanceData.push_back(renderItem);
            }
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommands(drawCommands, instanceView, viewportIndex, instanceStart);
    }

    void CreateDrawCommandsSkinned(DrawCommands& drawCommands, std::vector<RenderItem>& renderItems) {
        SortRenderItems(renderItems);

        // Clear any commands from last frame
        for (int i = 0; i < 4; i++) {
            drawCommands.perViewport[i].clear();
        }

        // Iterate the viewports and build the draw commands
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            // Store the instance offset for this player
            int instanceStart = g_instanceData.size();

            // Preallocate an estimate
            g_instanceData.reserve(g_instanceData.size() + renderItems.size());

            // Append new render items to the global instance data
            for (const RenderItem& renderItem : renderItems) {
                if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == i) continue;
                if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != i) continue;

                g_instanceData.push_back(renderItem);
            }

            // Create indirect draw commands using the stored offset
            std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
            CreateMultiDrawIndirectCommandsSkinned(drawCommands.perViewport[i], instanceView, i, instanceStart);
        }
    }

    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, uint32_t faceIndex, GPULight& gpuLight) {
        drawCommands.clear();

        //std::string name = "light " + std::to_string(gpuLight.lightIndex);
        //Timer timer(name);
        
        Light* light = World::GetLightByIndex(gpuLight.lightIndex);
        if (!light) return;

        Frustum* frustum = light->GetFrustumByFaceIndex(faceIndex);
        if (!frustum) return;

        // Store the instance offset for this player
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + g_renderItems.size());

        // Append new render items to the global instance data if it's within light frustum
        // g_renderItems is already sorted by this point
        // but if anything breaks, check here! (maybe you re-ordered things)
        for (const RenderItem& renderItem : g_renderItems) {
            if (frustum->IntersectsAABBFast(renderItem)) {
                g_instanceData.push_back(renderItem);
            }
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommands(drawCommands, instanceView, -1, instanceStart);
    }

    void CreateMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int playerIndex, int instanceOffset) {
        std::unordered_map<int, std::size_t> commandMap;
        commands.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            int meshIndex = renderItem.meshIndex;
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);

            // If the command exists, increment its instance count
            auto it = commandMap.find(meshIndex);
            if (it != commandMap.end()) {
                commands[it->second].instanceCount++;
            }
            // Otherwise create a new command
            else {
                std::size_t index = commands.size();
                auto& cmd = commands.emplace_back();
                cmd.indexCount = mesh->indexCount;
                cmd.firstIndex = mesh->baseIndex;
                cmd.baseVertex = mesh->baseVertex;
                cmd.baseInstance = EncodeBaseInstance(playerIndex, instanceOffset);
                cmd.instanceCount = 1;

                commandMap[meshIndex] = index;
            }
            instanceOffset++;
        }
    }

    void CreateMultiDrawIndirectCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int playerIndex, int instanceOffset) {
        std::unordered_map<int, std::size_t> commandMap;
        commands.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            int meshIndex = renderItem.meshIndex;
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(meshIndex);

            // If the command exists, increment its instance count
            auto it = commandMap.find(meshIndex);
            if (it != commandMap.end()) {
                commands[it->second].instanceCount++;
            }
            // Otherwise create a new command
            else {
                std::size_t index = commands.size();
                auto& cmd = commands.emplace_back();
                cmd.indexCount = mesh->indexCount;
                cmd.firstIndex = mesh->baseIndex;
                cmd.baseVertex = renderItem.baseSkinnedVertex;
                cmd.baseInstance = EncodeBaseInstance(playerIndex, instanceOffset);
                cmd.instanceCount = 1;

                commandMap[meshIndex] = index;
            }
            instanceOffset++;
        }
    }

    void SubmitGPULightHighRes(Light& light) {
        
    }

    int EncodeBaseInstance(int playerIndex, int instanceOffset) {
        return (playerIndex << VIEWPORT_INDEX_SHIFT) | instanceOffset;
    }

    void DecodeBaseInstance(int baseInstance, int& playerIndex, int& instanceOffset) {
        playerIndex = baseInstance >> VIEWPORT_INDEX_SHIFT;
        instanceOffset = baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    }

    void SubmitAnimatedGameObjectForSkinning(AnimatedGameObject* animatedGameObject) {
        g_animatedGameObjectsToSkin.push_back(animatedGameObject);
    }

    void ResetBaseSkinnedVertex() {
        g_baseSkinnedVertex = 0;
    }
    void IncrementBaseSkinnedVertex(uint32_t vertexCount) {
        g_baseSkinnedVertex += vertexCount;
    }

    uint32_t GetBaseSkinnedVertex() {
        return g_baseSkinnedVertex;
    }

    std::vector<AnimatedGameObject*>& GetAnimatedGameObjectToSkin() {
        return g_animatedGameObjectsToSkin;
    }

    const RendererData& GetRendererData() {
        return g_rendererData;
    }

    const std::vector<HouseRenderItem>& GetHouseRenderItems() {
        return g_houseRenderItems;
    }

    const std::vector<HouseRenderItem>& GetHouseOutlineRenderItems() {
        return g_houseOutlineRenderItems;
    }

    const std::vector<ViewportData>& GetViewportData() {
        return g_viewportData;
    }

    const std::vector<RenderItem>& GetRenderItems() {
        return g_renderItems;
    }

    const std::vector<RenderItem>& GetDecalRenderItems() {
        return g_decalRenderItems;
    }

    const std::vector<RenderItem>& GetInstanceData() {
        return g_instanceData;
    }

    const std::vector<RenderItem>& GetOutlineRenderItems() {
        return g_outlineRenderItems;
    }

    const DrawCommandsSet& GetDrawInfoSet() {
        return g_drawCommandsSet;
    }

    const FlashLightShadowMapDrawInfo& GetFlashLightShadowMapDrawInfo() {
        return g_flashLightShadowMapDrawInfo;
    }

    const std::vector<GPULight>& GetGPULightsHighRes() {
        return g_gpuLightsHighRes;
    }

    // Submissions
    void SubmitGPULightHighRes(uint32_t lightIndex) {
        if (g_gpuLightsHighRes.size() >= SHADOWMAP_HI_RES_COUNT) return;

        Light* light = World::GetLightByIndex(lightIndex);
        if (!light) return;

        GPULight& gpuLight = g_gpuLightsHighRes.emplace_back();
        gpuLight.colorR = light->GetColor().r;
        gpuLight.colorG = light->GetColor().g;
        gpuLight.colorB = light->GetColor().b;
        gpuLight.posX = light->GetPosition().x;
        gpuLight.posY = light->GetPosition().y;
        gpuLight.posZ = light->GetPosition().z;
        gpuLight.radius = light->GetRadius();
        gpuLight.strength = light->GetStrength();
        gpuLight.shadowMapDirty = true;
        gpuLight.lightIndex = lightIndex;
    }

    void SubmitDecalRenderItem(const RenderItem& renderItem) {
        g_decalRenderItems.push_back(renderItem);
    }

    void SubmitRenderItem(const RenderItem& renderItem) {
        g_renderItems.push_back(renderItem);
    }

    void SubmitRenderItem(const HouseRenderItem& renderItem) {
        g_houseRenderItems.push_back(renderItem);
    }

    void SubmitRenderItems(const std::vector<RenderItem>& renderItems) {
        g_renderItems.insert(g_renderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitRenderItems(const std::vector<HouseRenderItem>& renderItems) {
        g_houseRenderItems.insert(g_houseRenderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitOutlineRenderItem(const RenderItem& renderItem) {
        g_outlineRenderItems.push_back(renderItem);
    }

    void SubmitOutlineRenderItem(const HouseRenderItem& renderItem) {
        g_houseOutlineRenderItems.push_back(renderItem);
    }

    void SubmitOutlineRenderItems(const std::vector<RenderItem>& renderItems) {
        g_outlineRenderItems.insert(g_outlineRenderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitOutlineRenderItems(const std::vector<HouseRenderItem>& renderItems) {
        g_houseOutlineRenderItems.insert(g_houseOutlineRenderItems.begin(), renderItems.begin(), renderItems.end());
    }
}
