#pragma once

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>

namespace Core::Util {
struct MappedFile {
	void* data;
	size_t size;
	int fd;

	[[nodiscard]] static auto open(const char* filename)
	    -> std::expected<MappedFile, std::error_code>;

	~MappedFile();

	MappedFile(const MappedFile&) = delete;
	MappedFile& operator=(const MappedFile&) = delete;

	MappedFile(MappedFile&& other) noexcept;
	MappedFile& operator=(MappedFile&& other) noexcept;

	[[nodiscard]] constexpr auto get_data() const noexcept -> const char* {
		return static_cast<const char*>(data);
	}
	[[nodiscard]] constexpr auto get_size() const noexcept -> size_t {
		return size;
	}
	[[nodiscard]] auto view() const noexcept -> std::string_view;
	[[nodiscard]] auto to_string() const -> std::string;

      private:
	MappedFile(void* data, size_t size, int fd) noexcept;
};
} // namespace Core::Util
