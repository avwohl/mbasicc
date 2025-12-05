// MBASICC - MBASIC 5.21 C++ Interpreter
// https://github.com/jwoehr/mbasic_cc
//
// File Handler Implementation - Native file system using std::fstream

#include "mbasic/file_handler.hpp"
#include <fstream>
#include <cstdio>  // for std::remove, std::rename

namespace mbasic {

// ============================================================================
// NativeFileHandle Implementation
// ============================================================================

struct NativeFileHandle::Impl {
    std::fstream file;
    FileSystem::Mode mode;
    int record_length;
    mutable int64_t cached_length = -1;  // Cached file length
};

NativeFileHandle::NativeFileHandle() : impl_(std::make_unique<Impl>()) {}

NativeFileHandle::~NativeFileHandle() {
    if (impl_ && impl_->file.is_open()) {
        impl_->file.close();
    }
}

bool NativeFileHandle::open_file(const std::string& filename,
                                  FileSystem::Mode mode,
                                  int record_length) {
    impl_->mode = mode;
    impl_->record_length = record_length;
    impl_->cached_length = -1;

    std::ios_base::openmode open_mode{};

    switch (mode) {
        case FileSystem::Mode::INPUT:
            open_mode = std::ios::in;
            break;
        case FileSystem::Mode::OUTPUT:
            open_mode = std::ios::out | std::ios::trunc;
            break;
        case FileSystem::Mode::APPEND:
            open_mode = std::ios::out | std::ios::app;
            break;
        case FileSystem::Mode::RANDOM:
            open_mode = std::ios::in | std::ios::out | std::ios::binary;
            break;
    }

    impl_->file.open(filename, open_mode);

    // For random access, try to create file if it doesn't exist
    if (!impl_->file && mode == FileSystem::Mode::RANDOM) {
        impl_->file.clear();
        impl_->file.open(filename, std::ios::out | std::ios::binary);
        impl_->file.close();
        impl_->file.open(filename, open_mode);
    }

    return impl_->file.is_open();
}

bool NativeFileHandle::is_open() const {
    return impl_->file.is_open();
}

void NativeFileHandle::close() {
    if (impl_->file.is_open()) {
        impl_->file.close();
    }
}

bool NativeFileHandle::read_line(std::string& line) {
    if (!impl_->file || impl_->file.eof()) {
        return false;
    }
    return static_cast<bool>(std::getline(impl_->file, line));
}

void NativeFileHandle::write_line(const std::string& line) {
    impl_->file << line << '\n';
}

void NativeFileHandle::write(const std::string& data) {
    impl_->file << data;
}

std::string NativeFileHandle::read_chars(int n) {
    std::string result(n, '\0');
    impl_->file.read(&result[0], n);
    result.resize(static_cast<size_t>(impl_->file.gcount()));
    return result;
}

bool NativeFileHandle::eof() const {
    return impl_->file.eof() || impl_->file.peek() == std::char_traits<char>::eof();
}

int64_t NativeFileHandle::position() const {
    auto pos = impl_->file.tellg();
    if (pos == std::streampos(-1)) {
        return 0;
    }

    // For random files, return record number
    if (impl_->mode == FileSystem::Mode::RANDOM && impl_->record_length > 0) {
        return static_cast<int64_t>(pos) / impl_->record_length + 1;
    }

    // For sequential files, return byte position
    return static_cast<int64_t>(pos);
}

int64_t NativeFileHandle::length() const {
    // Cache the length to avoid seeking on every call
    if (impl_->cached_length >= 0) {
        return impl_->cached_length;
    }

    // Save current position
    auto current_pos = impl_->file.tellg();

    // Seek to end
    impl_->file.seekg(0, std::ios::end);
    auto end_pos = impl_->file.tellg();

    // Restore position
    impl_->file.seekg(current_pos);

    impl_->cached_length = static_cast<int64_t>(end_pos);
    return impl_->cached_length;
}

void NativeFileHandle::seek_record(int record, int record_length) {
    // Records are 1-based in BASIC
    int64_t offset = static_cast<int64_t>(record - 1) * record_length;
    impl_->file.seekg(offset);
    impl_->file.seekp(offset);
}

void NativeFileHandle::read_raw(char* buffer, int size) {
    impl_->file.read(buffer, size);
}

void NativeFileHandle::write_raw(const char* buffer, int size) {
    impl_->file.write(buffer, size);
    impl_->cached_length = -1;  // Invalidate cached length
}

void NativeFileHandle::flush() {
    impl_->file.flush();
}

// ============================================================================
// NativeFileSystem Implementation
// ============================================================================

std::unique_ptr<FileHandle> NativeFileSystem::open(
    const std::string& filename,
    Mode mode,
    int record_length) {

    auto handle = std::make_unique<NativeFileHandle>();
    if (handle->open_file(filename, mode, record_length)) {
        return handle;
    }
    return nullptr;
}

bool NativeFileSystem::exists(const std::string& filename) {
    std::ifstream f(filename);
    return f.good();
}

bool NativeFileSystem::remove(const std::string& filename) {
    return std::remove(filename.c_str()) == 0;
}

bool NativeFileSystem::rename(const std::string& old_name, const std::string& new_name) {
    return std::rename(old_name.c_str(), new_name.c_str()) == 0;
}

// ============================================================================
// Factory function
// ============================================================================

std::unique_ptr<FileSystem> FileSystem::create_native() {
    return std::make_unique<NativeFileSystem>();
}

} // namespace mbasic
