#include <cstdint>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <cstdlib>

namespace {
    bool read_exact(int fd, void *buf, size_t count) {
        auto *p = static_cast<char *>(buf);
        size_t total = 0;
        while (total < count) {
            ssize_t r = read(fd, p + total, count - total);
            if (r <= 0) return false;
            total += static_cast<size_t>(r);
        }
        return true;
    }

    bool write_exact(int fd, const void *buf, size_t count) {
        const auto *p = static_cast<const char *>(buf);
        size_t total = 0;
        while (total < count) {
            ssize_t w = write(fd, p + total, count - total);
            if (w <= 0) return false;
            total += static_cast<size_t>(w);
        }
        return true;
    }

    bool send_msg(int fd, const std::string &s) {
        uint32_t len = static_cast<uint32_t>(s.size());
        return write_exact(fd, &len, sizeof(len)) &&
               (len == 0 || write_exact(fd, s.data(), len));
    }

    bool recv_msg(int fd, std::string &out) {
        uint32_t len = 0;
        if (!read_exact(fd, &len, sizeof(len))) return false;
        out.assign(len, '\0');
        return len == 0 || read_exact(fd, &out[0], len);
    }

    struct Pipe {
        int fds[2] = {-1, -1};
        Pipe() { if (pipe(fds) != 0) fds[0] = fds[1] = -1; }

        ~Pipe() {
            if (fds[0] >= 0) close(fds[0]);
            if (fds[1] >= 0) close(fds[1]);
        }

        bool valid() const { return fds[0] >= 0 && fds[1] >= 0; }
    };
}

int run_as_root_daemon() {
    if (geteuid() != 0) return 1;

    std::string cmd, output;
    while (recv_msg(STDIN_FILENO, cmd) && !cmd.empty()) {
        output.clear();
        int exit_code = -1;

        FILE *fp = popen((cmd + " 2>&1").c_str(), "r");
        if (fp) {
            char buf[512];
            while (fgets(buf, sizeof(buf), fp)) output += buf;
            int status = pclose(fp);
            if (WIFEXITED(status)) exit_code = WEXITSTATUS(status);
        }

        if (!write_exact(STDOUT_FILENO, &exit_code, sizeof(exit_code)) ||
            !send_msg(STDOUT_FILENO, output)) {
            break;
        }
    }
    return 0;
}

class RootSession {
public:
    static RootSession &instance() {
        static RootSession s;
        return s;
    }

    int exec(const std::string &cmd, std::string *out_output = nullptr) {
        if (!ensure_started()) return -1;

        if (!send_msg(to_child_, cmd)) {
            cleanup();
            return -1;
        }

        int exit_code = -1;
        std::string output;
        if (!read_exact(from_child_, &exit_code, sizeof(exit_code)) ||
            !recv_msg(from_child_, output)) {
            cleanup();
            return -1;
        }
        if (out_output) *out_output = std::move(output);
        return exit_code;
    }

    ~RootSession() { cleanup(); }

    RootSession(const RootSession &) = delete;

    RootSession &operator=(const RootSession &) = delete;

private:
    RootSession() = default;

    bool ensure_started() {
        if (pid_ > 0) return true;

        Pipe in, out;
        if (!in.valid() || !out.valid()) return false;

        pid_ = fork();
        if (pid_ < 0) {
            pid_ = -1;
            return false;
        }

        if (pid_ == 0) {
            dup2(in.fds[0], STDIN_FILENO);
            dup2(out.fds[1], STDOUT_FILENO);
            char self_path[PATH_MAX] = {};
            if (!realpath("/proc/self/exe", self_path)) _exit(127);
            execlp("pkexec", "pkexec", self_path, "--root-daemon", nullptr);
            _exit(127); // execlp 失败
        }

        to_child_ = in.fds[1];
        from_child_ = out.fds[0];
        in.fds[1] = -1;
        out.fds[0] = -1;
        return true;
    }

    void cleanup() {
        if (pid_ <= 0) return;
        uint32_t stop = 0;
        write(to_child_, &stop, sizeof(stop));
        close(to_child_);
        close(from_child_);
        waitpid(pid_, nullptr, 0);
        pid_ = -1;
        to_child_ = from_child_ = -1;
    }

    int to_child_ = -1;
    int from_child_ = -1;
    pid_t pid_ = -1;
};

// 对外接口
int root_system(const std::string &cmd, std::string *output) {
    return RootSession::instance().exec(cmd, output);
}

int root_system(const std::string &cmd) {
    return RootSession::instance().exec(cmd, nullptr);
}
