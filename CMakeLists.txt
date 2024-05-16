set(source_files
    model/p4-controller.cc
    model/p4-switch-interface.cc
    model/p4-model.cc
    model/p4-net-device.cc
    helper/p4-helper.cc
    model/primitives.cpp
    model/csma-topology-reader.cc
    model/p4-topology-reader.cc
    model/helper.cc
    model/global.cc
    model/switch-api.cc
    model/exception-handle.cc
    helper/p4-topology-reader-helper.cc
    helper/binary-tree-topo-helper.cc
    helper/fattree-topo-helper.cc
    helper/build-flowtable-helper.cc
    model/key-hash.cc
)

set(header_files
    model/p4-controller.h
    model/p4-switch-interface.h
    model/p4-model.h
    model/p4-net-device.h
    model/helper.h
    helper/p4-helper.h
    helper/p4-topology-reader-helper.h
    model/csma-topology-reader.h
    model/p4-topology-reader.h
    helper/binary-tree-topo-helper.h
    helper/fattree-topo-helper.h
    helper/build-flowtable-helper.h
    model/global.h
    model/switch-api.h
    model/exception-handle.h
    model/key-hash.h
)

build_lib(
  LIBNAME p4simulator
  SOURCE_FILES ${source_files}
  HEADER_FILES ${header_files}
  LIBRARIES_TO_LINK ${libnetwork}
                    ${libbridge}

  TEST_SOURCES 
)
