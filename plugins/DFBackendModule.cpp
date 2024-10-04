/**
 * @file DFBackendModule.cpp
 *
 * Implementations of DFBackendModule's functions
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "DFBackendModule.hpp"

#include "dfbackend/dfbackendmodule/Nljs.hpp"
#include "dfbackend/dfbackendmoduleinfo/InfoNljs.hpp"

#include <string>

namespace dunedaq::dfbackend {

DFBackendModule::DFBackendModule(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
{
  register_command("conf", &DFBackendModule::do_conf);
}

void
DFBackendModule::init(const data_t& /* structured args */)
{}

void
DFBackendModule::get_info(opmonlib::InfoCollector& ci, int /* level */)
{
  dfbackendmoduleinfo::Info info;
  info.total_amount = m_total_amount;
  info.amount_since_last_get_info_call = m_amount_since_last_get_info_call.exchange(0);

  ci.add(info);
}

void
DFBackendModule::do_conf(const data_t& conf_as_json)
{
  auto conf_as_cpp = conf_as_json.get<dfbackendmodule::Conf>();
  m_some_configured_value = conf_as_cpp.some_configured_value;
}

} // namespace dunedaq::dfbackend

DEFINE_DUNE_DAQ_MODULE(dunedaq::dfbackend::DFBackendModule)
