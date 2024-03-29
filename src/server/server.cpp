#include "server.hpp"

#include "common/argsvec.hpp"
#include "common/common.hpp"
#include "common/process_helper.hpp"
#include "common/scopeguard.hpp"
#include "common/tempfile.hpp"
#include "parser.hpp"

#include <algorithm>
#include <array>
#include <boost/core/demangle.hpp>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstdlib>
#include <distplusplus.pb.h>
#include <exception>
#include <fstream>
#include <functional>
#include <grpcpp/server_context.h>
#include <gsl/gsl_util>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string_view>
#include <typeinfo>

using distplusplus::common::ArgsVec;

namespace distplusplus::server {

namespace _internal {

// NOLINTNEXTLINE(readability-identifier-length)
bool ReservationCompare::operator()(const reservationType &a, const reservationType &b) const {
    return std::get<1>(a) < std::get<1>(b);
}
// NOLINTNEXTLINE(readability-identifier-length)
bool ReservationCompare::operator()(const reservationTypeB &a, const reservationType &b) const {
    return a < std::get<1>(b);
}
// NOLINTNEXTLINE(readability-identifier-length)
bool ReservationCompare::operator()(const reservationType &a, const reservationTypeB &b) const {
    return std::get<1>(a) < b;
}

} // namespace _internal

namespace {

[[noreturn]] void exceptionAbortHandler(const std::exception &exception) noexcept {
    const std::string typeName = boost::core::demangle(typeid(exception).name());
    BOOST_LOG_TRIVIAL(fatal) << "caught exception in gRPC function: " << typeName << " " << exception.what();
    std::terminate();
}

[[noreturn]] void exceptionAbortHandler() noexcept {
    BOOST_LOG_TRIVIAL(fatal) << "caught exception of unexpected type in gRPC function";
    std::terminate();
}

// TODO: finish

[[nodiscard]] bool sanitizeFile(const distplusplus::File &file) {
    const std::filesystem::path filename = file.name();
    if (filename.filename() == filename) {
        return true;
    }
    BOOST_LOG_TRIVIAL(warning) << "rejected file " << filename << " due to malformed name";
    return false;
}

[[nodiscard]] bool sanitizeRequest(const distplusplus::CompileRequest &request) {
    if (const std::filesystem::path compiler = request.compiler(); compiler.filename() != compiler) {
        BOOST_LOG_TRIVIAL(warning) << "rejected requested compiler " << compiler
                                   << " as it does not look like a basename";
        return false;
    }
    return sanitizeFile(request.inputfile());
}

// This does not check for dir privileges and doing so would implement many TOCTOU pitfalls
// However we can assume that /usr is only modifiable by root
[[nodiscard]] bool checkCompilerAllowed(const std::string &compiler) {
    const std::function<bool(const std::filesystem::directory_entry &)> checkIfSymlink =
        [&compiler](const auto &file) {
            if (file.is_symlink() && compiler == file.path().filename()) {
                BOOST_LOG_TRIVIAL(trace)
                    << "detected compiler " << compiler << " as allowed by comparing against " << file;
                return true;
            }
            return false;
        };

    bool ret1 = false;
    if (std::filesystem::is_directory("/usr/lib/distcc")) {
        ret1 = std::ranges::any_of(std::filesystem::directory_iterator("/usr/lib/distcc"), checkIfSymlink);
    }
    // lazy copy
    bool ret2 = false;
    if (std::filesystem::is_directory("/usr/libexec/distplusplus")) {
        ret2 = std::ranges::any_of(std::filesystem::directory_iterator("/usr/libexec/distplusplus"),
                                   checkIfSymlink);
    }
    return ret1 || ret2;
}

} // namespace

void Server::reservationReaper() {
    const std::stop_token token = reservationReaperThread.get_stop_token();
    while (!token.stop_requested()) {
        const _internal::reservationTypeB timestamp = std::chrono::system_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(reservationTimeout));
        {
            std::lock_guard lockGuard(reservationLock);
            const auto before = reservations.size();
            const auto bound = reservations.lower_bound(timestamp);
            reservations.erase(reservations.begin(), bound);
            const auto after = reservations.size();
            const auto diff = before - after;
            jobsRunning -= diff;
            if (diff > 0) {
                BOOST_LOG_TRIVIAL(info)
                    << "reaped " << diff << " stale reservations - consider increasing reservation timeout";
            }
        }
    }
}

grpc::Status Server::Query(grpc::ServerContext *context, const distplusplus::ServerQuery *query,
                           distplusplus::QueryAnswer *answer) {
    try {
        const std::string clientIP = context->peer();
        const std::string &compiler = query->compiler();
        if (std::filesystem::path(compiler).filename() != compiler) {
            BOOST_LOG_TRIVIAL(warning) << "client " << clientIP << " queried for compiler " << compiler
                                       << " which does not look like a basename";
            return {grpc::StatusCode::INVALID_ARGUMENT,
                    "compiler " + compiler + " does not look like a basename"};
        }
        if (!checkCompilerAllowed(compiler)) {
            BOOST_LOG_TRIVIAL(warning)
                << "client " << clientIP << " queried for non-allowed compiler " << compiler;
            return {grpc::StatusCode::INVALID_ARGUMENT, "compiler " + compiler + " is not on allowlist"};
        }
        answer->set_compilersupported(true);

        if (std::array<double, 1> loadarr = {0}; getloadavg(loadarr.data(), 1) != 1) {
            BOOST_LOG_TRIVIAL(error) << "failed to get load average for client " << clientIP;
            answer->set_currentload(0);
        } else {
            auto loadavg = gsl::narrow_cast<uint32_t>(loadarr[0] / gsl::narrow_cast<double>(jobsMax) * 100);
            loadavg = std::min(loadavg, 100U);
            BOOST_LOG_TRIVIAL(debug) << "reported load average " << std::to_string(loadavg) << " for client "
                                     << clientIP;
            answer->set_currentload(loadavg);
        }

        switch (query->desiredcompressiontype()) {
        // NOLINTNEXTLINE(bugprone-branch-clone)
        case NONE:
            answer->set_compressiontypesupported(true);
            BOOST_LOG_TRIVIAL(debug) << "desired compression type NONE supported for client " << clientIP;
            break;
        case zstd:
#ifdef DISTPLUSPLUS_WITH_ZSTD
            answer->set_compressiontypesupported(true);
            BOOST_LOG_TRIVIAL(debug) << "desired compression type zstd supported for client " << clientIP;
#else
            answer->set_compressiontypesupported(false);
            BOOST_LOG_TRIVIAL(info) << "desired compression type zstd not supported for client " << clientIP;
#endif
            break;
        // NOLINTNEXTLINE(bugprone-branch-clone)
        case CompressionType_INT_MAX_SENTINEL_DO_NOT_USE_:;
        case CompressionType_INT_MIN_SENTINEL_DO_NOT_USE_:;
        }

        BOOST_LOG_TRIVIAL(debug) << "reported max jobs " << std::to_string(jobsMax) << " for client "
                                 << clientIP;
        answer->set_maxjobs(jobsMax);
        return grpc::Status::OK;
    } catch (const std::exception &e) {
        exceptionAbortHandler(e);
    } catch (...) {
        exceptionAbortHandler();
    }
}

grpc::Status Server::Reserve(grpc::ServerContext *context, const distplusplus::Reservation * /*reservation*/,
                             distplusplus::ReservationAnswer *answer) {
    try {
        std::uint64_t jobsCur = jobsMax;
        if (jobsRunning < jobsMax) {
            jobsCur = jobsRunning.load();
            if (std::atomic_compare_exchange_strong(&jobsRunning, &jobsCur, jobsCur + 1)) {
                common::ScopeGuard jobCountGuard([this]() { jobsRunning--; });
                const std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
                {
                    std::lock_guard lockGuard(reservationLock);
                    reservations.emplace_hint(reservations.end(), uuid, std::chrono::system_clock::now());
                }
                answer->set_uuid(uuid);
                answer->set_success(true);
                BOOST_LOG_TRIVIAL(debug) << "reserved job " << uuid << " for client " << context->peer();
                jobCountGuard.defuse();
                return grpc::Status::OK;
            }
        }
        BOOST_LOG_TRIVIAL(debug) << "rejected reservation for client " << context->peer()
                                 << " as all slots are full";
        answer->set_success(false);
        return {grpc::StatusCode::RESOURCE_EXHAUSTED, "host is full", std::to_string(jobsCur)};
    } catch (const std::exception &e) {
        exceptionAbortHandler(e);
    } catch (...) {
        exceptionAbortHandler();
    }
}

grpc::Status Server::Distribute(grpc::ServerContext *context, const distplusplus::CompileRequest *request,
                                distplusplus::CompileAnswer *answer) {
    try {
        const std::string clientIP = context->peer();
        const std::string &clientUUID = request->uuid();
        BOOST_LOG_TRIVIAL(trace) << "client " << clientIP << " sent RPC:" << '\n'
                                 << "uuid: " << clientUUID << '\n'
                                 << "compiler: " << request->compiler() << '\n'
                                 << "filename: " << request->inputfile().name() << '\n'
                                 << "cwd: " << request->cwd();
        {
            std::lock_guard lockGuard(reservationLock);
            if (std::erase_if(reservations, [&clientUUID](const _internal::reservationType &comp) {
                    return std::get<0>(comp) == clientUUID;
                }) == 0) {
                const std::string errorMessage = "error: uuid " + clientUUID + " not in reservation list.";
                BOOST_LOG_TRIVIAL(warning) << "client " << clientIP << " sent job but uuid " << clientUUID
                                           << " was not in reservation list";
                return {grpc::StatusCode::FAILED_PRECONDITION, errorMessage};
            }
        }
        common::ScopeGuard jobCountGuard([this]() { jobsRunning--; });

        // TODO: do this proper
        if (!sanitizeRequest(*request)) {
            BOOST_LOG_TRIVIAL(warning)
                << "client " << clientIP << " with job " << clientUUID << " protocol violation!";
            return {grpc::StatusCode::INVALID_ARGUMENT, "protocol violation"};
        }

        if (!checkCompilerAllowed(request->compiler())) {
            const std::string errorMessage = "error: compiler " + request->compiler() + " not in allow list";
            BOOST_LOG_TRIVIAL(warning) << "client " << clientIP << " sent job but compiler "
                                       << request->compiler() << " was not in allow list";
            return {grpc::StatusCode::INVALID_ARGUMENT, errorMessage};
        }

        // the client could be a path to an unix socket too
        std::string clientIPDelimited = clientIP;
        std::ranges::replace(clientIPDelimited, '/', '-');

        // C++ is a pretty language
        const distplusplus::common::Decompressor decompressor = [&] {
            try {
                return distplusplus::common::Decompressor(request->inputfile().compressiontype(),
                                                          request->inputfile().content());
            } catch (const std::ios_base::failure &e) {
                BOOST_LOG_TRIVIAL(warning)
                    << "client " << clientIP << " sent job but raised decompressor error" << e.what();
                throw grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "error in decompressor", e.what());
            }
        }();

        const distplusplus::common::Tempfile inputFile(request->inputfile().name(), clientIPDelimited,
                                                       decompressor.data());

        const distplusplus::common::Tempfile outputFile(request->inputfile().name() + ".o",
                                                        clientIPDelimited);

        ArgsVec preArgs(request->argument().begin(), request->argument().end());
        ArgsVec args;
        try {
            parser::Parser parser(preArgs);
            args = parser.args();
        } catch (const parser::CannotProcessSignal &signal) {
            BOOST_LOG_TRIVIAL(warning)
                << "job " << clientUUID << " from host " << clientIP << " aborted due to invalid arguments:\n"
                << signal.what();
            return {grpc::StatusCode::INVALID_ARGUMENT, "job aborted due to invalid arguments",
                    signal.what()};
        }

        args.emplace_back("-o");
        args.emplace_back(outputFile.c_str());
        args.emplace_back(inputFile.c_str());

        const boost::filesystem::path compilerPath = boost::process::search_path(request->compiler());
        distplusplus::common::ProcessHelper compilerProcess(compilerPath, args);
        // TODO: proper logging

        answer->set_returncode(compilerProcess.returnCode());
        answer->set_stdout(compilerProcess.get_stdout());
        answer->set_stderr(compilerProcess.get_stderr());
        answer->mutable_outputfile()->set_name(outputFile);
        std::ifstream fileStream(outputFile);
        std::stringstream fileContent;
        fileContent << fileStream.rdbuf();
        // TODO: prevent copy
        const std::string fileContentString = fileContent.str();
        const distplusplus::common::Compressor compressor = compressorFactory(fileContentString);
        answer->mutable_outputfile()->set_compressiontype(compressorFactory.compressionType());
        answer->mutable_outputfile()->set_content(compressor.data().data(), compressor.data().size());
        return grpc::Status::OK;
    } catch (const grpc::Status &status) {
        return status;
    } catch (const std::exception &e) {
        exceptionAbortHandler(e);
    } catch (...) {
        exceptionAbortHandler();
    }
}

Server::Server(std::uint64_t maxJobs, std::uint64_t reservationTimeout,
               distplusplus::common::CompressorFactory compressorFactory)
    : compressorFactory(compressorFactory), jobsMax(maxJobs), reservationTimeout(reservationTimeout) {
    distplusplus::common::assertAndRaise(maxJobs != 0, "tried to construct server with 0 max jobs");
}

} // namespace distplusplus::server
