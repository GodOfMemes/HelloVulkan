#pragma once

#include "Ray.hpp"
#include "Minimal.hpp"

struct BoundingBox
{
    BoundingBox() = default;
    BoundingBox(const std::vector<glm::vec3>& points)
    {
        glm::vec3 vmin(std::numeric_limits<float>::max());
        glm::vec3 vmax(std::numeric_limits<float>::lowest());

        for (const glm::vec3& p : points)
        {
            vmin = glm::min(vmin, p);
            vmax = glm::max(vmax, p);
        }
        min = glm::vec4(vmin, 1.0);
        max = glm::vec4(vmax, 1.0);
    }

    [[nodiscard]] glm::vec3 GetSize() const { return glm::vec3(max[0] - min[0], max[1] - min[1], max[2] - min[2]); }
	[[nodiscard]] glm::vec3 GetCenter() const { return 0.5f * glm::vec3(max[0] + min[0], max[1] + min[1], max[2] + min[2]); }

    void Transform(const glm::mat4& t)
    {
        std::vector<glm::vec3> corners = 
        {
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(max.x, max.y, max.z),
        };
        
        for (auto& v : corners)
        {
            v = glm::vec3(t * glm::vec4(v, 1.0f));
        }

        *this = BoundingBox(corners);
    }

	[[nodiscard]] BoundingBox GetTransformed(const glm::mat4& t) const
    {
        auto box = *this;
        box.Transform(t);
        return box;
    }

    // gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
	bool Hit(const Ray& r, float &t)
    {
        auto rayFrac = 1.0f / r.direction;

        float t1 = (min.x - r.origin.x) * rayFrac.x;
        float t2 = (max.x - r.origin.x) * rayFrac.x;
        float t3 = (min.y - r.origin.y) * rayFrac.y;
        float t4 = (max.y - r.origin.y) * rayFrac.y;
        float t5 = (min.z - r.origin.z) * rayFrac.z;
        float t6 = (max.z - r.origin.z) * rayFrac.z;

        float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
        float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

        // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
        if (tmax < 0)
        {
            t = tmax;
            return false;
        }

        // if tmin > tmax, ray doesn't intersect AABB
        if (tmin > tmax)
        {
            t = tmax;
            return false;
        }

        t = tmin;
        return true;
    }
    
    glm::vec4 min, max;
};