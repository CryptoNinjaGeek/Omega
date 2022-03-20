#pragma once

#include "OMath.h"
#include "OPoint3.h"

namespace omega {
    namespace geometry {
        template<class T>
        class OSphere {
        public:
            OPoint3 <T> center;
            T radius;

        public:
            OSphere<T>() {}

            OSphere<T>(const OPoint3 <T> &in_rPosition, const T in_rRadius)
                    : center(in_rPosition),
                      radius(in_rRadius) {
                if (radius < 0.0f)
                    radius = 0.0f;
            }

            bool isContained(const OPoint3 <T> &in_rContain) const;

            bool isContained(const OSphere<T> &in_rContain) const;

            bool isIntersecting(const OSphere<T> &in_rIntersect) const;

            bool intersectsRay(const OPoint3 <T> &start, const OPoint3 <T> &end) const;

            T distanceTo(const OPoint3 <T> &pt) const;

            T squareDistanceTo(const OPoint3 <T> &pt) const;
        };

        template<class T>
        inline bool OSphere<T>::isContained(const OPoint3 <T> &in_rContain) const {
            T distSq = (center - in_rContain).lenSquared();

            return (distSq <= (radius * radius));
        }

        template<class T>
        inline bool OSphere<T>::isContained(const OSphere<T> &in_rContain) const {
            if (radius < in_rContain.radius)
                return false;

            // Since our radius is guaranteed to be >= other's, we
            //  can dodge the sqrt() here.
            //
            T dist = (in_rContain.center - center).lenSquared();

            return (dist <= ((radius - in_rContain.radius) *
                             (radius - in_rContain.radius)));
        }

        template<class T>
        inline bool OSphere<T>::isIntersecting(const OSphere<T> &in_rIntersect) const {
            T distSq = (in_rIntersect.center - center).lenSquared();

            return (distSq <= ((in_rIntersect.radius + radius) *
                               (in_rIntersect.radius + radius)));
        }

        template<class T>
        inline T OSphere<T>::distanceTo(const OPoint3 <T> &toPt) const {
            return (center - toPt).len() - radius;
        }
    }
}
