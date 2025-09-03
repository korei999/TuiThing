#include "Win.hh"

#include "keys.hh"

#include <sys/select.h>

using namespace adt;

namespace platform::ansi
{

/* https://github.com/termbox/termbox2/blob/master/termbox2.h */
Win::MouseInput
Win::parseMouse(Span<char> spBuff, ssize_t nRead)
{
    enum
    {
        TYPE_VT200 = 0,
        TYPE_1006,
        TYPE_1015,
        TYPE_MAX
    };

    const char* aCmpMap[TYPE_MAX] {
        "\x1b[M",
        "\x1b[<",
        "\x1b["
    };

    int type = 0;
    for (; type < TYPE_MAX; type++)
    {
        isize size = strlen(aCmpMap[type]);

        if (spBuff.size() >= size && (strncmp(aCmpMap[type], spBuff.data(), size)) == 0)
            break;
    }

    if (type == TYPE_MAX)
        return INVALID_MOUSE;

    MouseInput ret = INVALID_MOUSE;

    switch (type)
    {
        case TYPE_VT200:
        {
            if (nRead >= 6)
            {
                int b = spBuff[3] - 0x20;
                int fail = 0;

                switch (b & 3)
                {
                    case 0:
                    ret.eKey = ((b & 64) != 0) ? MouseInput::KEY::WHEEL_UP : MouseInput::KEY::LEFT;
                    break;

                    case 1:
                    ret.eKey = ((b & 64) != 0) ? MouseInput::KEY::WHEEL_DOWN : MouseInput::KEY::MIDDLE;
                    break;

                    case 2:
                    ret.eKey = MouseInput::KEY::RIGHT;
                    break;

                    case 3:
                    ret.eKey = MouseInput::KEY::RELEASE;
                    break;

                    default:
                    ret = INVALID_MOUSE;
                    fail = 1;
                    break;
                }

                if (!fail)
                {
                    /* the coord is 1,1 for upper left */
                    ret.x = ((u8)spBuff[4]) - 0x21;
                    ret.y = ((u8)spBuff[5]) - 0x21;
                }
            }
        }
        break; /* case TYPE_VT200: */

        case TYPE_1006:
        [[fallthrough]];
        case TYPE_1015:
        {
            isize indexFail = -1;

            enum
            {
                FIRST_M = 0,
                FIRST_SEMICOLON,
                LAST_SEMICOLON,
                FIRST_LAST_MAX
            };

            isize aIndices[FIRST_LAST_MAX] = {indexFail, indexFail, indexFail};
            bool bCapital = 0;

            for (isize i = 0; i < spBuff.size(); ++i)
            {
                if (spBuff[i] == ';')
                {
                    if (aIndices[FIRST_SEMICOLON] == indexFail)
                    {
                        aIndices[FIRST_SEMICOLON] = i;
                    }
                    else
                    {
                        aIndices[LAST_SEMICOLON] = i;
                    }
                }
                else if (aIndices[FIRST_M] == indexFail)
                {
                    if (spBuff[i] == 'm' || spBuff[i] == 'M')
                    {
                        bCapital = (spBuff[i] == 'M');
                        aIndices[FIRST_M] = i;
                    }
                }
            }

            if (aIndices[FIRST_M] == indexFail || aIndices[FIRST_SEMICOLON] == indexFail ||
                aIndices[LAST_SEMICOLON] == indexFail)
            {
                return INVALID_MOUSE;
            }
            else
            {
                int start = (type == TYPE_1015 ? 2 : 3);

                unsigned n1 = strtoul(&spBuff[start], nullptr, 10);
                unsigned n2 = strtoul(&spBuff[aIndices[FIRST_SEMICOLON] + 1], nullptr, 10);
                unsigned n3 = strtoul(&spBuff[aIndices[LAST_SEMICOLON] + 1], nullptr, 10);

                if (type == TYPE_1015)
                {
                    n1 -= 0x20;
                }

                int fail = 0;

                switch (n1 & 3)
                {
                    case 0:
                    ret.eKey = ((n1 & 64) != 0) ? MouseInput::KEY::WHEEL_UP : MouseInput::KEY::LEFT;
                    break;

                    case 1:
                    ret.eKey = ((n1 & 64) != 0) ? MouseInput::KEY::WHEEL_DOWN : MouseInput::KEY::MIDDLE;
                    break;

                    case 2:
                    ret.eKey = MouseInput::KEY::RIGHT;
                    break;

                    case 3:
                    ret.eKey = MouseInput::KEY::RELEASE;
                    break;

                    default:
                    fail = 1;
                    break;
                }

                if (!fail)
                {
                    if (!bCapital)
                    {
                        ret.eKey = MouseInput::KEY::RELEASE;
                    }

                    ret.x = (n2) - 1;
                    ret.y = (n3) - 1;
                }
            }
        }
        break; /* case TYPE_1015: */
    }

    return ret;
}

void
Win::procMouse(MouseInput in)
{
}

int
Win::parseSeq(Span<char> spBuff, ssize_t nRead)
{
    if (nRead <= 5)
    {
        switch (*(int*)(&spBuff[1]))
        {
            case 17487:
            return keys::ARROWLEFT;

            case 17231:
            return keys::ARROWRIGHT;

            case 16719:
            return keys::ARROWUP;

            case 16975:
            return keys::ARROWDOWN;

            case 8271195:
            return keys::PGUP;

            case 8271451:
            return keys::PGDN;

            case 18511:
            return keys::HOME;

            case 17999:
            return keys::END;
        }
    }

    return 0;
}

Win::Input
Win::readFromStdin(const int timeoutMS)
{
    char aBuff[128] {};
    fd_set fds {};
    FD_SET(STDIN_FILENO, &fds);

    timeval tv;
    tv.tv_sec = timeoutMS / 1000;
    tv.tv_usec = (timeoutMS - (tv.tv_sec * 1000)) * 1000;

    select(1, &fds, {}, {}, &tv);
    ssize_t nRead = read(STDIN_FILENO, aBuff, sizeof(aBuff));

    if (nRead > 1)
    {
        MouseInput mouse = parseMouse({aBuff, sizeof(aBuff)}, nRead);
        if (mouse != INVALID_MOUSE)
        {
            return {
                .mouse = mouse,
                .key = 0,
                .eType = Input::TYPE::MOUSE,
            };
        }

        int k = parseSeq({aBuff, sizeof(aBuff)}, nRead);
        if (k != 0)
        {
            return {
                .key = k,
                .eType = Input::TYPE::KB,
            };
        }
    }

    wchar_t wc {};
    mbtowc(&wc, aBuff, sizeof(aBuff));

    return {
        .mouse = INVALID_MOUSE,
        .key = static_cast<int>(wc),
        .eType = Input::TYPE::KB,
    };
}

void
Win::procInput()
{
    const Input in = readFromStdin(1000);

    switch (in.eType)
    {
        case Input::TYPE::KB:
        break;

        case Input::TYPE::MOUSE:
        procMouse(in.mouse);
        break;
    }
}

} /* namespace platform::ansi */
