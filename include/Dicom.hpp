#pragma once

#include <string>
#include "Volume.hpp"

Volume LoadDicomStack(const std::string& folder, int& result);
