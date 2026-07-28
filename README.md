# hDialogs

[![hDialogs](https://img.shields.io/badge/Fanorisky-hDialogs-lightgrey?style=for-the-badge)](github.com/Fanorisky/hDialogs)

hDialogs is a comprehensive open.mp library part of [H-Libs](https://github.com/Harwana/H-Libs) designed to provide multiple dialog mechanism: synchronous, asynchronous, named dialogs, and pagination dialog support built on top of PawnPlus capabilities.

## Usage

```pawn
#include <hDialogs>
```

**Requirements**: PawnPlus, sscanf2

## Examples

### Async Dialog
```pawn
CMD:buybait(cmdid, playerid, params[])
{
    yield 1;
    new baitamount = await ShowAsyncNumberInputDialog(playerid, "Bait Quantity", "Enter how much fishing bait you'd like to purchase below", "Buy", "Cancel");
    if(baitamount == cellmin) return 1; //cellmin is returned when a player cancels a dialog
    if(baitamount <= 0 || baitamount > 10000)
    {
        SendClientMessage(playerid, -1, "Invalid bait amount.");
        return 1;
    }
    SendClientMessage(playerid, 0xFFFFFFFF, "Anda membeli %d bait ygy", baitamount);
    return 1;
}
```

### Named Dialog
```pawn
Dialog:OnRegister(playerid, response, listitem, inputtext[])
{
    if(!response) return 0;
    SendClientMessage(playerid, 0xFFFFFFFF, "register with name %s", inputtext);
}

CMD:register(playerid) {
    Dialog_Show(playerid, "OnRegister", DIALOG_STYLE_INPUT, "Register", "Enter name:", "OK", "Cancel");
    return 1;
}
```

### Named Dialog Pagination
```pawn
PageDialog:OnSelectItem(playerid, response, listitem, extraid)
{
    if(!response) return 0;
    SendClientMessage(playerid, 0xFFFFFFFF, "success buy item %d for $%d", listitem, extraid);
}

CMD:shop(playerid) {
    new List:items = list_new();
    for(new i=0; i<35; i++) {
        AddDialogListItem(items, sprintf("Item %d", i+1), (random(1000)*i));
    }

    Dialog_ShowPages(playerid, "OnSelectItem", DIALOG_STYLE_LIST, 
        "Nigga store {page}/{maxPages}", items, "Select", "Close");
    return 1;
}
```

## API Reference

### Callback Macros
```pawn
#define Dialog:%0(%1) forward hdialog_%0(%1); public hdialog_%0(%1)
#define PageDialog:%0(%1) forward hpage_%0(%1); public hpage_%0(%1)
```

### 1. Named Callback System
```pawn
Dialog_Show(playerid, const callbackName[], DIALOG_STYLE:style, const caption[], const info[], const button1[], const button2[] = "");
Dialog_ShowPages(playerid, const callbackName[], DIALOG_STYLE:style, const caption[], List:items, const button1[], const button2[] = "", const nextButton[] = ">>>", const prevButton[] = "<<<", maxItems = MAX_DIALOG_PAGE_ITEMS, page = 0);
Dialog_Close(playerid);
```

### 2. Async Dialog Functions

All async `ShowAsync*` functions take optional `Task:existingtask = INVALID_TASK` as the last argument. Pass the same incomplete task to re-show a dialog without creating a new awaiter (validation / retry loops).

#### Generic Async
```pawn
ShowAsyncDialog(playerid, DIALOG_STYLE:style, const caption[], const info[], const button1[], const button2[] = "", Task:existingtask = INVALID_TASK);
ShowAsyncDialogStr(playerid, DIALOG_STYLE:style, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK);
```

#### Input Dialogs
```pawn
ShowAsyncNumberInputDialog(playerid, const title[], const body[], const button1[], const button2[], Task:existingtask = INVALID_TASK);
ShowAsyncNumberInputDialogStr(...);

ShowAsyncFloatInputDialog(...);
ShowAsyncStringInputDialog(...);
ShowAsyncPasswordDialog(...);
// + Str variants
```

#### List Dialogs
```pawn
ShowAsyncListitemTextDialog(...);
ShowAsyncListitemIndexDialog(...);
// + Str variants
```

#### Confirmation Dialog
```pawn
ShowAsyncConfirmationDialog(playerid, const title[], const body[], const button1[], const button2[] = "", Task:existingtask = INVALID_TASK);
ShowAsyncConfirmationDialogStr(...);
```

#### Entity index dialog
Maps each list row to an integer id in `PlayerDialogData` (helpers: `DialogData_*`).

```pawn
CMD:houses(playerid)
{
    yield 1;

    DialogData_Clear(playerid);

    new body[512];
    body[0] = EOS;
    for (new i = 0; i < HouseCount; i++) {
        if (!House[i][hOwned]) continue;
        DialogData_Add(playerid, House[i][hSQLID]);
        format(body, sizeof(body), "%s%s (ID %d)\n", body, House[i][hName], House[i][hSQLID]);
    }

    if (!DialogData_Size(playerid)) {
        SendClientMessage(playerid, -1, "No houses.");
        return 1;
    }

    new id = await ShowAsyncEntityIndexDialog(playerid, DIALOG_STYLE_LIST, "Houses", body, "Select", "Close");
    if (id == -1) return 1; // cancel / OOB

    SendClientMessage(playerid, -1, "Selected house SQL id %d", id);
    return 1;
}
```

```pawn
DialogData_Ensure(playerid);
DialogData_Clear(playerid);
DialogData_Add(playerid, extraid);
DialogData_Size(playerid);
DialogData_Get(playerid, index); // -1 if OOB
ShowAsyncEntityIndexDialog(...);     // returns selected id, or -1
ShowAsyncEntityIndexDialogStr(...);
```

**Note:** `PlayerDialogData` is **not** cleared when opening other dialogs — call `DialogData_Clear` before each entity list. List is created on connect and freed on disconnect.

#### Re-show / validation

Invalid number/float/empty string is re-shown **internally** (task stays incomplete until a valid value or cancel).

For **caller-side** range checks after a value is returned, just show again (new task is fine):

```pawn
yield 1;
for (;;) {
    new amount = await ShowAsyncNumberInputDialog(playerid, "Qty", "Enter 1-100:", "OK", "Cancel");
    if (amount == cellmin) return 1;
    if (amount >= 1 && amount <= 100) break;
}
```

`existingtask` is for re-showing while the task is still **incomplete** (same awaiter continues):

```pawn
new Task:t = ShowAsyncNumberInputDialog(playerid, "Qty", "Enter amount:", "OK", "Cancel");
// ... later, still waiting, update caption/body without a new task:
ShowAsyncNumberInputDialog(playerid, "Qty", "Still waiting — enter amount:", "OK", "Cancel", t);
new amount = await t;
```
### 3. Pagination System

#### Item Management
```pawn
AddDialogListItem(List:items, const string[], extraid = 0);
AddDialogListItemStr(List:items, ConstStringTag:str, extraid = 0);
GetPaginationDialogItemData(List:items, index, dest[], dest_size = sizeof dest, &extraid = 0);
CreatePaginationListFromArray(const items[][], const extraids[], count = sizeof(items));
```

#### Show Pagination
```pawn
ShowDialogPages(playerid, dialogid, DIALOG_STYLE:style, const caption[], List:items, const button1[], const button2[] = "", const nextButton[] = ">>>", const prevButton[] = "<<<", maxItems = MAX_DIALOG_PAGE_ITEMS, page = 0);
ShowAsyncDialogPages(playerid, DIALOG_STYLE:style, const caption[], List:items, const button1[], const button2[] = "", const nextButton[] = ">>>", const prevButton[] = "<<<", maxItems = MAX_DIALOG_PAGE_ITEMS, page = 0);
```

#### Control
```pawn
ClosePaginationDialog(playerid);
```

### 4. PawnPlus String-based Functions
```pawn
ShowPlayerDialogStr(playerid, dialogid, DIALOG_STYLE:style, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL);
```

## Configuration
```pawn
#define HL_DIALOG_ID_START                 (0x502B)
#define HL_DIALOG_INPUT_LENGTH             128
#define DIALOG_PAGE_ITEMS                  -1   // auto-fit page size
#define MAX_DIALOG_PAGE_ITEMS              DIALOG_PAGE_ITEMS
#define MAX_DIALOG_PAGE_CAPTION_LENGTH     64
#define MAX_PAGE_ITEM_STRING               256
```

## PawnPlus notes

- Async dialogs use `task_new` + `await` / `await_arr`; complete with `task_set_result` / `_arr` / `_str`.
- Replacing a dialog without `existingtask` releases the previous incomplete task (`task_delete`).
- Pass `existingtask` to re-show while keeping the same awaiter.
- Prefer `yield 1;` in commands before `await`.
- Pagination / lists use `list_*`, `pool_*`, and PP `str_*` where needed.

## Design

hDialogs is an all-in-one dialog library (async + named + pagination). It aims to cover the same capabilities as focused async dialog libs (typed inputs, entity lists, pages, task reuse) plus named callbacks.
