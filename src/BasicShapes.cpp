#include "BasicShapes.hpp"
#include "glm/gtc/constants.hpp"

namespace {

struct Vertex1p1n1st {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
};

constexpr auto kPlaneSize = 10.0f;
constexpr auto kPlaneScale = 10.0f;

// clang-format off
constexpr auto kNumPlaneVertices = 6u;
const Vertex1p1n1st kPlaneVertices[kNumPlaneVertices]{
  {{ kPlaneSize, 0.0f,  kPlaneSize}, {0.0f, 1.0f, 0.0f}, {kPlaneScale, 0.0f}},
  {{-kPlaneSize, 0.0f, -kPlaneSize}, {0.0f, 1.0f, 0.0f}, {0.0f, kPlaneScale}},
  {{-kPlaneSize, 0.0f,  kPlaneSize}, {0.0f, 1.0f, 0.0f}, {0.0f,        0.0f}},

  {{ kPlaneSize, 0.0f,  kPlaneSize}, {0.0f, 1.0f, 0.0f}, {kPlaneScale,        0.0f}},
  {{ kPlaneSize, 0.0f, -kPlaneSize}, {0.0f, 1.0f, 0.0f}, {kPlaneScale, kPlaneScale}},
  {{-kPlaneSize, 0.0f, -kPlaneSize}, {0.0f, 1.0f, 0.0f}, {0.0f,        kPlaneScale}}};

constexpr auto kNumCubeVertices = 36u;
const Vertex1p1n1st kCubeVertices[kNumCubeVertices]{
  // back
  {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
  {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
  {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}}, // bottom-right
  {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
  {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
  {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}}, // top-left
  // Front face
  {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},   // bottom-left
  {{ 1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},   // bottom-right
  {{ 1.0f,  1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},   // top-right
  {{ 1.0f,  1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},   // top-right
  {{-1.0f,  1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},   // top-left
  {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},   // bottom-left
  // Left face
  {{-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // top-right
  {{-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // top-left
  {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // bottom-left
  {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // bottom-left
  {{-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-right
  {{-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // top-right
  // Right face
  {{1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},   // top-left
  {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},   // bottom-right
  {{1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},   // top-right
  {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},   // bottom-right
  {{1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},   // top-left
  {{1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},   // bottom-left
  // Bottom face
  {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, // top-right
  {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}}, // top-left
  {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-left
  {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-left
  {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-right
  {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, // top-right
  // Top face
  {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},   // top-left
  {{ 1.0f, 1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},   // bottom-right
  {{ 1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},   // top-right
  {{ 1.0f, 1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},   // bottom-right
  {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},   // top-left
  {{-1.0f, 1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}    // bottom-left
};
// clang-format on

template <uint8_t _NumSectors = 36, uint8_t _NumStacks = 18>
[[nodiscard]] auto buildSphere(float radius) {
  // http://www.songho.ca/opengl/gl_sphere.html

  static_assert(_NumSectors >= 3 and _NumStacks >= 2);
  constexpr auto kPI = glm::pi<float>();

  std::vector<Vertex1p1n1st> vertices;
  vertices.reserve((_NumSectors + 1) * (_NumStacks + 1));

  Vertex1p1n1st vertex{};
  const auto invLength = 1.0f / radius;

  const auto kSectorStep = 2 * kPI / _NumSectors;
  const auto kStackStep = kPI / _NumStacks;

  for (uint8_t i{0}; i <= _NumStacks; ++i) {
    // Starting from pi / 2 to - pi / 2
    const float stackAngle{kPI / 2.0f - i * kStackStep};
    const auto xy = radius * glm::cos(stackAngle);
    vertex.position.z = radius * glm::sin(stackAngle);

    // Add (sectorCount + 1) vertices per stack
    // the first and last vertices have same position and normal, but different
    // tex coords
    for (uint8_t j{0}; j <= _NumSectors; ++j) {
      const float sectorAngle = j * kSectorStep; // starting from 0 to 2pi

      vertex.position.x = xy * glm::cos(sectorAngle); // r * cos(u) * cos(v)
      vertex.position.y = xy * glm::sin(sectorAngle); // r * cos(u) * sin(v)

      vertex.normal = vertex.position * invLength;

      vertex.texCoord = {
        static_cast<float>(j) / _NumSectors,
        static_cast<float>(i) / _NumStacks,
      };

      vertices.push_back(vertex);
    }
  }

  std::vector<uint16_t> indices;
  indices.reserve(_NumSectors * (_NumStacks - 1) * 6);

  //  k1--k1+1
  //  |  / |
  //  | /  |
  //  k2--k2+1
  for (uint8_t i{0}; i < _NumStacks; ++i) {
    uint16_t k1{i * (_NumSectors + 1u)}; // Beginning of current stack
    uint16_t k2{k1 + _NumSectors + 1u};  // Beginning of next stack

    for (uint16_t j{0}; j < _NumSectors; ++j, ++k1, ++k2) {
      // 2 triangles per sector excluding 1st and last stacks
      if (i != 0) indices.insert(indices.end(), {k1, k2, k1 + 1u});
      if (i != (_NumStacks - 1))
        indices.insert(indices.end(), {k1 + 1u, k2, k2 + 1u});
    }
  }
  return std::make_tuple(std::move(vertices), std::move(indices));
}

} // namespace

//
// BasicShapes class:
//

BasicShapes::BasicShapes(RenderContext &rc) : m_renderContext{rc} {
  auto [sphereVertices, sphereIndices] = buildSphere<>(1.0f);

  {
    auto vertexBuffer = rc.createVertexBuffer(
      sizeof(Vertex1p1n1st),
      kNumPlaneVertices + kNumCubeVertices + sphereVertices.size());
    m_vertexBuffer =
      std::shared_ptr<VertexBuffer>(new VertexBuffer{std::move(vertexBuffer)},
                                    RenderContext::ResourceDeleter{rc});

    auto indexBuffer = rc.createIndexBuffer(
      IndexType::UInt16, sphereIndices.size(), sphereIndices.data());
    m_indexBuffer =
      std::shared_ptr<IndexBuffer>(new IndexBuffer{std::move(indexBuffer)},
                                   RenderContext::ResourceDeleter{rc});
  }

  auto vertices =
    static_cast<Vertex1p1n1st *>(m_renderContext.map(*m_vertexBuffer));
  memcpy(vertices, kPlaneVertices, kNumPlaneVertices * sizeof(Vertex1p1n1st));
  vertices += kNumPlaneVertices;
  memcpy(vertices, kCubeVertices, kNumCubeVertices * sizeof(Vertex1p1n1st));
  vertices += kNumCubeVertices;
  memcpy(vertices, sphereVertices.data(),
         sphereVertices.size() * sizeof(Vertex1p1n1st));
  m_renderContext.unmap(*m_vertexBuffer);

  auto vertexFormat =
    VertexFormat::Builder{}
      .setAttribute(AttributeLocation::Position,
                    {
                      .type = VertexAttribute::Type::Float3,
                      .offset = 0,
                    })
      .setAttribute(AttributeLocation::Normal,
                    {
                      .type = VertexAttribute::Type::Float3,
                      .offset = offsetof(Vertex1p1n1st, normal),
                    })
      .setAttribute(AttributeLocation::TexCoord_0,
                    {
                      .type = VertexAttribute::Type::Float2,
                      .offset = offsetof(Vertex1p1n1st, texCoord),
                    })
      .build();

  m_plane = {
    .vertexFormat = vertexFormat,
    .vertexBuffer = m_vertexBuffer,
    .subMeshes =
      {
        {
          .geometryInfo =
            {
              .topology = PrimitiveTopology::TriangleList,
              .vertexOffset = 0,
              .numVertices = kNumPlaneVertices,
            },
        },
      },
    .aabb =
      {
        .min = {-kPlaneSize, 0.0f, -kPlaneSize},
        .max = {kPlaneSize, 0.0f, kPlaneSize},
      },
  };
  m_cube = {
    .vertexFormat = vertexFormat,
    .vertexBuffer = m_vertexBuffer,
    .subMeshes =
      {
        {
          .geometryInfo =
            {
              .topology = PrimitiveTopology::TriangleList,
              .vertexOffset = kNumPlaneVertices,
              .numVertices = kNumCubeVertices,
            },
        },
      },
    .aabb =
      {
        .min = glm::vec3{-1.0f},
        .max = glm::vec3{1.0f},
      },
  };
  m_sphere = {
    .vertexFormat = vertexFormat,
    .vertexBuffer = m_vertexBuffer,
    .indexBuffer = m_indexBuffer,
    .subMeshes =
      {
        {
          .geometryInfo =
            {
              .topology = PrimitiveTopology::TriangleList,
              .vertexOffset = kNumPlaneVertices + kNumCubeVertices,
              .numVertices = static_cast<uint32_t>(sphereVertices.size()),
              .numIndices = static_cast<uint32_t>(sphereIndices.size()),
            },
        },
      },
    .aabb =
      {
        .min = glm::vec3{-1.0f},
        .max = glm::vec3{1.0f},
      },
  };
}
BasicShapes::~BasicShapes() = default;

const Mesh &BasicShapes::getPlane() const { return m_plane; }
const Mesh &BasicShapes::getCube() const { return m_cube; }
const Mesh &BasicShapes::getSphere() const { return m_sphere; }
