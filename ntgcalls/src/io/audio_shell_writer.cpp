//
// Created by Laky64 on 08/10/24.
//

#ifdef BOOST_ENABLED
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/audio_shell_writer.hpp>

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
    AudioShellWriter::AudioShellWriter(const std::string &command, BaseSink* sink): BaseIO(sink), ThreadedAudioMixer(sink) {
        try {
            const auto cmd = bp::shell(command);
            #if defined(__unix__) || defined(__APPLE__)
            shellProcess = bp::process(ctx, cmd.exe(), cmd.args(), bp::process_stdio{stdIn, nullptr, {}}, StartNewProcessGroup{});
#else
            shellProcess = bp::process(ctx, cmd.exe(), cmd.args(), bp::process_stdio{stdIn, nullptr, {}});
#endif
        } catch (std::runtime_error &e) {
            throw ShellError(e.what());
        }
    }

    AudioShellWriter::~AudioShellWriter() {
        boost::system::error_code ec;
        if (stdIn.is_open()) {
            stdIn.close(ec);
        }
        terminateShellTree(shellProcess, ec);
    }

    void AudioShellWriter::write(const bytes::unique_binary& data) {
        boost::system::error_code ec;
        asio::write(stdIn, asio::buffer(data.get(), sink->frameSize()), ec);
        if (ec || !stdIn.is_open() || !shellProcess.running()) {
            throw EOFError("Reached end of the stream");
        }
    }
} // ntgcalls

#endif