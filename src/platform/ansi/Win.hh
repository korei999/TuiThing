#pragma once

#include "IWindow.hh"
#include "TextBuff.hh"
#include "TermSize.hh"

#include <termios.h>

namespace platform::ansi
{

struct Win : public IWindow
{
    struct MouseInput
    {
        enum class KEY : adt::u8 { NONE, WHEEL_UP, WHEEL_DOWN, LEFT, MIDDLE, RIGHT, RELEASE };

        /* */

        KEY eKey {};
        int x {};
        int y {};

        /* */

        friend bool operator==(MouseInput a, MouseInput b)
        {
            return a.eKey == b.eKey && a.x == b.x && a.y == b.y;
        }
    };

    struct Input
    {
        enum class TYPE : adt::u8 { KB, MOUSE };

        /* */

        MouseInput mouse {};
        int key {};
        TYPE eType {};
    };

    static constexpr MouseInput INVALID_MOUSE {.eKey = MouseInput::KEY::NONE, .x = -1, .y = -1};

    /* */

    adt::Arena* m_pArena {};
    TextBuff m_textBuff {};
    termios m_termOg {};
    TermSize m_termSize {};
    adt::i16 m_firstIdx {};
    int m_prevImgWidth = 0;
    adt::Mutex m_mtxUpdate {};
    adt::f64 m_time {};
    adt::f64 m_lastResizeTime {};
    Input m_lastInput {};
    int m_lastMouseSelection {};
    adt::f64 m_lastMouseSelectionTime {};
    bool m_bStarted {};

    /* */

public:
    virtual bool start(adt::Arena* pArena) override;
    virtual void destroy() override;
    virtual void update() override;
    virtual void procEvents() override;

    /* */

protected:
    void disableRawMode() noexcept(false); /* RuntimeException */
    void enableRawMode() noexcept(false); /* RuntimeException */

    /* input */
    void procInput();
    /* */

    void updateBoilerplate();

    friend void sigwinchHandler(int sig);

    Input readFromStdin(const int timeoutMS);
    [[nodiscard]] ADT_NO_UB int parseSeq(adt::Span<char> spBuff, ssize_t nRead);
    [[nodiscard]] ADT_NO_UB MouseInput parseMouse(adt::Span<char> spBuff, ssize_t nRead);
    void procMouse(MouseInput in);
};

} /* namespace platform::ansi */

namespace adt::print
{

inline isize
format(Context* ctx, FormatArgs fmtArgs, const platform::ansi::Win::MouseInput::KEY& x)
{
    constexpr StringView svMap[] { "NONE", "WHEEL_UP", "WHEEL_DOWN", "LEFT", "MIDDLE", "RIGHT", "RELEASE" };
    return format(ctx, fmtArgs, svMap[(int)x]);
}

inline isize
format(Context* ctx, FormatArgs fmtArgs, const platform::ansi::Win::MouseInput& x)
{
    fmtArgs.eFmtFlags |= FormatArgs::FLAGS::PARENTHESES;
    return formatVariadic(ctx, fmtArgs, x.eKey, x.x, x.y);
}

} /* namespace adt::print */
