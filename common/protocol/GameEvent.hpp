#pragma once

#include <cstdint>
#include <string>
#include <vector>


// TODO comment here.
class GameEvent final {
public:
    enum class Type : uint8_t {
        NewGame = 0,
        Pixel = 1,
        PlayerEliminated = 2,
        GameOver = 3,
    };

    enum class Format {
        Binary,
        Text,
    };

    struct NewGameData final {
        uint32_t maxx = 0;
        uint32_t maxy = 0;
        std::vector<std::string> players_names;

        // How many bytes can be used for player names.
        // Remember about terminating null bytes!
        static const std::size_t names_capacity;
        std::size_t calculate_used_names_capacity() const noexcept;

    private:
        std::size_t serialize_binary(std::string &buf, std::size_t offset) const noexcept;
        bool deserialize_binary(const std::string &data,
                                std::size_t offset, std::size_t size) noexcept;
        bool validate() const noexcept;
        friend class GameEvent;
    };

    struct PixelData final {
        uint8_t player_no = 0;  // only in Format::Binary
        uint32_t x = 0;
        uint32_t y = 0;
        std::string player_name;  // only in Format::Text

    private:
        std::size_t serialize_binary(std::string &buf, std::size_t offset) const noexcept;
        bool deserialize_binary(const std::string &data,
                                std::size_t offset, std::size_t size) noexcept;
        bool validate(Format fmt) const noexcept;
        friend class GameEvent;
    };

    struct PlayerEliminatedData final {
        uint8_t player_no = 0;  // only in Format::Binary
        std::string player_name;  // only in Format::Text

    private:
        std::size_t serialize_binary(std::string &buf, std::size_t offset) const noexcept;
        bool deserialize_binary(const std::string &data,
                                std::size_t offset, std::size_t size) noexcept;
        bool validate(Format fmt) const noexcept;
        friend class GameEvent;
    };

    uint32_t event_no = 0;
    Type type = Type::NewGame;
    // Can not be in union, because we have containers.
    NewGameData new_game_data;
    PixelData pixel_data;
    PlayerEliminatedData player_eliminated_data;

    // Prepares proper packet. Must be called on valid struct.
    std::string serialize(Format fmt) const noexcept;
    // Loads packet and returns true if action succeeded.
    // When error occurred, struct fields can be invalidated.
    bool deserialize(Format fmt, const std::string &data) noexcept;
    // Check if fields contain valid values.
    bool validate(Format fmt) const noexcept;

private:
    std::string serialize_binary() const noexcept;
    std::string serialize_text() const noexcept;
};


