#include "compression_helper.hpp"

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/log/trivial.hpp>
#include <gsl/gsl_util>
#include <sstream>
#include <stdexcept>
#include <utility>
#ifdef DISTPLUSPLUS_WITH_ZSTD
#include <boost/iostreams/filter/zstd.hpp>
#endif

namespace distplusplus::common {

namespace bios = boost::iostreams;

Compressor::Compressor(distplusplus::CompressionType compressionType, std::int64_t compressionLevel,
                       const std::string &input)
    : _compressionType(compressionType), _compressionLevel(compressionLevel) {
    switch (_compressionType) {
    case NONE: {
        _data = std::string_view(input);
        break;
    }
    case zstd: {
#ifdef DISTPLUSPLUS_WITH_ZSTD
        std::stringstream compressed;
        std::stringstream origin(input);
        bios::filtering_stream<bios::input> out;
        out.push(bios::zstd_compressor(bios::zstd_params(gsl::narrow_cast<uint32_t>(_compressionLevel))));
        out.push(origin);
        bios::copy(out, compressed);
        _data = std::move(compressed).str();
#else
        const std::string err = "attempted to use zstd but built without zstd support";
        BOOST_LOG_TRIVIAL(fatal) << err;
        throw std::runtime_error(err);
#endif
        break;
    }
    default: {
        const std::string err = "encountered unknown CompressionType enum value " +
                                std::to_string(compressionType) + " in compressor";
        BOOST_LOG_TRIVIAL(fatal) << err;
        throw std::runtime_error(err);
    }
    }
}

std::string_view Compressor::data() const {
    if (_data.index() == 0) {
        return std::get<0>(_data);
    }
    return std::get<1>(_data);
}

CompressorFactory::CompressorFactory(distplusplus::CompressionType compressionType,
                                     std::int64_t compressionLevel)
    : _compressionType(compressionType), _compressionLevel(compressionLevel) {}

distplusplus::CompressionType CompressorFactory::compressionType() const { return _compressionType; }

Compressor CompressorFactory::operator()(const std::string &input) const {
    return {_compressionType, _compressionLevel, input};
}

Decompressor::Decompressor(distplusplus::CompressionType compressionType, const std::string &input)
    : _compressionType(compressionType) {
    switch (_compressionType) {
    case NONE: {
        _data = std::string_view(input);
        break;
    }
    case zstd: {
#ifdef DISTPLUSPLUS_WITH_ZSTD
        std::stringstream decompressed;
        std::stringstream origin(input);
        bios::filtering_stream<bios::input> out;
        out.push(bios::zstd_decompressor());
        out.push(origin);
        bios::copy(out, decompressed);
        _data = std::move(decompressed).str();
#else
        // TODO: throw decomp error
#endif
        break;
    }
    default: {
        const std::string err = "encountered unknown CompressionType enum value " +
                                std::to_string(_compressionType) + " in decompressor";
        BOOST_LOG_TRIVIAL(fatal) << err;
        throw std::runtime_error(err);
    }
    }
}

std::string_view Decompressor::data() const {
    if (_data.index() == 0) {
        return std::get<0>(_data);
    }
    return std::get<1>(_data);
}

} // namespace distplusplus::common
