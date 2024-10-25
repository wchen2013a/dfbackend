/**
 * @file trdispatcher.cxx
 *
 * Developer(s) of this DAQ application have yet to replace this line with a
 * brief description of the application.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "dfbackend/trdispatcher.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "boost/program_options.hpp"
#include "logging/Logging.hpp"

using namespace dunedaq;
using namespace datafilter;

int main(int argc, char* argv[]) {
    logging::Logging().setup();

    dunedaq::datafilter::TRDispatcherConfig config;
    bool help_requested = false;
    bool is_hdf5file = false;
    namespace po = boost::program_options;
    po::options_description desc("TR Dispatcher");
    desc.add_options()(
        "server,s",
        po::value<std::string>(&config.server)->default_value(config.server),
        "server")("ifilename,f",
                  po::value<std::string>(&config.input_h5_filename)
                      ->default_value(config.input_h5_filename),
                  "input filename ")("hdf5,h5", po::bool_switch(&is_hdf5file),
                                     "toggle to hdf5 file")(
        "help,h", po::bool_switch(&help_requested), "For help.");

    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (std::exception& ex) {
        std::cerr << "Error parsing command line " << ex.what() << std::endl;
        std::cerr << desc << std::endl;
        return 1;
    }

    if (help_requested) {
        std::cout << desc << std::endl;
        return 0;
    }

    if (config.use_connectivity_service) {
        TLOG() << "Setting Connectivity Service Environment variables";
        config.configure_connsvc();
    }

    //    auto startup_time = std::chrono::steady_clock::now();
    // start fork process
    //    std::vector<pid_t> forked_pids;
    //    for (size_t ii = 0; ii < config.num_apps; ++ii) {
    //      auto pid = fork();
    //      std::cout<<"forked pid "<<pid<<" ii "<<ii<<"\n";
    //      if (pid<0) {
    //          TLOG()<<"fork error";
    //          exit(EXIT_FAILURE);
    //      } else if (pid == 0) { // child
    //          forked_pids.clear();
    //          config.my_id = ii;
    //
    //          TLOG() << "Dataflow emulator: child process " << config.my_id;
    //          break;
    //      } else { //parent
    //          TLOG() << "Datalfow emulator: parent process " << getpid();
    //          forked_pids.push_back(pid);
    //      }
    //    }

    //      std::this_thread::sleep_until(startup_time + 2s);

    TLOG() << "TR Dispatcher " << config.my_id << ": "
           << "Configuring IOManager";
    config.configure_iomanager();

    auto trdispatcher =
        std::make_unique<dunedaq::datafilter::TRDispatcher>(config);

    for (size_t run = 0; run < config.num_runs; ++run) {
        TLOG() << "TR Dispatcher" << config.my_id << ": "
               << "run " << run;
        if (config.num_apps > 1) trdispatcher->init(run);
        // trdispatcher->send(run, forked_pids[0]);
        if (is_hdf5file) {
            trdispatcher->receive(run, 0, true);
        } else {
            trdispatcher->receive(run, 0, false);
        }

        TLOG() << "TR Dispatcher " << config.my_id << ": "
               << "run " << run << " complete.";
    }

    TLOG() << "TR Dispatcher" << config.my_id << ": "
           << "Cleaning up";
    trdispatcher.reset(nullptr);

    dunedaq::iomanager::IOManager::get()->reset();
    TLOG() << "TR Dispatcher " << config.my_id << ": "
           << "DONE";

    //    if (forked_pids.size() > 0) {
    //      TLOG() << "Waiting for forked PIDs";
    //
    //      for (auto& pid : forked_pids) {
    //        siginfo_t status;
    //        auto sts = waitid(P_PID, pid, &status, WEXITED);
    //
    //        TLOG_DEBUG(6) << "Forked process " << pid << " exited with status
    //        " << status.si_status << " (wait status " << sts
    //                      << ")";
    //      }
    //    }

    return 0;
}
