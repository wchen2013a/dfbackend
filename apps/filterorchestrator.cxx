/**
 * @file filterorchestrator.cxx
 *
 * Developer(s) of this DAQ application have yet to replace this line with a
 * brief description of the application.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "dfbackend/FilterOrchestrator.hpp"

#include "boost/program_options.hpp"

using namespace dunedaq;
using namespace datafilter;

int main(int argc, char* argv[]) {
    dunedaq::datafilter::FilterOrchestratorConfig config;
    TLOG() << "Filter Orchestrator " << config.my_id << ": "
           << "Configuring IOManager";

    bool help_requested = false;
    size_t cnt = 0;
    namespace po = boost::program_options;
    po::options_description desc("filter orchestrator.");
    desc.add_options()(
        "server,s",
        po::value<std::string>(&config.server)->default_value(config.server),
        "datafilter server")("server_trdispatcher,st",
                             po::value<std::string>(&config.server_trdispatcher)
                                 ->default_value(config.server_trdispatcher),
                             "trdispatcher server")(
        "num_runs,r",
        po::value<size_t>(&config.num_runs)->default_value(config.num_runs),
        "Number of trigger record to send")(
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

    // config.configure_iomanager();

    auto filterorchestrator =
        std::make_unique<dunedaq::datafilter::FilterOrchestrator>(config);

    for (size_t run = 0; run < config.num_runs; ++run) {
        TLOG() << "Filter Orchestrator " << config.my_id << ": "
               << "run " << run;
        if (config.num_apps > 1) filterorchestrator->init(run);
        // filterorchestrator->send(run, forked_pids[0]);
        cnt = 0;
        while (true) {
            filterorchestrator->receive(run, 0);
            // filterorchestrator->filterorchestrators.pop_back();
            if (cnt % 1000000 == 0) {
                TLOG() << "No Filter Orchestrator activities since " << cnt
                       << " checkes.";
            }
        }
        TLOG() << "Filter Orchestrator " << config.my_id << ": "
               << "run " << run << " complete.";
    }

    TLOG() << "Filter Orchestrator" << config.my_id << ": "
           << "Cleaning up";
    filterorchestrator.reset(nullptr);

    // dunedaq::iomanager::IOManager::get()->reset();
    TLOG() << "Filter Orchestrator " << config.my_id << ": "
           << "DONE";

    return 0;
}
