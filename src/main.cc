#include "platform/ansi/Win.hh"

using namespace adt;

struct Window : platform::ansi::Win
{
    using Base = platform::ansi::Win;

    /* */

    virtual void update() final;
    virtual void procEvents() final;
};

void
Window::update()
{
    Arena* pArena = IThreadPool::inst()->arena();
    ArenaScope arenaScope {pArena};

    m_textBuff.clean();

    using STYLE = platform::ansi::TEXT_BUFF_STYLE;

    constexpr StringView svHello = "HELLO TUI";
    m_textBuff.string(
        (m_termSize.width-svHello.size()) / 2,
        (m_termSize.height - 1) / 2,
        STYLE::YELLOW | STYLE::BOLD | STYLE::ITALIC | STYLE::BLINK,
        svHello
    );

    for (isize i = 0; i < m_termSize.height; ++i)
    {
        char aBuff[8] {};
        const isize n = print::toSpan(aBuff, "{}", i);
        m_textBuff.string(0, i,
            STYLE::GREEN | STYLE::BOLD | STYLE::BLINK,
            aBuff, n
        );
    }

    m_textBuff.present();
}

void
Window::procEvents()
{
    const Input in = readFromStdin(1000);

    switch (in.eType)
    {
        case Input::TYPE::KB:
        if (in.key == L'q' || in.key == L'Q')
            m_bRunning = false;
        break;

        case Input::TYPE::MOUSE:
        // LogDebug{"mouse: {}\n", in.mouse};
        break;
    }
}

static void
go()
{
    Arena* pArena = IThreadPool::inst()->arena();

    IWindow* pWin = IWindow::inst();
    pWin->start(pArena);
    defer( pWin->destroy() );

    pWin->m_bRunning = true;

    while (pWin->m_bRunning)
    {
        pWin->redraw();
        pWin->procEvents();

        ADT_ASSERT(pArena->memoryUsed() == 0, "{}", pArena->memoryUsed());
    }
}

int
main()
{
    ThreadPool ztp {SIZE_1M * 64};
    IThreadPool::setGlobal(&ztp);
    defer( ztp.destroy() );

    Logger logger {stderr, ILogger::LEVEL::DEBUG, 512, true};
    Logger::setGlobal(&logger);
    defer( logger.destroy() );

    try
    {
        Window win {};
        IWindow::setGlobal(&win);

        go();
    }
    catch (const std::exception& ex)
    {
        LogError{"{}\n", ex.what()};
    }
}
