#pragma once

#include "distplusplus.pb.h"

#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace distplusplus::common {

class CompressorFactory final {
private:
    distplusplus::CompressionType _compressionType;
    std::int64_t _compressionLevel = 0;

public:
    class Compressor final {
    private:
        distplusplus::CompressionType _compressionType;
        std::int64_t _compressionLevel = 0;
        std::variant<std::string, std::string_view> _data;

    public:
        Compressor(distplusplus::CompressionType compressionType, std::int64_t compressionLevel,
                   const std::string &input);
        [[nodiscard]] distplusplus::CompressionType compressionType() const;
        [[nodiscard]] std::string_view data() const;
    };

    CompressorFactory(distplusplus::CompressionType compressionType, std::int64_t compressionLevel);
    [[nodiscard]] distplusplus::CompressionType compressionType() const;
    [[nodiscard]] Compressor operator()(const std::string &input) const;
};

using Compressor = CompressorFactory::Compressor;

class Decompressor final {
private:
    distplusplus::CompressionType _compressionType;
    std::variant<std::string, std::string_view> _data;

public:
    Decompressor(distplusplus::CompressionType compressionType, const std::string &input);
    [[nodiscard]] std::string_view data() const;
};

} // namespace distplusplus::common
