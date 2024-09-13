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
#include "nlohmann/json.hpp"

using namespace dunedaq;
using namespace trdispatcher;

int main(int argc, char* argv[]) {
    logging::Logging().setup();
    // dunedaq::trdispatcher::TRDispatcher trd;
    // trd.init();

    dunedaq::trdispatcher::TRDispatcherConfig config;
    bool help_requested = false;
    namespace po = boost::program_options;
    po::options_description desc("Dataflow emulator");
    desc.add_options()("use_connectivity_service,c",
                       po::bool_switch(&config.use_connectivity_service),
                       "enable the ConnectivityService in IOManager")(
        "num_apps,N",
        po::value<size_t>(&config.num_apps)->default_value(config.num_apps),
        "Number of applications to start")(
        "num_groups,g",
        po::value<size_t>(&config.num_groups)->default_value(config.num_groups),
        "Number of connection groups")(
        "num_connections,n",
        po::value<size_t>(&config.num_connections_per_group)
            ->default_value(config.num_connections_per_group),
        "Number of connections to register and use in each group")(
        "port,p", po::value<int>(&config.port)->default_value(config.port),
        "port to connect to on configuration server")(
        "server,s",
        po::value<std::string>(&config.server)->default_value(config.server),
        "Configuration server to connect to")(
        "num_messages,m",
        po::value<size_t>(&config.num_messages)
            ->default_value(config.num_messages),
        "Number of messages to send on each connection")(
        "message_size_kb,z",
        po::value<size_t>(&config.message_size_kb)
            ->default_value(config.message_size_kb),
        "Size of each message, in KB")(
        "num_runs,r",
        po::value<size_t>(&config.num_runs)->default_value(config.num_runs),
        "Number of times to clear the sender and send all messages")(
        "publish_interval,i",
        po::value<int>(&config.publish_interval)
            ->default_value(config.publish_interval),
        "Interval, in ms, for ConfigClient to re-publish connection info")(
        "send_interval,I",
        po::value<size_t>(&config.send_interval_ms)
            ->default_value(config.send_interval_ms),
        "Interval, in ms, for Publishers to send messages")(
        "output_h5_filename,o",
        po::value<std::string>(&config.output_h5_filename)
            ->default_value(config.output_h5_filename),
        "Base name for output info file (will have _sender.csv or "
        "_receiver.csv appended)")(
        "input_h5_filename,f",
        po::value<std::string>(&config.input_h5_filename)
            ->default_value(config.input_h5_filename),
        "Input hdf5 file")("session",
                           po::value<std::string>(&config.session_name)
                               ->default_value(config.session_name),
                           "Name of this DAQ session")(
        "help,h", po::bool_switch(&help_requested), "Print this help message");

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

    auto publisher =
        std::make_unique<dunedaq::trdispatcher::PublisherTest>(config);

    for (size_t run = 0; run < config.num_runs; ++run) {
        TLOG() << "TR Dispatcher" << config.my_id << ": "
               << "run " << run;
        if (config.num_apps > 1) publisher->init(run);
        // publisher->send(run, forked_pids[0]);
        publisher->send(run, 0);
        TLOG() << "TR Dispatcher " << config.my_id << ": "
               << "run " << run << " complete.";
    }

    TLOG() << "TR Dispatcher" << config.my_id << ": "
           << "Cleaning up";
    publisher.reset(nullptr);

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
