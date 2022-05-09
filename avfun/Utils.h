//
// Created by 何澄澄 on 2022/4/17.
//

#ifndef AVFUN_UTILS_H
#define AVFUN_UTILS_H

#include <array>

namespace avf {
    class Utils {
    public:
        static std::array<float, 20> FitIn(int sw, int sh, int tw, int th) {
            float l, r, t, b;
            auto ra_s = float(sw) / float(sh);
            auto ra_t = float(tw) / float(th);
            if (ra_s >= ra_t) {
                l = (1.0 - ra_t / ra_s) / 2.0;
                r = l + ra_t / ra_s;
                t = 0;
                b = 1.0;
            } else {
                l = 0;
                r = 1.0;
                t = (1.0 - ra_s / ra_t) / 2.0;
                b = t + ra_s / ra_t;
            }

            l = 2 * l - 1.0;
            r = 2 * r - 1.0;
            t = 2 * t - 1.0;
            b = 2 * b - 1.0;

            std::array<float, 20> vertices = {
                    // positions            // texture coords
                    r, t, 0.0f, 1.0f, 1.0f,   // top right
                    r, b, 0.0f, 1.0f, 0.0f,  // bottom right
                    l, b, 0.0f, 0.0f, 0.0f,   // bottom left
                    l, t, 0.0f, 0.0f, 1.0f,    // top left
            };

            return vertices;

        };
    };
}
#endif //AVFUN_MASTER_UTILS_H
