﻿#include "GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "API/OpenGL/GL_Util.h"
#include "API/OpenGL/Types/GL_indirectBuffer.hpp"
#include "API/OpenGL/Types/GL_pbo.hpp"
#include "API/OpenGL/Types/GL_shader.h"
#include "API/OpenGL/Types/GL_ssbo.hpp"
#include "AssetManagement/AssetManager.h"
#include "BackEnd/BackEnd.h"
#include "Audio/Audio.h"
#include "Core/Game.h"
#include "Config/Config.h"
#include "Input/Input.h"
#include "Player/Player.h"
#include "Renderer/RenderDataManager.h"
#include "Util/Util.h"
#include "UI/UIBackEnd.h"
#include "UI/TextBlitter.h"
#include "Types/Game/GameObject.h"
#include "../Hardcoded.hpp"
#include "../Timer.hpp"
#include <glm/gtx/matrix_decompose.hpp>

#include "../Editor/Editor.h"
#include "../Editor/Gizmo.h"
#include "../Renderer/RenderDataManager.h"
#include "../Viewport/ViewportManager.h"

#include "Input/Input.h"
#include "API/OpenGL/Types/GL_texture_readback.h"
#include "Tools/ImageTools.h"

namespace OpenGLRenderer {

    std::unordered_map<std::string, OpenGLShader> g_shaders;
    std::unordered_map<std::string, OpenGLFrameBuffer> g_frameBuffers;
    std::unordered_map<std::string, OpenGLShadowMap> g_shadowMaps;
    std::unordered_map<std::string, OpenGLCubemapView> g_cubemapViews;
    std::unordered_map<std::string, OpenGLSSBO> g_ssbos;
    std::unordered_map<std::string, OpenGLRasterizerState> g_rasterizerStates;
    std::unordered_map<std::string, OpenGLShadowCubeMapArray> g_shadowMapArrays;

    OpenGLFrameBuffer g_blurBuffers[4][4] = {};

    std::vector<float> g_shadowCascadeLevels{ FAR_PLANE / 50.0f, FAR_PLANE / 25.0f, FAR_PLANE / 10.0f, FAR_PLANE / 2.0f };
    const glm::vec3 g_lightDir = glm::normalize(glm::vec3(20.0f, 50, 20.0f));
    unsigned int g_lightFBO;
    unsigned int g_lightDepthMaps;
    constexpr unsigned int g_depthMapResolution = 4096;

    void LoadShaders();

    IndirectBuffer g_indirectBuffer;

    struct Cubemaps {
        OpenGLCubemapView g_skyboxView;
    } g_cubemaps;

    void ClearRenderTargets();

    void Init() {
        const Resolutions& resolutions = Config::GetResolutions();

        g_frameBuffers["GBuffer"] = OpenGLFrameBuffer("GBuffer", resolutions.gBuffer);
        g_frameBuffers["GBuffer"].CreateAttachment("BaseColor", GL_RGBA8);
        g_frameBuffers["GBuffer"].CreateAttachment("Normal", GL_RGBA16F);
        g_frameBuffers["GBuffer"].CreateAttachment("RMA", GL_RGBA8);
        g_frameBuffers["GBuffer"].CreateAttachment("FinalLighting", GL_RGBA16F);
        g_frameBuffers["GBuffer"].CreateAttachment("WorldSpacePosition", GL_RGBA32F);
        g_frameBuffers["GBuffer"].CreateAttachment("Emissive", GL_RGBA8);
        g_frameBuffers["GBuffer"].CreateAttachment("Glass", GL_RGBA8);
        g_frameBuffers["GBuffer"].CreateDepthAttachment(GL_DEPTH_COMPONENT32F);

        g_frameBuffers["WIP"] = OpenGLFrameBuffer("WIP", resolutions.gBuffer);
        g_frameBuffers["WIP"].CreateAttachment("WorldSpacePosition", GL_RGBA32F);

        g_frameBuffers["Outline"] = OpenGLFrameBuffer("Outline", resolutions.gBuffer);
        g_frameBuffers["Outline"].CreateAttachment("Mask", GL_R8);
        g_frameBuffers["Outline"].CreateAttachment("Result", GL_R8);

        g_frameBuffers["Hair"] = OpenGLFrameBuffer("Hair", resolutions.hair);
        g_frameBuffers["Hair"].CreateDepthAttachment(GL_DEPTH_COMPONENT32F);
        g_frameBuffers["Hair"].CreateAttachment("Lighting", GL_RGBA16F);
        g_frameBuffers["Hair"].CreateAttachment("ViewspaceDepth", GL_R32F);
        g_frameBuffers["Hair"].CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        g_frameBuffers["Hair"].CreateAttachment("Composite", GL_RGBA16F);

        g_frameBuffers["FinalImage"] = OpenGLFrameBuffer("FinalImage", resolutions.finalImage);
        g_frameBuffers["FinalImage"].CreateAttachment("Color", GL_RGBA16F);
        g_frameBuffers["FinalImage"].CreateAttachment("ViewportIndex", GL_R8UI);

        g_frameBuffers["UI"] = OpenGLFrameBuffer("UI", resolutions.ui);
        g_frameBuffers["UI"].CreateAttachment("Color", GL_RGBA8, GL_NEAREST, GL_NEAREST);

        g_frameBuffers["World"] = OpenGLFrameBuffer("World", 1, 1);
        g_frameBuffers["World"].CreateAttachment("HeightMap", GL_R16F);

        g_frameBuffers["HeightMapBlitBuffer"] = OpenGLFrameBuffer("HeightMapBlitBuffer", HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE);

        g_frameBuffers["HeightMap"] = OpenGLFrameBuffer("HeightMap", HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE);
        g_frameBuffers["HeightMap"].CreateAttachment("Color", GL_R16F);

        g_frameBuffers["FlashlightShadowMap"] = OpenGLFrameBuffer("Flashlight", FLASHLIGHT_SHADOWMAP_SIZE, FLASHLIGHT_SHADOWMAP_SIZE);
        g_frameBuffers["FlashlightShadowMap"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER, glm::vec4(1.0f));

        g_shadowMaps["FlashlightShadowMaps"] = OpenGLShadowMap("FlashlightShadowMaps", 1024, 1024, 4);

        // Create ssbos
        g_ssbos["Samplers"] = OpenGLSSBO(sizeof(glm::uvec2), GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["ViewportData"] = OpenGLSSBO(sizeof(ViewportData) * 4, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["RendererData"] = OpenGLSSBO(sizeof(RendererData), GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["InstanceData"] = OpenGLSSBO(sizeof(RenderItem) * MAX_INSTANCE_DATA_COUNT, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["SkinningTransforms"] = OpenGLSSBO(sizeof(glm::mat4) * MAX_ANIMATED_TRANSFORMS, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["LightSpaceMatrices"] = OpenGLSSBO(sizeof(glm::mat4) * MAX_VIEWPORT_COUNT * SHADOW_CASCADE_COUNT, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["Lights"] = OpenGLSSBO(sizeof(GPULight) * MAX_GPU_LIGHTS, GL_DYNAMIC_STORAGE_BIT);

        int tileXCount = g_frameBuffers["GBuffer"].GetWidth() / TILE_SIZE;
        int tileYCount = g_frameBuffers["GBuffer"].GetHeight() / TILE_SIZE;
        int tileCount = tileXCount * tileYCount;
        g_ssbos["TileLightData"] = OpenGLSSBO(tileCount * sizeof(TileLightData), GL_DYNAMIC_STORAGE_BIT);

        // Preallocate the indirect command buffer
        g_indirectBuffer.PreAllocate(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT);

        LoadShaders();

        // Allocate shadow map array memory
        g_shadowMapArrays["HiRes"] = OpenGLShadowCubeMapArray();
        g_shadowMapArrays["HiRes"].Init(SHADOWMAP_HI_RES_COUNT, SHADOW_MAP_HI_RES_SIZE);
    }

    void InitMain() {
        InitRasterizerStates();

        // Attempt to load skybox
        std::vector<Texture*> textures = {
            AssetManager::GetTextureByName("NightSky_Right"),
            AssetManager::GetTextureByName("NightSky_Left"),
            AssetManager::GetTextureByName("NightSky_Top"),
            AssetManager::GetTextureByName("NightSky_Bottom"),
            AssetManager::GetTextureByName("NightSky_Front"),
            AssetManager::GetTextureByName("NightSky_Back")
        };
        std::vector<GLuint> texturesHandles;
        for (Texture* texture : textures) {
            if (!texture) continue;
            texturesHandles.push_back(texture->GetGLTexture().GetHandle());
        }
        if (texturesHandles.size() == 6) {
            g_cubemapViews["SkyboxNightSky"] = OpenGLCubemapView(texturesHandles);
        }

        CreateBlurBuffers();
    }

    void LoadShaders() {
        g_shaders["BlurHorizontal"] = OpenGLShader({ "GL_blur_horizontal.vert", "GL_blur.frag" });
        g_shaders["BlurVertical"] = OpenGLShader({ "GL_blur_vertical.vert", "GL_blur.frag" });
        g_shaders["ComputeSkinning"] = OpenGLShader({ "GL_compute_skinning.comp" });
        g_shaders["DebugTextured"] = OpenGLShader({ "GL_debug_textured.vert", "GL_debug_textured.frag" });
        g_shaders["DebugTileView"] = OpenGLShader({ "GL_debug_tile_view.comp" });
        g_shaders["DebugVertex"] = OpenGLShader({ "gl_debug_vertex.vert", "gl_debug_vertex.frag" });
        g_shaders["Decals"] = OpenGLShader({ "GL_decals.vert", "GL_decals.frag" });
        g_shaders["EditorMesh"] = OpenGLShader({ "GL_editor_mesh.vert", "GL_editor_mesh.frag" });
        g_shaders["EmissiveComposite"] = OpenGLShader({ "GL_emissive_composite.comp" });
        g_shaders["GBuffer"] = OpenGLShader({ "GL_GBuffer.vert", "GL_gBuffer.frag" });
        g_shaders["Gizmo"] = OpenGLShader({ "GL_gizmo.vert", "GL_gizmo.frag" });
        g_shaders["Glass"] = OpenGLShader({ "GL_glass.vert", "GL_glass.frag" });
        g_shaders["GlassComposite"] = OpenGLShader({ "GL_glass_composite.comp" });
        g_shaders["Grass"] = OpenGLShader({ "GL_grass.vert", "GL_grass.frag" });
        g_shaders["GrassGeometryGeneration"] = OpenGLShader({ "GL_grass_geometry_generation.comp" });
        g_shaders["GrassPositionGeneration"] = OpenGLShader({ "GL_grass_position_generation.comp" });
        g_shaders["HairDepthPeel"] = OpenGLShader({ "GL_hair_depth_peel.vert", "GL_hair_depth_peel.frag" });
        g_shaders["HairFinalComposite"] = OpenGLShader({ "GL_hair_final_composite.comp" });
        g_shaders["HairLayerComposite"] = OpenGLShader({ "GL_hair_layer_composite.comp" });
        g_shaders["HairLighting"] = OpenGLShader({ "GL_hair_lighting.vert", "GL_hair_lighting.frag" });
        g_shaders["HeightMapColor"] = OpenGLShader({ "GL_heightmap_color.vert", "GL_heightmap_color.frag" });
        g_shaders["HeightMapImageGeneration"] = OpenGLShader({ "GL_heightmap_image_generation.comp" });
        g_shaders["HeightMapPhysxTextureGeneration"] = OpenGLShader({ "GL_heightmap_physx_texture_generation.comp" });
        g_shaders["HeightMapVertexGeneration"] = OpenGLShader({ "GL_heightmap_vertex_generation.comp" });
        g_shaders["HeightMapPaint"] = OpenGLShader({ "GL_heightmap_paint.comp" });
        g_shaders["LightCulling"] = OpenGLShader({ "GL_light_culling.comp" });
        g_shaders["Lighting"] = OpenGLShader({ "GL_lighting.comp" });
        g_shaders["Outline"] = OpenGLShader({ "GL_outline.vert", "GL_outline.frag" });
        g_shaders["OutlineComposite"] = OpenGLShader({ "GL_outline_composite.comp" });
        g_shaders["OutlineMask"] = OpenGLShader({ "GL_outline_mask.vert", "GL_outline_mask.frag" });
        g_shaders["ShadowMap"] = OpenGLShader({ "GL_shadow_map.vert", "GL_shadow_map.frag" });
        g_shaders["ShadowCubeMap"] = OpenGLShader({ "GL_shadow_cube_map.vert", "GL_shadow_cube_map.frag" });
        g_shaders["SolidColor"] = OpenGLShader({ "GL_solid_color.vert", "GL_solid_color.frag" });
        g_shaders["Skybox"] = OpenGLShader({ "GL_skybox.vert", "GL_skybox.frag" });
        g_shaders["SpriteSheet"] = OpenGLShader({ "GL_sprite_sheet.vert", "GL_sprite_sheet.frag" });
        g_shaders["UI"] = OpenGLShader({ "GL_ui.vert", "GL_ui.frag" });
        g_shaders["Winston"] = OpenGLShader({ "GL_winston.vert", "GL_winston.frag" });
        g_shaders["CSMDepth"] = OpenGLShader({ "GL_csm_depth.vert", "GL_csm_depth.frag", "GL_csm_depth.geom" });
        g_shaders["ZeroOut"] = OpenGLShader({ "GL_zero_out.comp" });
    }

    void UpdateSSBOS() {
        const std::vector<GLuint64>& bindlessTextureIDs = OpenGLBackEnd::GetBindlessTextureIDs();
        g_ssbos["Samplers"].Update(bindlessTextureIDs.size() * sizeof(GLuint64), (void*)&bindlessTextureIDs[0]);
        g_ssbos["Samplers"].Bind(0);

        const RendererData& rendererData = RenderDataManager::GetRendererData();
        g_ssbos["RendererData"].Update(sizeof(RendererData), (void*)&rendererData);
        g_ssbos["RendererData"].Bind(1);

        const std::vector<ViewportData>& playerData = RenderDataManager::GetViewportData();
        g_ssbos["ViewportData"].Update(playerData.size() * sizeof(ViewportData), (void*)&playerData[0]);
        g_ssbos["ViewportData"].Bind(2);

        const std::vector<RenderItem>& instanceData = RenderDataManager::GetInstanceData();
        g_ssbos["InstanceData"].Update(instanceData.size() * sizeof(RenderItem), (void*)&instanceData[0]);
        g_ssbos["InstanceData"].Bind(3);

        const std::vector<GPULight>& gpuLightsHighRes = RenderDataManager::GetGPULightsHighRes();
        g_ssbos["Lights"].Update(gpuLightsHighRes.size() * sizeof(GPULight), (void*)&gpuLightsHighRes[0]);
        g_ssbos["Lights"].Bind(4);

        g_ssbos["TileLightData"].Bind(5);
    }

    void RenderGame() {

        glDisable(GL_DITHER);

        ComputeSkinningPass();
        ClearRenderTargets();
        UpdateSSBOS();
        RenderShadowMaps();
        SkyBoxPass();
        HeightMapPass();
        GrassPass();
        GeometryPass();
        //TextureReadBackPass();
        LightCullingPass();
        LightingPass();
        GlassPass();
        DecalPass();
        EmissivePass();
        HairPass();
        DebugTileViewPass();
        WinstonPass();
        SpriteSheetPass();
        DebugPass();
        EditorPass();
        OutlinePass();

        OpenGLFrameBuffer& gBuffer = g_frameBuffers["GBuffer"];
        OpenGLFrameBuffer& hairFrameBuffer = g_frameBuffers["Hair"];
        OpenGLFrameBuffer& finalImageBuffer = g_frameBuffers["FinalImage"];

        // Downscale blit
        OpenGLRenderer::BlitFrameBuffer(&gBuffer, &finalImageBuffer, "FinalLighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Blit to swapchain
        OpenGLRenderer::BlitToDefaultFrameBuffer(&finalImageBuffer, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

                 
        UIPass();
        ImGuiPass();

        //OpenGLFrameBuffer& worldFramebuffer = g_frameBuffers["World"];
        //BlitRect srcRect;
        //srcRect.x0 = 0;
        //srcRect.y0 = 0;
        //srcRect.x1 = worldFramebuffer.GetWidth();
        //srcRect.y1 = worldFramebuffer.GetHeight();
        //
        //BlitRect dstRect = srcRect;
        //dstRect.x1 = worldFramebuffer.GetWidth() * 0.5f;
        //dstRect.y1 = worldFramebuffer.GetHeight() * 0.5f;
        //OpenGLRenderer::BlitToDefaultFrameBuffer(&worldFramebuffer, "HeightMap", srcRect, dstRect, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        

        //BlitRect srcRect;
        //srcRect.x0 = 0;
        //srcRect.y0 = 0;
        //srcRect.x1 = gBuffer.GetWidth();
        //srcRect.y1 = gBuffer.GetHeight();
        //BlitRect dstRect = srcRect;
        //dstRect.x1 = gBuffer.GetWidth() * 0.6f;
        //dstRect.y1 = gBuffer.GetHeight() * 0.6f;
        //OpenGLRenderer::BlitToDefaultFrameBuffer(&gBuffer, "Glass", srcRect, dstRect, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        
    }

    void ClearRenderTargets() {
        OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer* finalImageFBO = GetFrameBuffer("FinalImage");

        // GBuffer
        glDepthMask(GL_TRUE);
        gBuffer->Bind();
        gBuffer->ClearAttachment("BaseColor", 0, 0, 0, 0);
        gBuffer->ClearAttachment("Normal", 0, 0, 0, 0);
        gBuffer->ClearAttachment("RMA", 0, 0, 0, 0);
        gBuffer->ClearAttachment("WorldSpacePosition", 0, 0);
        gBuffer->ClearAttachment("Emissive", 0, 0, 0, 0);
        gBuffer->ClearAttachment("Glass", 0, 1, 0, 0);
        gBuffer->ClearDepthAttachment();

        // Viewport index
        for (unsigned int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::ClearFrameBufferByViewportUInt(finalImageFBO, "ViewportIndex", viewport, i);
            }
        }
    }

    void MultiDrawIndirect(const std::vector<DrawIndexedIndirectCommand>& commands) {
        if (commands.size()) {
            // Feed the draw command data to the gpu
            g_indirectBuffer.Bind();
            g_indirectBuffer.Update(sizeof(DrawIndexedIndirectCommand) * commands.size(), commands.data());

            // Fire of the commands
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (GLvoid*)0, (GLsizei)commands.size(), 0);
        }
    }

    void SplitMultiDrawIndirect(OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand>& commands) {
        const std::vector<RenderItem>& instanceData = RenderDataManager::GetInstanceData();

        for (const DrawIndexedIndirectCommand& command : commands) {
            int viewportIndex = command.baseInstance >> VIEWPORT_INDEX_SHIFT;
            int instanceOffset = command.baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);

            for (GLuint i = 0; i < command.instanceCount; ++i) {
                const RenderItem& renderItem = instanceData[instanceOffset + i];

                shader->SetInt("u_viewportIndex", viewportIndex);
                shader->SetInt("u_globalInstanceIndex", instanceOffset + i);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

                glDrawElementsBaseVertex(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (GLvoid*)(command.firstIndex * sizeof(GLuint)), command.baseVertex);
            }
        }
    }


    void HotloadShaders() {
        bool allSucceeded = true;
        for (auto& [_, shader] : g_shaders) {
            if (!shader.Hotload()) {
                allSucceeded = false;
            }
        }
        if (allSucceeded) {
            std::cout << "Hotloaded shaders\n";
        }
    }

    void DrawQuad() {
        static Mesh* mesh = AssetManager::GetMeshByModelNameMeshName("Primitives", "Quad");
        glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
    }

    void CreateBlurBuffers() {
        const Resolutions& resolutions = Config::GetResolutions();

        // Iterate each viewport
        for (int x = 0; x < 4; x++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(x);

            // Start the first blur buffer at the full viewport dimensions
            SpaceCoords spaceCoords = viewport->GetGBufferSpaceCoords();
            float width = resolutions.gBuffer.x;
            float height = resolutions.gBuffer.y;

            // Create framebuffers, downscale by 50% each time
            for (int y = 0; y < 4; y++) {

                // Clean up existing framebuffer
                g_blurBuffers[x][y].Create("BlurBuffer", width, height);
                g_blurBuffers[x][y].CreateAttachment("ColorA", GL_RGBA8);
                g_blurBuffers[x][y].CreateAttachment("ColorB", GL_RGBA8);
                width *= 0.5f;
                height *= 0.5f;
            }
        }
    }
    
    void CreateSSBO(const std::string& name, float size, GLbitfield flags) {
        g_ssbos[name] = OpenGLSSBO(size, flags);
    }

    OpenGLShader* GetShader(const std::string& name) {
        auto it = g_shaders.find(name);
        return (it != g_shaders.end()) ? &it->second : nullptr;
    }

    OpenGLFrameBuffer* GetFrameBuffer(const std::string& name) {
        auto it = g_frameBuffers.find(name);
        return (it != g_frameBuffers.end()) ? &it->second : nullptr;
    }

    OpenGLShadowMap* GetShadowMap(const std::string& name) {
        auto it = g_shadowMaps.find(name);
        return (it != g_shadowMaps.end()) ? &it->second : nullptr;
    }

    OpenGLShadowCubeMapArray* GetShadowMapArray(const std::string& name) {
        auto it = g_shadowMapArrays.find(name);
        return (it != g_shadowMapArrays.end()) ? &it->second : nullptr;
    }

    OpenGLFrameBuffer* GetBlurBuffer(int viewportIndex, int bufferIndex) {
        if (viewportIndex < 0 || viewportIndex >= 4) return nullptr;
        if (bufferIndex < 0 || bufferIndex >= 4) return nullptr;
        return &g_blurBuffers[viewportIndex][bufferIndex];
    }

    OpenGLSSBO* GetSSBO(const std::string& name) {
        auto it = g_ssbos.find(name);
        return (it != g_ssbos.end()) ? &it->second : nullptr;
    }

    OpenGLCubemapView* GetCubemapView(const std::string& name) {
        auto it = g_cubemapViews.find(name);
        return (it != g_cubemapViews.end()) ? &it->second : nullptr;
    }

    OpenGLRasterizerState* GetRasterizerState(const std::string& name) {
        auto it = g_rasterizerStates.find(name);
        return (it != g_rasterizerStates.end()) ? &it->second : nullptr;
    }

    OpenGLRasterizerState* CreateRasterizerState(const std::string& name) {
        g_rasterizerStates[name] = OpenGLRasterizerState();
        return &g_rasterizerStates[name];
    }

    void SetRasterizerState(const std::string& name) {
        OpenGLRasterizerState* rasterizerState = GetRasterizerState(name);
        if (!rasterizerState) {
            std::cout << "OpenGLRenderer::SetRasterizerState(const std::string& name) failed! " << name << " does not exist!\n";
            return;
        }

        rasterizerState->blendEnable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        rasterizerState->cullfaceEnable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        rasterizerState->depthMask ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
        rasterizerState->depthTestEnabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);

        if (rasterizerState->blendEnable) {
            glBlendFunc(rasterizerState->blendFuncSrcfactor, rasterizerState->blendFuncDstfactor);
        }
        if (rasterizerState->depthTestEnabled) {
            glDepthFunc(rasterizerState->depthFunc);
        }
        if (rasterizerState->pointSize > 1.0f) {
            glPointSize(rasterizerState->pointSize);
        }
    }

    std::vector<glm::mat4> getLightSpaceMatrices();

    //void CascadedShadowMappingPass() {
    //
    //    const std::vector<glm::mat4> lightMatrices = getLightSpaceMatrices();
    //
    //    g_ssbos["LightSpaceMatrices"].Update(lightMatrices.size() * sizeof(glm::mat4), (void*)&lightMatrices[0]);
    //    g_ssbos["LightSpaceMatrices"].Bind(0);           
    //
    //
    //    for (int i = 0; i < lightMatrices.size(); i++) {
    //
    //        std::cout << i << "\n";
    //        std::cout << Util::Mat4ToString(lightMatrices[i]);
    //
    //        std::cout << "\n\n";
    //    }
    //
    //
    //    std::cout <<  "\n";
    //
    //    OpenGLShader* csmDepthShader = GetShader("CSMDepth");        
    //    csmDepthShader->Use();
    //
    //    glBindFramebuffer(GL_FRAMEBUFFER, g_lightFBO);
    //    glViewport(0, 0, g_depthMapResolution, g_depthMapResolution);
    //    glClear(GL_DEPTH_BUFFER_BIT);
    //    glCullFace(GL_FRONT);  // peter panning
    //
    //    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    //
    //    for (RenderItem& renderItem : Scene::GetRenderItems()) {
    //        Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
    //        csmDepthShader->SetMat4("model", renderItem.modelMatrix);
    //        glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex, 0);
    //    }
    //
    //    glCullFace(GL_BACK);
    //    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //}











    std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& projview)
    {
        const auto inv = glm::inverse(projview);

        std::vector<glm::vec4> frustumCorners;
        for (unsigned int x = 0; x < 2; ++x)
        {
            for (unsigned int y = 0; y < 2; ++y)
            {
                for (unsigned int z = 0; z < 2; ++z)
                {
                    const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    frustumCorners.push_back(pt / pt.w);
                }
            }
        }

        return frustumCorners;
    }


    std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
    {
        return getFrustumCornersWorldSpace(proj * view);
    }

    glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane) {

        const std::vector<ViewportData>& playerData = RenderDataManager::GetViewportData();
        glm::mat4 projectionMatrx = playerData[0].projection;
        glm::mat4 viewMatrix = playerData[0].view;

        std::vector<glm::vec4> corners = getFrustumCornersWorldSpace(projectionMatrx, viewMatrix);

        glm::vec3 center = glm::vec3(0, 0, 0);
        for (const auto& v : corners) {
            center += glm::vec3(v);

        }
        center /= corners.size();




        for (glm::vec4& corner : corners) {
            DrawPoint(corner, RED);
        }

        DrawPoint(glm::vec3(0, 0, 0), YELLOW);


        const auto lightView = glm::lookAt(center + g_lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        for (const auto& v : corners) {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        // Tune this parameter according to the scene
        constexpr float zMult = 10.0f;
        if (minZ < 0) {
            minZ *= zMult;
        }
        else {
            minZ /= zMult;
        }
        if (maxZ < 0) {
            maxZ /= zMult;
        }
        else {
            maxZ *= zMult;
        }

       // const glm::mat4 lightProjection = glm::orthoZO(minX, maxX, minY, maxY, minZ, maxZ);
        const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        return lightProjection * lightView;
    }

    std::vector<glm::mat4> getLightSpaceMatrices() {
        std::vector<glm::mat4> ret;
        for (size_t i = 0; i < g_shadowCascadeLevels.size() + 1; ++i) {
            if (i == 0)  {
                ret.push_back(getLightSpaceMatrix(NEAR_PLANE, g_shadowCascadeLevels[i]));
            }
            else if (i < g_shadowCascadeLevels.size()) {
                ret.push_back(getLightSpaceMatrix(g_shadowCascadeLevels[i - 1], g_shadowCascadeLevels[i]));
            }
            else {
                ret.push_back(getLightSpaceMatrix(g_shadowCascadeLevels[i - 1], FAR_PLANE));
            }
        }
        return ret;
    }




















}
























//if (rasterizerState->depthTestEnabled) {
//    glEnable(GL_DEPTH_TEST);
//}
//else {
//    glDisable(GL_DEPTH_TEST);
//}
//