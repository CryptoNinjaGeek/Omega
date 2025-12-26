#include <geometry/PortalPair.h>

using namespace omega::geometry;

PortalPair::PortalPair() = default;

PortalPair::PortalPair(std::shared_ptr<Portal> portalA,
                       std::shared_ptr<Portal> portalB)
    : portalA_(portalA), portalB_(portalB) {
  link();
}

void PortalPair::setPortals(std::shared_ptr<Portal> portalA,
                            std::shared_ptr<Portal> portalB) {
  portalA_ = portalA;
  portalB_ = portalB;
  link();
}

void PortalPair::link() {
  if (portalA_ && portalB_) {
    portalA_->setLinkedPortal(portalB_);
    portalB_->setLinkedPortal(portalA_);
  }
}

bool PortalPair::isValid() const {
  return portalA_ != nullptr && portalB_ != nullptr;
}

void PortalPair::setEnabled(bool enabled) {
  enabled_ = enabled;
  if (portalA_) {
    portalA_->setEnabled(enabled);
  }
  if (portalB_) {
    portalB_->setEnabled(enabled);
  }
}

