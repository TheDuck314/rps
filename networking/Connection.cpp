#include "Connection.h"

#include "BotError.h"

#include <algorithm>
#include <cassert>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>

using namespace std::chrono;

/** Try a call, and throw NetworkingError if it fails. */
#define CHECK(x) do { if ((x) < 0) { throw NetworkingError("failed to execute " #x); } } while (false)

/** The number of slots in a pipe array. */
static constexpr int PIPE_PAIR = 2;
/** The index of the head of the pipe. */
static constexpr int PIPE_HEAD = 0;
/** The index of the tail of the pipe. */
static constexpr int PIPE_TAIL = 1;

/** The maximum length of reading from stderr, in bytes. */
static constexpr int MAX_STDERR_LENGTH = 1024 * 1024;

Connection::Connection(const std::string &command) {
    int write_pipe[PIPE_PAIR];  // will be child's stdin
    int read_pipe[PIPE_PAIR];   // will be child's stdout
    int error_pipe[PIPE_PAIR];  // will be child's stderr
    // Ignore SIGPIPE, as we want to detect bot exit gracefully.
    signal(SIGPIPE, SIG_IGN);
    CHECK(pipe(write_pipe));
    CHECK(pipe(read_pipe));
    CHECK(pipe(error_pipe));
    // Make the write pipe non-blocking
    CHECK(fcntl(write_pipe[PIPE_TAIL], F_SETFL, O_NONBLOCK));
    // Make the error pipe non-blocking
    CHECK(fcntl(error_pipe[PIPE_HEAD], F_SETFL, O_NONBLOCK));

    pid_t ppid_before_fork = getpid();
    pid_t pid = fork();
    if (pid == 0) {
        // This is the child
        setpgid(getpid(), getpid());  // sets child pgid equal to its pid? why do we do this?
        if (getppid() != ppid_before_fork) {
            throw NetworkingError("fork failed");
        }

        // Redirect stdin, stdout, and stderr
        CHECK(dup2(write_pipe[PIPE_HEAD], STDIN_FILENO));
        CHECK(close(write_pipe[PIPE_HEAD]));
        CHECK(dup2(read_pipe[PIPE_TAIL], STDOUT_FILENO));
        CHECK(close(read_pipe[PIPE_TAIL]));
        CHECK(dup2(error_pipe[PIPE_TAIL], STDERR_FILENO));
        CHECK(close(error_pipe[PIPE_TAIL]));

        // Execute the command
        CHECK(execl("/bin/sh", "sh", "-c", command.c_str(), nullptr));
        // Nothing past here should be run
        throw NetworkingError("exec failed");
    } else if (pid < 0) {
        throw NetworkingError("fork failed");
    } else {
        // This is the parent
        this->read_pipe = read_pipe[PIPE_HEAD];
        close(read_pipe[PIPE_TAIL]);
        this->write_pipe = write_pipe[PIPE_TAIL];
        close(write_pipe[PIPE_HEAD]);
        this->error_pipe = error_pipe[PIPE_HEAD];
        close(error_pipe[PIPE_TAIL]);
        this->child_pid = pid;
    }
}

Connection::~Connection() noexcept {
    // minus sign means send to the whole process group
    // (is this in case the child spawns sub-children?)
    kill(-child_pid, SIGKILL);
}

static milliseconds milliseconds_since(high_resolution_clock::time_point initial_time) {
    return duration_cast<milliseconds>(high_resolution_clock::now() - initial_time);
}

/**
 * Send a string along this connection.
 * @param message The string to send.
 * @throws NetworkingError if message could not be sent.
 */
void Connection::send_string(const std::string &message, milliseconds timeout) {
    const char* message_ptr = message.c_str();
    size_t chars_remaining =  message.length();

    const high_resolution_clock::time_point initial_time = high_resolution_clock::now();
    while (chars_remaining > 0) {
        if (milliseconds_since(initial_time) > timeout) {
            throw TimeoutError("when sending string", timeout);
        }

        const ssize_t chars_written = write(write_pipe, message_ptr, chars_remaining);
        if (chars_written < 0) {
            switch (errno) {
            case EAGAIN:
                continue;
            default:
                throw NetworkingError("could not send string");
            }
        }
        message_ptr += chars_written;
        chars_remaining -= chars_written;
    }
}

/**
 * Check a pipe to see if it has available bytes, blocking until timeout.
 * @param pipe The pipe to check.
 * @param timeout The timeout.
 * @return Result of select(2) on the pipe.
 */
static int check_pipe(int pipe, milliseconds timeout) {
    // make an fd_set that just contains one descriptor, pipe
    fd_set set;
    FD_ZERO(&set);
    assert(pipe <= FD_SETSIZE);
    FD_SET(pipe, &set);
    // make the timeout argument to select()
    struct timeval timeout_struct{};
    const seconds sec = duration_cast<seconds>(timeout);  // truncates
    const microseconds usec = duration_cast<microseconds>(timeout) - sec;  // converts to us
    timeout_struct.tv_sec = sec.count();
    timeout_struct.tv_usec = usec.count();
    // block for timeout, or until pipe can be read
    // "pipe + 1" is b/c select applies to all fd's in set that are < first arg
    return select(pipe + 1, &set, nullptr, nullptr, &timeout_struct);
}

/**
 * Get a string from this connection.
 * @param timeout The timeout to use.
 * @return The string read.
 * @throws NetworkingError on error while reading.
 */
std::string Connection::get_line(milliseconds timeout) {
    // Try the queue first
    if (!message_queue.empty()) {
        // COPY not reference since pop_front invalidates references
        const std::string message = message_queue.front();
        message_queue.pop_front();
        return message;
    }

    const high_resolution_clock::time_point initial_time = high_resolution_clock::now();
    while (true) {
        // Check if there are bytes in the pipe
        // First try uses no timeout, for some reason (which means non-blocking)
        int select_result = check_pipe(read_pipe, milliseconds::zero());
        if (select_result < 0) {
            throw NetworkingError("select failed", current_read);
        } else if (select_result == 0) {
            // non-blocking select said nothing was ready to be read. Try again, blocking for the
            // remaining time available.
            const milliseconds remaining = timeout - milliseconds_since(initial_time);
            if (remaining < milliseconds::zero()) {
                throw TimeoutError("when reading string", timeout, current_read);
            }
            select_result = check_pipe(read_pipe, remaining);
        }

        if (select_result < 0) {
            throw NetworkingError("select failed", current_read);
        } else if (select_result > 0) {
            // pipe can be read!
            // Read from the pipe, as many as we can into the buffer
            const ssize_t bytes_read = read(read_pipe, buffer.begin(), buffer.size());
            if (bytes_read <= 0) {
                throw NetworkingError("read failed", current_read);
            }
            // Iterator one past the last read character
            auto read_end = buffer.begin() + bytes_read;
            // Identify whether a newline exists in the buffer
            auto newline_pos = std::find(buffer.begin(), read_end, '\n');
            if (newline_pos == read_end) {
                // No newline was found, append the entire read to the result
                current_read += std::string(buffer.begin(), read_end);
                // Keep reading until we find a newline eventually
                continue;
            } else {
                // Newline was found; append the read to the result, up to but not including the newline
                current_read += std::string(buffer.begin(), newline_pos);
                const std::string result = current_read;

                // There may be more newlines; tokenize the remainder
                auto string_end = ++newline_pos;
                const std::string read = std::string(string_end, read_end);
                std::stringstream stream(read);
                bool more_messages = false;
                while (std::getline(stream, current_read)) {
                    more_messages = true;
                    message_queue.push_back(current_read);
                }
                if (more_messages && read.back() != '\n') {
                    // The last element is not yet done, don't queue it.
                    // Leave the partial line in current_read.
                    message_queue.pop_back();
                } else {
                    current_read = "";
                }
                return result;
            }
        }
    }
}

/**
 * Get the error output from this connection.
 * @return The error output.
 */
std::string Connection::get_errors() {
    std::string result;
    ssize_t bytes_read = 0;
    while ((bytes_read = read(error_pipe, buffer.begin(), buffer.size())) > 0
           && result.size() < MAX_STDERR_LENGTH) {
        result += std::string(buffer.begin(), buffer.begin() + bytes_read);
    }
    return result;
}