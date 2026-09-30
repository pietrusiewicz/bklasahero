// B-Klasa Hero — serializacja JSON (save'y i protokół mostka JNI).
//
// Reguły:
//   * klucze obiektów są w porządku alfabetycznym (właściwość nlohmann::json) →
//     ten sam stan daje zawsze ten sam bajtowo JSON (ważne dla eksportu kopii
//     zapasowej i dla testów porównawczych),
//   * każdy save ma pole `schemaVersion`; wczytanie nowszej wersji kończy się
//     błędem `error.schema_version`, starsze wersje przechodzą migrację,
//   * JSON jest JEDYNYM formatem wymiany z warstwą Kotlin — brak binarnych
//     struktur przez JNI (mniej klas błędów, łatwiejsze debugowanie),
//   * nagłówek nlohmann/json jest włączany WYŁĄCZNIE w json_io.cpp i facade.cpp,
//     żeby publiczne nagłówki rdzenia pozostały wolne od zależności zewnętrznych.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bkh/career.h"
#include "bkh/club.h"
#include "bkh/league.h"
#include "bkh/place.h"
#include "bkh/result.h"
#include "bkh/shot.h"
#include "bkh/shootout.h"
#include "bkh/types.h"

namespace bkh::json_io {

/// Aktualna wersja schematu save'a.
inline constexpr i32 kSaveSchemaVersion = CareerState::kSchemaVersion;

// --- Zapis ------------------------------------------------------------------
[[nodiscard]] std::string saveToJson(const CareerState& state, i64 savedAtEpochMs = 0);
[[nodiscard]] std::string toJson(const CareerState& state);
[[nodiscard]] std::string toJson(const CareerStats& stats);
[[nodiscard]] std::string toJson(const Progression& progression);
[[nodiscard]] std::string toJson(const PlayerAttributes& attributes);
[[nodiscard]] std::string toJson(const CosmeticItem& item);
[[nodiscard]] std::vector<std::string> toJsonList(const std::vector<CosmeticItem>& items);
[[nodiscard]] std::string toJson(const Club& club);
[[nodiscard]] std::vector<std::string> toJsonList(const std::vector<Club>& clubs);
[[nodiscard]] std::string toJson(const Place& place);
[[nodiscard]] std::vector<std::string> toJsonList(const std::vector<const Place*>& places);
[[nodiscard]] std::string toJson(const LeagueRow& row);
[[nodiscard]] std::vector<std::string> toJsonList(const std::vector<LeagueRow>& rows);
[[nodiscard]] std::string toJson(const Fixture& fixture);
[[nodiscard]] std::string toJson(const MatchResultRecord& record);
[[nodiscard]] std::string toJson(const ShotResolution& resolution);
[[nodiscard]] std::string toJson(const KickRecord& kick);
[[nodiscard]] std::vector<std::string> toJsonList(const std::vector<KickRecord>& kicks);
[[nodiscard]] std::string toJson(const MatchSetup& setup);
[[nodiscard]] std::string toJson(const SeasonSummary& summary);
[[nodiscard]] std::string toJson(const SeasonRecord& record);
[[nodiscard]] std::string toJson(const ShootoutState& state);

// --- Odczyt -----------------------------------------------------------------
[[nodiscard]] Result<CareerState> loadFromJson(std::string_view json);
[[nodiscard]] Result<CareerState> careerFromJson(std::string_view json);

// --- Parsowanie wejścia gracza (protokół poleceń) ----------------------------
[[nodiscard]] Result<PlayerShotInput> parsePlayerShotInput(std::string_view json);
[[nodiscard]] Result<PlayerDiveInput> parsePlayerDiveInput(std::string_view json);
[[nodiscard]] Result<NewCareerParams> parseNewCareerParams(std::string_view json);

// --- Pomocnicze: mapowanie nazw pól na typy (stabilne klucze ASCII) ----------
[[nodiscard]] std::optional<AttributeKind> attributeKindFromKey(std::string_view key);
[[nodiscard]] const char* attributeKey(AttributeKind kind);
[[nodiscard]] std::optional<CosmeticKind> cosmeticKindFromKey(std::string_view key);
[[nodiscard]] const char* cosmeticKey(CosmeticKind kind);
[[nodiscard]] std::optional<DiveSide> diveSideFromKey(std::string_view key);
[[nodiscard]] std::optional<DiveHeight> diveHeightFromKey(std::string_view key);
[[nodiscard]] std::optional<Lang> langFromCode(std::string_view code);

/// Walidacja i migracja save'a (na razie tylko wersja 1 — funkcja istnieje, żeby
/// dodanie migracji w przyszłości nie zmieniło interfejsu).
[[nodiscard]] Result<CareerState> migrate(CareerState state);

}  // namespace bkh::json_io
