#pragma once
// MBASICC - MBASIC 5.21 C++ Interpreter
// https://github.com/jwoehr/mbasic_cc
//
// File Handler Abstraction
// This interface allows the interpreter's file I/O to be portable across
// different platforms (native filesystem, WebAssembly virtual FS, etc.)

#include <string>
#include <memory>
#include <cstdint>

namespace mbasic {

// ============================================================================
// FileHandle - Abstract interface for file operations
// ============================================================================
// Implement this interface for platform-specific file I/O.
// Used by BASIC's OPEN, CLOSE, INPUT#, PRINT#, GET, PUT statements.

class FileHandle {
public:
    virtual ~FileHandle() = default;

    // Check if file is open
    virtual bool is_open() const = 0;

    // Close the file
    virtual void close() = 0;

    // Read a line (for sequential files)
    // Returns false on EOF or error
    virtual bool read_line(std::string& line) = 0;

    // Write a line (for sequential files)
    virtual void write_line(const std::string& line) = 0;

    // Write string without newline
    virtual void write(const std::string& data) = 0;

    // Read n characters
    virtual std::string read_chars(int n) = 0;

    // Check for end of file
    virtual bool eof() const = 0;

    // Get current position (LOC function)
    virtual int64_t position() const = 0;

    // Get file length (LOF function)
    virtual int64_t length() const = 0;

    // For random access files:

    // Seek to record number (1-based)
    virtual void seek_record(int record, int record_length) = 0;

    // Read raw bytes into buffer
    virtual void read_raw(char* buffer, int size) = 0;

    // Write raw bytes from buffer
    virtual void write_raw(const char* buffer, int size) = 0;

    // Flush output
    virtual void flush() = 0;
};

// ============================================================================
// FileSystem - Abstract factory for file operations
// ============================================================================
// Implement this interface to provide custom file system access.
// Default implementation uses std::fstream for native file system.

class FileSystem {
public:
    virtual ~FileSystem() = default;

    // File open modes matching BASIC's OPEN statement
    enum class Mode {
        INPUT,      // "I" - Sequential input
        OUTPUT,     // "O" - Sequential output (truncates existing file)
        APPEND,     // "A" - Sequential append
        RANDOM      // "R" - Random access (default)
    };

    // Open a file
    // @param filename: The file path
    // @param mode: Open mode
    // @param record_length: Record length for random access (default 128)
    // @return: A FileHandle, or nullptr on failure
    virtual std::unique_ptr<FileHandle> open(
        const std::string& filename,
        Mode mode,
        int record_length = 128) = 0;

    // Check if a file exists
    virtual bool exists(const std::string& filename) = 0;

    // Delete a file (KILL command)
    virtual bool remove(const std::string& filename) = 0;

    // Rename a file (NAME command)
    virtual bool rename(const std::string& old_name, const std::string& new_name) = 0;

    // Get the default/native file system implementation
    static std::unique_ptr<FileSystem> create_native();
};

// ============================================================================
// NativeFileHandle - std::fstream-based implementation
// ============================================================================
// Default implementation for native filesystem I/O

class NativeFileHandle : public FileHandle {
public:
    NativeFileHandle();
    ~NativeFileHandle() override;

    // Open file with specified mode
    bool open_file(const std::string& filename, FileSystem::Mode mode, int record_length);

    // FileHandle interface
    bool is_open() const override;
    void close() override;
    bool read_line(std::string& line) override;
    void write_line(const std::string& line) override;
    void write(const std::string& data) override;
    std::string read_chars(int n) override;
    bool eof() const override;
    int64_t position() const override;
    int64_t length() const override;
    void seek_record(int record, int record_length) override;
    void read_raw(char* buffer, int size) override;
    void write_raw(const char* buffer, int size) override;
    void flush() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// NativeFileSystem - std::fstream-based implementation
// ============================================================================

class NativeFileSystem : public FileSystem {
public:
    std::unique_ptr<FileHandle> open(
        const std::string& filename,
        Mode mode,
        int record_length = 128) override;

    bool exists(const std::string& filename) override;
    bool remove(const std::string& filename) override;
    bool rename(const std::string& old_name, const std::string& new_name) override;
};

} // namespace mbasic
