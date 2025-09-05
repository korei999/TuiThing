#pragma once

struct IWindow
{
    enum class READ_STATUS : adt::u8 { OK_, DONE, BACKSPACE };

    /* */

    static inline IWindow* g_pInstance {};
    static inline volatile bool g_bSigResize {};

    /* */

    bool m_bRedraw {};
    bool m_bClear {};
    bool m_bRunning {};

    /* */

    [[nodiscard]] static IWindow* inst() noexcept { return g_pInstance; }
    static IWindow* setGlobal(IWindow* pWin) noexcept
    {
        if (g_pInstance != nullptr)
        {
            adt::LogWarn{"global instance has been already set\n"};
            return g_pInstance;
        }
        g_pInstance = pWin;
        return g_pInstance;
    }

    /* */

    virtual bool start(adt::Arena* pArena) = 0;
    virtual void destroy() = 0;
    virtual void redraw() = 0;
    virtual void procEvents() = 0;

protected:
    virtual void update() = 0;
};

struct DummyWindow : IWindow
{
    virtual bool start(adt::Arena*) final { return true; };
    virtual void destroy() final { };
    virtual void redraw() final {};
    virtual void update() final {};
    virtual void procEvents() final {};
};
