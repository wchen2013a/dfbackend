# This module facilitates the generation of dfbackend DAQModules within dfbackend apps


# Set moo schema search path                                                                              
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types                                                                                
import moo.otypes
moo.otypes.load_types("dfbackend/dfbackendmodule.jsonnet")

import dunedaq.dfbackend.dfbackendmodule as dfbackendmodule

from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
#from daqconf.core.conf_utils import Endpoint, Direction

def get_dfbackend_app(nickname, num_dfbackendmodules, some_configured_value, host="localhost"):
    """
    Here the configuration for an entire daq_application instance using DAQModules from dfbackend is generated.
    """

    modules = []

    for i in range(num_dfbackendmodules):
        modules += [DAQModule(name = f"nickname{i}", 
                              plugin = "DFBackendModule", 
                              conf = dfbackendmodule.Conf(some_configured_value = some_configured_value
                                )
                    )]

    mgraph = ModuleGraph(modules)
    dfbackend_app = App(modulegraph = mgraph, host = host, name = nickname)

    return dfbackend_app
