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

    std::vector<std::filesystem::path> files;

    dunedaq::datafilter::TRDispatcherConfig config;
    bool help_requested = false;
    bool is_hdf5file = false;
    bool is_from_storage = false;
    size_t cnt = 0;
    std::string input_h5_filename = config.input_h5_filename;
    namespace po = boost::program_options;
    po::options_description desc("TR Dispatcher");
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
        "ifilename,f",
        po::value<std::string>(&config.input_h5_filename)
            ->default_value(config.input_h5_filename),
        "input filename ")("from_storage,from",
                           po::bool_switch(&is_from_storage),
                           "hdf5 files from storage")(
        "storage_pathname",
        po::value<std::string>(&config.storage_pathname)
            ->default_value(config.storage_pathname),
        "storage_pathname")("hdf5,h5", po::bool_switch(&is_hdf5file),
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

    TLOG() << "TR Dispatcher " << config.my_id << ": "
           << "Configuring IOManager";
    // config.configure_iomanager();

    cnt = 0;
    auto trdispatcher =
        std::make_unique<dunedaq::datafilter::TRDispatcher>(config);

    for (size_t run = 0; run < config.num_runs; ++run) {
        TLOG() << "TR Dispatcher" << config.my_id << ": "
               << "run " << run;
        if (config.num_apps > 1) trdispatcher->init(run);
        if (is_from_storage) {
            while (true) {
                files = trdispatcher->get_hdf5files_from_storage();
                if (cnt % 1000000 == 0) {
                    TLOG() << "IDLE: No new HDF5 files after " << cnt
                           << " checks.";
                }
                if (files.size() > 0) {
                    for (auto file : files) {
                        trdispatcher->config.input_h5_filename = file;
                        TLOG() << "Sending "
                               << trdispatcher->config.input_h5_filename;
                        trdispatcher->receive(run, 0, true);
                        // make sure no duplicate
                        trdispatcher->trdispatchers.pop_back();
                    }
                    cnt = 0;
                }
                ++cnt;
            }
        } else if (is_hdf5file) {
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

    // dunedaq::iomanager::IOManager::get()->reset();
    TLOG() << "TR Dispatcher " << config.my_id << ": "
           << "DONE";

    return 0;
}
