#include "AppExceptions.h"

AppException::AppException(const std::string &msg)
    : message(msg) {}

const char* AppException::what() const noexcept {
    return message.c_str();
}

MemoryAllocationException::MemoryAllocationException(const std::string &msg)
    : AppException(msg) {}

FileIOException::FileIOException(const std::string &path, const std::string &detail)
    : AppException("Lỗi tệp tin [" + path + "]: " + detail) {}

DataFormatException::DataFormatException(const std::string &detail)
    : AppException("Lỗi định dạng dữ liệu: " + detail) {}

DataFormatException::DataFormatException(const std::string &file, int line, const std::string &detail)
    : AppException("Lỗi dữ liệu tại [" + file + ":" + std::to_string(line) + "]: " + detail) {}

ValidationException::ValidationException(const std::string &msg)
    : AppException("Dữ liệu không hợp lệ: " + msg) {}
