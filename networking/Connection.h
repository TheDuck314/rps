#pragma once

#include <array>
#include <deque>
#include <string>
#include <chrono>

class Connection final {
private:
    /** The maximum bytes that will be read after a bot dies. */
    constexpr static int MAX_TRAILING_INPUT_READ = 4 * 1024;
    
    // stuff for storing what we read from the child process
    static constexpr size_t READ_BUFFER_SIZE = 256; /**< The buffer size for reading from bots. */
    std::array<char, READ_BUFFER_SIZE> buffer{};  /**< The bot read buffer. */
    std::string current_read;                     /**< Accumulated last read, waiting for a newline. */
    std::deque<std::string> message_queue;        /**< Queue for command output lines that have not been retrieved. */

    pid_t child_pid;
    // file descriptors for the pipes used to talk to the child process:
    int read_pipe;
    int write_pipe;
    int error_pipe;

public:
    // run command in another process and set up pipes to talk to it
    Connection(const std::string &command);

    // kills the child process
    ~Connection();

    /**
     * Send a string along this connection.
     * @param message The string to send.
     * @throws NetworkingError if message could not be sent.
     */
    void send_string(const std::string &message, std::chrono::milliseconds timeout);

    /**
     * Get a string from this connection.
     * We read from the connection until we see a newline.
     * @param timeout The timeout to use.
     * @return The string read.
     * @throws NetworkingError on error while reading.
     */
    std::string get_line(std::chrono::milliseconds timeout);

    /**
     * Read any remaining input from the pipe.
     * @return The remaining input.
     */
    std::string read_trailing_input();

    /**
     * Get the error output from this connection (everything the command printed to
     * stderr).
     * @return The error output.
     */
    std::string get_errors();
};