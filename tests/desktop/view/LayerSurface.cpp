#include <Compositor.hpp>
#include <animation/AnimationManager.hpp>
#include <config/ConfigValue.hpp>
#include <config/lua/ConfigManager.hpp>
#include <config/shared/animation/AnimationTree.hpp>
#include <config/shared/inotify/ConfigWatcher.hpp>
#include <config/supplementary/jeremy/Jeremy.hpp>
#include <desktop/state/LayerState.hpp>
#include <desktop/state/WindowState.hpp>
#include <desktop/view/LayerSurface.hpp>
#include <desktop/view/Popup.hpp>
#include <desktop/view/Subsurface.hpp>
#include <desktop/view/WLSurface.hpp>
#include <desktop/view/window/WaylandBackend.hpp>
#include <desktop/view/window/Window.hpp>
#include <desktop/view/window/WindowPresentation.hpp>
#include <event/EventBus.hpp>
#include <layout/LayoutManager.hpp>
#include <managers/SeatManager.hpp>
#include <managers/SessionLockManager.hpp>
#include <managers/eventLoop/EventLoopManager.hpp>
#include <output/Monitor.hpp>
#include <protocols/FractionalScale.hpp>
#include <protocols/LayerShell.hpp>
#include <protocols/LockNotify.hpp>
#include <protocols/PresentationTime.hpp>
#include <protocols/SessionLock.hpp>
#include <protocols/XDGShell.hpp>
#include <protocols/core/Compositor.hpp>
#include <protocols/core/Output.hpp>
#include <protocols/core/Seat.hpp>
#include <protocols/core/Subcompositor.hpp>
#include <render/ElementRenderer.hpp>
#include <render/Renderer.hpp>
#include <render/SyncFDManager.hpp>
#include <state/MonitorState.hpp>
#include <state/WorkspaceState.hpp>
#include <workspace/HLWorkspace.hpp>

#include <array>
#include <gtest/gtest.h>
#include <initializer_list>
#include <memory>
#include <sys/socket.h>

using namespace Hyprutils::OS;

class CLayerSurfaceTestOutput : public Aquamarine::IOutput {
  public:
    bool commit() override {
        ADD_FAILURE() << "Unexpected output commit";
        return false;
    }
    bool test() override {
        ADD_FAILURE() << "Unexpected output test";
        return false;
    }
    SP<Aquamarine::IBackendImplementation> getBackend() override {
        return nullptr;
    }
    std::vector<SDRMFormat> getRenderFormats() override {
        return {};
    }
    bool pendingPageFlip() override {
        return false;
    }
    bool pendingIdleFrame() override {
        return false;
    }
};

class CLayerSurfaceTestRenderer : public Render::IHyprRenderer {
  public:
    CLayerSurfaceTestRenderer() {
        m_cursorHidden = true;
    }

    eType type() override {
        return RT_GL;
    }
    Render::SRenderResult endRender(const std::function<void()>&) override {
        ADD_FAILURE() << "Unexpected rendering";
        return {};
    }
    UP<Render::ISyncFDManager> createSyncFDManager() override {
        return nullptr;
    }
    WP<Render::IElementRenderer> elementRenderer() override {
        return nullptr;
    }
    SP<Render::ITexture> createStencilTexture(int, int) override {
        return nullptr;
    }
    using Render::IHyprRenderer::createTexture;
    SP<Render::ITexture> createTexture(bool) override {
        return nullptr;
    }
    SP<Render::ITexture> createTexture(uint32_t, uint8_t*, uint32_t, const Vector2D&, bool, bool) override {
        return nullptr;
    }
    SP<Render::ITexture> createTexture(const Aquamarine::SDMABUFAttrs&, bool, const NColorManagement::SColorRepresentation&) override {
        return nullptr;
    }
    SP<Render::ITexture> createTexture(int, int, unsigned char*) override {
        return nullptr;
    }
    SP<Render::ITexture> createTexture(cairo_surface_t*) override {
        return nullptr;
    }
    SP<Render::ITexture> createTexture(std::span<const float>, size_t) override {
        return nullptr;
    }
    bool explicitSyncSupported() override {
        return false;
    }
    bool fp16Supported() override {
        return false;
    }
    std::vector<SDRMFormat> getDRMFormats() override {
        return {};
    }
    std::vector<uint64_t> getDRMFormatModifiers(DRMFormat) override {
        return {};
    }
    SP<Render::IFramebuffer> createFB(const std::string&) override {
        ADD_FAILURE() << "Unexpected framebuffer allocation";
        return nullptr;
    }
    void disableScissor() override {
        ;
    }
    void blend(bool) override {
        ;
    }
    void drawShadow(Render::CRenderContext&, const CBox&, Math::SCornerRadii, float, int, const Config::CGradientValueData&, float,
                    const Render::SWindowRenderPresentation&) override {
        ADD_FAILURE() << "Unexpected shadow draw";
    }
    void drawShadow(Render::CRenderContext&, const CBox&, Math::SCornerRadii, float, int, const Config::CGradientValueData&, const Config::CGradientValueData&, float, float,
                    const Render::SWindowRenderPresentation&) override {
        ADD_FAILURE() << "Unexpected shadow draw";
    }
    void drawGlow(Render::CRenderContext&, const CBox&, Math::SCornerRadii, float, int, const Config::CGradientValueData&, float) override {
        ADD_FAILURE() << "Unexpected glow draw";
    }
    void drawGlow(Render::CRenderContext&, const CBox&, Math::SCornerRadii, float, int, const Config::CGradientValueData&, const Config::CGradientValueData&, float,
                  float) override {
        ADD_FAILURE() << "Unexpected glow draw";
    }
    void setViewport(int, int, int, int) override {
        ;
    }
    SP<Render::IFramebuffer> blurFramebuffer(Render::CRenderContext&, SP<Render::IFramebuffer>, float, const CRegion&, const Render::SBlurContext&) override {
        ADD_FAILURE() << "Unexpected blur";
        return nullptr;
    }
    void refreshBlurProvider() override {
        ;
    }
    void expandBlurDamage(CRegion&, float) const override {
        ;
    }
    bool blurProviderIsAnimated(Render::CRenderContext&) const override {
        return false;
    }
    bool blurProviderRequiresLiveBlur() const override {
        return false;
    }
    bool reloadShaders(const std::string&) override {
        return false;
    }

  private:
    void renderOffToMain(Render::CRenderContext&, SP<Render::IFramebuffer>) override {
        ADD_FAILURE() << "Unexpected rendering";
    }
    SP<Render::IRenderbuffer> getOrCreateRenderbufferInternal(SP<Aquamarine::IBuffer>, uint32_t) override {
        ADD_FAILURE() << "Unexpected renderbuffer allocation";
        return nullptr;
    }
};

class CLayerSurfaceTestTexture : public Render::ITexture {
  public:
    void setTexParameter(GLenum, GLint) override {
        ADD_FAILURE();
    }
    void allocate(const Vector2D&, uint32_t) override {
        ADD_FAILURE();
    }
    void update(uint32_t, uint8_t*, uint32_t, const CRegion&) override {
        ADD_FAILURE();
    }
};

class CLayerSurfaceTestBuffer : public IHLBuffer {
  public:
    Aquamarine::eBufferCapability caps() override {
        return Aquamarine::BUFFER_CAPABILITY_NONE;
    }
    Aquamarine::eBufferType type() override {
        return Aquamarine::BUFFER_TYPE_MISC;
    }
    void update(const CRegion&) override {
        ADD_FAILURE();
    }
    bool isSynchronous() override {
        return false;
    }
    bool good() override {
        return true;
    }
};

static bool& layerSurfaceTestMapped(PHLLS layer);

template <bool Desktop::View::CLayerSurface::* Mapped>
struct SLayerSurfaceTestAccess {
    friend bool& layerSurfaceTestMapped(PHLLS layer) {
        return layer.get()->*Mapped;
    }
};

template struct SLayerSurfaceTestAccess<&Desktop::View::CLayerSurface::m_mapped>;

static bool& windowTestMapped(PHLWINDOW window);
static void  windowTestInitSurfaceTree(PHLWINDOW window);

template <bool Desktop::View::CWindow::* Mapped, void (Desktop::View::CSubsurfaceOwner::*SetSub)(SP<Desktop::View::CSubsurface>),
          void (Desktop::View::CPopupOwner::*SetPopup)(SP<Desktop::View::CPopup>)>
struct SWindowTestAccess {
    friend bool& windowTestMapped(PHLWINDOW window) {
        return window.get()->*Mapped;
    }
    friend void windowTestInitSurfaceTree(PHLWINDOW window) {
        (window.get()->*SetSub)(Desktop::View::CSubsurface::create(window));
        (window.get()->*SetPopup)(Desktop::View::CPopup::create(window));
    }
};

template struct SWindowTestAccess<&Desktop::View::CWindow::m_isMapped, &Desktop::View::CSubsurfaceOwner::setSubsurfaceHead, &Desktop::View::CPopupOwner::setPopupHead>;

class CLayerSurfaceTestWorkspace : public Workspace::CHLWorkspace {
  public:
    explicit CLayerSurfaceTestWorkspace(PHLMONITOR monitor) : CHLWorkspace(Workspace::SWorkspaceNumberedID{1}, monitor, "layer-test", "1", Workspace::eWorkspaceType::NORMAL) {
        ;
    }
};

class CLayerSurfacePartialDamageTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_display.reset(wl_display_create());
        ASSERT_TRUE(m_display);
        std::array<int, 2> sockets = {
            -1,
            -1,
        };
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets.data()), 0);
        CFileDescriptor server{sockets[0]};
        m_socket = CFileDescriptor{sockets[1]};
        m_client = wl_client_create(m_display.get(), server.get());
        ASSERT_NE(m_client, nullptr);
        server.take();

        m_previousCompositor     = std::move(g_pCompositor);
        m_previousEventLoop      = std::move(g_pEventLoopManager);
        m_previousRenderer       = std::move(g_pHyprRenderer);
        m_previousSeatManager    = std::move(g_pSeatManager);
        m_previousSessionLockMgr = std::move(g_pSessionLockManager);
        m_previousLayoutManager  = std::move(g_layoutManager);
        m_previousMonitorState   = std::move(State::monitorState());
        m_previousWorkspaceState = std::move(State::workspaceState());
        m_previousWindowState    = std::move(Desktop::windowState());
        m_previousEventBus       = std::move(Event::bus());
        m_previousConfig         = std::move(Config::mgr());
        m_previousWatcher        = std::move(Config::watcher());
        m_previousAnimations     = std::move(Animation::mgr());
        m_previousTree           = std::move(Config::animationTree());
        m_previousPresentation   = std::move(PROTO::presentation);
        m_previousCoreCompositor = std::move(PROTO::compositor);
        m_previousSubcompositor  = std::move(PROTO::subcompositor);
        m_previousFractional     = std::move(PROTO::fractional);
        m_previousSeat           = std::move(PROTO::seat);
        m_previousLockNotify     = std::move(PROTO::lockNotify);
        m_previousSessionLock    = std::move(PROTO::sessionLock);
        m_previousOutputs        = std::move(PROTO::outputs);
        m_installedGlobals       = true;

        Event::bus()                        = makeUnique<Event::CEventBus>();
        State::monitorState()               = makeUnique<State::CMonitorStateTracker>();
        State::workspaceState()             = makeUnique<State::CWorkspaceStateTracker>();
        Desktop::windowState()              = makeUnique<Desktop::CWindowState>();
        g_pCompositor                       = makeUnique<CCompositor>(true);
        g_pCompositor->m_wlDisplay          = m_display.get();
        g_pCompositor->m_wlEventLoop        = wl_display_get_event_loop(m_display.get());
        g_pCompositor->m_explicitConfigPath = "layer-surface-test.lua";
        Aquamarine::SBackendImplementationOptions backend;
        backend.backendType        = Aquamarine::AQ_BACKEND_NULL;
        g_pCompositor->m_aqBackend = Aquamarine::CBackend::create({backend}, Aquamarine::SBackendOptions{});
        ASSERT_TRUE(g_pCompositor->m_aqBackend);
        g_pCompositor->m_sessionActive = false;
        Config::Supplementary::Jeremy::flushCachedCfgPath();
        Config::watcher() = makeUnique<Config::CConfigWatcher>();
        Config::mgr()     = makeUnique<Config::Lua::CConfigManager>();
        CConfigValueBase::flushCaches();
        Animation::mgr()        = makeUnique<Animation::CHyprAnimationManager>();
        Config::animationTree() = makeUnique<Config::CAnimationTreeController>();
        g_layoutManager         = makeUnique<Layout::CLayoutManager>();
        g_pEventLoopManager     = makeUnique<CEventLoopManager>(m_display.get(), g_pCompositor->m_wlEventLoop);
        g_pHyprRenderer         = makeUnique<CLayerSurfaceTestRenderer>();
        wl_event_source_timer_update(g_pHyprRenderer->m_cursorTicker, 0);

        PROTO::presentation   = makeUnique<CPresentationProtocol>(&wp_presentation_interface, 2, "layer-surface-test");
        PROTO::compositor     = makeUnique<CWLCompositorProtocol>(&wl_compositor_interface, 6, "layer-surface-test-compositor");
        PROTO::subcompositor  = makeUnique<CWLSubcompositorProtocol>(&wl_subcompositor_interface, 1, "layer-surface-test-subcompositor");
        PROTO::fractional     = makeUnique<CFractionalScaleProtocol>(&wp_fractional_scale_manager_v1_interface, 1, "layer-surface-test-fractional");
        PROTO::seat           = makeUnique<CWLSeatProtocol>(&wl_seat_interface, 9, "layer-surface-test-seat");
        PROTO::lockNotify     = makeUnique<CLockNotifyProtocol>(&hyprland_lock_notifier_v1_interface, 1, "layer-surface-test-lock-notify");
        PROTO::sessionLock    = makeUnique<CSessionLockProtocol>(&ext_session_lock_manager_v1_interface, 1, "layer-surface-test-session-lock");
        g_pSeatManager        = makeUnique<CSeatManager>();
        g_pSessionLockManager = makeUnique<CSessionLockManager>();

        m_monitor                    = makeShared<Monitor::CMonitor>(makeShared<CLayerSurfaceTestOutput>());
        m_monitor->m_self            = m_monitor;
        m_monitor->m_id              = 1;
        m_monitor->m_name            = "TEST-1";
        m_monitor->m_enabled         = true;
        m_monitor->m_dpmsStatus      = true;
        m_monitor->m_position        = {0, 0};
        m_monitor->m_size            = {1920, 1080};
        m_monitor->m_pixelSize       = {1920, 1080};
        m_monitor->m_transformedSize = {1920, 1080};
        m_monitor->m_scale           = 1.F;
        m_monitor->m_damage.setSize(m_monitor->m_transformedSize);
        m_monitor->m_damage.rotate();
        Event::bus()->m_events.monitor.added.emit(m_monitor);
        PROTO::outputs[m_monitor->m_name] = makeShared<CWLOutputProtocol>(&wl_output_interface, 4, "layer-surface-test-output", m_monitor);

        m_compositorId    = m_nextID++;
        m_subcompositorId = m_nextID++;
        m_sessionLockId   = m_nextID++;
        m_outputId        = m_nextID++;
        PROTO::compositor->bindManager(m_client, nullptr, 6, m_compositorId);
        PROTO::subcompositor->bindManager(m_client, nullptr, 1, m_subcompositorId);
        PROTO::sessionLock->bindManager(m_client, nullptr, 1, m_sessionLockId);
        PROTO::outputs[m_monitor->m_name]->bindManager(m_client, nullptr, 4, m_outputId);
    }

    void TearDown() override {
        if (m_monitor) {
            m_monitor->m_activeWorkspace.reset();
            for (auto& la : m_monitor->m_layerSurfaceLayers)
                la.clear();
        }
        for (const auto& layer : m_layers) {
            layerSurfaceTestMapped(layer) = false;
            layer->m_monitor.reset();
        }
        for (const auto& window : m_windows)
            Desktop::windowState()->removeSafe(window);
        m_windows.clear();
        m_xdgToplevels.clear();
        m_xdgSurfaces.clear();
        m_workspace.reset();
        m_layers.clear();
        m_layerResources.clear();
        if (m_display)
            wl_display_destroy_clients(m_display.get());
        m_monitor.reset();
        if (!m_installedGlobals)
            return;

        PROTO::outputs          = std::move(m_previousOutputs);
        g_pSessionLockManager   = std::move(m_previousSessionLockMgr);
        PROTO::sessionLock      = std::move(m_previousSessionLock);
        PROTO::lockNotify       = std::move(m_previousLockNotify);
        g_pSeatManager          = std::move(m_previousSeatManager);
        PROTO::seat             = std::move(m_previousSeat);
        PROTO::subcompositor    = std::move(m_previousSubcompositor);
        PROTO::compositor       = std::move(m_previousCoreCompositor);
        PROTO::fractional       = std::move(m_previousFractional);
        PROTO::presentation     = std::move(m_previousPresentation);
        g_pHyprRenderer         = std::move(m_previousRenderer);
        g_pEventLoopManager     = std::move(m_previousEventLoop);
        g_layoutManager         = std::move(m_previousLayoutManager);
        Animation::mgr()        = std::move(m_previousAnimations);
        Config::animationTree() = std::move(m_previousTree);
        Config::mgr()           = std::move(m_previousConfig);
        Config::watcher()       = std::move(m_previousWatcher);
        if (Config::mgr())
            CConfigValueBase::flushCaches();
        g_pCompositor           = std::move(m_previousCompositor);
        Desktop::windowState()  = std::move(m_previousWindowState);
        State::workspaceState() = std::move(m_previousWorkspaceState);
        State::monitorState()   = std::move(m_previousMonitorState);
        Event::bus()           = std::move(m_previousEventBus);
        Config::Supplementary::Jeremy::flushCachedCfgPath();
    }

    void request(uint32_t id, uint32_t opcode, std::initializer_list<uint32_t> args = {}) {
        std::vector<uint32_t> message = {
            id,
            sc<uint32_t>((args.size() + 2) * sizeof(uint32_t) << 16) | opcode,
        };
        message.insert(message.end(), args);
        const auto size = message.size() * sizeof(uint32_t);
        ASSERT_EQ(send(m_socket.get(), message.data(), size, MSG_NOSIGNAL), sc<ssize_t>(size));
        ASSERT_EQ(wl_event_loop_dispatch(wl_display_get_event_loop(m_display.get()), 0), 0);
    }

    SP<IHLBuffer> createBuffer(const Vector2D& size) {
        auto buffer                  = makeShared<CLayerSurfaceTestBuffer>();
        auto bufWire                 = makeShared<CWlBuffer>(m_client, 1, m_nextID++);
        buffer->m_resource           = CWLBufferResource::create(bufWire);
        buffer->m_resource->m_buffer = buffer;
        buffer->size                 = size;
        buffer->m_texture            = makeShared<CLayerSurfaceTestTexture>();
        m_buffers.emplace_back(buffer);
        return buffer;
    }

    struct SCreatedLayer {
        uint32_t                surfId = 0;
        SP<CLayerShellResource> resource;
        PHLLS                   layer;
        SP<IHLBuffer>           buffer;
    };

    SCreatedLayer createLayerSurface(const std::string& ns, const CBox& geom, uint32_t anchor = 0, const Vector2D& desiredSize = {}) {
        const uint32_t         surfId = m_nextID++;
        SP<CWLSurfaceResource> surf;
        auto                   newSurfListener = PROTO::compositor->m_events.newSurface.listen([&](SP<CWLSurfaceResource> s) { surf = s; });
        request(m_compositorId, 0, {surfId}); // wl_compositor.create_surface
        EXPECT_TRUE(surf);

        auto wire     = makeShared<CZwlrLayerSurfaceV1>(m_client, 4, m_nextID++);
        auto resource = makeShared<CLayerShellResource>(wire, surf, ns, m_monitor, ZWLR_LAYER_SHELL_V1_LAYER_TOP);
        EXPECT_TRUE(resource->good());
        wire->setDestroy([](CZwlrLayerSurfaceV1*) { ; });
        wire->setOnDestroy([](CZwlrLayerSurfaceV1*) { ; });

        const Vector2D desired          = desiredSize == Vector2D{} ? geom.size() : desiredSize;
        resource->m_pending.desiredSize = desired;
        resource->m_current.desiredSize = desired;
        resource->m_pending.anchor      = anchor;
        resource->m_current.anchor      = anchor;
        resource->m_configured          = true;
        resource->m_mapped              = true;

        auto layer                    = Desktop::View::CLayerSurface::create(resource);
        layerSurfaceTestMapped(layer) = true;
        layer->m_monitor              = m_monitor;
        layer->m_geometry             = geom;
        layer->m_position             = geom.pos();
        layer->positionAnimation()->setValueAndWarp(geom.pos());
        layer->sizeAnimation()->setValueAndWarp(geom.size());
        layer->alpha()[Desktop::View::LS_ALPHA_FADE]->setValueAndWarp(1.F);

        m_layerResources.emplace_back(resource);
        m_layers.emplace_back(layer);

        auto buffer = createBuffer(geom.size());
        request(surfId, 1, {wl_resource_get_id(buffer->m_resource->getResource()), 0, 0}); // wl_surface.attach
        commit(surfId);
        takeDamage();

        return SCreatedLayer{
            .surfId   = surfId,
            .resource = resource,
            .layer    = layer,
            .buffer   = buffer,
        };
    }

    struct SCreatedLock {
        uint32_t      lockId     = 0;
        uint32_t      surfId     = 0;
        uint32_t      lockSurfId = 0;
        SP<IHLBuffer> buffer;
    };

    SCreatedLock createSessionLock() {
        const uint32_t lockId     = m_nextID++;
        const uint32_t surfId     = m_nextID++;
        const uint32_t lockSurfId = m_nextID++;

        request(m_sessionLockId, 1, {lockId});                                // ext_session_lock_manager_v1.lock
        request(m_compositorId, 0, {surfId});                                 // wl_compositor.create_surface
        request(lockId, 1, {lockSurfId, surfId, m_outputId});                 // ext_session_lock_v1.get_lock_surface
        request(lockSurfId, 1, {1});                                          // ext_session_lock_surface_v1.ack_configure

        auto buffer = createBuffer(m_monitor->m_size);
        request(surfId, 1, {wl_resource_get_id(buffer->m_resource->getResource()), 0, 0}); // wl_surface.attach
        commit(surfId);

        return SCreatedLock{
            .lockId     = lockId,
            .surfId     = surfId,
            .lockSurfId = lockSurfId,
            .buffer     = buffer,
        };
    }

    struct SCreatedWindow {
        uint32_t      surfId = 0;
        PHLWINDOW     window;
        SP<IHLBuffer> buffer;
    };

    SCreatedWindow createWindow(const CBox& box) {
        if (!m_workspace) {
            m_workspace                  = makeShared<CLayerSurfaceTestWorkspace>(m_monitor);
            m_workspace->m_self          = m_workspace;
            m_workspace->setVisible(true);
            m_monitor->m_activeWorkspace = m_workspace;
            Animation::mgr()->createAnimation(Vector2D{}, m_workspace->m_renderOffset, Config::animationTree()->getAnimationPropertyConfig("workspacesIn"), m_workspace,
                                              AVARDAMAGE_ENTIRE);
            Animation::mgr()->createAnimation(1.F, m_workspace->m_alpha, Config::animationTree()->getAnimationPropertyConfig("workspacesIn"), m_workspace, AVARDAMAGE_ENTIRE);
        }

        const uint32_t         surfId = m_nextID++;
        SP<CWLSurfaceResource> surf;
        auto                   newSurfListener = PROTO::compositor->m_events.newSurface.listen([&](SP<CWLSurfaceResource> s) { surf = s; });
        request(m_compositorId, 0, {surfId}); // wl_compositor.create_surface
        EXPECT_TRUE(surf);

        auto xdg                = makeShared<CXDGSurfaceResource>(makeShared<CXdgSurface>(m_client, 6, m_nextID++), nullptr, surf);
        xdg->m_self             = xdg;
        xdg->m_pending.geometry = {{}, box.size()};
        xdg->m_current.geometry = {{}, box.size()};
        xdg->m_mapped           = true;
        xdg->m_initialCommit    = false;
        auto toplevel           = makeShared<CXDGToplevelResource>(makeShared<CXdgToplevel>(m_client, 6, m_nextID++), xdg);
        toplevel->m_self        = toplevel;
        xdg->m_toplevel         = toplevel;
        surf->m_role            = makeShared<CXDGSurfaceRole>(xdg);

        auto window              = Desktop::View::CWindow::create(makeUnique<Desktop::View::CWaylandBackend>(xdg));
        window->m_monitor        = m_monitor;
        window->m_workspace      = m_workspace;
        window->positionAnimation()->setValueAndWarp(box.pos());
        window->sizeAnimation()->setValueAndWarp(box.size());
        window->presentation().alpha(Desktop::View::WINDOW_ALPHA_FADE)->setValueAndWarp(1.F);
        windowTestInitSurfaceTree(window);
        windowTestMapped(window) = true;
        surf->m_mapped           = true;

        m_xdgSurfaces.emplace_back(xdg);
        m_xdgToplevels.emplace_back(toplevel);
        m_windows.emplace_back(window);

        auto buffer = createBuffer(box.size());
        request(surfId, 1, {wl_resource_get_id(buffer->m_resource->getResource()), 0, 0}); // wl_surface.attach
        commit(surfId);
        takeDamage();
        m_monitor->m_output->needsFrame = false;

        return SCreatedWindow{
            .surfId = surfId,
            .window = window,
            .buffer = buffer,
        };
    }

    void commit(uint32_t surfId) {
        request(surfId, 6); // wl_surface.commit
    }

    CRegion takeDamage() {
        CRegion damage = m_monitor->m_damage.getBufferDamage(1);
        m_monitor->m_damage.rotate();
        return damage;
    }

    void expectDamage(CRegion expected) {
        CRegion    actual = takeDamage();
        const auto actExt = actual.getExtents();
        const auto expExt = expected.getExtents();
        EXPECT_TRUE(pixman_region32_equal(actual.pixman(), expected.pixman()))
            << "actual=(" << actExt.x << "," << actExt.y << "," << actExt.w << "," << actExt.h << ") empty=" << actual.empty() << " expected=(" << expExt.x << "," << expExt.y
            << "," << expExt.w << "," << expExt.h << ")";
    }

    wl_client*                                 m_client          = nullptr;
    uint32_t                                   m_nextID          = 2;
    uint32_t                                   m_compositorId    = 0;
    uint32_t                                   m_subcompositorId = 0;
    uint32_t                                   m_sessionLockId   = 0;
    uint32_t                                   m_outputId        = 0;
    PHLMONITOR                                 m_monitor;
    PHLWORKSPACE                               m_workspace;
    std::vector<SP<IHLBuffer>>                 m_buffers;
    std::vector<SP<CLayerShellResource>>       m_layerResources;
    std::vector<PHLLS>                         m_layers;
    std::vector<SP<CXDGSurfaceResource>>       m_xdgSurfaces;
    std::vector<SP<CXDGToplevelResource>>      m_xdgToplevels;
    std::vector<PHLWINDOW>                     m_windows;

  private:
    std::unique_ptr<wl_display, decltype(&wl_display_destroy)> m_display{nullptr, wl_display_destroy};
    CFileDescriptor                                            m_socket;
    bool                                                       m_installedGlobals = false;
    UP<CCompositor>                                            m_previousCompositor;
    UP<CEventLoopManager>                                      m_previousEventLoop;
    UP<Render::IHyprRenderer>                                  m_previousRenderer;
    UP<CSeatManager>                                           m_previousSeatManager;
    UP<CSessionLockManager>                                    m_previousSessionLockMgr;
    UP<Layout::CLayoutManager>                                 m_previousLayoutManager;
    UP<State::CMonitorStateTracker>                            m_previousMonitorState;
    UP<State::CWorkspaceStateTracker>                          m_previousWorkspaceState;
    UP<Desktop::CWindowState>                                  m_previousWindowState;
    UP<Event::CEventBus>                                       m_previousEventBus;
    UP<Config::IConfigManager>                                 m_previousConfig;
    UP<Config::CConfigWatcher>                                 m_previousWatcher;
    UP<Animation::CHyprAnimationManager>                       m_previousAnimations;
    UP<Config::CAnimationTreeController>                       m_previousTree;
    UP<CPresentationProtocol>                                  m_previousPresentation;
    UP<CWLCompositorProtocol>                                  m_previousCoreCompositor;
    UP<CWLSubcompositorProtocol>                               m_previousSubcompositor;
    UP<CFractionalScaleProtocol>                               m_previousFractional;
    UP<CWLSeatProtocol>                                        m_previousSeat;
    UP<CLockNotifyProtocol>                                    m_previousLockNotify;
    UP<CSessionLockProtocol>                                   m_previousSessionLock;
    std::unordered_map<std::string, SP<CWLOutputProtocol>>     m_previousOutputs;
};

TEST_F(CLayerSurfacePartialDamageTest, NonLayoutCommitPreservesPartialDamageAndShrinkDamagesOldGeometry) {
    auto created = createLayerSurface("layer-partial-test", {50, 100, 400, 300});

    // Subsequent commit with partial surface damage must NOT damage the full layer geometry.
    request(created.surfId, 1, {wl_resource_get_id(created.buffer->m_resource->getResource()), 0, 0}); // wl_surface.attach
    request(created.surfId, 2, {10, 20, 30, 15});                                                      // wl_surface.damage
    commit(created.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{60, 120, 30, 15}.pixman()));

    // Keyboard interactivity alone (without buffer attach or surface damage) must not damage anything.
    created.resource->m_pending.committed |= CLayerShellResource::eCommittedState::STATE_KEYBOARD_INTERACTIVITY;
    created.resource->m_pending.keyboardInteractivity              = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND;
    created.resource->m_surface->m_pending.updated.bits.layershell = true;
    commit(created.surfId);
    EXPECT_TRUE(takeDamage().empty());

    // Keyboard interactivity with partial damage must preserve partial damage.
    created.resource->m_pending.committed |= CLayerShellResource::eCommittedState::STATE_KEYBOARD_INTERACTIVITY;
    created.resource->m_pending.keyboardInteractivity = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND;
    request(created.surfId, 1, {wl_resource_get_id(created.buffer->m_resource->getResource()), 0, 0}); // wl_surface.attach
    request(created.surfId, 2, {10, 20, 30, 15});                                                      // wl_surface.damage
    commit(created.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{60, 120, 30, 15}.pixman()));

    // Shrinking surface size without layer-shell set_size must still damage the old geometry.
    auto buf200 = createBuffer({200, 150});
    request(created.surfId, 1, {wl_resource_get_id(buf200->m_resource->getResource()), 0, 0}); // wl_surface.attach
    commit(created.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{50, 100, 400, 300}.pixman()));
}

TEST_F(CLayerSurfacePartialDamageTest, CommittedLayoutStateDamagesChangedGeometryWithoutFullMonitorDamage) {
    auto created = createLayerSurface("layer-committed-test", {0, 0, 400, 300}, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);

    // Margin change moves the layer from {0, 0, 400, 300} to {0, 10, 400, 300} without changing usableArea.
    // Only the old and new layer geometries ({0, 0, 400, 310}) should be damaged, not the entire monitor.
    created.resource->m_pending.committed |= CLayerShellResource::eCommittedState::STATE_MARGIN;
    created.resource->m_pending.margin.top = 10;
    commit(created.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{0, 0, 400, 310}.pixman()));
    EXPECT_EQ(created.layer->m_position, Vector2D(0, 10));

    // Re-committing an unchanged margin alongside partial surface damage must preserve
    // partial damage at the updated layer position rather than damaging the full box.
    created.resource->m_pending.committed |= CLayerShellResource::eCommittedState::STATE_MARGIN;
    created.resource->m_pending.margin.top = 10;
    request(created.surfId, 1, {wl_resource_get_id(created.buffer->m_resource->getResource()), 0, 0}); // wl_surface.attach
    request(created.surfId, 2, {10, 20, 30, 15});                                                      // wl_surface.damage
    commit(created.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{10, 30, 30, 15}.pixman()));

    // Re-arranging layers when nothing changed must not produce any damage.
    g_pHyprRenderer->arrangeLayersForMonitor(m_monitor->m_id);
    EXPECT_TRUE(takeDamage().empty());
}

TEST_F(CLayerSurfacePartialDamageTest, LayerPlaneChangeDamagesLayerOrMonitorWhenDimAround) {
    auto created                            = createLayerSurface("layer-plane-test", {50, 100, 400, 300}, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
    created.resource->m_pending.margin.left = 50;
    created.resource->m_pending.margin.top  = 100;
    created.resource->m_current.margin.left = 50;
    created.resource->m_current.margin.top  = 100;

    // Moving a regular layer between planes with unchanged geometry damages only its box.
    created.resource->m_pending.committed |= CLayerShellResource::eCommittedState::STATE_LAYER;
    created.resource->m_pending.layer                              = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    created.resource->m_surface->m_pending.updated.bits.layershell = true;
    commit(created.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{50, 100, 400, 300}.pixman()));

    // Moving a dimAround layer between planes must damage the full monitor.
    created.layer->m_ruleApplicator->dimAround().set(true, Desktop::Types::PRIORITY_SET_PROP);
    created.resource->m_pending.committed |= CLayerShellResource::eCommittedState::STATE_LAYER;
    created.resource->m_pending.layer                              = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
    created.resource->m_surface->m_pending.updated.bits.layershell = true;
    commit(created.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{0, 0, 1920, 1080}.pixman()));
}

TEST_F(CLayerSurfacePartialDamageTest, OrderChangeDamagesReorderedLayersAndUnmappedConfigureDoesNotDamage) {
    auto first                             = createLayerSurface("layer-order-first", {0, 0, 200, 100}, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
    auto second                            = createLayerSurface("layer-order-second", {100, 50, 200, 100}, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
    second.resource->m_pending.margin.left = 100;
    second.resource->m_pending.margin.top  = 50;
    second.resource->m_current.margin.left = 100;
    second.resource->m_current.margin.top  = 50;
    g_pHyprRenderer->arrangeLayersForMonitor(m_monitor->m_id);
    takeDamage();

    // Swapping layer order must damage both reordered layers without full-monitor damage.
    second.layer->m_ruleApplicator->order().set(1, Desktop::Types::PRIORITY_SET_PROP);
    g_pHyprRenderer->arrangeLayersForMonitor(m_monitor->m_id);
    CRegion expectedOrder{0, 0, 200, 100};
    expectedOrder.add(CBox{100, 50, 200, 100});
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), expectedOrder.pixman()));

    // Subsequent arrange with unchanged order must not damage anything.
    g_pHyprRenderer->arrangeLayersForMonitor(m_monitor->m_id);
    EXPECT_TRUE(takeDamage().empty());

    // Initial layout arrange for an unmapped layer must update its geometry without damaging.
    auto unmapped                          = createLayerSurface("layer-unmapped-test", {0, 0, 300, 80}, ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
    layerSurfaceTestMapped(unmapped.layer) = false;
    unmapped.resource->m_mapped            = false;
    unmapped.layer->m_geometry             = {};
    g_pHyprRenderer->arrangeLayersForMonitor(m_monitor->m_id);
    EXPECT_EQ(unmapped.layer->m_geometry, CBox(0, 1000, 300, 80));
    EXPECT_TRUE(takeDamage().empty());
}

TEST_F(CLayerSurfacePartialDamageTest, ArrangeLayersDamagesMonitorWhenUsableAreaChanges) {
    auto created = createLayerSurface("layer-exclusive-test", {0, 0, 1920, 40},
                                      ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT, {0, 40});

    // Changing exclusive zone modifies usableArea, which must damage the full monitor.
    created.resource->m_pending.committed |= CLayerShellResource::eCommittedState::STATE_EXCLUSIVE;
    created.resource->m_pending.exclusive = 40;
    commit(created.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{0, 0, 1920, 1080}.pixman()));

    // Resetting exclusive zone back to 0 restores usableArea and must also damage the full monitor.
    created.resource->m_pending.committed |= CLayerShellResource::eCommittedState::STATE_EXCLUSIVE;
    created.resource->m_pending.exclusive                          = 0;
    created.resource->m_surface->m_pending.updated.bits.layershell = true;
    commit(created.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{0, 0, 1920, 1080}.pixman()));
}

TEST_F(CLayerSurfacePartialDamageTest, SessionLockSurfacePreservesPartialDamageAndSubsurfaceDamage) {
    *CConfigValue<Config::BOOL>("misc:session_lock_xray").ptr() = false;

    auto lock = createSessionLock();
    // Initial map damages the full monitor.
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{0, 0, 1920, 1080}.pixman()));

    g_pSessionLockManager->onLockscreenRenderedOnMonitor(m_monitor->m_id);
    ASSERT_TRUE(g_pSessionLockManager->clientLocked());

    // Subsequent commit on ext_session_lock_surface_v1 must preserve partial damage.
    request(lock.surfId, 1, {wl_resource_get_id(lock.buffer->m_resource->getResource()), 0, 0}); // wl_surface.attach
    request(lock.surfId, 2, {10, 20, 30, 15});                                                   // wl_surface.damage
    commit(lock.surfId);
    expectDamage(CRegion{10, 20, 30, 15});

    // A subsurface attached to the lock surface must also produce partial damage at its position.
    const uint32_t subSurfId = m_nextID++;
    const uint32_t subRoleId = m_nextID++;
    request(m_compositorId, 0, {subSurfId});                          // wl_compositor.create_surface
    request(m_subcompositorId, 1, {subRoleId, subSurfId, lock.surfId}); // wl_subcompositor.get_subsurface
    request(subRoleId, 1, {100, 120});                                // wl_subsurface.set_position
    request(subRoleId, 5);                                            // wl_subsurface.set_desync
    auto subBuf = createBuffer({80, 40});
    request(subSurfId, 1, {wl_resource_get_id(subBuf->m_resource->getResource()), 0, 0});
    commit(subSurfId);
    takeDamage();

    request(subSurfId, 1, {wl_resource_get_id(subBuf->m_resource->getResource()), 0, 0});
    request(subSurfId, 2, {5, 6, 20, 10});
    commit(subSurfId);
    expectDamage(CRegion{105, 126, 20, 10});

    // With session_lock_xray enabled, lock surface commits must still preserve partial damage.
    *CConfigValue<Config::BOOL>("misc:session_lock_xray").ptr() = true;
    request(lock.surfId, 1, {wl_resource_get_id(lock.buffer->m_resource->getResource()), 0, 0});
    request(lock.surfId, 2, {12, 24, 36, 18});
    commit(lock.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{12, 24, 36, 18}.pixman()));

    // Unlocking must damage the full monitor.
    request(lock.lockId, 2); // ext_session_lock_v1.unlock_and_destroy
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{0, 0, 1920, 1080}.pixman()));
}

TEST_F(CLayerSurfacePartialDamageTest, SessionLockSuppressesDamageFromHiddenLayersAndWindowsBelow) {
    *CConfigValue<Config::BOOL>("misc:session_lock_xray").ptr() = false;

    auto belowLayer = createLayerSurface("layer-below-lock", {0, 0, 1920, 40},
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT, {0, 40});
    auto aboveLayer = createLayerSurface("layer-above-lock", {50, 100, 400, 300}, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
    aboveLayer.resource->m_pending.margin.left = 50;
    aboveLayer.resource->m_pending.margin.top  = 100;
    aboveLayer.resource->m_current.margin.left = 50;
    aboveLayer.resource->m_current.margin.top  = 100;
    aboveLayer.resource->m_pending.exclusive   = -1;
    aboveLayer.resource->m_current.exclusive   = -1;
    aboveLayer.layer->m_ruleApplicator->aboveLock().set(1, Desktop::Types::PRIORITY_SET_PROP);
    auto window = createWindow({200, 200, 600, 400});

    auto lock = createSessionLock();
    takeDamage();
    g_pSessionLockManager->onLockscreenRenderedOnMonitor(m_monitor->m_id);
    ASSERT_TRUE(g_pSessionLockManager->clientLocked());
    m_monitor->m_output->needsFrame = false;

    // Regular layer below the lockscreen must not damage or schedule a frame on surface commit.
    request(belowLayer.surfId, 1, {wl_resource_get_id(belowLayer.buffer->m_resource->getResource()), 0, 0});
    request(belowLayer.surfId, 2, {10, 5, 40, 20});
    request(belowLayer.surfId, 3, {m_nextID++}); // wl_surface.frame
    commit(belowLayer.surfId);
    EXPECT_TRUE(takeDamage().empty());
    EXPECT_FALSE(m_monitor->m_output->needsFrame);

    // Layout / exclusive zone / plane changes on hidden layers must not damage the monitor.
    belowLayer.resource->m_pending.committed |= CLayerShellResource::eCommittedState::STATE_EXCLUSIVE | CLayerShellResource::eCommittedState::STATE_LAYER;
    belowLayer.resource->m_pending.exclusive                          = 40;
    belowLayer.resource->m_pending.layer                              = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    belowLayer.resource->m_surface->m_pending.updated.bits.layershell = true;
    commit(belowLayer.surfId);
    EXPECT_TRUE(takeDamage().empty());
    EXPECT_FALSE(m_monitor->m_output->needsFrame);

    // An aboveLock layer must still emit partial damage.
    request(aboveLayer.surfId, 1, {wl_resource_get_id(aboveLayer.buffer->m_resource->getResource()), 0, 0});
    request(aboveLayer.surfId, 2, {15, 25, 40, 20});
    commit(aboveLayer.surfId);
    EXPECT_TRUE(pixman_region32_equal(takeDamage().pixman(), CRegion{65, 125, 40, 20}.pixman()));
    m_monitor->m_output->needsFrame = false;

    // Windows below the lockscreen must not be considered renderable or damage the monitor.
    EXPECT_FALSE(g_pHyprRenderer->shouldRenderWindow(window.window, m_monitor));
    EXPECT_FALSE(g_pHyprRenderer->shouldRenderWindow(window.window));
    g_pHyprRenderer->damageWindow(window.window);
    g_pHyprRenderer->damageWindow(window.window, true);
    EXPECT_TRUE(takeDamage().empty());

    request(window.surfId, 1, {wl_resource_get_id(window.buffer->m_resource->getResource()), 0, 0});
    request(window.surfId, 2, {20, 30, 50, 25});
    request(window.surfId, 3, {m_nextID++}); // wl_surface.frame
    commit(window.surfId);
    EXPECT_TRUE(takeDamage().empty());
    EXPECT_FALSE(m_monitor->m_output->needsFrame);

    // With session_lock_xray enabled, hidden windows and layers below the lockscreen are rendered and must damage again.
    *CConfigValue<Config::BOOL>("misc:session_lock_xray").ptr() = true;
    EXPECT_TRUE(g_pHyprRenderer->shouldRenderWindow(window.window, m_monitor));
    request(window.surfId, 1, {wl_resource_get_id(window.buffer->m_resource->getResource()), 0, 0});
    request(window.surfId, 2, {20, 30, 50, 25});
    commit(window.surfId);
    expectDamage(CRegion{220, 230, 50, 25});

    request(belowLayer.surfId, 1, {wl_resource_get_id(belowLayer.buffer->m_resource->getResource()), 0, 0});
    request(belowLayer.surfId, 2, {10, 5, 40, 20});
    commit(belowLayer.surfId);
    expectDamage(CRegion{10, 5, 40, 20});

    // Unlocking restores normal damage even when session_lock_xray is false.
    *CConfigValue<Config::BOOL>("misc:session_lock_xray").ptr() = false;
    request(lock.lockId, 2); // ext_session_lock_v1.unlock_and_destroy
    takeDamage();

    EXPECT_TRUE(g_pHyprRenderer->shouldRenderWindow(window.window, m_monitor));
    request(window.surfId, 1, {wl_resource_get_id(window.buffer->m_resource->getResource()), 0, 0});
    request(window.surfId, 2, {5, 10, 30, 20});
    commit(window.surfId);
    expectDamage(CRegion{205, 210, 30, 20});
}
