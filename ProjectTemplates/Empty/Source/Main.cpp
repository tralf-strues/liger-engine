#include <Liger-Engine/Core/Log/ConsoleWriter.hpp>
#include <Liger-Engine/Core/Log/Log.hpp>

int main() {
  liger::Log::Instance().AddWriter(std::make_unique<liger::ConsoleLogWriter>());

  LIGER_LOG_INFO("Empty", "Empty project hello world!");

  return 0;
}