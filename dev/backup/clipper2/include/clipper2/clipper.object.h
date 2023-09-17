#ifndef CLIPPER_OBJECT_H
#define CLIPPER_OBJECT_H

namespace Clipper2Lib {

    template <typename T>
    class Point {

        public:

            T x;
            T y;

            template <typename T2>
            inline void Init(const T2 x_ = 0, const T2 y_ = 0) {
                if constexpr (std::numeric_limits<T>::is_integer && !std::numeric_limits<T2>::is_integer) {
                    x = static_cast<T>(std::round(x_));
                    y = static_cast<T>(std::round(y_));
                }
                else {
                    x = static_cast<T>(x_);
                    y = static_cast<T>(y_);
                }
            }

            explicit Point() : x(0), y(0) {
            };

            template <typename T2>
            Point(const T2 x_, const T2 y_) {
                Init(x_, y_);
            }

            template <typename T2>
            explicit Point<T>(const Point<T2>& p) {
                Init(p.x, p.y);
            }

            Point operator * (const double scale) const {
                return Point(x * scale, y * scale);
            }

            friend bool operator==(const Point& a, const Point& b) {
                return a.x == b.x && a.y == b.y;
            }

            friend bool operator!=(const Point& a, const Point& b) {
                return !(a == b);
            }

            inline Point<T> operator-() const {
                return Point<T>(-x, -y);
            }

            inline Point operator+(const Point& b) const {
                return Point(x + b.x, y + b.y);
            }

            inline Point operator-(const Point& b) const {
                return Point(x - b.x, y - b.y);
            }

            bool operator< (const Point& b) const {

                bool result = false;

                if (b.x < x) {
                    result = true;
                }
                if (b.y < y) {
                    result = true;
                }
                return result;
            }

            inline void Negate() {
                x = -x;
                y = -y;
            }

            // friend std::ostream& operator<<(std::ostream& os, const Point& point) {
            //     os << point.x << "," << point.y << " ";
            //     return os;
            // }
    };

}

#endif

