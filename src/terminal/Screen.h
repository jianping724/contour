/**
 * This file is part of the "libterminal" project
 *   Copyright (c) 2019 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <terminal/Color.h>
#include <terminal/Commands.h>
#include <terminal/Logger.h>
#include <terminal/OutputHandler.h>
#include <terminal/Parser.h>
#include <terminal/WindowSize.h>

#include <fmt/format.h>

#include <algorithm>
#include <functional>
#include <list>
#include <stack>
#include <string>
#include <string_view>
#include <vector>
#include <set>

namespace terminal {

class CharacterStyleMask {
  public:
	enum Mask : uint16_t {
		Bold = (1 << 0),
		Faint = (1 << 1),
		Italic = (1 << 2),
		Underline = (1 << 3),
		Blinking = (1 << 4),
		Inverse = (1 << 5),
		Hidden = (1 << 6),
		CrossedOut = (1 << 7),
		DoublyUnderlined = (1 << 8),
	};

	constexpr CharacterStyleMask() : mask_{} {}
	constexpr CharacterStyleMask(Mask m) : mask_{m} {}
	constexpr CharacterStyleMask(unsigned m) : mask_{m} {}
	constexpr CharacterStyleMask(CharacterStyleMask const& _other) noexcept : mask_{_other.mask_} {}

	constexpr CharacterStyleMask& operator=(CharacterStyleMask const& _other) noexcept
	{
		mask_ = _other.mask_;
		return *this;
	}

	constexpr unsigned mask() const noexcept { return mask_; }

	constexpr operator unsigned () const noexcept { return mask_; }

  private:
	unsigned mask_;
};

std::string to_string(CharacterStyleMask _mask);

constexpr bool operator==(CharacterStyleMask a, CharacterStyleMask b) noexcept
{
	return a.mask() == b.mask();
}

// constexpr CharacterStyleMask operator|(CharacterStyleMask a, CharacterStyleMask b) noexcept
// {
// 	return CharacterStyleMask{a.mask() | b.mask()};
// }
//
// constexpr CharacterStyleMask operator|(CharacterStyleMask a, CharacterStyleMask::Mask b) noexcept
// {
// 	return CharacterStyleMask{a.mask() | static_cast<unsigned>(b)};
// }
//
// constexpr CharacterStyleMask operator~(CharacterStyleMask a) noexcept
// {
// 	return CharacterStyleMask{~a.mask()};
// }

constexpr CharacterStyleMask& operator|=(CharacterStyleMask& a, CharacterStyleMask b) noexcept
{
    a = a | b;
	return a;
}

// constexpr CharacterStyleMask operator&(CharacterStyleMask a, CharacterStyleMask b) noexcept
// {
//     return CharacterStyleMask{a.mask() & b.mask()};
// }
//
// constexpr CharacterStyleMask operator&(CharacterStyleMask a, CharacterStyleMask::Mask b) noexcept
// {
//     return CharacterStyleMask{a.mask() & static_cast<unsigned>(b)};
// }

constexpr CharacterStyleMask& operator&=(CharacterStyleMask& a, CharacterStyleMask b) noexcept
{
    a = a & b;
	return a;
}

constexpr bool operator!(CharacterStyleMask a) noexcept
{
	return a.mask() == 0;
}

/**
 * Terminal Screen.
 *
 * Implements the all Command types and applies all instruction
 * to an internal screen buffer, maintaining width, height, and history,
 * allowing the object owner to control which part of the screen (or history)
 * to be viewn.
 */
class Screen {
  public:
    using Hook = std::function<void(std::vector<Command> const& commands)>;

    /// Character graphics rendition information.
    struct GraphicsAttributes {
        Color foregroundColor{DefaultColor{}};
        Color backgroundColor{DefaultColor{}};
        CharacterStyleMask styles{};
    };

    /// Grid cell with character and graphics rendition information.
    struct Cell {
        char32_t character{};
        GraphicsAttributes attributes{};
    };

    struct Cursor : public Coordinate {
        bool visible = true;

        Cursor& operator=(Coordinate const& coords) noexcept {
            column = coords.column;
            row = coords.row;
            return *this;
        }
    };

    using Reply = std::function<void(std::string const&)>;
    using Renderer = std::function<void(cursor_pos_t row, cursor_pos_t col, Cell const& cell)>;
    using ModeSwitchCallback = std::function<void(bool)>;

  public:
    /**
     * Initializes the screen with the given screen size and callbaks.
     *
     * @param _size screen dimensions in number of characters per line and number of lines.
     * @param _reply reply-callback with the data to send back to terminal input.
     * @param _logger an optional logger for logging various events.
     * @param _error an optional logger for errors.
     * @param _onCommands hook to the commands being executed by the screen.
     */
    Screen(WindowSize const& _size,
           ModeSwitchCallback _useApplicationCursorKeys,
           Reply _reply,
           Logger _logger,
           Hook _onCommands);

    explicit Screen(WindowSize const& _size) :
        Screen{_size, {}, {}, {}, {}} {}

    /// Writes given data into the screen.
    void write(char const* _data, size_t _size);

    /// Writes given data into the screen.
    void write(std::string_view const& _text) { write(_text.data(), _text.size()); }

    /// Renders the full screen by passing every grid cell to the callback.
    void render(Renderer const& _renderer) const;

    /// Renders a single text line.
    std::string renderTextLine(cursor_pos_t _row) const;

    /// Renders the full screen as text into the given string. Each line will be terminated by LF.
    std::string renderText() const;

    /// Takes a screenshot by outputting VT sequences needed to render the current state of the screen.
    ///
    /// @note Only the screenshot of the current buffer is taken, not both (main and alternate).
    ///
    /// @returns necessary commands needed to draw the current screen state,
    ///          including initial clear screen, and initial cursor hide.
    std::string screenshot() const;

    void operator()(Bell const& v);
    void operator()(FullReset const& v);
    void operator()(Linefeed const& v);
    void operator()(Backspace const& v);
    void operator()(DeviceStatusReport const& v);
    void operator()(ReportCursorPosition const& v);
    void operator()(ReportExtendedCursorPosition const& v);
    void operator()(SendDeviceAttributes const& v);
    void operator()(SendTerminalId const& v);
    void operator()(ClearToEndOfScreen const& v);
    void operator()(ClearToBeginOfScreen const& v);
    void operator()(ClearScreen const& v);
    void operator()(ClearScrollbackBuffer const& v);
    void operator()(EraseCharacters const& v);
    void operator()(ScrollUp const& v);
    void operator()(ScrollDown const& v);
    void operator()(ClearToEndOfLine const& v);
    void operator()(ClearToBeginOfLine const& v);
    void operator()(ClearLine const& v);
    void operator()(CursorNextLine const& v);
    void operator()(CursorPreviousLine const& v);
    void operator()(InsertLines const& v);
    void operator()(DeleteLines const& v);
    void operator()(DeleteCharacters const& v);
    void operator()(MoveCursorUp const& v);
    void operator()(MoveCursorDown const& v);
    void operator()(MoveCursorForward const& v);
    void operator()(MoveCursorBackward const& v);
    void operator()(MoveCursorToColumn const& v);
    void operator()(MoveCursorToBeginOfLine const& v);
    void operator()(MoveCursorTo const& v);
    void operator()(MoveCursorToLine const& v);
    void operator()(MoveCursorToNextTab const& v);
    void operator()(SaveCursor const& v);
    void operator()(RestoreCursor const& v);
    void operator()(Index const& v);
    void operator()(ReverseIndex const& v);
    void operator()(BackIndex const& v);
    void operator()(ForwardIndex const& v);
    void operator()(SetForegroundColor const& v);
    void operator()(SetBackgroundColor const& v);
    void operator()(SetGraphicsRendition const& v);
    void operator()(SetMode const& v);
    void operator()(SetTopBottomMargin const& v);
    void operator()(SetLeftRightMargin const& v);
    void operator()(ScreenAlignmentPattern const& v);
    void operator()(SendMouseEvents const& v);
    void operator()(AlternateKeypadMode const& v);
    void operator()(DesignateCharset const& v);
    void operator()(SingleShiftSelect const& v);
    void operator()(ChangeWindowTitle const& v);
    void operator()(ChangeIconName const& v);
    void operator()(AppendChar const& v);

    // reset screen
    void reset();
    void resetHard();

    WindowSize const& size() const noexcept { return size_; }
    void resize(WindowSize const& _newSize);

    bool isCursorInsideMargins() const noexcept {
        if (!state_->margin_.vertical.contains(state_->cursor.row))
            return false;
        else if (isModeEnabled(Mode::LeftRightMargin) && !state_->margin_.horizontal.contains(state_->cursor.column))
            return false;
        else
            return true;
    }

    Coordinate realCursorPosition() const noexcept { return state_->cursor; }

    Coordinate cursorPosition() const noexcept {
        if (!state_->cursorRestrictedToMargin)
            return realCursorPosition();
        else
            return Coordinate{
                state_->cursor.row - state_->margin_.vertical.from + 1,
                state_->cursor.column - state_->margin_.horizontal.from + 1
            };
    }

    Cursor const& realCursor() const noexcept
    {
        return state_->cursor;
    }

    Cell const& currentCell() const noexcept
    {
        return *state_->currentColumn;
    }

    Cell& currentCell() noexcept
    {
        return *state_->currentColumn;
    }

    Cell& currentCell(Cell value)
    {
        *state_->currentColumn = std::move(value);
        return *state_->currentColumn;
    }

    void moveCursorTo(Coordinate to);

    Cell const& at(cursor_pos_t row, cursor_pos_t col) const noexcept;
    Cell& at(cursor_pos_t row, cursor_pos_t col) noexcept;

    /// Retrieves the cell at given cursor, respecting origin mode.
    Cell& withOriginAt(cursor_pos_t row, cursor_pos_t col) { return state_->withOriginAt(row, col); }

    bool isPrimaryScreen() const noexcept { return state_ == &primaryBuffer_; }
    bool isAlternateScreen() const noexcept { return state_ == &alternateBuffer_; }

    bool isModeEnabled(Mode m) const noexcept { return enabledModes_.find(m) != enabledModes_.end(); }
    bool verticalMarginsEnabled() const noexcept { return isModeEnabled(Mode::CursorRestrictedToMargin); }
    bool horizontalMarginsEnabled() const noexcept { return isModeEnabled(Mode::LeftRightMargin); }

  private:
    void setMode(Mode mode, bool enable);

    // interactive replies
    void reply(std::string const& message)
    {
        if (reply_)
            reply_(message);
    }

    template <typename... Args>
    void reply(std::string const& fmt, Args&&... args)
    {
        reply(fmt::format(fmt, std::forward<Args>(args)...));
    }

    struct Range {
        unsigned int from;
        unsigned int to;

        constexpr unsigned int length() const noexcept { return to - from + 1; }
        constexpr bool operator==(Range const& rhs) const noexcept { return from == rhs.from && to == rhs.to; }
        constexpr bool operator!=(Range const& rhs) const noexcept { return !(*this == rhs); }

        constexpr bool contains(unsigned int _value) const noexcept { return from <= _value && _value <= to; }
    };

    struct Margin {
        Range vertical{}; // top-bottom
        Range horizontal{}; // left-right
    };

  private:
    struct Buffer {
        using Line = std::vector<Cell>;
        using Lines = std::list<Line>;

        // Savable states for DECSC & DECRC
        struct Save {
            Cursor cursor;
            GraphicsAttributes graphicsRendition{};
            bool blinking = false;
        };

        explicit Buffer(WindowSize const& _size)
            : size_{ _size },
              margin_{
                  {1, _size.rows},
                  {1, _size.columns}
              },
              lines{ _size.rows, Line{_size.columns, Cell{}} }
        {
            verifyState();
        }

        WindowSize size_;
        Margin margin_;
        Cursor cursor{};
        Lines lines;
        Lines savedLines{};
        bool autoWrap{false};
        bool wrapPending{false};
        bool cursorRestrictedToMargin{false};
        GraphicsAttributes graphicsRendition{};
        std::stack<Save> saveStack{};

        Lines::iterator currentLine{std::begin(lines)};
        Line::iterator currentColumn{std::begin(*currentLine)};

        void appendChar(char32_t ch);
        void linefeed();

        void resize(WindowSize const& _winSize);
        WindowSize const& size() const noexcept { return size_; }
        [[deprecated]] cursor_pos_t numLines() const noexcept { return size_.rows; }
        [[deprecated]] cursor_pos_t numColumns() const noexcept { return size_.columns; }

        void scrollUp(cursor_pos_t n);
        void scrollUp(cursor_pos_t n, Margin const& margin);
        void scrollDown(cursor_pos_t n);
        void scrollDown(cursor_pos_t n, Margin const& margin);

        void verifyState() const;
        void updateCursorIterators();

        constexpr Coordinate origin() const noexcept {
            if (cursorRestrictedToMargin)
                return {margin_.vertical.from, margin_.horizontal.from};
            else
                return {1, 1};
        }

        Cell& at(cursor_pos_t row, cursor_pos_t col);
        Cell const& at(cursor_pos_t row, cursor_pos_t col) const;

        /// Retrieves the cell at given cursor, respecting origin mode.
        Cell& withOriginAt(cursor_pos_t row, cursor_pos_t col);

		/// Returns identity if DECOM is disabled (default), but returns translated coordinates if DECOM is enabled.
		Coordinate toRealCoordinate(Coordinate const& pos) const noexcept
		{
			if (!cursorRestrictedToMargin)
				return pos;
			else
				return { pos.row + margin_.vertical.from - 1, pos.column + margin_.horizontal.from - 1 };
		}

		/// Clamps given coordinates, respecting DECOM (Origin Mode).
		Coordinate clampCoordinate(Coordinate const& to) const noexcept
		{
			if (!cursorRestrictedToMargin)
				return {
					std::clamp(to.row, cursor_pos_t{ 1 }, size_.rows),
					std::clamp(to.column, cursor_pos_t{ 1 }, size_.columns)
				};
			else
				return {
					std::clamp(to.row, cursor_pos_t{ 1 }, margin_.vertical.to),
					std::clamp(to.column, cursor_pos_t{ 1 }, margin_.horizontal.to)
				};
		}

		void moveCursorTo(Coordinate to);
    };

  public:
    Margin const& margin() const noexcept { return state_->margin_; }
    Buffer::Lines const& scrollbackLines() const noexcept { return state_->savedLines; }

    /**
     * Returns the n'th saved line into the history scrollback buffer.
     *
     * @param _lineNumberIntoHistory the 1-based offset into the history buffer.
     *
     * @returns the textual representation of the n'th line into the history.
     */
    std::string renderHistoryTextLine(cursor_pos_t _lineNumberIntoHistory) const;

  private:
    Hook const onCommands_;
    Logger const logger_;
    ModeSwitchCallback useApplicationCursorKeys_;
    Reply const reply_;

    OutputHandler handler_;
    Parser parser_;

    Buffer primaryBuffer_;
    Buffer alternateBuffer_;
    Buffer* state_;

    std::set<Mode> enabledModes_{};

    WindowSize size_;
};

constexpr bool operator==(Screen::GraphicsAttributes const& a, Screen::GraphicsAttributes const& b) noexcept
{
    return a.backgroundColor == b.backgroundColor
        && a.foregroundColor == b.foregroundColor
        && a.styles == b.styles;
}

constexpr bool operator==(Screen::Cell const& a, Screen::Cell const& b) noexcept
{
    return a.character == b.character && a.attributes == b.attributes;
}

}  // namespace terminal
