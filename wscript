# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def configure(conf):
    # Check and configure required libraries
    libraries = {
        'bm': 'BM',
        'boost_system': 'BOOST',
        'simple_switch': 'SW'
    }

    for lib, store in libraries.items():
        conf.check_cfg(package=lib, uselib_store=store, args=['--cflags', '--libs'], mandatory=True)
    
    # Additional environment variables can be set if needed
    # conf.env.P4SIM_MODULE_PATH = conf.path.abspath() + '/src/p4simulator/'

def build(bld):
    ns3_modules = [
        'antenna',
        'aodv',
        'applications',
        'bridge',
        'buildings',
        'config-store',
        'core',
        'csma',
        'csma-layout',
        'dsdv',
        'dsr',
        'energy',
        'fd-net-device',
        'flow-monitor',
        'internet',
        'internet-apps',
        'lr-wpan',
        'lte',
        'mesh',
        'mobility',
        'mpi',
        'netanim',
        'network',
        'nix-vector-routing',
        'olsr',
        'point-to-point',
        'point-to-point-layout',
        'propagation',
        'sixlowpan',
        'spectrum',
        'stats',
        'test',
        'topology-read',
        'traffic-control',
        'uan',
        'virtual-net-device',
        'wave',
        'wifi',
        'wimax'
    ]

    module = bld.create_ns3_module('p4simulator', ns3_modules)
    module.source = [
        'model/p4-controller.cc', 
        'model/p4-switch-interface.cc', 
        'model/p4-model.cc',
        'model/p4-net-device.cc', 
        'helper/p4-helper.cc', 
        'model/primitives.cpp',
        'model/csma-topology-reader.cc', 
        'model/p4-topology-reader.cc', 
        'model/helper.cc',
        'model/global.cc', 
        'model/switch-api.cc', 
        'model/exception-handle.cc',
        'helper/p4-topology-reader-helper.cc', 
        'helper/binary-tree-topo-helper.cc',
        'helper/fattree-topo-helper.cc', 
        'helper/build-flowtable-helper.cc',
        'model/key-hash.cc'
    ]

    module_test = bld.create_ns3_module_test_library('p4simulator')
    module_test.source = ['test/p4-test-suite.cc']

    headers = bld(features='ns3header')
    headers.module = 'p4simulator'
    headers.source = [
        'model/p4-controller.h',
        'model/p4-switch-interface.h',
        'model/p4-model.h',
        'model/p4-net-device.h',
        'model/helper.h',
        'helper/p4-helper.h',
        'helper/p4-topology-reader-helper.h',
        'model/csma-topology-reader.h',
        'model/p4-topology-reader.h',
        'helper/binary-tree-topo-helper.h',
        'helper/fattree-topo-helper.h',
        'helper/build-flowtable-helper.h',
        'model/global.h',
        'model/switch-api.h',
        'model/exception-handle.h',
        'model/key-hash.h'
    ]

    # Add library dependencies
    module.use += ['BM', 'BOOST', 'SW']

    # Recursive compilation example (if enabled)
    if bld.env['ENABLE_EXAMPLES']:
        bld.recurse('examples')

    # Generate Python bindings (if needed)
    # bld.ns3_python_bindings()

