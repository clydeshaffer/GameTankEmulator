// Improved ComboAutoSelect and ComboFilter from contributions of people in https://github.com/ocornut/imgui/issues/1658

// Changes:
//  - Templated ComboAutoSelect function instead of void*
//  - Incompatible container and getter callback can be found in compile time
//  - A callback for string search can be user-defined/user-implemented but a default algorithm is provided using fuzzy search algorithm from https://github.com/forrestthewoods/lib_fts/blob/master/code/fts_fuzzy_match.h
//  - An overload is made using the default string search algorithm if you don't need/want to implement your own fuzzy search algorithm
//  - Improved scrolling interaction
//  - Improved widget interaction
//  - Improved widget algorithm
//  - Now uses ImGuiListClipper so long lists will not impact performance
//  - C++17 standard
// 
// V2 CombAutoSelect/ComboFilter
//  - V2 aims to simplify the API by using an internal storage using a hash_table to store combo data
//  - However, the API for querying the combo data is still quite bad as of now, but I will try to improve it

#pragma once

#include <type_traits> // For std::enable_if to minimize template error dumps
#include <vector>
#include "imgui.h"
#include "imgui_internal.h"

//----------------------------------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------------------------------------------------

namespace ImGui
{

struct ComboAutoSelectData;
struct ComboFilterData;
struct ComboFilterSearchResultData;

template<typename T1>
struct ComboAutoSelectSearchCallbackData;
template<typename T>
struct ComboFilterSearchCallbackData;

using ComboFilterSearchResults = std::vector<ComboFilterSearchResultData>;

// Callback for container of your choice
// Index can be negative or out of range so you can customize the return value for invalid index
template<typename T>
using ComboItemGetterCallback = const char* (*)(T items, int index);

// ComboAutoSelect search callback
// Creating the callback can be templated (recommended) or made for a specific container type
// The callback should return the index of an item choosen by the fuzzy search algorithm. Return -1 for failure.
template<typename T>
using ComboAutoSelectSearchCallback = int (*)(const ComboAutoSelectSearchCallbackData<T>& callback_data);

// ComboFilter search callback
// The callback for filtering out a list of items depending on the input string
// The output 'out_items' will always start as empty everytime the function is called
// The template type should have the same type as the template type of ItemGetterCallback
template<typename T>
using ComboFilterSearchCallback = void (*)(const ComboFilterSearchCallbackData<T>& callback_data);

// ComboData related queries
// Lookup requires the combo_id gotten from hashing the combo_name/label
// Alternatively, if you know the window the combo is in, you can input the window_name and combo_name
// Or... just the combo_name if you're querying it on the same window the combo is in
void ClearComboData(const char* window_label, const char* combo_label);
void ClearComboData(const char* combo_label);
void ClearComboData(ImGuiID combo_id);

void SortFilterResultsDescending(ComboFilterSearchResults& filtered_items);
void SortFilterResultsAscending(ComboFilterSearchResults& filtered_items);

// Combo box with text filter
// T1 should be a container.
// T2 can be, but not necessarily, the same as T1 but it should be convertible from T1 (e.g. std::vector<...> -> std::span<...>)
// It should be noted that because T1 is a const reference, T2 should be correctly const
// Template deduction should work so no need for typing out the types when using the function (C++17 or later)
// To work with c-style arrays, you might need to use std::span<...>, or make your own wrapper if not applicable, for T2 to query for its size inside ItemGetterCallback
template<typename T1, typename T2, typename = std::enable_if<std::is_convertible<T1, T2>::value>::type>
bool ComboAutoSelect(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ComboAutoSelectSearchCallback<T2> autoselect_callback, ImGuiComboFlags flags = ImGuiComboFlags_None);
template<typename T1, typename T2, typename = std::enable_if<std::is_convertible<T1, T2>::value>::type>
bool ComboAutoSelect(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ImGuiComboFlags flags = ImGuiComboFlags_None);
template<typename T1, typename T2, typename = std::enable_if<std::is_convertible<T1, T2>::value>::type>
bool ComboFilter(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ComboFilterSearchCallback<T2> filter_callback, ImGuiComboFlags flags = ImGuiComboFlags_None);
template<typename T1, typename T2, typename = std::enable_if<std::is_convertible<T1, T2>::value>::type>
bool ComboFilter(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ImGuiComboFlags flags = ImGuiComboFlags_None);

namespace Internal
{

struct ComboData;

template<class T>
T* AddComboData(const char* window_label , const char* combo_label);
template<class T>
T* AddComboData(const char* combo_label);
template<class T>
T* AddComboData(ImGuiID combo_id);
template<class T>
T* GetComboData(const char* window_label, const char* combo_label);
template<class T>
T* GetComboData(const char* combo_label);
template<class T>
T* GetComboData(ImGuiID combo_id);

// ImGui helpers for most widget needs
float CalcComboItemHeight(int item_count, float offset_multiplier = 1.0f);
void SetScrollToComboItemJump(ImGuiWindow* listbox_window, int index);
void SetScrollToComboItemUp(ImGuiWindow* listbox_window, int index);
void SetScrollToComboItemDown(ImGuiWindow* listbox_window, int index);
void UpdateInputTextAndCursor(char* buf, int buf_capacity, const char* new_str);

// Created my own std::size and std::empty implementation to avoid additional header dependency
template<typename T>
constexpr auto GetContainerSize(const T& item);
template<typename T, unsigned long long N>
constexpr unsigned long long GetContainerSize(const T(&array)[N]) noexcept;
template<typename T>
constexpr bool IsContainerEmpty(const T& item);
template<typename T, unsigned long long N>
constexpr bool IsContainerEmpty(const T(&array)[N]) noexcept;

bool FuzzySearchEX(char const* pattern, char const* src, int& out_score);
bool FuzzySearchEX(char const* pattern, char const* haystack, int& out_score, unsigned char matches[], int maxMatches, int& outMatches);

template<typename T>
int DefaultComboAutoSelectSearchCallback(const ComboAutoSelectSearchCallbackData<T>& callback_data);
template<typename T>
void DefaultComboFilterSearchCallback(const ComboFilterSearchCallbackData<T>& callback_data);

template<typename T1, typename T2, typename = std::enable_if<std::is_convertible<T1, T2>::value>::type>
bool ComboAutoSelectEX(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ComboAutoSelectSearchCallback<T2> autoselect_callback, ImGuiComboFlags flags);
template<typename T1, typename T2, typename = std::enable_if<std::is_convertible<T1, T2>::value>::type>
bool ComboFilterEX(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ComboFilterSearchCallback<T2> filter_callback, ImGuiComboFlags flags);

} // Internal namespace
} // ImGui namespace

//----------------------------------------------------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------------------------------------------------

namespace ImGui
{

namespace Internal
{

// Base ComboData struct
// Only meant for destructing polymorphically but not used polymorphically everywhere
struct ComboData
{
	static constexpr int StringCapacity = 128;
	char InputText[StringCapacity + 1]{ 0 };
	struct
	{
		const char* Preview;
		int         Index;
	} InitialValues{ "", -1 };
	int CurrentSelection{ -1 };

	virtual ~ComboData() = default;
};

}

struct ComboAutoSelectData : Internal::ComboData
{
	bool SetNewValue(const char* new_val, int new_index) noexcept;
	bool SetNewValue(const char* new_val) noexcept;
	void ResetToInitialValue() noexcept;
	void Reset() noexcept;
};

struct ComboFilterData : Internal::ComboData
{
	ComboFilterSearchResults FilteredItems;
	bool FilterStatus{ false };

	bool SetNewValue(const char* new_val, int new_index) noexcept;
	bool SetNewValue(const char* new_val) noexcept;
	void ResetToInitialValue() noexcept;
	void ResetAll() noexcept;
};

// Result data from a search algorithm
// Contains the index of the item from list and the score of the item
struct ComboFilterSearchResultData
{
	int Index;
	int Score;

	bool operator < (const ComboFilterSearchResultData& other) const noexcept
	{
		return this->Score < other.Score;
	}
};

template<typename T>
struct ComboAutoSelectSearchCallbackData
{
	T                          Items;         // Read-only
	const char*                SearchString;  // Read-only
	ComboItemGetterCallback<T> ItemGetter;    // Read-only
};

template<typename T>
struct ComboFilterSearchCallbackData
{
	T                          Items;          // Read-only
	const char*                SearchString;   // Read-only
	ComboItemGetterCallback<T> ItemGetter;     // Read-only
	ComboFilterSearchResults*  FilterResults;  // Output value
};

template<typename T1, typename T2, typename>
bool ComboAutoSelect(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ComboAutoSelectSearchCallback<T2> autoselect_callback, ImGuiComboFlags flags)
{
	ImGui::BeginDisabled(Internal::IsContainerEmpty(items));
	bool ret = Internal::ComboAutoSelectEX(combo_label, selected_item, items, item_getter, autoselect_callback, flags);
	ImGui::EndDisabled();

	return ret;
}

template<typename T1, typename T2, typename>
bool ComboAutoSelect(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ImGuiComboFlags flags)
{
	return ComboAutoSelect(combo_label, selected_item, items, item_getter, Internal::DefaultComboAutoSelectSearchCallback, flags);
}

template<typename T1, typename T2, typename>
bool ComboFilter(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ComboFilterSearchCallback<T2> filter_callback, ImGuiComboFlags flags)
{
	ImGui::BeginDisabled(Internal::IsContainerEmpty(items));
	auto ret = Internal::ComboFilterEX(combo_label, selected_item, items, item_getter, filter_callback, flags);
	ImGui::EndDisabled();

	return ret;
}

template<typename T1, typename T2, typename>
bool ComboFilter(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ImGuiComboFlags flags)
{
	return ComboFilter(combo_label, selected_item, items, item_getter, Internal::DefaultComboFilterSearchCallback, flags);
}

namespace Internal
{

template<typename T> 
constexpr auto GetContainerSize(const T& item)
{
    return item.size();
}

template<typename T, unsigned long long N>
constexpr unsigned long long GetContainerSize(const T(&array)[N]) noexcept
{
	return N;
}

template<typename T>
constexpr bool IsContainerEmpty(const T& item)
{
	return item.size() == 0;
}

template<typename T, unsigned long long N>
constexpr bool IsContainerEmpty(const T(&array)[N]) noexcept
{
	return false;
}

template<typename T>
int DefaultComboAutoSelectSearchCallback(const ComboAutoSelectSearchCallbackData<T>& callback_data)
{
	if (callback_data.SearchString[0] == '\0')
		return -1;

	const int item_count = static_cast<int>(Internal::GetContainerSize(callback_data.Items));
	constexpr int max_matches = 128;
	unsigned char matches[max_matches];
	int best_item = -1;
	int prevmatch_count;
	int match_count;
	int best_score;
	int score;
	int i = 0;

	for (; i < item_count; ++i) {
		if (FuzzySearchEX(callback_data.SearchString, callback_data.ItemGetter(callback_data.Items, i), score, matches, max_matches, match_count)) {
			prevmatch_count = match_count;
			best_score = score;
			best_item = i;
			break;
		}
	}
	for (; i < item_count; ++i) {
		if (FuzzySearchEX(callback_data.SearchString, callback_data.ItemGetter(callback_data.Items, i), score, matches, max_matches, match_count)) {
			if ((score > best_score && prevmatch_count >= match_count) || (score == best_score && match_count > prevmatch_count)) {
				prevmatch_count = match_count;
				best_score = score;
				best_item = i;
			}
		}
	}

	return best_item;
}

template<typename T>
void DefaultComboFilterSearchCallback(const ComboFilterSearchCallbackData<T>& callback_data)
{
	const int item_count = static_cast<int>(GetContainerSize(callback_data.Items));
	constexpr int max_matches = 128;
	unsigned char matches[max_matches];
	int best_item = -1;
	int match_count;
	int score = 0;

	for (int i = 0; i < item_count; ++i) {
		if (FuzzySearchEX(callback_data.SearchString, callback_data.ItemGetter(callback_data.Items, i), score, matches, max_matches, match_count)) {
			callback_data.FilterResults->emplace_back(i, score);
		}
	}

	SortFilterResultsDescending(*callback_data.FilterResults);
}

template<typename T1, typename T2, typename>
bool ComboAutoSelectEX(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ComboAutoSelectSearchCallback<T2> autoselect_callback, ImGuiComboFlags flags)
{
	// Always consume the SetNextWindowSizeConstraint() call in our early return paths
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow();

	ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
	g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
	if (window->SkipItems)
		return false;

	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const ImGuiStyle& style = g.Style;
	const ImGuiID combo_id = window->GetID(combo_label);

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
	const ImVec2 label_size = CalcTextSize(combo_label, NULL, true);
	const float expected_w = CalcItemWidth();
	const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : CalcItemWidth();
	const ImVec2 bb_max(window->DC.CursorPos.x + w, window->DC.CursorPos.y + (label_size.y + style.FramePadding.y * 2.0f));
	const ImRect bb(window->DC.CursorPos, bb_max);
	const ImVec2 total_bb_max(bb.Max.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), bb.Max.y);
	const ImRect total_bb(bb.Min, total_bb_max);
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, combo_id, &bb))
		return false;

	ComboAutoSelectData* combo_data = GetComboData<ComboAutoSelectData>(combo_id);
	if (!combo_data) {
		combo_data = AddComboData<ComboAutoSelectData>(combo_id);
	}

	// Open on click
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, combo_id, &hovered, &held);
	bool popupIsAlreadyOpened = IsPopupOpen(combo_id, ImGuiPopupFlags_None);
	bool popupJustOpened = false;

	const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
	if (!popupIsAlreadyOpened) {
		if (pressed) {
			OpenPopupEx(combo_id);
			popupIsAlreadyOpened = true;
			popupJustOpened = true;
		}
		const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
		RenderNavHighlight(bb, combo_id);
		if (!(flags & ImGuiComboFlags_NoPreview))
			window->DrawList->AddRectFilled(bb.Min, ImVec2(value_x2, bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
	}
	if (!(flags & ImGuiComboFlags_NoArrowButton)) {
		ImU32 bg_col = GetColorU32((popupIsAlreadyOpened || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		ImU32 text_col = GetColorU32(ImGuiCol_Text);
		window->DrawList->AddRectFilled(ImVec2(value_x2, bb.Min.y), bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
		if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
			RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y), text_col, popupIsAlreadyOpened ? ImGuiDir_Up : ImGuiDir_Down, 1.0f);
	}

	if (!popupIsAlreadyOpened) {
		RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);
		if (combo_data->InitialValues.Preview != NULL && !(flags & ImGuiComboFlags_NoPreview)) {
			RenderTextClipped(
				ImVec2(bb.Min.x + style.FramePadding.x, bb.Min.y + style.FramePadding.y),
				ImVec2(value_x2, bb.Max.y),
				combo_data->InitialValues.Preview,
				NULL,
				NULL,
				ImVec2(0.0f, 0.0f)
			);
		}
	}

	if (label_size.x > 0)
		RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), combo_label);
	if (!popupIsAlreadyOpened)
		return false;

	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3.50f, 5.00f));
	const float popup_width = flags & (ImGuiComboFlags_NoPreview | ImGuiComboFlags_NoArrowButton) ? expected_w : w - arrow_size;
	int popup_item_count = -1;
	if (!(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)) {
		if ((flags & ImGuiComboFlags_HeightMask_) == 0)
			flags |= ImGuiComboFlags_HeightRegular;
		IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_)); // Only one
		if (flags & ImGuiComboFlags_HeightRegular)    popup_item_count = 8 + 1;
		else if (flags & ImGuiComboFlags_HeightSmall) popup_item_count = 4 + 1;
		else if (flags & ImGuiComboFlags_HeightLarge) popup_item_count = 20 + 1;
		SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(popup_width, CalcComboItemHeight(popup_item_count, 4.00f)));
	}

	char name[16];
	ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

	// Peek into expected window size so we can position it
	if (ImGuiWindow* popup_window = FindWindowByName(name)) {
		if (popup_window->WasActive) {
			// Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
			ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
			popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ? ImGuiDir_Left : ImGuiDir_Down; // Left = "Below, Toward Left", Down = "Below, Toward Right (default)"
			ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
			ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
			const float ypos_offset = flags & ImGuiComboFlags_NoPreview ? 0.0f : label_size.y + (style.FramePadding.y * 2.0f);
			if (pos.y < bb.Min.y)
				pos.y += ypos_offset;
			else
				pos.y -= ypos_offset;
			if (pos.x > bb.Min.x)
				pos.x += bb.Max.x;

			SetNextWindowPos(pos);
		}
	}

	// Horizontally align ourselves with the framed text
	constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
	if (!Begin(name, NULL, window_flags)) {
		PopStyleVar();
		EndPopup();
		IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
		return false;
	}

	if (popupJustOpened) {
		SetKeyboardFocusHere(0);
	}

	const float items_max_width = popup_width - style.WindowPadding.x * 2.0f;
	SetCursorPos(ImVec2(style.WindowPadding.x, window->DC.CurrLineTextBaseOffset));
	PushItemWidth(items_max_width);
	PushStyleVar(ImGuiStyleVar_FrameRounding, 2.50f);
	PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(240, 240, 240, 255));
	PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0, 255));
	const bool buffer_changed = InputTextEx("##inputText", NULL, combo_data->InputText, ComboData::StringCapacity, ImVec2(0, 0), ImGuiInputTextFlags_AutoSelectAll, NULL, NULL);
	PopStyleColor(2);
	PopStyleVar(1);
	PopItemWidth();

	const bool clicked_outside = !IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AnyWindow) && IsMouseClicked(0);
	bool selection_changed = false;

	char listbox_name[16];
	ImFormatString(listbox_name, 16, "##lbn%u", combo_id); // Create different listbox name/id per combo so scroll position does not persist on every combo
	const int items_count = static_cast<int>(GetContainerSize(items));
	if (--popup_item_count > items_count || popup_item_count < 0)
		popup_item_count = items_count;
	if (BeginListBox(listbox_name, ImVec2(items_max_width, CalcComboItemHeight(popup_item_count, 1.25f)))) {
		ImGuiWindow* listbox_window = ImGui::GetCurrentWindow();
		listbox_window->Flags |= ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus;

		ImGuiListClipper list_clipper;
		list_clipper.Begin(items_count);
		char select_item_id[128];
		while (list_clipper.Step()) {
			for (int n = list_clipper.DisplayStart; n < list_clipper.DisplayEnd; n++) {
				bool is_selected = n == combo_data->CurrentSelection;
				const char* select_value = item_getter(items, n);

				// allow empty item / in case of duplicate item name on different index
				ImFormatString(select_item_id, sizeof(select_item_id), "%s##item_%02d", select_value, n);
				if (Selectable(select_item_id, is_selected)) {
					if (combo_data->SetNewValue(select_value, n)) {
						selection_changed = true;
						SetScrollToComboItemJump(listbox_window, n);
						selected_item = combo_data->CurrentSelection;
					}
					CloseCurrentPopup();
				}
			}
		}

		if (clicked_outside || IsKeyPressed(ImGuiKey_Escape)) { // Resets the selection to it's initial value if the user exits the combo (clicking outside the combo or the combo arrow button)
			if (combo_data->CurrentSelection != combo_data->InitialValues.Index)
				combo_data->ResetToInitialValue();
			if (combo_data->InitialValues.Index < 0)
				SetScrollY(0.0f);
			else
				SetScrollToComboItemJump(listbox_window, combo_data->InitialValues.Index);
			CloseCurrentPopup();
		}
		else if (buffer_changed) {
			combo_data->CurrentSelection = autoselect_callback({ items, combo_data->InputText, item_getter });
			if (combo_data->CurrentSelection < 0)
				SetScrollY(0.0f);
			else
				SetScrollToComboItemJump(listbox_window, combo_data->CurrentSelection);
		}
		else if (IsKeyPressed(ImGuiKey_Enter) || IsKeyPressed(ImGuiKey_KeypadEnter)) { // Automatically exit the combo popup on selection
			if (combo_data->SetNewValue(item_getter(items, combo_data->CurrentSelection))) {
				selection_changed = true;
				SetScrollToComboItemJump(listbox_window, combo_data->CurrentSelection);
				selected_item = combo_data->CurrentSelection;
			}
			CloseCurrentPopup();
		}

		if (IsKeyPressed(ImGuiKey_UpArrow)) {
			if (combo_data->CurrentSelection > 0)
			{
				SetScrollToComboItemUp(listbox_window, --combo_data->CurrentSelection);
				UpdateInputTextAndCursor(combo_data->InputText, ComboData::StringCapacity, item_getter(items, combo_data->CurrentSelection));
			}
		}
		else if (IsKeyPressed(ImGuiKey_DownArrow)) {
			if (combo_data->CurrentSelection >= -1 && combo_data->CurrentSelection < items_count - 1)
			{
				SetScrollToComboItemDown(listbox_window, ++combo_data->CurrentSelection);
				UpdateInputTextAndCursor(combo_data->InputText, ComboData::StringCapacity, item_getter(items, combo_data->CurrentSelection));
			}
		}

		EndListBox();
	}
	EndPopup();
	PopStyleVar();

	return selection_changed;
}

template<typename T1, typename T2, typename>
bool ComboFilterEX(const char* combo_label, int& selected_item, const T1& items, ComboItemGetterCallback<T2> item_getter, ComboFilterSearchCallback<T2> filter_callback, ImGuiComboFlags flags)
{
	ImGuiContext* g = GImGui;
	ImGuiWindow* window = GetCurrentWindow();

	ImGuiNextWindowDataFlags backup_next_window_data_flags = g->NextWindowData.Flags;
	g->NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
	if (window->SkipItems)
		return false;

	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const ImGuiStyle& style = g->Style;
	const ImGuiID combo_id = window->GetID(combo_label);

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
	const ImVec2 label_size = CalcTextSize(combo_label, NULL, true);
	const float expected_w = CalcItemWidth();
	const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : CalcItemWidth();
	const ImVec2 bb_max(window->DC.CursorPos.x + w, window->DC.CursorPos.y + (label_size.y + style.FramePadding.y * 2.0f));
	const ImRect bb(window->DC.CursorPos, bb_max);
	const ImVec2 total_bb_max(bb.Max.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), bb.Max.y);
	const ImRect total_bb(bb.Min, total_bb_max);
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, combo_id, &bb))
		return false;

	ComboFilterData* combo_data = GetComboData<ComboFilterData>(combo_id);
	if (!combo_data) {
		combo_data = AddComboData<ComboFilterData>(combo_id);
		combo_data->FilteredItems.reserve(GetContainerSize(items) / 2);
	}

	// Open on click
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, combo_id, &hovered, &held);
	bool popup_open = IsPopupOpen(combo_id, ImGuiPopupFlags_None);
	bool popup_just_opened = false;
	if (pressed && !popup_open)
	{
		OpenPopupEx(combo_id, ImGuiPopupFlags_None);
		popup_open = true;
		popup_just_opened = true;
	}

	// Render shape
	const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
	RenderNavHighlight(bb, combo_id);
	if (!(flags & ImGuiComboFlags_NoPreview))
		window->DrawList->AddRectFilled(bb.Min, ImVec2(value_x2, bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
	if (!(flags & ImGuiComboFlags_NoArrowButton))
	{
		ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		ImU32 text_col = GetColorU32(ImGuiCol_Text);
		window->DrawList->AddRectFilled(ImVec2(value_x2, bb.Min.y), bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
		if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
			RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y), text_col, popup_open ? ImGuiDir_Up : ImGuiDir_Down, 1.0f);
	}
	RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

	// Render preview and label
	if (combo_data->InitialValues.Preview != NULL && !(flags & ImGuiComboFlags_NoPreview)) {
		const ImVec2 min_pos(bb.Min.x + style.FramePadding.x, bb.Min.y + style.FramePadding.y);
		RenderTextClipped(min_pos, ImVec2(value_x2, bb.Max.y), combo_data->InitialValues.Preview, NULL, NULL);
	}
	if (label_size.x > 0)
		RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), combo_label);
	if (!popup_open)
		return false;

	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3.50f, 5.00f));
	int popup_item_count = -1;
	if (!(g->NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)) {
		if ((flags & ImGuiComboFlags_HeightMask_) == 0)
			flags |= ImGuiComboFlags_HeightRegular;
		IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_)); // Only one
		if (flags & ImGuiComboFlags_HeightRegular) popup_item_count = 8 + 1;
		else if (flags & ImGuiComboFlags_HeightSmall) popup_item_count = 4 + 1;
		else if (flags & ImGuiComboFlags_HeightLarge) popup_item_count = 20 + 1;
		const float popup_height = CalcComboItemHeight(popup_item_count, 5.0f); // Increment popup_item_count to account for the InputText widget
		SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(expected_w, popup_height));
	}

	char name[16];
	ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g->BeginPopupStack.Size); // Recycle windows based on depth

	if (ImGuiWindow* popup_window = FindWindowByName(name)) {
		if (popup_window->WasActive)
		{
			// Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
			ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
			popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ? ImGuiDir_Left : ImGuiDir_Down; // Left = "Below, Toward Left", Down = "Below, Toward Right (default)"
			ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
			ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
			SetNextWindowPos(pos);
		}
	}

	constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
	if (!Begin(name, NULL, window_flags)) {
		PopStyleVar();
		EndPopup();
		IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
		return false;
	}

	if (popup_just_opened) {
		SetKeyboardFocusHere();
	}

	const float items_max_width = expected_w - (style.WindowPadding.x * 2.00f);
	PushItemWidth(items_max_width);
	PushStyleVar(ImGuiStyleVar_FrameRounding, 5.00f);
	PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(240, 240, 240, 255));
	PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0, 255));
	const bool buffer_changed = InputTextEx("##inputText", NULL, combo_data->InputText, ComboData::StringCapacity, ImVec2(0, 0), ImGuiInputTextFlags_AutoSelectAll, NULL, NULL);
	PopStyleColor(2);
	PopStyleVar(1);
	PopItemWidth();

	bool selection_changed     = false;
	const bool clicked_outside = !IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AnyWindow) && IsMouseClicked(0);

	auto item_getter2 = [&](int index) -> const char* {
		return item_getter(items, combo_data->FilterStatus ? combo_data->FilteredItems[index].Index : index);
	};

	const int item_count = static_cast<int>(combo_data->FilterStatus ? GetContainerSize(combo_data->FilteredItems) : GetContainerSize(items));
	char listbox_name[16];
	ImFormatString(listbox_name, 16, "##lbn%u", combo_id);
	if (--popup_item_count > item_count || popup_item_count < 0)
		popup_item_count = item_count;
	if (BeginListBox(listbox_name, ImVec2(items_max_width, CalcComboItemHeight(popup_item_count, 1.50f)))) {
		ImGuiWindow* listbox_window = ImGui::GetCurrentWindow();
		listbox_window->Flags |= ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus;

		if (listbox_window->Appearing)
			SetScrollToComboItemJump(listbox_window, combo_data->InitialValues.Index);

		ImGuiListClipper listclipper;
		listclipper.Begin(item_count);
		char select_item_id[128];
		while (listclipper.Step()) {
			for (int i = listclipper.DisplayStart; i < listclipper.DisplayEnd; ++i) {
				bool is_selected = i == combo_data->CurrentSelection;
				const char* select_value = item_getter2(i);

				ImFormatString(select_item_id, 128, "%s##id%d", select_value, i);
				if (Selectable(select_item_id, is_selected)) {
					if (combo_data->SetNewValue(select_value, i)) {
						selection_changed = true;
						selected_item = combo_data->CurrentSelection;
					}
					CloseCurrentPopup();
				}
			}
		}

		if (clicked_outside || IsKeyPressed(ImGuiKey_Escape)) {
			combo_data->ResetToInitialValue();
			CloseCurrentPopup();
		}
		else if (buffer_changed) {
			combo_data->FilteredItems.clear();
			if (combo_data->FilterStatus = combo_data->InputText[0] != '\0')
				filter_callback({ items, combo_data->InputText, item_getter, &combo_data->FilteredItems });
			combo_data->CurrentSelection = GetContainerSize(combo_data->FilteredItems) != 0 ? 0 : -1;
			SetScrollY(0.0f);
		}
		else if (IsKeyPressed(ImGuiKey_Enter) || IsKeyPressed(ImGuiKey_KeypadEnter)) { // Automatically exit the combo popup on selection
			if (combo_data->SetNewValue(combo_data->CurrentSelection < 0 ? item_getter(items, -1) : item_getter2(combo_data->CurrentSelection))) {
				selection_changed = true;
				selected_item = combo_data->CurrentSelection;
			}
			CloseCurrentPopup();
		}

		if (IsKeyPressed(ImGuiKey_UpArrow)) {
			if (combo_data->CurrentSelection > 0) {
				SetScrollToComboItemUp(listbox_window, --combo_data->CurrentSelection);
			}
		}
		else if (IsKeyPressed(ImGuiKey_DownArrow)) {
			if (combo_data->CurrentSelection >= -1 && combo_data->CurrentSelection < item_count - 1) {
				SetScrollToComboItemDown(listbox_window, ++combo_data->CurrentSelection);
			}
		}

		EndListBox();
	}
	EndPopup();
	PopStyleVar();

	return selection_changed;
}

} // Internal namespace
} // ImGui namespace