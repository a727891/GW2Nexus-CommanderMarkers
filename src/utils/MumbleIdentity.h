#pragma once

#include "mumble/Mumble.h"

#include <string>

namespace cm {
namespace MumbleIdentity {

int ParseUisz(const Mumble::Data* data);
float ParseUiScale(const Mumble::Data* data);
bool ParseCommander(const Mumble::Data* data);
std::string ParseCharacterName(const Mumble::Data* data);

// Parsed DL_MUMBLE_LINK_IDENTITY is unreliable until mumble has ticked at least once.
bool IsParsedIdentityUsable(const Mumble::Data* mumble, const Mumble::Identity* identity);

}  // namespace MumbleIdentity
}  // namespace cm
