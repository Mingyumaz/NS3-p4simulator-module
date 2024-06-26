#ifndef P4_CONTROLLER_H
#define P4_CONTROLLER_H

#include "ns3/object.h"
#include "ns3/p4-switch-interface.h"
#include <string>
#include <vector>

namespace ns3 {

class P4SwitchInterface;

/**
 * @brief Controller for P4Switch. (deprecated)
 * It can view all p4 switch flow table info, view p4 switch flow table info,
 * set p4 switch view flow table path, set p4 switch flow table path, and get
 * p4 switch num.
 *
 */
class P4Controller : public Object {

public:
  P4Controller();

  ~P4Controller();

  void ViewAllSwitchFlowTableInfo();

  void ViewP4SwitchFlowTableInfo(
      size_t index); // view p4 switch flow table, counter, register, meter info

  void SetP4SwitchViewFlowTablePath(
      size_t index,
      const std::string &viewFlowTablePath); // set p4 switch viewFlowTablePath

  void SetP4SwitchFlowTablePath(
      size_t index,
      const std::string &flowTablePath); // set p4 switch populate flowTablePath

  P4SwitchInterface *GetP4Switch(size_t index);

  P4SwitchInterface *AddP4Switch();

  unsigned int GetP4SwitchNum() { return m_p4Switches.size(); }

  static TypeId GetTypeId(void);

private:
  // index represents p4 switch id
  std::vector<P4SwitchInterface *>
      m_p4Switches; // use raw pointer, may be using unique_ptr smart pointer
                    // better.
  P4Controller(const P4Controller &);
  P4Controller &operator=(const P4Controller &);
};

} // namespace ns3

#endif // !P4_CONTROLLER_H
