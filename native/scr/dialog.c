#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dazzle.h"
#include "scr.h"
#include "resource.h"

struct dialog_state {
    struct dz_scr_settings settings;
    HWND pages[3];
    HWND image_list, image_size;
    HWND minimum, maximum, pause;
    HWND mode, palette, delay, sound, presentation, renderer, classic;
    int selected_image;
    int filling;
};

static HWND control(HWND parent, const char *kind, const char *label, DWORD style,
                    int id, int x, int y, int width, int height)
{
    return CreateWindowExA(0, kind, label, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, parent, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
}

static void label(HWND parent, const char *text, int x, int y, int width)
{
    control(parent, "STATIC", text, SS_LEFT, 0, x, y, width, 20);
}

static void add_combo_items(HWND combo, const char *const *items, unsigned count, unsigned selection)
{
    for (unsigned i = 0; i < count; ++i) SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)items[i]);
    SendMessageA(combo, CB_SETCURSEL, selection, 0);
}

static void add_images_page(HWND dialog, struct dialog_state *state, RECT bounds)
{
    HWND page = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_CLIPCHILDREN,
        bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
        dialog, NULL, GetModuleHandleA(NULL), NULL);
    state->pages[0] = page;
    int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
    state->image_list = control(page, WC_LISTVIEWA, "", WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL,
        IDC_IMAGE_LIST, 8, 8, width - 24, height - 66);
    ListView_SetExtendedListViewStyle(state->image_list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    LVCOLUMNA column = {0};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.pszText = "Image"; column.cx = width - 260; column.iSubItem = 0;
    ListView_InsertColumn(state->image_list, 0, &column);
    column.pszText = "Size / duration"; column.cx = 150; column.iSubItem = 1;
    ListView_InsertColumn(state->image_list, 1, &column);
    for (unsigned i = 0; i < 42; ++i) {
        ImageConfigRow *image = i < 36 ? &alt_image_records[i] : &ctrl_image_records[i - 36];
        char size[16];
        snprintf(size, sizeof size, "%u", state->settings.image_size[i]);
        LVITEMA item = {0};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = (int)i;
        item.pszText = image->name;
        item.lParam = (LPARAM)i;
        ListView_InsertItem(state->image_list, &item);
        ListView_SetItemText(state->image_list, (int)i, 1, size);
        ListView_SetCheckState(state->image_list, (int)i, state->settings.image_size[i] != 0);
    }
    label(page, "Selected image size (0 disables this image):", 10, height - 44, 300);
    state->image_size = control(page, "EDIT", "", WS_TABSTOP | ES_NUMBER | WS_BORDER | ES_AUTOHSCROLL,
        IDC_IMAGE_SIZE, 320, height - 48, 90, 24);
    HWND spinner = control(page, UPDOWN_CLASSA, "", UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_ARROWKEYS,
        IDC_IMAGE_SIZE_SPIN, 0, 0, 0, 0);
    SendMessageA(spinner, UDM_SETBUDDY, (WPARAM)state->image_size, 0);
    SendMessageA(spinner, UDM_SETRANGE32, 0, 65535);
    state->selected_image = 0;
    ListView_SetItemState(state->image_list, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    SetDlgItemInt(page, IDC_IMAGE_SIZE, state->settings.image_size[0], FALSE);
}

static void add_fades_page(HWND dialog, struct dialog_state *state, RECT bounds)
{
    HWND page = CreateWindowExA(0, "STATIC", "", WS_CHILD,
        bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
        dialog, NULL, GetModuleHandleA(NULL), NULL);
    state->pages[1] = page;
    int width = bounds.right - bounds.left;
    for (unsigned i = 0; i < 24; ++i) {
        int column = (int)(i / 8), row = (int)(i % 8);
        HWND check = control(page, "BUTTON", fade_records[i].name,
            BS_AUTOCHECKBOX | WS_TABSTOP, IDC_FADE_FIRST + (int)i,
            14 + column * (width / 3), 14 + row * 34, width / 3 - 20, 26);
        SendMessageA(check, BM_SETCHECK, state->settings.fade_enabled[i] ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

static void add_auto_display_page(HWND dialog, struct dialog_state *state, RECT bounds)
{
    HWND page = CreateWindowExA(0, "STATIC", "", WS_CHILD,
        bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
        dialog, NULL, GetModuleHandleA(NULL), NULL);
    state->pages[2] = page;
    const int left = 16, field = 260;
    label(page, "Auto scheduling", left, 12, 300);
    label(page, "Minimum images per display (1-256):", left, 44, 290);
    state->minimum = control(page, "EDIT", "", WS_TABSTOP | ES_NUMBER | WS_BORDER,
        IDC_MINIMUM_IMAGES, field, 42, 80, 24);
    label(page, "Maximum images per display (1-256):", left, 76, 290);
    state->maximum = control(page, "EDIT", "", WS_TABSTOP | ES_NUMBER | WS_BORDER,
        IDC_MAXIMUM_IMAGES, field, 74, 80, 24);
    label(page, "Pause between displays in seconds (0–256):", left, 108, 300);
    state->pause = control(page, "EDIT", "", WS_TABSTOP | ES_NUMBER | WS_BORDER,
        IDC_FADE_PAUSE, field, 106, 80, 24);

    label(page, "Display", left, 154, 300);
    static const char *const modes[] = {"VGA 320 x 400", "VGA 320 x 200", "EGA 640 x 350", "EGA 640 x 700"};
    static const char *const palettes[] = {"Normal", "Alternate", "Random", "Fixed"};
    static const char *const presentations[] = {"Original 4:3", "Crop-zoom", "Stretch"};
    static const char *const renderers[] = {"Auto", "Software"};
    label(page, "Adapter / mode:", left, 184, 230);
    state->mode = control(page, WC_COMBOBOXA, "", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL,
        IDC_DISPLAY_MODE, field, 180, 250, 180);
    add_combo_items(state->mode, modes, 4, (unsigned)state->settings.mode);
    label(page, "Palette style:", left, 218, 230);
    state->palette = control(page, WC_COMBOBOXA, "", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL,
        IDC_PALETTE, field, 214, 250, 180);
    add_combo_items(state->palette, palettes, 4, (unsigned)state->settings.palette);
    label(page, "Draw delay (1–8191):", left, 252, 230);
    state->delay = control(page, "EDIT", "", WS_TABSTOP | ES_NUMBER | WS_BORDER,
        IDC_DRAW_DELAY, field, 250, 90, 24);
    label(page, "Presentation:", left, 286, 230);
    state->presentation = control(page, WC_COMBOBOXA, "", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL,
        IDC_PRESENTATION, field, 282, 250, 180);
    add_combo_items(state->presentation, presentations, 3, state->settings.presentation);
    label(page, "Renderer:", left, 320, 230);
    state->renderer = control(page, WC_COMBOBOXA, "", CBS_DROPDOWNLIST | WS_TABSTOP,
        IDC_RENDERER, field, 316, 250, 100);
    add_combo_items(state->renderer, renderers, 2,
        state->settings.renderer == DZ_RENDERER_RGBA ? 1u : 0u);
    state->classic = control(page, "BUTTON", "Rendering: Classic (future)", BS_AUTOCHECKBOX,
        IDC_CLASSIC, field + 120, 316, 250, 24);
    EnableWindow(state->classic, FALSE);
    state->sound = control(page, "BUTTON", "Sound on", BS_AUTOCHECKBOX | WS_TABSTOP,
        IDC_SOUND, left, 354, 160, 24);
    SendMessageA(state->sound, BM_SETCHECK, state->settings.sound ? BST_CHECKED : BST_UNCHECKED, 0);
    SetDlgItemInt(page, IDC_MINIMUM_IMAGES, state->settings.minimum_images, FALSE);
    SetDlgItemInt(page, IDC_MAXIMUM_IMAGES, state->settings.maximum_images, FALSE);
    SetDlgItemInt(page, IDC_FADE_PAUSE, state->settings.fade_pause_seconds, FALSE);
    SetDlgItemInt(page, IDC_DRAW_DELAY, state->settings.draw_delay, FALSE);
}

static void fill_pages(HWND dialog, struct dialog_state *state)
{
    RECT bounds;
    GetClientRect(GetDlgItem(dialog, IDC_TABS), &bounds);
    TabCtrl_AdjustRect(GetDlgItem(dialog, IDC_TABS), FALSE, &bounds);
    MapWindowPoints(GetDlgItem(dialog, IDC_TABS), dialog, (POINT *)&bounds, 2);
    state->filling = 1;
    add_images_page(dialog, state, bounds);
    add_fades_page(dialog, state, bounds);
    add_auto_display_page(dialog, state, bounds);
    ShowWindow(state->pages[0], SW_SHOW);
    state->filling = 0;
}

static void set_current_image_size(struct dialog_state *state)
{
    if (state->selected_image < 0) return;
    BOOL translated;
    UINT value = GetDlgItemInt(GetParent(state->image_size), IDC_IMAGE_SIZE, &translated, FALSE);
    if (!translated) value = state->settings.image_size[state->selected_image];
    state->settings.image_size[state->selected_image] = (uint16_t)(value > 65535u ? 65535u : value);
    char text[16];
    snprintf(text, sizeof text, "%u", state->settings.image_size[state->selected_image]);
    ListView_SetItemText(state->image_list, state->selected_image, 1, text);
    if (ListView_GetCheckState(state->image_list, state->selected_image) && !state->settings.image_size[state->selected_image]) {
        state->settings.image_size[state->selected_image] = 10;
        SetDlgItemInt(GetParent(state->image_size), IDC_IMAGE_SIZE, 10, FALSE);
        ListView_SetItemText(state->image_list, state->selected_image, 1, "10");
    }
    if (!ListView_GetCheckState(state->image_list, state->selected_image))
        state->settings.image_size[state->selected_image] = 0;
}

static void load_selected_image(struct dialog_state *state, int index)
{
    state->selected_image = index;
    SetDlgItemInt(GetParent(state->image_size), IDC_IMAGE_SIZE,
        state->settings.image_size[index], FALSE);
}

static unsigned read_number(HWND control_handle, unsigned fallback)
{
    BOOL translated;
    UINT value = (UINT)GetDlgItemInt(GetParent(control_handle), GetDlgCtrlID(control_handle), &translated, FALSE);
    return translated ? value : fallback;
}

static void commit_controls(struct dialog_state *state)
{
    set_current_image_size(state);
    for (unsigned i = 0; i < 42; ++i)
        if (!ListView_GetCheckState(state->image_list, (int)i)) state->settings.image_size[i] = 0;
        else if (!state->settings.image_size[i]) state->settings.image_size[i] = 10;
    for (unsigned i = 0; i < 24; ++i)
        state->settings.fade_enabled[i] = (unsigned char)(SendMessageA(GetDlgItem(state->pages[1],
            IDC_FADE_FIRST + (int)i), BM_GETCHECK, 0, 0) == BST_CHECKED);
    state->settings.minimum_images = read_number(state->minimum, state->settings.minimum_images);
    state->settings.maximum_images = read_number(state->maximum, state->settings.maximum_images);
    state->settings.fade_pause_seconds = read_number(state->pause, state->settings.fade_pause_seconds);
    state->settings.draw_delay = read_number(state->delay, state->settings.draw_delay);
    state->settings.mode = (enum dz_scr_mode)SendMessageA(state->mode, CB_GETCURSEL, 0, 0);
    state->settings.palette = (enum dz_scr_palette)SendMessageA(state->palette, CB_GETCURSEL, 0, 0);
    state->settings.presentation = (unsigned)SendMessageA(state->presentation, CB_GETCURSEL, 0, 0);
    state->settings.renderer = SendMessageA(state->renderer, CB_GETCURSEL, 0, 0) == 1 ? DZ_RENDERER_RGBA : DZ_RENDERER_AUTO;
    state->settings.sound = SendMessageA(state->sound, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

static void show_tab(HWND dialog, struct dialog_state *state, int selected)
{
    for (unsigned i = 0; i < 3; ++i) ShowWindow(state->pages[i], (int)i == selected ? SW_SHOW : SW_HIDE);
    SetFocus(GetDlgItem(dialog, IDC_TABS));
}

static void refresh_dialog(HWND dialog, struct dialog_state *state)
{
    TabCtrl_SetCurSel(GetDlgItem(dialog, IDC_TABS), 0);
    DestroyWindow(state->pages[0]); DestroyWindow(state->pages[1]); DestroyWindow(state->pages[2]);
    fill_pages(dialog, state);
}

static void import_dzl(HWND dialog, struct dialog_state *state)
{
    char filename[MAX_PATH] = "";
    OPENFILENAMEA open = {0};
    open.lStructSize = sizeof open;
    open.hwndOwner = dialog;
    open.lpstrFilter = "DAZZLE configuration (*.DZL)\0*.DZL\0All files (*.*)\0*.*\0\0";
    open.lpstrFile = filename;
    open.nMaxFile = sizeof filename;
    open.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameA(&open)) return;
    commit_controls(state);
    if (!dz_scr_settings_import(&state->settings, filename)) {
        MessageBoxA(dialog, "That file is not a valid DAZZLE configuration.", "DAZZLE 5.0", MB_OK | MB_ICONWARNING);
        return;
    }
    refresh_dialog(dialog, state);
}

INT_PTR CALLBACK dz_scr_settings_dialog(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam)
{
    struct dialog_state *state = (struct dialog_state *)GetWindowLongPtrA(dialog, GWLP_USERDATA);
    if (message == WM_INITDIALOG) {
        INITCOMMONCONTROLSEX common = {sizeof common, ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS};
        InitCommonControlsEx(&common);
        state = calloc(1, sizeof *state);
        if (!state) { EndDialog(dialog, IDCANCEL); return TRUE; }
        SetWindowLongPtrA(dialog, GWLP_USERDATA, (LONG_PTR)state);
        char dzl_path[4096];
        dz_scr_settings_load(&state->settings, dzl_path, sizeof dzl_path);
        HWND tabs = GetDlgItem(dialog, IDC_TABS);
        const char *titles[] = {"Images", "Fades", "Auto and Display"};
        for (int i = 0; i < 3; ++i) {
            TCITEMA item = {0}; item.mask = TCIF_TEXT; item.pszText = (char *)titles[i];
            TabCtrl_InsertItem(tabs, i, &item);
        }
        fill_pages(dialog, state);
        return TRUE;
    }
    if (!state) return FALSE;
    switch (message) {
    case WM_NOTIFY: {
        NMHDR *header = (NMHDR *)lparam;
        if (header->idFrom == IDC_TABS && header->code == TCN_SELCHANGE) {
            show_tab(dialog, state, TabCtrl_GetCurSel(header->hwndFrom));
            return TRUE;
        }
        if (header->idFrom == IDC_IMAGE_LIST && header->code == LVN_ITEMCHANGED && !state->filling) {
            NMLISTVIEW *change = (NMLISTVIEW *)lparam;
            if ((change->uChanged & LVIF_STATE) && (change->uNewState & LVIS_SELECTED) && change->iItem != state->selected_image) {
                set_current_image_size(state);
                load_selected_image(state, change->iItem);
            }
            return TRUE;
        }
        return FALSE;
    }
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDC_IMPORT: import_dzl(dialog, state); return TRUE;
        case IDC_DEFAULTS:
            dz_scr_settings_defaults(&state->settings);
            refresh_dialog(dialog, state);
            return TRUE;
        case IDOK: {
            commit_controls(state);
            if (!dz_scr_settings_validate(&state->settings))
                MessageBoxA(dialog, "Some values were adjusted to the supported DAZZLE ranges.", "DAZZLE 5.0", MB_OK | MB_ICONINFORMATION);
            if (!dz_scr_settings_save(&state->settings)) {
                MessageBoxA(dialog, "DAZZLE could not save the settings.", "DAZZLE 5.0", MB_OK | MB_ICONERROR);
                return TRUE;
            }
            EndDialog(dialog, IDOK);
            return TRUE;
        }
        case IDCANCEL: EndDialog(dialog, IDCANCEL); return TRUE;
        }
        break;
    case WM_DESTROY:
        free(state);
        SetWindowLongPtrA(dialog, GWLP_USERDATA, 0);
        return TRUE;
    }
    return FALSE;
}
