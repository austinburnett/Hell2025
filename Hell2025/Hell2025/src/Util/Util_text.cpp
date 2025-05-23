#include "Util.h"

namespace Util {
    bool StrCmp(const char* queryA, const char* queryB) {
        if (strcmp(queryA, queryB) == 0)
            return true;
        else
            return false;
    }

    std::string Lowercase(std::string& str) {
        std::string result;
        for (auto& c : str) {
            result += std::tolower(c);
        }
        return result;
    }

    std::string Uppercase(std::string& str) {
        std::string result;
        for (auto& c : str) {
            result += std::toupper(c);
        }
        return result;
    }

    std::string BoolToString(bool b) {
        return b ? "TRUE" : "FALSE";
    }

    std::string Vec2ToString(glm::vec2 v) {
        return std::format("({:.2f}, {:.2f})", v.x, v.y);
    }

    std::string Vec3ToString(glm::vec3 v) {
        return std::format("({:.2f}, {:.2f}, {:.2f})", v.x, v.y, v.z);
    }

    glm::vec3 Vec3Min(const glm::vec3& a, const glm::vec3& b) {
        return glm::vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    }

    glm::vec3 Vec3Max(const glm::vec3& a, const glm::vec3& b) {
        return glm::vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    }

    std::string Mat4ToString(glm::mat4 m) {
        return std::format("{:.2f} {:.2f} {:.2f} {:.2f}\n{:.2f} {:.2f} {:.2f} {:.2f}\n{:.2f} {:.2f} {:.2f} {:.2f}\n{:.2f} {:.2f} {:.2f} {:.2f}",
            m[0][0], m[1][0], m[2][0], m[3][0],
            m[0][1], m[1][1], m[2][1], m[3][1],
            m[0][2], m[1][2], m[2][2], m[3][2],
            m[0][3], m[1][3], m[2][3], m[3][3]);
    }

    const char* CopyConstChar(const char* text) {
        char* b = new char[strlen(text) + 1] {};
        std::copy(text, text + strlen(text), b);
        return b;
    }
}