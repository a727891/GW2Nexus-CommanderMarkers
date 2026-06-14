#include "utils/MarkerBindHelper.h"

namespace cm {

namespace {

std::optional<EGameBinds> MarkerIndexToGroundBind(int index) {
    switch (index) {
        case 1:
            return GB_SquadMarkerPlaceWorld1;
        case 2:
            return GB_SquadMarkerPlaceWorld2;
        case 3:
            return GB_SquadMarkerPlaceWorld3;
        case 4:
            return GB_SquadMarkerPlaceWorld4;
        case 5:
            return GB_SquadMarkerPlaceWorld5;
        case 6:
            return GB_SquadMarkerPlaceWorld6;
        case 7:
            return GB_SquadMarkerPlaceWorld7;
        case 8:
            return GB_SquadMarkerPlaceWorld8;
        default:
            return std::nullopt;
    }
}

std::optional<EGameBinds> MarkerIndexToObjectBind(int index) {
    switch (index) {
        case 1:
            return GB_SquadMarkerSetAgent1;
        case 2:
            return GB_SquadMarkerSetAgent2;
        case 3:
            return GB_SquadMarkerSetAgent3;
        case 4:
            return GB_SquadMarkerSetAgent4;
        case 5:
            return GB_SquadMarkerSetAgent5;
        case 6:
            return GB_SquadMarkerSetAgent6;
        case 7:
            return GB_SquadMarkerSetAgent7;
        case 8:
            return GB_SquadMarkerSetAgent8;
        default:
            return std::nullopt;
    }
}

bool IsBindAvailable(AddonAPI_t* api, EGameBinds bind) {
    return api && api->GameBinds_IsBound && api->GameBinds_IsBound(bind);
}

void InvokeBind(AddonAPI_t* api, EGameBinds bind, int durationMs) {
    if (!api || !api->GameBinds_InvokeAsync) {
        return;
    }
    api->GameBinds_InvokeAsync(bind, durationMs);
}

void CheckBind(AddonAPI_t* api, EGameBinds bind, const char* label, BindCheckResult& result) {
    if (!IsBindAvailable(api, bind)) {
        result.ok = false;
        result.missing.emplace_back(label);
    }
}

}  // namespace

std::optional<EGameBinds> GroundMarkerToBind(SquadMarker marker) {
    if (marker == SquadMarker::Clear) {
        return GB_SquadMarkerClearAllWorld;
    }
    return MarkerIndexToGroundBind(static_cast<int>(marker));
}

std::optional<EGameBinds> ObjectMarkerToBind(SquadMarker marker) {
    if (marker == SquadMarker::Clear) {
        return GB_SquadMarkerClearAllAgent;
    }
    return MarkerIndexToObjectBind(static_cast<int>(marker));
}

void InvokeGroundMarker(SquadMarker marker, AddonAPI_t* api, int durationMs) {
    const auto bind = GroundMarkerToBind(marker);
    if (bind) {
        InvokeBind(api, *bind, durationMs);
    }
}

void InvokeObjectMarker(SquadMarker marker, AddonAPI_t* api, int durationMs) {
    const auto bind = ObjectMarkerToBind(marker);
    if (bind) {
        InvokeBind(api, *bind, durationMs);
    }
}

BindCheckResult CheckRequiredBinds(AddonAPI_t* api, bool needObject) {
    BindCheckResult result{.ok = true, .missing = {}};

    CheckBind(api, GB_SquadMarkerClearAllWorld, "Clear All (Ground)", result);
    CheckBind(api, GB_SquadMarkerPlaceWorld1, "Arrow (Ground)", result);
    CheckBind(api, GB_SquadMarkerPlaceWorld2, "Circle (Ground)", result);
    CheckBind(api, GB_SquadMarkerPlaceWorld3, "Heart (Ground)", result);
    CheckBind(api, GB_SquadMarkerPlaceWorld4, "Square (Ground)", result);
    CheckBind(api, GB_SquadMarkerPlaceWorld5, "Star (Ground)", result);
    CheckBind(api, GB_SquadMarkerPlaceWorld6, "Spiral (Ground)", result);
    CheckBind(api, GB_SquadMarkerPlaceWorld7, "Triangle (Ground)", result);
    CheckBind(api, GB_SquadMarkerPlaceWorld8, "Cross (Ground)", result);

    if (needObject) {
        CheckBind(api, GB_SquadMarkerClearAllAgent, "Clear All (Object)", result);
        CheckBind(api, GB_SquadMarkerSetAgent1, "Arrow (Object)", result);
        CheckBind(api, GB_SquadMarkerSetAgent2, "Circle (Object)", result);
        CheckBind(api, GB_SquadMarkerSetAgent3, "Heart (Object)", result);
        CheckBind(api, GB_SquadMarkerSetAgent4, "Square (Object)", result);
        CheckBind(api, GB_SquadMarkerSetAgent5, "Star (Object)", result);
        CheckBind(api, GB_SquadMarkerSetAgent6, "Spiral (Object)", result);
        CheckBind(api, GB_SquadMarkerSetAgent7, "Triangle (Object)", result);
        CheckBind(api, GB_SquadMarkerSetAgent8, "Cross (Object)", result);
    }

    return result;
}

std::string SquadMarkerDisplayName(SquadMarker marker) {
    switch (marker) {
        case SquadMarker::None:
            return "None";
        case SquadMarker::Arrow:
            return "Arrow";
        case SquadMarker::Circle:
            return "Circle";
        case SquadMarker::Heart:
            return "Heart";
        case SquadMarker::Square:
            return "Square";
        case SquadMarker::Star:
            return "STAR";
        case SquadMarker::Spiral:
            return "Sprial";
        case SquadMarker::Triangle:
            return "Triangle";
        case SquadMarker::Cross:
            return "Cross";
        case SquadMarker::Clear:
            return "Clear";
    }
    return "None";
}

}  // namespace cm
