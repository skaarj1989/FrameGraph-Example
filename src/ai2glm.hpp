#pragma once

#include "assimp/color4.h"
#include "assimp/vector3.h"
#include "assimp/matrix3x3.h"
#include "assimp/matrix4x4.h"
#include "assimp/quaternion.h"
#include "assimp/aabb.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

#include "AABB.hpp"

[[nodiscard]] glm::vec2 to_vec2(const aiVector3D &);
[[nodiscard]] glm::vec3 to_vec3(const aiVector3D &);
[[nodiscard]] glm::vec3 to_vec3(const aiColor4D &);
[[nodiscard]] glm::vec4 to_vec4(const aiColor4D &);
[[nodiscard]] glm::mat4 to_mat4(const aiMatrix4x4 &);

[[nodiscard]] AABB to_aabb(const aiAABB &);
