/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once

#include <rocky/Common.h>
#include <glm/ext.hpp>
#include <iterator>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ROCKY_NAMESPACE
{
    //using dvec2 = glm::dvec2;
    //using dvec3 = glm::dvec3;
    //using dvec4 = glm::dvec4;
    //using dmat3 = glm::dmat3;
    //using dmat4 = glm::dmat4;

    //using fvec2 = glm::fvec2;
    //using fvec3 = glm::fvec3;
    //using fvec4 = glm::fvec4;
    //using fmat4 = glm::fmat4;

    //using u8vec4 = glm::u8vec4;

    // mult a vec3 x mat4
    inline glm::dvec3 operator * (const glm::dvec3& a, const glm::dmat4& b)
    {
        return glm::dvec3(glm::dvec4(a, 1) * b);
    }    
    inline glm::fvec3 operator * (const glm::fvec3& a, const glm::fmat4& b)
    {
        return glm::fvec3(glm::fvec4(a, 1) * b);
    }

#if 1
    class ROCKY_EXPORT Sphere
    {
    public:
        glm::dvec3 center;
        double radius;

        Sphere() : center(0, 0, 0), radius(-1) { }

        Sphere(const glm::dvec3& a, double b) :
            center(a), radius(b) { }

        void expandBy(const glm::dvec3& v) {
            if (valid()) {
                auto dv = v - center;
                double r = glm::length(dv);
                if (r > radius) {
                    double dr = 0.5*(r - radius);
                    center += dv * (dr / r);
                    radius += dr;
                }
            }
            else {
                center = v;
                radius = 0.0;
            }
        }

        bool valid() const {
            return radius >= 0.0;
        }
    };
#endif

    struct ROCKY_EXPORT Box
    {        
        double xmin, ymin, zmin;
        double xmax, ymax, zmax;

        inline double width() const {
            return xmax - xmin;
        }
        inline double height() const {
            return ymax - ymin;
        }
        inline double area2d() const {
            return width()*height();
        }
        inline glm::dvec3 center() const {
            return glm::dvec3(
                xmin + 0.5*(xmax - xmin),
                ymin + 0.5*(ymax - ymin),
                zmin + 0.5*(zmax - zmin));
        }
        bool intersects(const Box& rhs) const {
            bool exclusive =
                xmin > rhs.xmax || xmax < rhs.xmin ||
                ymin > rhs.ymax || ymax < rhs.ymin ||
                zmin > rhs.zmax || zmax < rhs.zmin;
            return !exclusive;
        }
        Box intersection_with(const Box&) const;

        Box union_with(const Box&) const;

        bool contains(const Box&) const;

        void expandBy(const glm::dvec3& p) {
            xmin = std::min(xmin, p.x);
            xmax = std::max(xmax, p.x);
            ymin = std::min(ymin, p.y);
            ymax = std::max(ymax, p.y);
            zmin = std::min(zmin, p.z);
            zmax = std::max(zmax, p.z);
        }

        void expandBy(const Box& rhs) {
            xmin = std::min(xmin, rhs.xmin);
            xmax = std::max(xmax, rhs.xmax);
            ymin = std::min(ymin, rhs.ymin);
            ymax = std::max(ymax, rhs.ymax);
            zmin = std::min(zmin, rhs.zmin);
            zmax = std::max(zmax, rhs.zmax);
        }

        glm::dvec3 corner(unsigned i) const {
            return glm::dvec3(
                (i & 0x1) ? xmax : xmin,
                (i & 0x2) ? ymax : ymin,
                (i & 0x4) ? zmax : zmin);
        }

#if 1
        void expandBy(const Sphere& rhs) {
            expandBy(Box(
                rhs.center.x - rhs.radius, rhs.center.x + rhs.radius,
                rhs.center.y - rhs.radius, rhs.center.y + rhs.radius,
                rhs.center.z - rhs.radius, rhs.center.z + rhs.radius));
        }
#endif

        bool valid() const {
            return
                xmin <= xmax &&
                ymin <= ymax &&
                zmin <= zmax;
        }

        Box() :
            xmin(DBL_MAX), ymin(DBL_MAX), zmin(DBL_MAX),
            xmax(-DBL_MAX), ymax(-DBL_MAX), zmax(-DBL_MAX) { }

        Box(double x0, double y0, double z0, double x1, double y1, double z1) :
            xmin(x0), ymin(y0), zmin(z0),
            xmax(x1), ymax(y1), zmax(z1) { }

        Box(double x0, double y0, double x1, double y1) :
            xmin(x0), ymin(y0), zmin(0.0),
            xmax(x1), ymax(y1), zmax(0.0) { }
    };

    template<typename T>
    inline double lengthSquared(const T& v) {
        return glm::dot(v, v);
    }

    template<typename T>
    inline T deg2rad(T v) {
        return v * static_cast<T>(M_PI)/static_cast<T>(180.0);
    }

    template<typename T>
    inline T rad2deg(T v) {
        return v * static_cast<T>(180.0) / static_cast<T>(M_PI);
    }

    template<typename T>
    inline T step(const T& edge, const T& x)
    {
        return x < edge ? static_cast<T>(0.0) : static_cast<T>(1.0);
    }
    
    template<typename T>
    inline T clamp(const T& x, const T& lo, const T& hi)
    {
        return x<lo ? lo : x>hi ? hi : x;
    }

    template<typename T>
    inline T lerpstep(T lo, T hi, T x)
    {
        if (x <= lo) return static_cast<T>(0.0);
        else if (x >= hi) return static_cast<T>(1.0);
        else return (x - lo) / (hi - lo);
    }

    template<typename T>
    inline T smoothstep(T lo, T hi, T x)
    {
        T t = clamp((x - lo) / (hi - lo), static_cast<T>(0.0), static_cast<T>(1.0));
        return t * t*(static_cast<T>(3.0) - static_cast<T>(2.0)*t);
    }

    // move closer to one
    template<typename T>
    inline T decel(T x)
    {
        return static_cast<T>(1.0) - (static_cast<T>(1.0) - x) * (static_cast<T>(1.0) - x);
    }

    // move closer to zero
    template<typename T>
    inline T accel(T x)
    { 
        return soften(x*x);
    }

    template<typename T>
    inline T threshold(T x, T thresh, T buf)
    {
        if (x < thresh - buf) return static_cast<T>(0.0);
        else if (x > thresh + buf) return static_cast<T>(1.0);
        else return clamp((x - (thresh - buf)) / (buf*static_cast<T>(2.0)), static_cast<T>(0.0), static_cast<T>(1.0));
    }

    template<typename T>
    inline T fract(T x)
    {
        return x - floor(x);
    }

    template<typename T>
    inline double unitremap(T a, T lo, T hi)
    {
        return clamp((a - lo) / (hi - lo), static_cast<T>(0.0), static_cast<T>(1.0));
    }

    template<typename T>
    inline T mix(const T& a, const T& b, float c)
    {
        return a * (1.0 - c) + b * c;
    }

    template<typename T>
    inline double dot2D(const T& a, const T& b)
    {        
        return a.x()*b.x() + a.y()*b.y();
    }

    template<typename T>
    inline double dot3D(const T& a, const T& b)
    {
        return a.x()*b.x() + a.y()*b.y() + a.z()*b.z();
    }

    template<typename T>
    inline double distanceSquared2D(const T& a, const T& b)
    {
        return 
            (b.x() - a.x())*(b.x() - a.x()) + 
            (b.y() - a.y())*(b.y() - a.y());
    }

    template<typename T>
    inline double distanceSquared3D(const T& a, const T& b)
    {
        return
            (b.x() - a.x())*(b.x() - a.x()) +
            (b.y() - a.y())*(b.y() - a.y()) +
            (b.z() - a.z())*(b.z() - a.z());
    }

    template<typename T>
    inline double distance2D(const T& a, const T& b)
    {
        return sqrt(distanceSquared2D(a, b));
    }

    template<typename T>
    inline double distance3D(const T& a, const T& b)
    {
        return sqrt(distanceSquared3D(a, b));
    }

    template<typename T>
    inline T square(const T& a)
    {
        return (a) * (a);
    }

    template<typename T>
    inline T normalize(const T& a)
    {
        T temp = a;
        temp.normalize();
        return temp;
    }

    // Round integral x to the nearest multiple of "multiple" greater than or equal to x
    template<typename T>
    inline T align(T x, T multiple) 
    {
        T isPositive = (T)(x >= 0);
        return ((x + isPositive * (multiple - 1)) / multiple) * multiple;
    }

    // equal within a threshold
    template<typename A, typename B>
    inline bool equiv(A x, B y, double epsilon) 
    {
        double delta = x - y;
        return delta < 0.0 ? delta >= -epsilon : delta <= epsilon;
    }

    // equal within a default threshold
    template<typename A, typename B>
    inline bool equiv(A x, B y) 
    {
        return equiv(x, y, 1e-6);
    }

    // equal within a default threshold
    template<>
    inline bool equiv<glm::dvec3>(glm::dvec3 a, glm::dvec3 b, double E)
    {
        return equiv(a.x, b.x, E) && equiv(a.y, b.y, E) && equiv(a.z, b.z, E);
    }

    // equal within a default threshold
    template<>
    inline bool equiv<glm::dvec3>(glm::dvec3 a, glm::dvec3 b)
    {
        return equiv(a.x, b.x) && equiv(a.y, b.y) && equiv(a.z, b.z);
    }

    inline int nextPowerOf2(int x)
    {
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x + 1;
    }

    template<typename... Args>
    inline double smallest(Args... a) {
        double r = DBL_MAX;
        for (auto v : std::initializer_list<double>({ a... })) {
            r = std::min(r, v);
        }
        return r;
    }

    template<typename... Args>
    inline double largest(Args... a) {
        double r = -DBL_MAX;
        for (auto v : std::initializer_list<double>({ a... })) {
            r = std::max(r, v);
        }
        return r;
    }

    // Adapted from Boost - see boost license
    // https://www.boost.org/users/license.html
    template <typename T>
    inline std::size_t hash_value_unsigned(T val)
    {
        const int size_t_bits = std::numeric_limits<std::size_t>::digits;
        const int length = (std::numeric_limits<T>::digits - 1) / size_t_bits;
        std::size_t seed = 0;
        for(unsigned int i = length * size_t_bits; i > 0; i -= size_t_bits)
            seed ^= (std::size_t) (val >> i) + (seed<<6) + (seed>>2);
        seed ^= (std::size_t) val + (seed<<6) + (seed>>2);
        return seed;
    }

    inline std::size_t hash_value_unsigned(bool val)
    {
        return hash_value_unsigned((unsigned)val ? 0x1111111 : 0x2222222);
    }

    template<typename T>
    inline std::size_t hash_value_unsigned(const optional<T>& val) {
        if (val.has_value())
            return hash_value_unsigned(0x3333333u, val.get());
        else
            return (std::size_t)0;
    }

    template <typename A, typename B> inline std::size_t hash_value_unsigned(A a, B b) {
        std::size_t seed = hash_value_unsigned(a);
        seed ^= hash_value_unsigned(b) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        return seed;
    }

    template <typename A, typename B, typename C> inline std::size_t hash_value_unsigned(A a, B b, C c) {
        std::size_t seed = hash_value_unsigned(a);
        seed ^= hash_value_unsigned(b) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        seed ^= hash_value_unsigned(c) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        return seed;
    }

    template <typename A, typename B, typename C, typename D> inline std::size_t hash_value_unsigned(A a, B b, C c, D d) {
        std::size_t seed = hash_value_unsigned(a);
        seed ^= hash_value_unsigned(b) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        seed ^= hash_value_unsigned(c) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        seed ^= hash_value_unsigned(d) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        return seed;
    }

    template <typename A, typename B, typename C, typename D, typename E> inline std::size_t hash_value_unsigned(A a, B b, C c, D d, E e) {
        std::size_t seed = hash_value_unsigned(a);
        seed ^= hash_value_unsigned(b) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash_value_unsigned(c) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash_value_unsigned(d) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash_value_unsigned(e) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }

    template<int C, int R, typename T, glm::qualifier Q>
    bool is_identity(const glm::mat<C, R, T, Q>& m) {
        for(int c=0; c<C; ++c)
            for(int r=0; r<R; ++r)
                if ((c == r && !equiv(m[c][r],static_cast<T>(1))) ||
                    (c != r && !equiv(m[c][r],static_cast<T>(0))))
                    return false;
        return true;
    }

#if 1
    // vec3 * mat4
    //template<typename A, typename B>
    //A pre_mult(const A& v, const B& _mat) {
    //    double d = 1.0f / (_mat[0][3] * v.x + _mat[1][3] * v.y + _mat[2][3] * v.z + _mat[3][3]);
    //    return A(
    //        (_mat[0][0] * v.x + _mat[1][0] * v.y + _mat[2][0] * v.z + _mat[3][0])*d,
    //        (_mat[0][1] * v.x + _mat[1][1] * v.y + _mat[2][1] * v.z + _mat[3][1])*d,
    //        (_mat[0][2] * v.x + _mat[1][2] * v.y + _mat[2][2] * v.z + _mat[3][2])*d);
    //}

    template<typename M>
    M pre_mult(const M& a, const M& b) {
        return a * b;
    }

    //! Convenience function to transform a 3D vector using a 4D matrix
    inline glm::fvec3 operator * (const glm::fmat4& mat, const glm::fvec3& v) {
        return glm::fvec3(mat * glm::fvec4(v, 1));
    }

    //! Convenience function to transform a 3D vector using a 4D matrix
    inline glm::dvec3 operator * (const glm::dmat4& mat, const glm::dvec3& v) {
        return glm::dvec3(mat * glm::dvec4(v, 1));
    }
#endif

#if 0
    /**
     * Utilities to manipulate a projection matrix.
     * These functions will automatically handle standard OGL versus Reverse-Z
     * projection matrices.
     */
    struct ROCKY_EXPORT ProjectionMatrix
    {
        //! Projection matrix type
        using Type = enum {
            STANDARD,
            REVERSE_Z,
            UNKNOWN
        };

        //! True for an orthographic projection matrix
        static bool isOrtho(
            const dmat4& m);

        //! True for a perspective projection matrix
        static bool isPerspective(
            const dmat4& m);

        //! Detected type of the projection matrix
        static Type getType(
            const dmat4& m);

        //! Construct a perspective matrix, deriving the type from the
        //! existing values in the matrix if possible
        static void setPerspective(
            dmat4& m,
            double vfov, double ar, double N, double F,
            Type = UNKNOWN);

        //! Extract values from a perspective matrix
        static bool getPerspective(
            const dmat4& m,
            double& vfov, double& ar, double& N, double& F);

        //! Extract raw frustum values from a perspective matrix
        static bool getPerspective(
            const dmat4& m,
            double& L, double& R, double& B, double& T, double& N, double& F);

        //! Construct an orthographc matrix, deriving the type from the
        //! existing values in the matrix if possible
        static void setOrtho(
            dmat4& m,
            double L, double R, double B, double T, double N, double F,
            Type = UNKNOWN);

        //! Extract frustum values from an orthographic matrix
        static bool getOrtho(
            const dmat4& m,
            double& L, double& R, double& B, double& T, double& N, double& F);
    };
#endif

}


