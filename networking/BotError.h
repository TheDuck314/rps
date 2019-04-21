#pragma once

#include <chrono>
#include <cstring>
#include <exception>
#include <sstream>
#include <string>

/** Abstract base class for an error a bot can make. */
struct BotError : public std::exception {
};

/** Generic networking exception. */
struct NetworkingError : public BotError {
private:
    std::string result;                /** The description buffer. */
public:
    const std::string message;         /** The message. */
    const std::string remaining_input; /**< The remaining input. */
    const int recorded_errno;          /** The captured errno. */

    /**
     * Construct NetworkingError from message and remaining input.
     * @param message The message.
     * @param remaining_input The remaining input.
     */
    explicit NetworkingError(std::string message, std::string remaining_input = "")
            : message(std::move(message)), remaining_input(std::move(remaining_input)), recorded_errno(errno) {
        std::stringstream stream;
        stream << "communication error with bot: "
               << this->message
               << ", errno was: "
               << recorded_errno
               << " ("
               << std::strerror(recorded_errno)
               << ")";
        result = stream.str();
    }

    /**
     * Get the exception description.
     * @return The exception description.
     */
    const char *what() const noexcept override {
        return result.c_str();
    }
};

/** Thrown when a network event times out. */
struct TimeoutError : public BotError {
private:
    std::string result;                   /**< The description buffer. */
public:
    const std::string message;            /**< The message. */
    const std::chrono::milliseconds time; /**< The time elapsed. */
    const std::string remaining_input;    /**< The remaining input. */

    /**
     * Construct TimeoutError from message, time, and remaining input.
     * @param message The message.
     * @param time The time elapsed.
     * @param remaining_input The remaining input.
     */
    explicit TimeoutError(std::string message,
                          std::chrono::milliseconds time,
                          std::string remaining_input = "")
            : message(std::move(message)), time(time), remaining_input(std::move(remaining_input)) {
        std::ostringstream stream;
        stream << "timed out after "
               << std::to_string(time.count())
               << " ms ("
               << this->message
               << ")";
        result = stream.str();
    }

    /**
     * Get the exception description.
     * @return The exception description.
     */
    const char *what() const noexcept override {
        return result.c_str();
    }
};

