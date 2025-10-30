#include "utils/load_file.h"
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Core::Util {
MappedFile::MappedFile(void* data, size_t size, int fd) noexcept
    : data(data), size(size), fd(fd) {
}

auto MappedFile::open(const char* filename)
    -> std::expected<MappedFile, std::error_code> {

	int fd = ::open(filename, O_RDONLY);
	if (fd == -1) {
		return std::unexpected(
		    std::error_code(errno, std::system_category()));
	}

	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		auto ec = std::error_code(errno, std::system_category());
		::close(fd);
		return std::unexpected(ec);
	}

	void* data = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED) {
		auto ec = std::error_code(errno, std::system_category());
		::close(fd);
		return std::unexpected(ec);
	}

	return MappedFile(data, sb.st_size, fd);
}

MappedFile::~MappedFile() {
	if (data != nullptr && data != MAP_FAILED) {
		munmap(data, size);
	}
	if (fd != -1) {
		::close(fd);
	}
}

MappedFile::MappedFile(MappedFile&& other) noexcept
    : data(other.data), size(other.size), fd(other.fd) {
	other.data = nullptr;
	other.size = 0;
	other.fd = -1;
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
	if (this != &other) {
		if (data != nullptr && data != MAP_FAILED) {
			munmap(data, size);
		}
		if (fd != -1) {
			::close(fd);
		}

		data = other.data;
		size = other.size;
		fd = other.fd;

		other.data = nullptr;
		other.size = 0;
		other.fd = -1;
	}
	return *this;
}

auto MappedFile::view() const noexcept -> std::string_view {
	return std::string_view(get_data(), size);
}

auto MappedFile::to_string() const -> std::string {
	return std::string(get_data(), size);
}
} // namespace Core::Util
