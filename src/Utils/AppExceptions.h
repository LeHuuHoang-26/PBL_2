#ifndef APPEXCEPTIONS_H
#define APPEXCEPTIONS_H

#include <exception>
#include <string>

class AppException : public std::exception {
protected:
    std::string message;
public:
    explicit AppException(const std::string &msg);
    virtual ~AppException() = default;
    const char* what() const noexcept override;
};

class MemoryAllocationException : public AppException {
public:
    explicit MemoryAllocationException(const std::string &msg = "Không đủ bộ nhớ để cấp phát đối tượng.");
};

class FileIOException : public AppException {
private:
    std::string filePath;
public:
    FileIOException(const std::string &path, const std::string &detail);
    const std::string& getFilePath() const;
};

class DataFormatException : public AppException {
public:
    explicit DataFormatException(const std::string &detail);
    DataFormatException(const std::string &file, int line, const std::string &detail);
};

class ValidationException : public AppException {
public:
    explicit ValidationException(const std::string &msg);
};

#endif // APPEXCEPTIONS_H
