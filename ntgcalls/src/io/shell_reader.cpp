//
// Created by Laky64 on 30/08/2023.
//

#ifdef BOOST_ENABLED
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/shell_reader.hpp>

#if defined(__unix__) || defined(__APPLE__)
#include <cerrno>
#include <csignal>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {
#if defined(__unix__) || defined(__APPLE__)
    struct StartNewProcessGroup {
        template<typename Launcher, typename Path, typename CmdLine>
        boost::system::error_code on_exec_setup(Launcher&, const Path&, CmdLine&) const {
            if (::setpgid(0, 0) == -1) {
                return {errno, boost::system::generic_category()};
            }
            return {};
        }
    };
#endif

    void terminateShellTree(bp::process& process, boost::system::error_code& ec) {
#if defined(__unix__) || defined(__APPLE__)
        const auto rawPid = process.id();
        const auto pid = static_cast<pid_t>(rawPid);
        if (pid > 0 && ::getpgid(pid) == pid) {
            (void) ::kill(-pid, SIGKILL);
        }
#endif
        if (process.running(ec)) {
            process.terminate(ec);
        }
        process.wait(ec);
    }
}

namespace ntgcalls {

    ShellReader::ShellReader(const std::string &command, BaseSink *sink):
    BaseIO(sink), ThreadedReader(sink) {
        try {
            const auto cmd = bp::shell(command);
            #if defined(__unix__) || defined(__APPLE__)
            shellProcess = bp::process(ctx, cmd.exe(), cmd.args(), bp::process_stdio{nullptr, stdOut, {}}, StartNewProcessGroup{});
#else
            shellProcess = bp::process(ctx, cmd.exe(), cmd.args(), bp::process_stdio{nullptr, stdOut, {}});
#endif
        } catch (std::runtime_error &e) {
            throw ShellError(e.what());
        }
    }

    ShellReader::~ShellReader() {
        boost::system::error_code ec;
        terminateShellTree(shellProcess, ec);
        if (stdOut.is_open()) {
            stdOut.close(ec);
        }
        close();
        RTC_LOG(LS_VERBOSE) << "ShellReader closed";
    }

    void ShellReader::open() {
        run([this](const int64_t size) {
            auto file_data = bytes::make_unique_binary(size);
            boost::system::error_code ec;
            asio::read(stdOut, asio::buffer(file_data.get(), size), ec);
            if (ec || !stdOut.is_open() || !shellProcess.running()) {
                RTC_LOG(LS_WARNING) << "Reached end of the file";
                throw EOFError("Reached end of the stream");
            }
            return std::move(file_data);
        });
    }
} // ntgcalls
#endif
