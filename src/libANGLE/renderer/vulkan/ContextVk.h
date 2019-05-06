//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ContextVk.h:
//    Defines the class interface for ContextVk, implementing ContextImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_CONTEXTVK_H_
#define LIBANGLE_RENDERER_VULKAN_CONTEXTVK_H_

#include <vulkan/vulkan.h>

#include "common/PackedEnums.h"
#include "libANGLE/renderer/ContextImpl.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace angle
{
struct FeaturesVk;
}

namespace rx
{
class RendererVk;
class WindowSurfaceVk;

class ContextVk : public ContextImpl, public vk::Context, public vk::CommandBufferOwner
{
  public:
    ContextVk(const gl::State &state, gl::ErrorSet *errorSet, RendererVk *renderer);
    ~ContextVk() override;

    angle::Result initialize() override;

    void onDestroy(const gl::Context *context) override;

    // Flush and finish.
    angle::Result flush(const gl::Context *context) override;
    angle::Result finish(const gl::Context *context) override;

    // Drawing methods.
    angle::Result drawArrays(const gl::Context *context,
                             gl::PrimitiveMode mode,
                             GLint first,
                             GLsizei count) override;
    angle::Result drawArraysInstanced(const gl::Context *context,
                                      gl::PrimitiveMode mode,
                                      GLint first,
                                      GLsizei count,
                                      GLsizei instanceCount) override;

    angle::Result drawElements(const gl::Context *context,
                               gl::PrimitiveMode mode,
                               GLsizei count,
                               gl::DrawElementsType type,
                               const void *indices) override;
    angle::Result drawElementsInstanced(const gl::Context *context,
                                        gl::PrimitiveMode mode,
                                        GLsizei count,
                                        gl::DrawElementsType type,
                                        const void *indices,
                                        GLsizei instanceCount) override;
    angle::Result drawRangeElements(const gl::Context *context,
                                    gl::PrimitiveMode mode,
                                    GLuint start,
                                    GLuint end,
                                    GLsizei count,
                                    gl::DrawElementsType type,
                                    const void *indices) override;
    angle::Result drawArraysIndirect(const gl::Context *context,
                                     gl::PrimitiveMode mode,
                                     const void *indirect) override;
    angle::Result drawElementsIndirect(const gl::Context *context,
                                       gl::PrimitiveMode mode,
                                       gl::DrawElementsType type,
                                       const void *indirect) override;

    // Device loss
    gl::GraphicsResetStatus getResetStatus() override;

    // Vendor and description strings.
    std::string getVendorString() const override;
    std::string getRendererDescription() const override;

    // EXT_debug_marker
    void insertEventMarker(GLsizei length, const char *marker) override;
    void pushGroupMarker(GLsizei length, const char *marker) override;
    void popGroupMarker() override;

    // KHR_debug
    void pushDebugGroup(GLenum source, GLuint id, const std::string &message) override;
    void popDebugGroup() override;

    bool isViewportFlipEnabledForDrawFBO() const;
    bool isViewportFlipEnabledForReadFBO() const;

    // State sync with dirty bits.
    angle::Result syncState(const gl::Context *context,
                            const gl::State::DirtyBits &dirtyBits,
                            const gl::State::DirtyBits &bitMask) override;

    // Disjoint timer queries
    GLint getGPUDisjoint() override;
    GLint64 getTimestamp() override;

    // Context switching
    angle::Result onMakeCurrent(const gl::Context *context) override;
    angle::Result onUnMakeCurrent(const gl::Context *context) override;

    // Native capabilities, unmodified by gl::Context.
    gl::Caps getNativeCaps() const override;
    const gl::TextureCapsMap &getNativeTextureCaps() const override;
    const gl::Extensions &getNativeExtensions() const override;
    const gl::Limitations &getNativeLimitations() const override;

    // Shader creation
    CompilerImpl *createCompiler() override;
    ShaderImpl *createShader(const gl::ShaderState &state) override;
    ProgramImpl *createProgram(const gl::ProgramState &state) override;

    // Framebuffer creation
    FramebufferImpl *createFramebuffer(const gl::FramebufferState &state) override;

    // Texture creation
    TextureImpl *createTexture(const gl::TextureState &state) override;

    // Renderbuffer creation
    RenderbufferImpl *createRenderbuffer(const gl::RenderbufferState &state) override;

    // Buffer creation
    BufferImpl *createBuffer(const gl::BufferState &state) override;

    // Vertex Array creation
    VertexArrayImpl *createVertexArray(const gl::VertexArrayState &state) override;

    // Query and Fence creation
    QueryImpl *createQuery(gl::QueryType type) override;
    FenceNVImpl *createFenceNV() override;
    SyncImpl *createSync() override;

    // Transform Feedback creation
    TransformFeedbackImpl *createTransformFeedback(
        const gl::TransformFeedbackState &state) override;

    // Sampler object creation
    SamplerImpl *createSampler(const gl::SamplerState &state) override;

    // Program Pipeline object creation
    ProgramPipelineImpl *createProgramPipeline(const gl::ProgramPipelineState &data) override;

    // Path object creation
    std::vector<PathImpl *> createPaths(GLsizei) override;

    // Memory object creation.
    MemoryObjectImpl *createMemoryObject() override;

    // Semaphore creation.
    SemaphoreImpl *createSemaphore() override;

    angle::Result dispatchCompute(const gl::Context *context,
                                  GLuint numGroupsX,
                                  GLuint numGroupsY,
                                  GLuint numGroupsZ) override;
    angle::Result dispatchComputeIndirect(const gl::Context *context, GLintptr indirect) override;

    angle::Result memoryBarrier(const gl::Context *context, GLbitfield barriers) override;
    angle::Result memoryBarrierByRegion(const gl::Context *context, GLbitfield barriers) override;

    VkDevice getDevice() const;

    ANGLE_INLINE const angle::FeaturesVk &getFeatures() const { return mRenderer->getFeatures(); }

    ANGLE_INLINE void invalidateVertexAndIndexBuffers()
    {
        // TODO: Make the pipeline invalidate more fine-grained. Only need to dirty here if PSO
        //  VtxInput state (stride, fmt, inputRate...) has changed. http://anglebug.com/3256
        invalidateCurrentPipeline();
        mDirtyBits.set(DIRTY_BIT_VERTEX_BUFFERS);
        mDirtyBits.set(DIRTY_BIT_INDEX_BUFFER);
    }

    ANGLE_INLINE void onVertexAttributeChange(size_t attribIndex,
                                              GLuint stride,
                                              GLuint divisor,
                                              VkFormat format,
                                              GLuint relativeOffset)
    {
        invalidateVertexAndIndexBuffers();
        mGraphicsPipelineDesc->updateVertexInput(&mGraphicsPipelineTransition,
                                                 static_cast<uint32_t>(attribIndex), stride,
                                                 divisor, format, relativeOffset);
    }

    void invalidateDefaultAttribute(size_t attribIndex);
    void invalidateDefaultAttributes(const gl::AttributesMask &dirtyMask);
    void onFramebufferChange(const vk::RenderPassDesc &renderPassDesc);

    vk::DynamicDescriptorPool *getDynamicDescriptorPool(uint32_t descriptorSetIndex);
    vk::DynamicQueryPool *getQueryPool(gl::QueryType queryType);

    const VkClearValue &getClearColorValue() const;
    const VkClearValue &getClearDepthStencilValue() const;
    VkColorComponentFlags getClearColorMask() const;
    angle::Result getIncompleteTexture(const gl::Context *context,
                                       gl::TextureType type,
                                       gl::Texture **textureOut);
    void updateColorMask(const gl::BlendState &blendState);
    void updateSampleMask(const gl::State &glState);

    void handleError(VkResult errorCode,
                     const char *file,
                     const char *function,
                     unsigned int line) override;
    const gl::ActiveTextureArray<TextureVk *> &getActiveTextures() const;

    void setIndexBufferDirty() { mDirtyBits.set(DIRTY_BIT_INDEX_BUFFER); }

    angle::Result flushImpl();
    angle::Result finishImpl();

    const vk::CommandPool &getCommandPool() const;

    Serial getCurrentQueueSerial() const { return mCurrentQueueSerial; }
    Serial getLastSubmittedQueueSerial() const { return mLastSubmittedQueueSerial; }
    Serial getLastCompletedQueueSerial() const { return mLastCompletedQueueSerial; }

    bool isSerialInUse(Serial serial) const;

    template <typename T>
    void releaseObject(Serial resourceSerial, T *object)
    {
        if (!isSerialInUse(resourceSerial))
        {
            object->destroy(getDevice());
        }
        else
        {
            object->dumpResources(resourceSerial, &mGarbage);
        }
    }

    // Check to see which batches have finished completion (forward progress for
    // mLastCompletedQueueSerial, for example for when the application busy waits on a query
    // result).
    angle::Result checkCompletedCommands();

    // Wait for completion of batches until (at least) batch with given serial is finished.
    angle::Result finishToSerial(Serial serial);
    // A variant of finishToSerial that can time out.  Timeout status returned in outTimedOut.
    angle::Result finishToSerialOrTimeout(Serial serial, uint64_t timeout, bool *outTimedOut);

    angle::Result getCompatibleRenderPass(const vk::RenderPassDesc &desc,
                                          vk::RenderPass **renderPassOut);
    angle::Result getRenderPassWithOps(const vk::RenderPassDesc &desc,
                                       const vk::AttachmentOpsArray &ops,
                                       vk::RenderPass **renderPassOut);

    // Get (or allocate) the fence that will be signaled on next submission.
    angle::Result getNextSubmitFence(vk::Shared<vk::Fence> *sharedFenceOut);
    vk::Shared<vk::Fence> getLastSubmittedFence() const;

    // This should only be called from ResourceVk.
    vk::CommandGraph *getCommandGraph();

    vk::ShaderLibrary &getShaderLibrary() { return mShaderLibrary; }
    UtilsVk &getUtils() { return mUtils; }

    angle::Result getTimestamp(uint64_t *timestampOut);

    // Create Begin/End/Instant GPU trace events, which take their timestamps from GPU queries.
    // The events are queued until the query results are available.  Possible values for `phase`
    // are TRACE_EVENT_PHASE_*
    ANGLE_INLINE angle::Result traceGpuEvent(vk::PrimaryCommandBuffer *commandBuffer,
                                             char phase,
                                             const char *name)
    {
        if (mGpuEventsEnabled)
            return traceGpuEventImpl(commandBuffer, phase, name);
        return angle::Result::Continue;
    }

    RenderPassCache &getRenderPassCache() { return mRenderPassCache; }

  private:
    // Dirty bits.
    enum DirtyBitType : size_t
    {
        DIRTY_BIT_DEFAULT_ATTRIBS,
        DIRTY_BIT_PIPELINE,
        DIRTY_BIT_TEXTURES,
        DIRTY_BIT_VERTEX_BUFFERS,
        DIRTY_BIT_INDEX_BUFFER,
        DIRTY_BIT_DRIVER_UNIFORMS,
        DIRTY_BIT_UNIFORM_BUFFERS,
        DIRTY_BIT_DESCRIPTOR_SETS,
        DIRTY_BIT_MAX,
    };

    using DirtyBits = angle::BitSet<DIRTY_BIT_MAX>;

    using DirtyBitHandler = angle::Result (ContextVk::*)(const gl::Context *,
                                                         vk::CommandBuffer *commandBuffer);

    std::array<DirtyBitHandler, DIRTY_BIT_MAX> mDirtyBitHandlers;

    angle::Result setupDraw(const gl::Context *context,
                            gl::PrimitiveMode mode,
                            GLint firstVertex,
                            GLsizei vertexOrIndexCount,
                            GLsizei instanceCount,
                            gl::DrawElementsType indexTypeOrInvalid,
                            const void *indices,
                            DirtyBits dirtyBitMask,
                            vk::CommandBuffer **commandBufferOut);
    angle::Result setupIndexedDraw(const gl::Context *context,
                                   gl::PrimitiveMode mode,
                                   GLsizei indexCount,
                                   GLsizei instanceCount,
                                   gl::DrawElementsType indexType,
                                   const void *indices,
                                   vk::CommandBuffer **commandBufferOut);
    angle::Result setupLineLoopDraw(const gl::Context *context,
                                    gl::PrimitiveMode mode,
                                    GLint firstVertex,
                                    GLsizei vertexOrIndexCount,
                                    gl::DrawElementsType indexTypeOrInvalid,
                                    const void *indices,
                                    vk::CommandBuffer **commandBufferOut);

    void updateViewport(FramebufferVk *framebufferVk,
                        const gl::Rectangle &viewport,
                        float nearPlane,
                        float farPlane,
                        bool invertViewport);
    void updateDepthRange(float nearPlane, float farPlane);
    void updateScissor(const gl::State &glState);
    void updateFlipViewportDrawFramebuffer(const gl::State &glState);
    void updateFlipViewportReadFramebuffer(const gl::State &glState);

    angle::Result updateActiveTextures(const gl::Context *context);
    angle::Result updateDefaultAttribute(size_t attribIndex);

    ANGLE_INLINE void invalidateCurrentPipeline() { mDirtyBits.set(DIRTY_BIT_PIPELINE); }

    void invalidateCurrentTextures();
    void invalidateCurrentUniformBuffers();
    void invalidateDriverUniforms();

    angle::Result handleDirtyDefaultAttribs(const gl::Context *context,
                                            vk::CommandBuffer *commandBuffer);
    angle::Result handleDirtyPipeline(const gl::Context *context, vk::CommandBuffer *commandBuffer);
    angle::Result handleDirtyTextures(const gl::Context *context, vk::CommandBuffer *commandBuffer);
    angle::Result handleDirtyVertexBuffers(const gl::Context *context,
                                           vk::CommandBuffer *commandBuffer);
    angle::Result handleDirtyIndexBuffer(const gl::Context *context,
                                         vk::CommandBuffer *commandBuffer);
    angle::Result handleDirtyDriverUniforms(const gl::Context *context,
                                            vk::CommandBuffer *commandBuffer);
    angle::Result handleDirtyUniformBuffers(const gl::Context *context,
                                            vk::CommandBuffer *commandBuffer);
    angle::Result handleDirtyDescriptorSets(const gl::Context *context,
                                            vk::CommandBuffer *commandBuffer);

    angle::Result submitFrame(const VkSubmitInfo &submitInfo,
                              vk::PrimaryCommandBuffer &&commandBuffer);
    void freeAllInFlightResources();
    angle::Result flushCommandGraph(vk::PrimaryCommandBuffer *commandBatch);

    angle::Result synchronizeCpuGpuTime(const vk::Semaphore *waitSemaphore,
                                        const vk::Semaphore *signalSemaphore);
    angle::Result traceGpuEventImpl(vk::PrimaryCommandBuffer *commandBuffer,
                                    char phase,
                                    const char *name);
    angle::Result checkCompletedGpuEvents();
    void flushGpuEvents(double nextSyncGpuTimestampS, double nextSyncCpuTimestampS);

    void handleDeviceLost();

    vk::PipelineHelper *mCurrentPipeline;
    gl::PrimitiveMode mCurrentDrawMode;

    WindowSurfaceVk *mCurrentWindowSurface;

    // Keep a cached pipeline description structure that can be used to query the pipeline cache.
    // Kept in a pointer so allocations can be aligned, and structs can be portably packed.
    std::unique_ptr<vk::GraphicsPipelineDesc> mGraphicsPipelineDesc;
    vk::GraphicsPipelineTransitionBits mGraphicsPipelineTransition;

    // The descriptor pools are externally sychronized, so cannot be accessed from different
    // threads simultaneously. Hence, we keep them in the ContextVk instead of the RendererVk.
    // Note that this implementation would need to change in shared resource scenarios. Likely
    // we'd instead share a single set of dynamic descriptor pools between the share groups.
    // Same with query pools.
    vk::DescriptorSetLayoutArray<vk::DynamicDescriptorPool> mDynamicDescriptorPools;
    angle::PackedEnumMap<gl::QueryType, vk::DynamicQueryPool> mQueryPools;

    // Dirty bits.
    DirtyBits mDirtyBits;
    DirtyBits mNonIndexedDirtyBitsMask;
    DirtyBits mIndexedDirtyBitsMask;
    DirtyBits mNewCommandBufferDirtyBits;

    // Cached back-end objects.
    VertexArrayVk *mVertexArray;
    FramebufferVk *mDrawFramebuffer;
    ProgramVk *mProgram;

    // The offset we had the last time we bound the index buffer.
    const GLvoid *mLastIndexBufferOffset;
    gl::DrawElementsType mCurrentDrawElementsType;

    // Cached clear value/mask for color and depth/stencil.
    VkClearValue mClearColorValue;
    VkClearValue mClearDepthStencilValue;
    VkColorComponentFlags mClearColorMask;

    IncompleteTextureSet mIncompleteTextures;

    // If the current surface bound to this context wants to have all rendering flipped vertically.
    // Updated on calls to onMakeCurrent.
    bool mFlipYForCurrentSurface;
    bool mFlipViewportForDrawFramebuffer;
    bool mFlipViewportForReadFramebuffer;

    // For shader uniforms such as gl_DepthRange and the viewport size.
    struct DriverUniforms
    {
        std::array<float, 4> viewport;

        float halfRenderAreaHeight;
        float viewportYScale;
        float negViewportYScale;
        float padding;

        // We'll use x, y, z for near / far / diff respectively.
        std::array<float, 4> depthRange;
    };

    vk::DynamicBuffer mDriverUniformsBuffer;
    VkDescriptorSet mDriverUniformsDescriptorSet;
    vk::BindingPointer<vk::DescriptorSetLayout> mDriverUniformsSetLayout;
    vk::RefCountedDescriptorPoolBinding mDriverUniformsDescriptorPoolBinding;

    // This cache should also probably include the texture index (shader location) and array
    // index (also in the shader). This info is used in the descriptor update step.
    gl::ActiveTextureArray<TextureVk *> mActiveTextures;

    // "Current Value" aka default vertex attribute state.
    gl::AttributesMask mDirtyDefaultAttribsMask;
    gl::AttribArray<vk::DynamicBuffer> mDefaultAttribBuffers;

    vk::CommandPool mCommandPool;
    Serial mLastCompletedQueueSerial;
    Serial mLastSubmittedQueueSerial;
    Serial mCurrentQueueSerial;

    struct CommandBatch final : angle::NonCopyable
    {
        CommandBatch();
        ~CommandBatch();
        CommandBatch(CommandBatch &&other);
        CommandBatch &operator=(CommandBatch &&other);

        void destroy(VkDevice device);

        vk::CommandPool commandPool;
        vk::Shared<vk::Fence> fence;
        Serial serial;
    };

    std::vector<CommandBatch> mInFlightCommands;
    std::vector<vk::GarbageObject> mGarbage;

    RenderPassCache mRenderPassCache;

    // mSubmitFence is the fence that's going to be signaled at the next submission.  This is used
    // to support SyncVk objects, which may outlive the context (as EGLSync objects).
    //
    // TODO(geofflang): this is in preparation for moving RendererVk functionality to ContextVk, and
    // is otherwise unnecessary as the SyncVk objects don't actually outlive the renderer currently.
    // http://anglebug.com/2701
    vk::Shared<vk::Fence> mSubmitFence;

    // Pool allocator used for command graph but may be expanded to other allocations
    angle::PoolAllocator mPoolAllocator;

    // See CommandGraph.h for a desription of the Command Graph.
    vk::CommandGraph mCommandGraph;

    // Internal shader library.
    vk::ShaderLibrary mShaderLibrary;
    UtilsVk mUtils;

    // The GpuEventQuery struct holds together a timestamp query and enough data to create a
    // trace event based on that. Use traceGpuEvent to insert such queries.  They will be readback
    // when the results are available, without inserting a GPU bubble.
    //
    // - eventName will be the reported name of the event
    // - phase is either 'B' (duration begin), 'E' (duration end) or 'i' (instant // event).
    //   See Google's "Trace Event Format":
    //   https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU
    // - serial is the serial of the batch the query was submitted on.  Until the batch is
    //   submitted, the query is not checked to avoid incuring a flush.
    struct GpuEventQuery final
    {
        const char *name;
        char phase;

        uint32_t queryIndex;
        size_t queryPoolIndex;

        Serial serial;
    };

    // Once a query result is available, the timestamp is read and a GpuEvent object is kept until
    // the next clock sync, at which point the clock drift is compensated in the results before
    // handing them off to the application.
    struct GpuEvent final
    {
        uint64_t gpuTimestampCycles;
        const char *name;
        char phase;
    };

    bool mGpuEventsEnabled;
    vk::DynamicQueryPool mGpuEventQueryPool;
    // A list of queries that have yet to be turned into an event (their result is not yet
    // available).
    std::vector<GpuEventQuery> mInFlightGpuEventQueries;
    // A list of gpu events since the last clock sync.
    std::vector<GpuEvent> mGpuEvents;

    // Hold information from the last gpu clock sync for future gpu-to-cpu timestamp conversions.
    struct GpuClockSyncInfo
    {
        double gpuTimestampS;
        double cpuTimestampS;
    };
    GpuClockSyncInfo mGpuClockSync;

    // The very first timestamp queried for a GPU event is used as origin, so event timestamps would
    // have a value close to zero, to avoid losing 12 bits when converting these 64 bit values to
    // double.
    uint64_t mGpuEventTimestampOrigin;
};
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_CONTEXTVK_H_
