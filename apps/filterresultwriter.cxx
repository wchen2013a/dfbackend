/**
 * @file filterresultwriter.cxx
 *
 * Developer(s) of this DAQ application have yet to replace this line with a
 * brief description of the application.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "dfbackend/FilterResultWriter.hpp"

#include <sys/wait.h>

#include "boost/program_options.hpp"

using namespace dunedaq;
using namespace filterresultwriter;

int main(int argc, char* argv[]) {
    dunedaq::filterresultwriter::FilterResultWriterConfig config;

    bool help_requested = false;
    namespace po = boost::program_options;
    po::options_description desc("Filter Result Writer");
    desc.add_options()("ofilename,o",
                       po::value<std::string>(&config.output_h5_filename)
                           ->default_value(config.output_h5_filename),
                       "output filename ")(
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

    TLOG() << "Filter Result Writer " << config.my_id << ": "
           << "Configuring IOManager";
    config.configure_iomanager();

    auto publisher =
        std::make_unique<dunedaq::filterresultwriter::PublisherTest>(config);

    for (size_t run = 0; run < config.num_runs; ++run) {
        TLOG() << "Filter Result Writer " << config.my_id << ": "
               << "run " << run;
        if (config.num_apps > 1) publisher->init(run);
        // publisher->send(run, forked_pids[0]);
        // publisher->send(run, 0);
        publisher->receive_tr(run);
        TLOG() << "Filter Result Writer " << config.my_id << ": "
               << "run " << run << " complete.";
    }

    TLOG() << "Filter Result Writer" << config.my_id << ": "
           << "Cleaning up";
    publisher.reset(nullptr);

    dunedaq::iomanager::IOManager::get()->reset();
    TLOG() << "Filter Result Writer " << config.my_id << ": "
           << "DONE";

    //    if (forked_pids.size() > 0) {
    //        TLOG() << "Waiting for forked PIDs";
    //
    //        for (auto& pid : forked_pids) {
    //            siginfo_t status;
    //            auto sts = waitid(P_PID, pid, &status, WEXITED);
    //
    //            TLOG_DEBUG(6) << "Forked process " << pid << " exited with
    //            status " << status.si_status << " (wait status " << sts
    //                          << ")";
    //        }
    //    }

    return 0;
}
