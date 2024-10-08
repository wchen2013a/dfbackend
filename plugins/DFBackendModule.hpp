/**
 * @file DFBackendModule.hpp
 *
 * Developer(s) of this DAQModule have yet to replace this line with a brief description of the DAQModule.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef DFBACKEND_PLUGINS_DFBACKENDMODULE_HPP_
#define DFBACKEND_PLUGINS_DFBACKENDMODULE_HPP_

#include "appfwk/DAQModule.hpp"

#include <atomic>
#include <limits>
#include <string>

namespace dunedaq::dfbackend {

class DFBackendModule : public dunedaq::appfwk::DAQModule
{
public:
  explicit DFBackendModule(const std::string& name);

  void init(const data_t&) override;

  void get_info(opmonlib::InfoCollector&, int /*level*/) override;

  DFBackendModule(const DFBackendModule&) = delete;
  DFBackendModule& operator=(const DFBackendModule&) = delete;
  DFBackendModule(DFBackendModule&&) = delete;
  DFBackendModule& operator=(DFBackendModule&&) = delete;

  ~DFBackendModule() = default;

private:
  // Commands DFBackendModule can receive

  // TO dfbackend DEVELOPERS: PLEASE DELETE THIS FOLLOWING COMMENT AFTER READING IT
  // For any run control command it is possible for a DAQModule to
  // register an action that will be executed upon reception of the
  // command. do_conf is a very common example of this; in
  // DFBackendModule.cpp you would implement do_conf so that members of
  // DFBackendModule get assigned values from a configuration passed as 
  // an argument and originating from the CCM system.
  // To see an example of this value assignment, look at the implementation of 
  // do_conf in DFBackendModule.cpp

  void do_conf(const data_t&);

  int m_some_configured_value { std::numeric_limits<int>::max() }; // Intentionally-ridiculous value pre-configuration

  // TO dfbackend DEVELOPERS: PLEASE DELETE THIS FOLLOWING COMMENT AFTER READING IT 
  // m_total_amount and m_amount_since_last_get_info_call are examples
  // of variables whose values get reported to OpMon
  // (https://github.com/mozilla/opmon) each time get_info() is
  // called. "amount" represents a (discrete) value which changes as DFBackendModule
  // runs and whose value we'd like to keep track of during running;
  // obviously you'd want to replace this "in real life"

  std::atomic<int64_t> m_total_amount {0};
  std::atomic<int>     m_amount_since_last_get_info_call {0};
};

} // namespace dunedaq::dfbackend

#endif // DFBACKEND_PLUGINS_DFBACKENDMODULE_HPP_
