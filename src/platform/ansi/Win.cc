#include "Win.hh"

#include <csignal>
#include <unistd.h>
#include <sys/ioctl.h>

using namespace adt;

namespace platform::ansi
{

void
Win::disableRawMode()
{
    ADT_RUNTIME_EXCEPTION(tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_termOg) != -1);
}

void
Win::enableRawMode()
{
    ADT_RUNTIME_EXCEPTION(tcgetattr(STDIN_FILENO, &m_termOg) != -1);

    struct termios raw = m_termOg;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0; /* disable blocking on read() */
    /*raw.c_cc[VTIME] = 1;*/

    ADT_RUNTIME_EXCEPTION(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != -1);
}

void
Win::resizeHandler()
{
    if (g_bSigResize)
    {
        g_bSigResize = false;

        m_textBuff.m_bResize = true;
        m_bClear = true;

        TermSize termSize = getTermSize();
        if (termSize.width == -1)
        {
            LogError{"termSize.width: {}, (setting to 0)", termSize.width};
            termSize.width = 0;
        }
        if (termSize.height == -1)
        {
            LogError{"termSize.height: {}, (setting to 0)", termSize.height};
            termSize.height = 0;
        }

        m_termSize = termSize;
        m_textBuff.resize(m_termSize.width, m_termSize.height);
        LogInfo{"resize: {}\n", m_termSize};
    }

    if (m_bClear)
    {
        m_bClear = false;
        m_textBuff.erase();
    }
}

void
sigwinchHandler(int)
{
    IWindow::g_bSigResize = true;
}

bool
Win::start(Arena* pArena)
{
    m_pArena = pArena;
    m_termSize = getTermSize();

    m_mtxUpdate = Mutex(Mutex::TYPE::PLAIN);

    enableRawMode();
    m_textBuff.start(m_pArena, m_termSize.width, m_termSize.height);

    signal(SIGWINCH, sigwinchHandler);

    m_bStarted = true;
    LogDebug("start()\n");

    return true;
}

void
Win::destroy()
{
    if (!m_bStarted) return;

    disableRawMode();
    m_mtxUpdate.destroy();

    m_textBuff.destroy();

    LogDebug("ansi::WinDestroy()\n");
}

void
Win::redraw()
{
    resizeHandler();
    update();
}

void
Win::update()
{
}

void
Win::procEvents()
{
    procInput();
}

} /* namespace platform::ansi */
