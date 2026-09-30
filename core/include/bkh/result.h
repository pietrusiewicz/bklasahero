// B-Klasa Hero — obsługa błędów bez wyjątków na granicy warstw.
//
// Rdzeń używa wyjątków tylko w razie prawdziwych błędów programistycznych, ale
// granica z JNI/Kotlin MUSI być bezwyjątkowa (wyjątek przelecany przez JNI to
// niezdefiniowane zachowanie). Dlatego wszystkie operacje, które mogą się nie udać
// z powodów „danych" (zły JSON, brak kariery, za mało punktów), zwracają
// `std::expected` (C++23) z kodem błędu w ASCII i komunikatem dla logów.
//
// Kody błędów są stabilnymi kluczami — warstwa Android mapuje je na strings.xml.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace bkh {

struct Error {
    std::string code = "error.unknown";
    std::string message;

    Error() = default;
    Error(std::string_view code_, std::string message_)
        : code(code_), message(std::move(message_)) {}

    [[nodiscard]] bool operator==(const Error&) const = default;
};

template <typename T>
using Result = std::expected<T, Error>;

// Najczęściej używane kody błędów — trzymane w jednym miejscu, żeby warstwa UI
// mogła je obsłużyć bez porównywania literałów rozsianych po kodzie.
namespace errc {
inline constexpr std::string_view kInvalidArgument = "error.invalid_argument";
inline constexpr std::string_view kNotFound = "error.not_found";
inline constexpr std::string_view kNoCareer = "error.no_career";
inline constexpr std::string_view kNoMatch = "error.no_match";
inline constexpr std::string_view kMatchAlreadyStarted = "error.match_already_started";
inline constexpr std::string_view kMatchFinished = "error.match_finished";
inline constexpr std::string_view kShootoutFinished = "error.shootout_finished";
inline constexpr std::string_view kNotPlayersTurn = "error.not_players_turn";
inline constexpr std::string_view kWrongPhase = "error.wrong_phase";
inline constexpr std::string_view kNotEnoughPoints = "error.not_enough_skill_points";
inline constexpr std::string_view kAttributeMaxed = "error.attribute_maxed";
inline constexpr std::string_view kAlreadyUnlocked = "error.already_unlocked";
inline constexpr std::string_view kLocked = "error.locked";
inline constexpr std::string_view kBadJson = "error.bad_json";
inline constexpr std::string_view kBadParam = "error.bad_param";
inline constexpr std::string_view kSchemaVersion = "error.schema_version";
inline constexpr std::string_view kPlacesNotLoaded = "error.places_not_loaded";
inline constexpr std::string_view kCityNotFound = "error.city_not_found";
inline constexpr std::string_view kUnknownCommand = "error.unknown_command";
inline constexpr std::string_view kInternal = "error.internal";
inline constexpr std::string_view kNoSkillPoints = "error.no_skill_points";
inline constexpr std::string_view kMaxedOut = "error.maxed_out";
inline constexpr std::string_view kNotUnlocked = "error.not_unlocked";
inline constexpr std::string_view kIncomplete = "error.incomplete";
}  // namespace errc

/// Skrót do konstrukcji błędu.
[[nodiscard]] inline Error makeError(std::string_view code, std::string message) {
    return Error{code, std::move(message)};
}

/// Skrót do konstrukcji nieoczekiwanego wyniku (bezpośrednio zwracanego z `Result<T>`).
[[nodiscard]] inline std::unexpected<Error> unexpected(std::string_view code, std::string message) {
    return std::unexpected(Error{code, std::move(message)});
}

[[nodiscard]] inline std::unexpected<Error> unexpected(const Error& e) {
    return std::unexpected(e);
}

}  // namespace bkh
