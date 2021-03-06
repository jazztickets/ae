/******************************************************************************
* Copyright (c) 2021 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/console.h>
#include <ae/assets.h>
#include <ae/ui.h>
#include <ae/font.h>
#include <ae/graphics.h>
#include <ae/util.h>
#include <SDL_scancode.h>

namespace ae {

const float PADDING_X = 5.0f;
const float PADDING_Y = 2.0f;
const float SPACING_Y = 2.0f;
const glm::vec4 CONSOLE_BG_COLOR = glm::vec4(0.0f, 0.0f, 0.0f, 0.95f);
const glm::vec4 TEXTBOX_BG_COLOR = glm::vec4(0.05f, 0.05f, 0.05f, 0.5f);

// Initialize
_Console::_Console(const _Program *Program, const _Font *Font) :
	Element(nullptr),
	Font(Font) {

	// Add style
	Style = new _Style();
	Style->Program = Program;
	Style->HasBackgroundColor = true;
	Style->BackgroundColor = CONSOLE_BG_COLOR;

	// Add textbox style
	InputStyle = new _Style();
	InputStyle->Program = Program;
	InputStyle->HasBackgroundColor = true;
	InputStyle->BackgroundColor = TEXTBOX_BG_COLOR;

	// Add background element
	Element = new _Element();
	Element->Parent = Graphics.Element;
	Element->Style = Style;
	Element->Alignment = LEFT_TOP;
	Element->BaseSize = glm::vec2(100, 50);
	Element->SizePercent[0] = true;
	Element->SizePercent[1] = true;
	Graphics.Element->Children.push_back(Element);

	// Add textbox background element
	_Element *TextboxBackgroundElement = new _Element();
	TextboxBackgroundElement->Parent = Element;
	TextboxBackgroundElement->Style = InputStyle;
	TextboxBackgroundElement->Alignment = LEFT_BOTTOM;
	Element->Children.push_back(TextboxBackgroundElement);

	// Add textbox element
	TextboxElement = new _Element();
	TextboxElement->Parent = TextboxBackgroundElement;
	TextboxElement->Alignment = LEFT_BASELINE;
	TextboxElement->MaxLength = 255;
	TextboxElement->Font = Font;
	TextboxElement->Text = "";
	TextboxBackgroundElement->Children.push_back(TextboxElement);

	// Update size of main element
	UpdateSize();
}

// Destructor
_Console::~_Console() {
	HistoryFile.close();
	delete Style;
	delete InputStyle;
}

// Load history file
void _Console::LoadHistory(const std::string &Path) {
	HistoryFile.open(Path, std::ios::in | std::ios::app);
	if(!HistoryFile.is_open())
		return;

	// Prepopulate command history
	std::string Line;
	while(std::getline(HistoryFile, Line))
		CommandHistory.push_back(Line);

	HistoryFile.clear();
	CommandHistoryIterator = CommandHistory.end();
}

// Update size of console based on parent element
void _Console::UpdateSize() {

	// Update element
	Element->CalculateBounds();

	// Update textbox bg size
	Element->Children.front()->Size.x = Element->Parent->Size.x;
	Element->Children.front()->Size.y = Font->MaxAbove + Font->MaxBelow + PADDING_Y;
	Element->Children.front()->CalculateBounds(false);

	// Update textbox offset
	Element->Children.front()->Children.front()->Offset = glm::vec2(PADDING_X, Font->MaxAbove + PADDING_Y / 2);
	Element->Children.front()->Children.front()->CalculateBounds(false);
}

// Update
void _Console::Update(double FrameTime) {
	if(Element->Active)
		ae::FocusedElement = TextboxElement;

	// Handle commands
	switch(TextboxElement->LastKeyPressed) {
		case SDL_SCANCODE_KP_ENTER:
		case SDL_SCANCODE_RETURN: {
			if(!TextboxElement->Text.empty()) {

				// Separate command from parameters
				std::size_t SpaceIndex = TextboxElement->Text.find_first_of(' ');
				Command = TextboxElement->Text.substr(0, SpaceIndex);

				// Handle parameters
				Parameters = "";
				if(SpaceIndex != std::string::npos)
					Parameters = TrimString(TextboxElement->Text.substr(SpaceIndex + 1));

				// Add command to history if not a repeat
				bool Repeat = CommandHistory.size() > 0 && CommandHistory.back() == TextboxElement->Text;
				if(CommandHistory.size() == 0 || !Repeat)
					CommandHistory.push_back(TextboxElement->Text);

				CommandHistoryIterator = CommandHistory.end();

				// Add to console
				AddMessage(TextboxElement->Text, !Repeat, glm::vec4(1.0f));
			}

			TextboxElement->Text = "";
		} break;
		case SDL_SCANCODE_TAB: {

			// Compare input with all commands
			std::list<std::string> PossibleCommands;
			std::size_t CompareLength = TextboxElement->Text.length();
			for(const auto &Token : CommandList) {
				if(Token.substr(0, CompareLength) == TextboxElement->Text)
					PossibleCommands.push_back(Token);
			}

			// Check for one possible match
			if(PossibleCommands.size() == 1) {
				TextboxElement->Text = PossibleCommands.front() + " ";
				TextboxElement->CursorPosition = TextboxElement->Text.length();
			}
			// Autocomplete rest of matching characters
			else if(PossibleCommands.size()) {
				bool Done = false;
				while(!Done) {

					// Compare character for each token
					char LastChar = 0;
					for(const auto &Token : PossibleCommands) {
						if(LastChar == 0) {
							LastChar = Token[CompareLength];
						}
						else if(LastChar != Token[CompareLength]) {
							Done = true;
							break;
						}
					}

					// Add last character to command
					if(!Done) {
						TextboxElement->Text += LastChar;
						TextboxElement->CursorPosition++;
					}

					CompareLength++;
				}

				// Display available commands;
				AddMessage("");
				for(const auto &Token : PossibleCommands)
					AddMessage(Token);
			}

			TextboxElement->ResetCursor();
		} break;
		case SDL_SCANCODE_ESCAPE: {
			Toggle();
		} break;
		case SDL_SCANCODE_UP: {
			UpdateHistory(-1);
		} break;
		case SDL_SCANCODE_DOWN: {
			UpdateHistory(1);
		} break;
	}

	TextboxElement->LastKeyPressed = SDL_SCANCODE_UNKNOWN;
}

// Render
void _Console::Render(double BlendFactor) {
	if(!IsOpen())
		return;

	// Draw background
	Element->Render();

	// Draw messages
	glm::vec2 DrawPosition(PADDING_X * ae::_Element::GetUIScale(), Element->Bounds.End.y - TextboxElement->Parent->Size.y - Font->MaxBelow - SPACING_Y);
	for(auto Iterator = Messages.rbegin(); Iterator != Messages.rend(); ++Iterator) {
		_Message &Message = (*Iterator);
		Font->DrawText(Message.Text, DrawPosition, LEFT_BASELINE, Message.Color);

		DrawPosition.y -= Font->MaxAbove + Font->MaxBelow + SPACING_Y;
		if(DrawPosition.y < 0)
			break;
	}
}

// Return if true if console is open
bool _Console::IsOpen() {
	return Element->Active;
}

// Toggle display of console
void _Console::Toggle() {
	Element->SetActive(!Element->Active);
	TextboxElement->ResetCursor();

	if(!Element->Active) {
		ae::FocusedElement = nullptr;
		TextboxElement->Text = "";
	}
}

// Add message to console
void _Console::AddMessage(const std::string &Text, bool Log, const glm::vec4 &Color) {

	// Append to messages
	_Message Message;
	Message.Text = Text;
	Message.Color = Color;
	Messages.push_back(Message);

	// Log to file
	if(Log && HistoryFile.is_open())
		HistoryFile << Text << std::endl;
}

// Set command textbox string based on history
void _Console::UpdateHistory(int Direction) {
	TextboxElement->ResetCursor();
	if(CommandHistory.size() == 0)
		return;

	if(Direction < 0 && CommandHistoryIterator != CommandHistory.begin()) {
		CommandHistoryIterator--;

		TextboxElement->Text = *CommandHistoryIterator;
		TextboxElement->CursorPosition = TextboxElement->Text.length();
	}
	else if(Direction > 0 && CommandHistoryIterator != CommandHistory.end()) {

		CommandHistoryIterator++;
		if(CommandHistoryIterator == CommandHistory.end()) {
			TextboxElement->Text = "";
			TextboxElement->CursorPosition = 0;
			return;
		}

		TextboxElement->Text = *CommandHistoryIterator;
		TextboxElement->CursorPosition = TextboxElement->Text.length();
	}
}

}
