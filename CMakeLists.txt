

# This is a skeleton CMakeLists.txt file, auto-generated on
# Wed Mar 13 09:41:09 EDT 2024.  The developer(s) of this package should delete
# this comment as well as adding dependent targets, packages,
# etc. specific to the package. For details on how to write a package,
# please see
# https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/

cmake_minimum_required(VERSION 3.12)
project(dfbackend VERSION 0.1.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(logging REQUIRED)
find_package(appfwk REQUIRED)
find_package(hdf5libs REQUIRED)
find_package(iomanager REQUIRED)
find_package(opmonlib REQUIRED)
find_package(datafilter REQUIRED)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

##############################################################################


# See https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/#daq_codegen

daq_codegen(dfbackendmodule.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2)
daq_codegen(dfbackendmoduleinfo.jsonnet DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2)
daq_codegen(trdispatcher.jsonnet DEP_PKGS iomanager rcif cmdlib TEMPLATES Structs.hpp.j2 Nljs.hpp.j2)
daq_codegen(trdispatcherinfo.jsonnet DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2)
daq_codegen(filterresultwriter.jsonnet DEP_PKGS iomanager rcif cmdlib TEMPLATES Structs.hpp.j2 Nljs.hpp.j2)
daq_codegen(filterresultwriterinfo.jsonnet DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2)



##############################################################################


# See https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/#daq_add_library

#daq_add_library( LINK_LIBRARIES ) # Any source files and/or dependent libraries to link in not yet determined
daq_add_library (*.cpp LINK_LIBRARIES stdc++fs ers::ers HighFive hdf5libs daqdataformats::daqdataformats detdataformats::detdataformats fddetdataformats::fddetdataformats detchannelmaps::detchannelmaps iomanager::iomanager logging::logging nlohmann_json::nlohmann_json ${CETLIB} ${CETLIB_EXCEPT})
##############################################################################


# See https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/#daq_add_python_bindings


daq_add_python_bindings(*.cpp LINK_LIBRARIES ${PROJECT_NAME} ) # Any additional libraries to link in beyond the main library not yet determined


##############################################################################


# See https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/#daq_add_plugin

daq_add_plugin(DFBackendModule duneDAQModule LINK_LIBRARIES appfwk::appfwk) # Replace appfwk library with a more specific library when appropriate

##############################################################################


# See https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/#daq_add_application

daq_add_application(trdispatcher trdispatcher.cxx LINK_LIBRARIES datafilter::datafilter iomanager::iomanager hdf5libs::hdf5libs logging::logging nlohmann_json::nlohmann_json) 
daq_add_application(filterorchestrator filterorchestrator.cxx LINK_LIBRARIES datafilter::datafilter iomanager::iomanager hdf5libs::hdf5libs logging::logging nlohmann_json::nlohmann_json) 
daq_add_application(filterresultwriter filterresultwriter.cxx LINK_LIBRARIES datafilter::datafilter iomanager::iomanager hdf5libs::hdf5libs logging::logging nlohmann_json::nlohmann_json) 
#daq_add_application(test_HDF5FromStorage test_HDF5FromStorage.cxx LINK_LIBRARIES datafilter::datafilter iomanager::iomanager hdf5libs::hdf5libs logging::logging nlohmann_json::nlohmann_json) 

##############################################################################


# See https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/#daq_add_unit_test

#daq_add_unit_test(Placeholder_test LINK_LIBRARIES)  # Placeholder_test should be replaced with real unit tests

##############################################################################

daq_install()

