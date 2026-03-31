#pragma once

#include <string>

#include "iq_decoder_types.h"

namespace iqd {

bool TryLoadIqMeta(const std::string& iqPath, const std::string& explicitMetaPath, IqMeta& meta);

} // namespace iqd

