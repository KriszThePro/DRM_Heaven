#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace iqd {

struct TxConditioningConfig {
	float scale = 1.0F;
	float headroomDb = 0.0F;
	bool clip = false;
	bool shape = false;
};

struct TxConditioningTelemetry {
	size_t scalarSamples = 0U;
	size_t overrunSamples = 0U;
	size_t clippedSamples = 0U;
	size_t underrunEvents = 0U;
	float peakBefore = 0.0F;
	float peakAfter = 0.0F;
};

std::vector<float> ApplyTxConditioning(const std::vector<float>& interleavedIq,
									   const TxConditioningConfig& cfg,
									   TxConditioningTelemetry* telemetry);

std::vector<uint8_t> ConvertIqToFl2kU8(const std::vector<float>& interleavedIq,
									   const TxConditioningConfig& cfg,
									   TxConditioningTelemetry* telemetry);

std::vector<uint8_t> ConvertIqToFl2kU8(const std::vector<float>& interleavedIq);
void StreamBytesToCommandStdin(const std::string& command, const std::vector<uint8_t>& bytes);
bool RunTxConditioningSelfTest();

} // namespace iqd

