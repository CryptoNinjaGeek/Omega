#pragma once

#include <system/Global.h>
#include <geometry/Portal.h>
#include <memory>

namespace omega {
namespace geometry {

/**
 * PortalPair - Links two portals together
 * Manages the relationship between source and destination portals
 */
class OMEGA_EXPORT PortalPair {
public:
  PortalPair();
  PortalPair(std::shared_ptr<Portal> portalA, std::shared_ptr<Portal> portalB);

  // Set/get portals
  void setPortals(std::shared_ptr<Portal> portalA, std::shared_ptr<Portal> portalB);
  std::shared_ptr<Portal> getPortalA() const { return portalA_; }
  std::shared_ptr<Portal> getPortalB() const { return portalB_; }

  // Link portals together (bidirectional)
  void link();

  // Check if pair is valid (both portals exist)
  bool isValid() const;

  // Enable/disable portal pair
  void setEnabled(bool enabled);
  bool isEnabled() const { return enabled_; }

private:
  std::shared_ptr<Portal> portalA_{nullptr};
  std::shared_ptr<Portal> portalB_{nullptr};
  bool enabled_{true};
};

}  // namespace geometry
}  // namespace omega

