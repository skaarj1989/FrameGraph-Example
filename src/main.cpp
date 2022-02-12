#include "App.hpp"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[]) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

  try {
    App app{{
      .caption = "OpenGL FrameGraph",
      .width = 1280,
      .height = 720,
      .verticalSync = false,
    }};
    app.run();
  } catch (const std::exception &e) {
    SPDLOG_ERROR(e.what());
    return -1;
  }

  return 0;
}
