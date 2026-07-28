// -
// Include Guards
// -

#if defined _INC_hlib_dialogs
    #endinput
#endif
#define _INC_hlib_dialogs

// -
// External Packages
// -

#if !defined _INC_PawnPlus && !defined _PawnPlus_included
    #error Missing PawnPlus (github.com/IS4Code/PawnPlus).
#elseif !defined _INC_SSCANF
    #error Missing sscanf2
#endif

// -
// Pre-Hooks
// -

#if !defined CHAIN_ORDER
    #define CHAIN_ORDER() 0
#endif

#define CHAIN_HOOK(%0) forward @CO_%0();public @CO_%0(){return CHAIN_ORDER()+1;}
#define CHAIN_NEXT(%0) @CO_%0

#define CHAIN_FORWARD:%0_%2(%1)=%3; \
    forward %0_%2(%1); \
    public %0_%2(%1) <_ALS : _ALS_x0, _ALS : _ALS_x1> { return (%3); } \
    public %0_%2(%1) <> { return (%3); }

#define CHAIN_PUBLIC:%0(%1) %0(%1) <_ALS : _ALS_go>

CHAIN_HOOK(HLD)
#undef CHAIN_ORDER
#define CHAIN_ORDER CHAIN_NEXT(HLD)

static stock _HLD_IncludeStates() <_ALS : _ALS_x0, _ALS : _ALS_x1, _ALS : _ALS_x2, _ALS : _ALS_x3> {}
static stock _HLD_IncludeStates() <_ALS : _ALS_go> {}

// -
// Declarations
// -

#if !defined FLOAT_NAN
    #define FLOAT_NAN    (Float:0xFFFFFFFF)
#endif

#if !defined isnull
    #define isnull(%1) ((!(%1[0])) || (((%1[0]) == '\1') && (!(%1[1]))))
#endif

#if !defined DIALOG_STYLE
    #define DIALOG_STYLE                        _
#endif

#if !defined HL_DIALOG_ID_START
    #define HL_DIALOG_ID_START                 (0x502B) // "P+" in ascii hex
#endif

#if !defined HL_DIALOG_INPUT_LENGTH
    #define HL_DIALOG_INPUT_LENGTH             128
#endif

#if !defined DIALOG_PAGE_ITEMS
    #define DIALOG_PAGE_ITEMS             -1
#endif

#if !defined MAX_DIALOG_PAGE_ITEMS
    #define MAX_DIALOG_PAGE_ITEMS         DIALOG_PAGE_ITEMS
#endif

#if !defined MAX_DIALOG_PAGE_CAPTION_LENGTH
    #define MAX_DIALOG_PAGE_CAPTION_LENGTH       64
#endif

#if !defined MAX_PAGE_ITEM_STRING
    #define MAX_PAGE_ITEM_STRING          256
#endif


// -
// Macro named callback dialogs
// -

#define Dialog:%0(%1) forward hdialog_%0(%1); public hdialog_%0(%1)
#define PageDialog:%0(%1) forward hpage_%0(%1); public hpage_%0(%1)

// -
// Enums
// -

static enum
{
    HL_DIALOG_ID = HL_DIALOG_ID_START,
    HL_DIALOG_ID_NAMED,
    HL_DIALOG_ID_NUMBER_INPUT,
    HL_DIALOG_ID_FLOAT_INPUT,
    HL_DIALOG_ID_STRING_INPUT,
    HL_DIALOG_ID_PASSWORD_INPUT,
    HL_DIALOG_ID_LISTITEM_TEXT,
    HL_DIALOG_ID_LISTITEM_INDEX,
    HL_DIALOG_ID_CONFIRMATION,
    HL_DIALOG_ID_ENTITY_INDEX, // listitem → PlayerDialogData[listitem]
}

static const HL_DIALOG_ID_END = HL_DIALOG_ID_ENTITY_INDEX;

enum _:E_ASYNC_DIALOG {
    E_ASYNC_DIALOG_RESPONSE,
    E_ASYNC_DIALOG_LISTITEM,
    E_ASYNC_DIALOG_INPUTTEXT[HL_DIALOG_INPUT_LENGTH],
};

enum _:E_ASYNC_PAGE_DIALOG {
    bool:E_ASYNC_PAGE_DIALOG_RESPONSE,
    E_ASYNC_PAGE_DIALOG_LISTITEM,
    E_ASYNC_PAGE_DIALOG_EXTRAID,
};

enum _:E_PAGINATION_ITEM {
    E_PAGINATION_ITEM_STRING[MAX_PAGE_ITEM_STRING],
    E_PAGINATION_ITEM_EXTRAID
};

enum _:E_PAGINATION_DATA {
    List:E_ITEMS,
    E_MAX_PAGES,
    E_CURRENT_PAGE,
    E_MAX_PAGE_ITEMS,
    E_ITEM_START_INDEX,
    E_ITEM_END_INDEX,
    E_CAPTION[MAX_DIALOG_PAGE_CAPTION_LENGTH],
    E_BUTTON_1[MAX_DIALOG_PAGE_CAPTION_LENGTH],
    E_BUTTON_2[MAX_DIALOG_PAGE_CAPTION_LENGTH],
    E_PREV_BUTTON[MAX_DIALOG_PAGE_CAPTION_LENGTH],
    E_NEXT_BUTTON[MAX_DIALOG_PAGE_CAPTION_LENGTH],
    DIALOG_STYLE:E_STYLE,
    E_DIALOG_ID,
    String:E_HEADER,
    bool:E_ASYNC_MODE,
    Task:E_TASK,
    bool:E_NAMED_MODE,
    E_NAMED_CALLBACK[MAX_DIALOG_PAGE_CAPTION_LENGTH]
};

enum _:E_DIALOG_DATA {
    DIALOG_STYLE:E_DIALOG_STYLE,
    E_DIALOG_CAPTION[64],
    E_DIALOG_INFO[4096],
    E_DIALOG_BUTTON1[64],
    E_DIALOG_BUTTON2[64]
};

// -
// Static Variables
// -

static stock Task:gs_dialogTask[MAX_PLAYERS] = {INVALID_TASK, ...};
static stock Pool:gs_pagePool = INVALID_POOL;
static stock bool:gs_onDialogPageResponse = false;
static stock gs_namedCb[MAX_PLAYERS][MAX_DIALOG_PAGE_CAPTION_LENGTH];
static stock gs_dialogData[MAX_PLAYERS][E_DIALOG_DATA];

// Per-player entity id list: row index N → PlayerDialogData[N].
// Clear before each entity dialog; not wiped by HLD_ResetDialogs.
new List:PlayerDialogData[MAX_PLAYERS] = {INVALID_LIST, ...};

// -
// Static Functions
// -

// Incomplete tasks: task_valid → task_delete (faults awaiters via amx_err_exit).

static stock HLD_ReleaseTask(Task:t)
{
    if (task_valid(t)) {
        task_delete(t);
    }
    return 1;
}

static stock HLD_FreePlayerDialogData(playerid)
{
    if (list_valid(PlayerDialogData[playerid])) {
        // deep: in case callers stored variants/strings in the list
        list_delete_deep(PlayerDialogData[playerid]);
    }
    PlayerDialogData[playerid] = INVALID_LIST;
    return 1;
}

// Clear named/page/caption buffers. Does not touch gs_dialogTask.
static stock HLD_ClearDialogSurface(playerid)
{
    gs_namedCb[playerid][0] = EOS;

    gs_dialogData[playerid][E_DIALOG_STYLE] = DIALOG_STYLE_MSGBOX;
    gs_dialogData[playerid][E_DIALOG_CAPTION][0] = EOS;
    gs_dialogData[playerid][E_DIALOG_INFO][0] = EOS;
    gs_dialogData[playerid][E_DIALOG_BUTTON1][0] = EOS;
    gs_dialogData[playerid][E_DIALOG_BUTTON2][0] = EOS;

    if (gs_pagePool != INVALID_POOL && pool_has(gs_pagePool, playerid)) {
        new paginationData[E_PAGINATION_DATA];
        pool_get_arr(gs_pagePool, playerid, paginationData);

        if (paginationData[E_TASK] != INVALID_TASK) {
            HLD_ReleaseTask(paginationData[E_TASK]);
            paginationData[E_TASK] = INVALID_TASK;
        }

        if (list_valid(paginationData[E_ITEMS])) {
            list_delete(paginationData[E_ITEMS]);
        }

        if (str_valid(paginationData[E_HEADER])) {
            str_delete(paginationData[E_HEADER]);
        }

        pool_remove(gs_pagePool, playerid);

        if (!pool_size(gs_pagePool)) {
            pool_delete(gs_pagePool);
            gs_pagePool = INVALID_POOL;
        }
    }

    // PlayerDialogData kept across surface resets — DialogData_Clear before entity lists.
    return 1;
}

static stock HLD_ResetDialogs(playerid)
{
    if (gs_dialogTask[playerid] != INVALID_TASK) {
        HLD_ReleaseTask(gs_dialogTask[playerid]);
        gs_dialogTask[playerid] = INVALID_TASK;
    }
    return HLD_ClearDialogSurface(playerid);
}

// Assign slot task: new task, or reuse an incomplete one (caller-driven re-show before complete).
static stock Task:HLD_BindDialogTask(playerid, Task:existingtask = INVALID_TASK)
{
    if (task_valid(existingtask) && !task_completed(existingtask)) {
        if (gs_dialogTask[playerid] != INVALID_TASK && gs_dialogTask[playerid] != existingtask) {
            HLD_ReleaseTask(gs_dialogTask[playerid]);
        }
        gs_dialogTask[playerid] = INVALID_TASK;
        HLD_ClearDialogSurface(playerid); // does not touch gs_dialogTask / existingtask
        gs_dialogTask[playerid] = existingtask;
        return existingtask;
    }

    HLD_ResetDialogs(playerid);
    gs_dialogTask[playerid] = task_new();
    return gs_dialogTask[playerid];
}

static stock getMaxPages(List:items, maxItems) 
{
    new size = list_size(items);
    if (size == 0) return 0;
    if (maxItems <= 0) {maxItems = 1;}
    new totalPages = size / maxItems, leftover = size % maxItems;
    if (leftover > 0) {totalPages++;}
    return totalPages;
}

static stock getMaxPageItems(List:items, maxItems) 
{
    const maxDialogInfo = 4096;
    new maxCalculatedItems = 0;
    new totalLen = 0;

    for (new i = 0, j = list_size(items); i < j; i++) {
        new item[E_PAGINATION_ITEM];
        list_get_arr(items, i, item);
        
        new strLen = strlen(item[E_PAGINATION_ITEM_STRING]) + 3;

        if (totalLen + strLen >= maxDialogInfo) {
            break;
        }
        
        totalLen += strLen;
        maxCalculatedItems++;
    }

    // MAX_DIALOG_PAGE_ITEMS default is -1 → auto-fit to body capacity
    if (maxItems == MAX_DIALOG_PAGE_ITEMS) {
        return maxCalculatedItems > 0 ? maxCalculatedItems : 1;
    }

    if (maxItems > maxCalculatedItems) {
        return maxCalculatedItems > 0 ? maxCalculatedItems : 1;
    }

    return maxItems > 0 ? maxItems : 1;
}

static stock bool:hasNextPage(currentPage, maxPages) return (currentPage < (maxPages - 1));

static stock bool:hasPrevPage(currentPage) return (currentPage > 0);

static stock parseDialogCaption(const paginationData[E_PAGINATION_DATA], dest[], len = sizeof dest)
{
    new const pageParse[] = "{page}";
    new const maxPagesParse[] = "{maxPages}";

    dest[0] = EOS;
    strcat(dest, paginationData[E_CAPTION], len);

    new startPageParsePos = strfind(dest, pageParse);
    if (startPageParsePos != -1) {
        strdel(dest, startPageParsePos, (startPageParsePos + (sizeof pageParse) - 1));
        
        new string[11];
        valstr(string, paginationData[E_CURRENT_PAGE] + 1);
        strins(dest, string, startPageParsePos, len);
    }

    new startMaxPagesParsePos = strfind(dest, maxPagesParse);
    if (startMaxPagesParsePos != -1) {
        strdel(dest, startMaxPagesParsePos, (startMaxPagesParsePos + (sizeof maxPagesParse) - 1));
        
        new string[11];
        valstr(string, paginationData[E_MAX_PAGES]);
        strins(dest, string, startMaxPagesParsePos, len);
    }
}

static stock bool:OpenDialogPage(playerid, page)
{   
    if (gs_pagePool == INVALID_POOL || !pool_has(gs_pagePool, playerid)) {
        return false;
    }

    new paginationData[E_PAGINATION_DATA];
    pool_get_arr(gs_pagePool, playerid, paginationData);
    
    if (!list_valid(paginationData[E_ITEMS])) {
        return false;
    }
    
    new size = list_size(paginationData[E_ITEMS]);
    if (size == 0) {
        return false;
    }

    if (page < 0) page = 0;
    if (page >= paginationData[E_MAX_PAGES]) {
        page = paginationData[E_MAX_PAGES] - 1;
    }

    paginationData[E_ITEM_START_INDEX] = page * paginationData[E_MAX_PAGE_ITEMS];
    paginationData[E_ITEM_END_INDEX] = (paginationData[E_ITEM_START_INDEX] + paginationData[E_MAX_PAGE_ITEMS]) - 1;
    
    new maxSize = size - 1;
    if (paginationData[E_ITEM_END_INDEX] > maxSize) {
        paginationData[E_ITEM_END_INDEX] = maxSize;
    }

    static dialogInfo[4096];
    dialogInfo[0] = EOS;

    for (new i = paginationData[E_ITEM_START_INDEX]; i <= paginationData[E_ITEM_END_INDEX]; i++) {
        new item[E_PAGINATION_ITEM];
        list_get_arr(paginationData[E_ITEMS], i, item);

        if (i != paginationData[E_ITEM_START_INDEX]) {
            strcat(dialogInfo, "\n");
        }

        strcat(dialogInfo, item[E_PAGINATION_ITEM_STRING]);
    }

    new bool:hasNext = hasNextPage(page, paginationData[E_MAX_PAGES]);
    new bool:hasPrev = hasPrevPage(page);

    if (hasPrev) {
        new buttonString[MAX_DIALOG_PAGE_CAPTION_LENGTH + 3];
        format(buttonString, sizeof buttonString, "%s\n", paginationData[E_PREV_BUTTON]);
        strins(dialogInfo, buttonString, 0);
    }
    
    if (hasNext) {
        new buttonString[MAX_DIALOG_PAGE_CAPTION_LENGTH + 3];
        format(buttonString, sizeof buttonString, "\n%s", paginationData[E_NEXT_BUTTON]);
        strcat(dialogInfo, buttonString);
    }

    if (str_valid(paginationData[E_HEADER])) {
        new header[256];
        str_get(paginationData[E_HEADER], header);
        
        strcat(header, "\n");
        strins(dialogInfo, header, 0);
    }
    
    paginationData[E_CURRENT_PAGE] = page;
    pool_set_arr(gs_pagePool, playerid, paginationData);

    #if defined DIALOG_STYLE
        new DIALOG_STYLE:style = DIALOG_STYLE:paginationData[E_STYLE];
    #else
        new style = _:paginationData[E_STYLE];
    #endif

    new caption[64];
    parseDialogCaption(paginationData, caption);

    ShowPlayerDialog(
        playerid,
        HL_DIALOG_ID,
        style,
        caption,
        dialogInfo,
        paginationData[E_BUTTON_1],
        paginationData[E_BUTTON_2]
    );
    return true;
}

static stock callDialogResponse(playerid, dialogid, bool:response, listitem, extraid)
{
    if (gs_onDialogPageResponse) {
        CallLocalFunction("OnDialogPageResponse", "iiiii", playerid, dialogid, response, listitem, extraid);
    }
    return 1;
}

// -
// API
// -

forward _pp@showplayerdialogstr@call(playerid, dialogid, DIALOG_STYLE:style, const caption[], const info[], const button1[], const button2[]);
public _pp@showplayerdialogstr@call(playerid, dialogid, DIALOG_STYLE:style, const caption[], const info[], const button1[], const button2[])
{
    return ShowPlayerDialog(playerid, dialogid, style, caption, info, button1, button2);
}

stock ShowPlayerDialogStr(playerid, dialogid, DIALOG_STYLE:style, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL)
{
    return CallLocalFunction(pawn_nameof(_pp@showplayerdialogstr@call), "iiiSSSS", playerid, dialogid, _:style, _:str_addr_const(caption), _:str_addr_const(info), _:str_addr_const(button1), _:str_addr_const(button2));
}

static stock Task:TaskDialog(playerid, dialogid, DIALOG_STYLE:style, const caption[], const info[], const button1[], const button2[], Task:existingtask = INVALID_TASK)
{
    if (!IsPlayerConnected(playerid)) {
        return INVALID_TASK;
    }

    // New dialog: old incomplete awaiters are released. Pass existingtask to keep the same await.
    new Task:t = HLD_BindDialogTask(playerid, existingtask);

    gs_dialogData[playerid][E_DIALOG_STYLE] = style;
    // Buffers cleared in bind/surface — strcat is safe.
    strcat(gs_dialogData[playerid][E_DIALOG_CAPTION], caption, 64);
    strcat(gs_dialogData[playerid][E_DIALOG_INFO], info, 4096);
    strcat(gs_dialogData[playerid][E_DIALOG_BUTTON1], button1, 64);
    strcat(gs_dialogData[playerid][E_DIALOG_BUTTON2], button2, 64);

    ShowPlayerDialog(playerid, dialogid, style, caption, info, button1, button2);
    return t;
}

static stock Task:TaskDialogStr(playerid, dialogid, DIALOG_STYLE:style, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    if (!IsPlayerConnected(playerid)) {
        return INVALID_TASK;
    }

    new Task:t = HLD_BindDialogTask(playerid, existingtask);

    gs_dialogData[playerid][E_DIALOG_STYLE] = style;
    str_get(caption, gs_dialogData[playerid][E_DIALOG_CAPTION], 64);
    str_get(info, gs_dialogData[playerid][E_DIALOG_INFO], 4096);
    str_get(button1, gs_dialogData[playerid][E_DIALOG_BUTTON1], 64);
    str_get(button2, gs_dialogData[playerid][E_DIALOG_BUTTON2], 64);

    ShowPlayerDialogStr(playerid, dialogid, style, caption, info, button1, button2);
    return t;
}

stock Task:ShowAsyncDialog(playerid, DIALOG_STYLE:style, const caption[], const info[], const button1[], const button2[] = "", Task:existingtask = INVALID_TASK)
{
    return TaskDialog(playerid, HL_DIALOG_ID, style, caption, info, button1, button2, existingtask);
}

stock Task:ShowAsyncDialogStr(playerid, DIALOG_STYLE:style, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    return TaskDialogStr(playerid, HL_DIALOG_ID, style, caption, info, button1, button2, existingtask);
}

stock Task:ShowAsyncNumberInputDialog(playerid, const title[], const body[], const button1[], const button2[], Task:existingtask = INVALID_TASK)
{
    return TaskDialog(playerid, HL_DIALOG_ID_NUMBER_INPUT, DIALOG_STYLE_INPUT, title, body, button1, button2, existingtask);
}

stock Task:ShowAsyncNumberInputDialogStr(playerid, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    return TaskDialogStr(playerid, HL_DIALOG_ID_NUMBER_INPUT, DIALOG_STYLE_INPUT, caption, info, button1, button2, existingtask);
}

stock Task:ShowAsyncFloatInputDialog(playerid, const title[], const body[], const button1[], const button2[], Task:existingtask = INVALID_TASK)
{
    return TaskDialog(playerid, HL_DIALOG_ID_FLOAT_INPUT, DIALOG_STYLE_INPUT, title, body, button1, button2, existingtask);
}

stock Task:ShowAsyncFloatInputDialogStr(playerid, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    return TaskDialogStr(playerid, HL_DIALOG_ID_FLOAT_INPUT, DIALOG_STYLE_INPUT, caption, info, button1, button2, existingtask);
}

stock Task:ShowAsyncStringInputDialog(playerid, const title[], const body[], const button1[], const button2[], Task:existingtask = INVALID_TASK)
{
    return TaskDialog(playerid, HL_DIALOG_ID_STRING_INPUT, DIALOG_STYLE_INPUT, title, body, button1, button2, existingtask);
}

stock Task:ShowAsyncStringInputDialogStr(playerid, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    return TaskDialogStr(playerid, HL_DIALOG_ID_STRING_INPUT, DIALOG_STYLE_INPUT, caption, info, button1, button2, existingtask);
}

stock Task:ShowAsyncPasswordDialog(playerid, const title[], const body[], const button1[], const button2[], Task:existingtask = INVALID_TASK)
{
    return TaskDialog(playerid, HL_DIALOG_ID_PASSWORD_INPUT, DIALOG_STYLE_PASSWORD, title, body, button1, button2, existingtask);
}

stock Task:ShowAsyncPasswordDialogStr(playerid, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    return TaskDialogStr(playerid, HL_DIALOG_ID_PASSWORD_INPUT, DIALOG_STYLE_PASSWORD, caption, info, button1, button2, existingtask);
}

stock Task:ShowAsyncListitemTextDialog(playerid, DIALOG_STYLE:style, const title[], const body[], const button1[], const button2[], Task:existingtask = INVALID_TASK)
{
    return TaskDialog(playerid, HL_DIALOG_ID_LISTITEM_TEXT, style, title, body, button1, button2, existingtask);
}

stock Task:ShowAsyncListitemTextDialogStr(playerid, DIALOG_STYLE:style, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    return TaskDialogStr(playerid, HL_DIALOG_ID_LISTITEM_TEXT, style, caption, info, button1, button2, existingtask);
}

stock Task:ShowAsyncListitemIndexDialog(playerid, DIALOG_STYLE:style, const title[], const body[], const button1[], const button2[], Task:existingtask = INVALID_TASK)
{
    return TaskDialog(playerid, HL_DIALOG_ID_LISTITEM_INDEX, style, title, body, button1, button2, existingtask);
}

stock Task:ShowAsyncListitemIndexDialogStr(playerid, DIALOG_STYLE:style, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    return TaskDialogStr(playerid, HL_DIALOG_ID_LISTITEM_INDEX, style, caption, info, button1, button2, existingtask);
}

stock Task:ShowAsyncConfirmationDialog(playerid, const title[], const body[], const button1[], const button2[] = "", Task:existingtask = INVALID_TASK)
{
    return TaskDialog(playerid, HL_DIALOG_ID_CONFIRMATION, DIALOG_STYLE_MSGBOX, title, body, button1, button2, existingtask);
}

stock Task:ShowAsyncConfirmationDialogStr(playerid, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    return TaskDialogStr(playerid, HL_DIALOG_ID_CONFIRMATION, DIALOG_STYLE_MSGBOX, caption, info, button1, button2, existingtask);
}

// -
// Entity index dialog
// List rows must match PlayerDialogData indices 0..n-1.
// Returns: selected entity id, or -1 on cancel / OOB.
// Caller: DialogData_Clear → DialogData_Add (same order as body) → await Show…
// -

stock DialogData_Ensure(playerid)
{
    if (!list_valid(PlayerDialogData[playerid])) {
        PlayerDialogData[playerid] = list_new();
    }
    return 1;
}

stock DialogData_Clear(playerid)
{
    if (list_valid(PlayerDialogData[playerid])) {
        list_clear(PlayerDialogData[playerid]);
    } else {
        PlayerDialogData[playerid] = list_new();
    }
    return 1;
}

stock DialogData_Add(playerid, extraid)
{
    DialogData_Ensure(playerid);
    return list_add(PlayerDialogData[playerid], extraid);
}

stock DialogData_Size(playerid)
{
    if (!list_valid(PlayerDialogData[playerid])) {
        return 0;
    }
    return list_size(PlayerDialogData[playerid]);
}

stock DialogData_Get(playerid, index)
{
    if (!list_valid(PlayerDialogData[playerid]) || index < 0 || index >= list_size(PlayerDialogData[playerid])) {
        return -1;
    }
    return list_get(PlayerDialogData[playerid], index);
}

stock Task:ShowAsyncEntityIndexDialog(playerid, DIALOG_STYLE:style, const title[], const body[], const button1[], const button2[] = "", Task:existingtask = INVALID_TASK)
{
    DialogData_Ensure(playerid);
    return TaskDialog(playerid, HL_DIALOG_ID_ENTITY_INDEX, style, title, body, button1, button2, existingtask);
}

stock Task:ShowAsyncEntityIndexDialogStr(playerid, DIALOG_STYLE:style, ConstString:caption, ConstString:info, ConstString:button1, ConstString:button2 = ConstString:STRING_NULL, Task:existingtask = INVALID_TASK)
{
    DialogData_Ensure(playerid);
    return TaskDialogStr(playerid, HL_DIALOG_ID_ENTITY_INDEX, style, caption, info, button1, button2, existingtask);
}

// -
// NAMED CALLBACK SYSTEM (Seperti easyDialog)
// -

stock Dialog_Show(playerid, const callbackName[], DIALOG_STYLE:style, const caption[], const info[], const button1[], const button2[] = "")
{
    if (!IsPlayerConnected(playerid)) {
        return 0;
    }

    HLD_ResetDialogs(playerid);
    
    format(gs_namedCb[playerid], MAX_DIALOG_PAGE_CAPTION_LENGTH, "hdialog_%s", callbackName);
    
    ShowPlayerDialog(playerid, HL_DIALOG_ID_NAMED, style, caption, info, button1, button2);
    return 1;
}

stock Dialog_Close(playerid)
{
    HLD_ResetDialogs(playerid);
    ShowPlayerDialog(playerid, -1, DIALOG_STYLE_MSGBOX, " ", " ", " ", " ");
    return 1;
}

// -
// Pagination Dialogs
// -

forward OnDialogPageResponse(playerid, dialogid, bool:response, listitem, extraid);

stock AddDialogListItem(List:items, const string[], extraid = 0)
{
    if (!list_valid(items)) {
        return -1;
    }

    new item[E_PAGINATION_ITEM];
    strcat(item[E_PAGINATION_ITEM_STRING], string, MAX_PAGE_ITEM_STRING);
    item[E_PAGINATION_ITEM_EXTRAID] = extraid;

    return list_add_arr(items, item);
}

stock AddDialogListItemStr(List:items, ConstStringTag:str, extraid = 0)
{
    if (!list_valid(items)) {
        return -1;
    }

    new item[E_PAGINATION_ITEM];
    str_get(str, item[E_PAGINATION_ITEM_STRING], MAX_PAGE_ITEM_STRING);
    item[E_PAGINATION_ITEM_EXTRAID] = extraid;

    return list_add_arr(items, item);
}

stock bool:ShowDialogPages(playerid, dialogid, DIALOG_STYLE:style, const caption[], List:items, const button1[], const button2[] = "", const nextButton[] = ">>>", const prevButton[] = "<<<", maxItems = MAX_DIALOG_PAGE_ITEMS, page = 0)
{
    if (!IsPlayerConnected(playerid)) {
        return false;
    }

    if (!list_valid(items) || list_size(items) <= 0) {
        return false;
    }

    HLD_ResetDialogs(playerid);

    if (gs_pagePool == INVALID_POOL) {
        gs_pagePool = pool_new();
    } else {
        if (pool_has(gs_pagePool, playerid)) {
            new paginationData[E_PAGINATION_DATA];
            pool_get_arr(gs_pagePool, playerid, paginationData);
            list_delete(paginationData[E_ITEMS]);

            if (str_valid(paginationData[E_HEADER])) {
                str_delete(paginationData[E_HEADER]);
            }
        }
    }

    new paginationData[E_PAGINATION_DATA];
    
    if (style == DIALOG_STYLE_TABLIST_HEADERS) {
        if (list_size(items) > 0) {
            new item[E_PAGINATION_ITEM];
            list_get_arr(items, 0, item);
            paginationData[E_HEADER] = str_acquire(str_new(item[E_PAGINATION_ITEM_STRING]));
            list_remove(items, 0);
        } else {
            paginationData[E_HEADER] = INVALID_STRING;
        }
    } else {
        paginationData[E_HEADER] = INVALID_STRING;
    }
    
    paginationData[E_ITEMS] = items;
    paginationData[E_MAX_PAGE_ITEMS] = getMaxPageItems(items, maxItems);
    paginationData[E_MAX_PAGES] = getMaxPages(items, paginationData[E_MAX_PAGE_ITEMS]);
    paginationData[E_CURRENT_PAGE] = page;
    strcat(paginationData[E_CAPTION], caption, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_BUTTON_1], button1, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_BUTTON_2], button2, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_PREV_BUTTON], prevButton, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_NEXT_BUTTON], nextButton, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    paginationData[E_STYLE] = style;
    paginationData[E_DIALOG_ID] = dialogid;
    paginationData[E_ASYNC_MODE] = false;
    paginationData[E_TASK] = INVALID_TASK;
    paginationData[E_NAMED_MODE] = false;
    paginationData[E_NAMED_CALLBACK][0] = EOS;

    if (paginationData[E_STYLE] == DIALOG_STYLE_TABLIST) {
        new curTabs = 0;
        new pos = -1;
        
        while ((pos = strfind(paginationData[E_PREV_BUTTON], "\t", false, pos + 1)) != -1) {
            curTabs++;
        }

        if (list_size(paginationData[E_ITEMS]) > 0) {
            new item[E_PAGINATION_ITEM];
            list_get_arr(paginationData[E_ITEMS], 0, item);
            
            new firstItemTabs = 0;
            pos = -1;
            
            while ((pos = strfind(item[E_PAGINATION_ITEM_STRING], "\t", false, pos + 1)) != -1) {
                firstItemTabs++;
            }

            for (new i = curTabs; i <= firstItemTabs; i++) {
                strcat(paginationData[E_PREV_BUTTON], "\t ", MAX_DIALOG_PAGE_CAPTION_LENGTH);
            }
        }
    }

    pool_set_arr(gs_pagePool, playerid, paginationData);
    return OpenDialogPage(playerid, page);
}

// -
// Pagination named dialog
// -

stock Dialog_ShowPages(playerid, const callbackName[], DIALOG_STYLE:style, const caption[], List:items, const button1[], const button2[] = "", const nextButton[] = ">>>", const prevButton[] = "<<<", maxItems = MAX_DIALOG_PAGE_ITEMS, page = 0)
{
    if (!IsPlayerConnected(playerid)) {
        return false;
    }

    if (!list_valid(items) || list_size(items) <= 0) {
        return false;
    }

    HLD_ResetDialogs(playerid);

    if (gs_pagePool == INVALID_POOL) {
        gs_pagePool = pool_new();
    } else {
        if (pool_has(gs_pagePool, playerid)) {
            new paginationData[E_PAGINATION_DATA];
            pool_get_arr(gs_pagePool, playerid, paginationData);
            list_delete(paginationData[E_ITEMS]);

            if (str_valid(paginationData[E_HEADER])) {
                str_delete(paginationData[E_HEADER]);
            }
        }
    }

    new paginationData[E_PAGINATION_DATA];
    
    if (style == DIALOG_STYLE_TABLIST_HEADERS) {
        if (list_size(items) > 0) {
            new item[E_PAGINATION_ITEM];
            list_get_arr(items, 0, item);
            paginationData[E_HEADER] = str_acquire(str_new(item[E_PAGINATION_ITEM_STRING]));
            list_remove(items, 0);
        } else {
            paginationData[E_HEADER] = INVALID_STRING;
        }
    } else {
        paginationData[E_HEADER] = INVALID_STRING;
    }
    
    paginationData[E_ITEMS] = items;
    paginationData[E_MAX_PAGE_ITEMS] = getMaxPageItems(items, maxItems);
    paginationData[E_MAX_PAGES] = getMaxPages(items, paginationData[E_MAX_PAGE_ITEMS]);
    paginationData[E_CURRENT_PAGE] = page;
    strcat(paginationData[E_CAPTION], caption, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_BUTTON_1], button1, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_BUTTON_2], button2, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_PREV_BUTTON], prevButton, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_NEXT_BUTTON], nextButton, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    paginationData[E_STYLE] = style;
    paginationData[E_DIALOG_ID] = 0;
    paginationData[E_ASYNC_MODE] = false;
    paginationData[E_TASK] = INVALID_TASK;
    paginationData[E_NAMED_MODE] = true;
    format(paginationData[E_NAMED_CALLBACK], MAX_DIALOG_PAGE_CAPTION_LENGTH, "hpage_%s", callbackName);

    if (paginationData[E_STYLE] == DIALOG_STYLE_TABLIST) {
        new curTabs = 0;
        new pos = -1;
        
        while ((pos = strfind(paginationData[E_PREV_BUTTON], "\t", false, pos + 1)) != -1) {
            curTabs++;
        }

        if (list_size(paginationData[E_ITEMS]) > 0) {
            new item[E_PAGINATION_ITEM];
            list_get_arr(paginationData[E_ITEMS], 0, item);
            
            new firstItemTabs = 0;
            pos = -1;
            
            while ((pos = strfind(item[E_PAGINATION_ITEM_STRING], "\t", false, pos + 1)) != -1) {
                firstItemTabs++;
            }

            for (new i = curTabs; i <= firstItemTabs; i++) {
                strcat(paginationData[E_PREV_BUTTON], "\t ", MAX_DIALOG_PAGE_CAPTION_LENGTH);
            }
        }
    }

    pool_set_arr(gs_pagePool, playerid, paginationData);
    return OpenDialogPage(playerid, page);
}

// -
// Pagination dialog - Async
// -

stock Task:ShowAsyncDialogPages(playerid, DIALOG_STYLE:style, const caption[], List:items, const button1[], const button2[] = "", const nextButton[] = ">>>", const prevButton[] = "<<<", maxItems = MAX_DIALOG_PAGE_ITEMS, page = 0)
{
    if (!IsPlayerConnected(playerid)) {
        return Task:0;
    }

    if (!list_valid(items) || list_size(items) <= 0) {
        return Task:0;
    }

    HLD_ResetDialogs(playerid);

    if (gs_pagePool == INVALID_POOL) {
        gs_pagePool = pool_new();
    } else {
        if (pool_has(gs_pagePool, playerid)) {
            new paginationData[E_PAGINATION_DATA];
            pool_get_arr(gs_pagePool, playerid, paginationData);
            list_delete(paginationData[E_ITEMS]);

            if (str_valid(paginationData[E_HEADER])) {
                str_delete(paginationData[E_HEADER]);
            }
        }
    }

    new Task:t = task_new();
    
    new paginationData[E_PAGINATION_DATA];
    
    if (style == DIALOG_STYLE_TABLIST_HEADERS) {
        if (list_size(items) > 0) {
            new item[E_PAGINATION_ITEM];
            list_get_arr(items, 0, item);
            paginationData[E_HEADER] = str_acquire(str_new(item[E_PAGINATION_ITEM_STRING]));
            list_remove(items, 0);
        } else {
            paginationData[E_HEADER] = INVALID_STRING;
        }
    } else {
        paginationData[E_HEADER] = INVALID_STRING;
    }
    
    paginationData[E_ITEMS] = items;
    paginationData[E_MAX_PAGE_ITEMS] = getMaxPageItems(items, maxItems);
    paginationData[E_MAX_PAGES] = getMaxPages(items, paginationData[E_MAX_PAGE_ITEMS]);
    paginationData[E_CURRENT_PAGE] = page;
    strcat(paginationData[E_CAPTION], caption, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_BUTTON_1], button1, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_BUTTON_2], button2, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_PREV_BUTTON], prevButton, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    strcat(paginationData[E_NEXT_BUTTON], nextButton, MAX_DIALOG_PAGE_CAPTION_LENGTH);
    paginationData[E_STYLE] = style;
    paginationData[E_DIALOG_ID] = HL_DIALOG_ID;
    paginationData[E_ASYNC_MODE] = true;
    paginationData[E_TASK] = t;
    paginationData[E_NAMED_MODE] = false;
    paginationData[E_NAMED_CALLBACK][0] = EOS;

    if (paginationData[E_STYLE] == DIALOG_STYLE_TABLIST) {
        new curTabs = 0;
        new pos = -1;
        
        while ((pos = strfind(paginationData[E_PREV_BUTTON], "\t", false, pos + 1)) != -1) {
            curTabs++;
        }

        if (list_size(paginationData[E_ITEMS]) > 0) {
            new item[E_PAGINATION_ITEM];
            list_get_arr(paginationData[E_ITEMS], 0, item);
            
            new firstItemTabs = 0;
            pos = -1;
            
            while ((pos = strfind(item[E_PAGINATION_ITEM_STRING], "\t", false, pos + 1)) != -1) {
                firstItemTabs++;
            }

            for (new i = curTabs; i <= firstItemTabs; i++) {
                strcat(paginationData[E_PREV_BUTTON], "\t ", MAX_DIALOG_PAGE_CAPTION_LENGTH);
            }
        }
    }

    pool_set_arr(gs_pagePool, playerid, paginationData);
    OpenDialogPage(playerid, page);
    
    return t;
}

stock bool:ClosePaginationDialog(playerid)
{
    return HLD_ResetDialogs(playerid);
}

stock GetPaginationDialogItemData(List:items, index, dest[], dest_size = sizeof dest, &extraid = 0)
{
    if (!list_valid(items) || index < 0 || index >= list_size(items)) {
        dest[0] = EOS;
        extraid = -1;
        return false;
    }
    
    new item[E_PAGINATION_ITEM];
    list_get_arr(items, index, item);
    
    strcat(dest, item[E_PAGINATION_ITEM_STRING], dest_size);
    extraid = item[E_PAGINATION_ITEM_EXTRAID];
    
    return true;
}

stock List:CreatePaginationListFromArray(const items[][], const extraids[], count = sizeof(items))
{
    new List:list = list_new();
    for (new i = 0; i < count && i < sizeof(items); i++) {
        AddDialogListItem(list, items[i], extraids[i]);
    }
    return list;
}

// -
// Internals
// -

public OnGameModeInit()
{
    state _ALS : _ALS_go;
    gs_onDialogPageResponse = (funcidx("OnDialogPageResponse") != -1);
    return HLD_OnGameModeInit();
}

public OnFilterScriptInit()
{
    state _ALS : _ALS_go;
    gs_onDialogPageResponse = (funcidx("OnDialogPageResponse") != -1);
    return HLD_OnFilterScriptInit();
}

public OnPlayerConnect(playerid)
{
    HLD_FreePlayerDialogData(playerid);
    DialogData_Ensure(playerid);
    return HLD_OnPlayerConnect(playerid);
}

public OnPlayerDisconnect(playerid, reason)
{
    HLD_ResetDialogs(playerid);
    HLD_FreePlayerDialogData(playerid);
    return HLD_OnPlayerDisconnect(playerid, reason);
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    // Not ours — forward the ALS chain so MenuStore / other dialog libs still run.
    if (dialogid < HL_DIALOG_ID_START || dialogid > HL_DIALOG_ID_END)
    {
        return HLD_OnDialogResponse(playerid, dialogid, response, listitem, inputtext);
    }

    // Detach handle from slot so Reset cannot double-free while we complete it.
    new Task:t = gs_dialogTask[playerid];
    gs_dialogTask[playerid] = INVALID_TASK;

    switch(dialogid)
    {
        case HL_DIALOG_ID_NUMBER_INPUT:
        {
            if (!task_valid(t)) {
                return 1;
            }

            if(!response)
            {
                task_set_result(t, cellmin);
                return 1;
            }

            new value;
            if(sscanf(inputtext, "i", value))
            {
                // Re-show same dialog; keep the same incomplete task.
                ShowPlayerDialog(
                    playerid, 
                    HL_DIALOG_ID_NUMBER_INPUT, 
                    gs_dialogData[playerid][E_DIALOG_STYLE],
                    gs_dialogData[playerid][E_DIALOG_CAPTION],
                    gs_dialogData[playerid][E_DIALOG_INFO],
                    gs_dialogData[playerid][E_DIALOG_BUTTON1],
                    gs_dialogData[playerid][E_DIALOG_BUTTON2]
                );
                gs_dialogTask[playerid] = t;
                return 1;
            }

            task_set_result(t, value);
            return 1;
        }
        
        case HL_DIALOG_ID_FLOAT_INPUT:
        {
            if (!task_valid(t)) {
                return 1;
            }

            if(!response)
            {
                task_set_result(t, FLOAT_NAN);
                return 1;
            }

            new Float:value;
            if(sscanf(inputtext, "f", value))
            {
                ShowPlayerDialog(
                    playerid, 
                    HL_DIALOG_ID_FLOAT_INPUT, 
                    gs_dialogData[playerid][E_DIALOG_STYLE],
                    gs_dialogData[playerid][E_DIALOG_CAPTION],
                    gs_dialogData[playerid][E_DIALOG_INFO],
                    gs_dialogData[playerid][E_DIALOG_BUTTON1],
                    gs_dialogData[playerid][E_DIALOG_BUTTON2]
                );
                gs_dialogTask[playerid] = t;
                return 1;
            }

            task_set_result(t, value);
            return 1;
        }
        
        case HL_DIALOG_ID_STRING_INPUT, HL_DIALOG_ID_PASSWORD_INPUT, HL_DIALOG_ID_LISTITEM_TEXT:
        {
            if (!task_valid(t)) {
                return 1;
            }

            if(!response)
            {
                task_set_result_str(t, "");
                return 1;
            }

            if((dialogid == HL_DIALOG_ID_STRING_INPUT || dialogid == HL_DIALOG_ID_PASSWORD_INPUT) && isnull(inputtext))
            {
                ShowPlayerDialog(
                    playerid, 
                    dialogid, 
                    gs_dialogData[playerid][E_DIALOG_STYLE],
                    gs_dialogData[playerid][E_DIALOG_CAPTION],
                    gs_dialogData[playerid][E_DIALOG_INFO],
                    gs_dialogData[playerid][E_DIALOG_BUTTON1],
                    gs_dialogData[playerid][E_DIALOG_BUTTON2]
                );
                gs_dialogTask[playerid] = t;
                return 1;
            }

            task_set_result_str(t, inputtext);
            return 1;
        }
        
        case HL_DIALOG_ID_LISTITEM_INDEX:
        {
            if (!task_valid(t)) {
                return 1;
            }

            if(!response)
            {
                task_set_result(t, -1);
                return 1;
            }

            task_set_result(t, listitem);
            return 1;
        }
        
        case HL_DIALOG_ID_CONFIRMATION:
        {
            if (task_valid(t)) {
                task_set_result(t, bool:response);
            }
            return 1;
        }

        case HL_DIALOG_ID_ENTITY_INDEX:
        {
            if (!task_valid(t)) {
                return 1;
            }

            if (!response) {
                task_set_result(t, -1);
                return 1;
            }

            // Map listitem → id stored in PlayerDialogData (must be filled by caller).
            task_set_result(t, DialogData_Get(playerid, listitem));
            return 1;
        }
        
        case HL_DIALOG_ID:
        {
            if (gs_pagePool != INVALID_POOL && pool_has(gs_pagePool, playerid)) {
                new paginationData[E_PAGINATION_DATA];
                pool_get_arr(gs_pagePool, playerid, paginationData);
                new startIdx = paginationData[E_ITEM_START_INDEX];
    
                new bool:hasNext = hasNextPage(paginationData[E_CURRENT_PAGE], paginationData[E_MAX_PAGES]);
                new bool:hasPrev = hasPrevPage(paginationData[E_CURRENT_PAGE]);
                new endIdx = paginationData[E_ITEM_END_INDEX];
                new bool:isPageTransition = false;
    
                if (hasPrev) {
                    if (listitem == 0) {
                        isPageTransition = true;
                        if (response) {
                            OpenDialogPage(playerid, paginationData[E_CURRENT_PAGE] - 1);
                        }
                    }
                }
    
                if (hasNext) {
                    new nextButtonPosition = (endIdx - startIdx) + (hasPrev ? 2 : 1);
                    if (listitem == nextButtonPosition) {
                        isPageTransition = true;
                        if (response) {
                            OpenDialogPage(playerid, paginationData[E_CURRENT_PAGE] + 1);
                        }
                    }
                }
    
                if (isPageTransition) {
                    if (!response) {
                        if (!paginationData[E_ASYNC_MODE]) {
                            if (paginationData[E_NAMED_MODE] && paginationData[E_NAMED_CALLBACK][0]) {
                                CallLocalFunction(paginationData[E_NAMED_CALLBACK], "iiii", 
                                    playerid, false, -1, -1
                                );
                            } else if (!paginationData[E_NAMED_MODE]) {
                                callDialogResponse(playerid, paginationData[E_DIALOG_ID], false, -1, -1);
                            }
                        }
                        HLD_ResetDialogs(playerid);
                    }
                    return 1;
                }
                
                new actualIndex = startIdx + (hasPrev ? (listitem - 1) : listitem);
                new extraid = -1;
                
                if (response && actualIndex >= 0 && actualIndex < list_size(paginationData[E_ITEMS])) {
                    new item[E_PAGINATION_ITEM];
                    list_get_arr(paginationData[E_ITEMS], actualIndex, item);
                    extraid = item[E_PAGINATION_ITEM_EXTRAID];
                }

                // Snapshot completion fields, then free page state BEFORE task_set_result /
                // callbacks. PawnPlus resumes awaiters immediately; they often open another
                // dialog (ShowAsyncDialog / ShowAsyncDialogPages) which deletes or recreates
                // gs_pagePool. Writing the pool or HLD_ResetDialogs after that causes:
                //   pool_set_arr: pool reference is invalid (value 0x0)
                // and can wipe the newly opened dialog.
                new bool:asyncMode = paginationData[E_ASYNC_MODE];
                new bool:namedMode = paginationData[E_NAMED_MODE];
                new Task:pageTask = paginationData[E_TASK];
                new pageDialogId = paginationData[E_DIALOG_ID];
                new namedCb[MAX_DIALOG_PAGE_CAPTION_LENGTH];
                namedCb[0] = EOS;
                if (namedMode) {
                    strcat(namedCb, paginationData[E_NAMED_CALLBACK], sizeof namedCb);
                }

                paginationData[E_TASK] = INVALID_TASK;
                if (list_valid(paginationData[E_ITEMS])) {
                    list_delete(paginationData[E_ITEMS]);
                }
                if (str_valid(paginationData[E_HEADER])) {
                    str_delete(paginationData[E_HEADER]);
                }
                if (gs_pagePool != INVALID_POOL && pool_has(gs_pagePool, playerid)) {
                    pool_remove(gs_pagePool, playerid);
                    if (!pool_size(gs_pagePool)) {
                        pool_delete(gs_pagePool);
                        gs_pagePool = INVALID_POOL;
                    }
                }

                if (asyncMode) {
                    new data[E_ASYNC_PAGE_DIALOG];
                    data[E_ASYNC_PAGE_DIALOG_RESPONSE] = bool:!!response;
                    data[E_ASYNC_PAGE_DIALOG_LISTITEM] = response ? actualIndex : -1;
                    data[E_ASYNC_PAGE_DIALOG_EXTRAID] = response ? extraid : -1;

                    if (task_valid(pageTask) && !task_completed(pageTask)) {
                        task_set_result_arr(pageTask, data);
                    }
                } else {
                    if (namedMode && namedCb[0]) {
                        CallLocalFunction(namedCb, "iiii",
                            playerid,
                            bool:!!response,
                            response ? actualIndex : -1,
                            response ? extraid : -1
                        );
                    } else if (!namedMode) {
                        callDialogResponse(playerid, pageDialogId, bool:!!response, actualIndex, extraid);
                    }
                }
                return 1;
            }

            // Regular async dialog (non-pagination)
            if (t != INVALID_TASK && task_valid(t)) {
                new responses[E_ASYNC_DIALOG];
                responses[E_ASYNC_DIALOG_RESPONSE] = response;
                responses[E_ASYNC_DIALOG_LISTITEM] = listitem;
                responses[E_ASYNC_DIALOG_INPUTTEXT][0] = EOS;
                strcat(responses[E_ASYNC_DIALOG_INPUTTEXT], inputtext, HL_DIALOG_INPUT_LENGTH);

                // Immediate: same as async-dialogs task_set_result_arr
                task_set_result_arr(t, responses);
                // Task completed — do not leave handle in gs_dialogTask for delete-on-reset
                // (already cleared above to INVALID_TASK before switch)
                return 1;
            }
        }
        case HL_DIALOG_ID_NAMED:
        {
            if (gs_namedCb[playerid][0]) {
                new callback[MAX_DIALOG_PAGE_CAPTION_LENGTH];
                callback[0] = EOS;
                strcat(callback, gs_namedCb[playerid], sizeof(callback));

                gs_namedCb[playerid][0] = EOS;

                if (funcidx(callback) != -1) {
                    // "\1" empty trick for CallLocalFunction string (isnull-compatible)
                    CallLocalFunction(callback, "iiis",
                        playerid,
                        response,
                        listitem,
                        inputtext[0] ? inputtext : "\1"
                    );
                }
            }
            return 1;
        }
    }

    return HLD_OnDialogResponse(playerid, dialogid, response, listitem, inputtext);
}

// -
// Hooks
// -

#if defined _ALS_OnGameModeInit
    #undef OnGameModeInit
#else
    #define _ALS_OnGameModeInit
#endif
#define OnGameModeInit(%0) CHAIN_PUBLIC:HLD_OnGameModeInit(%0)
CHAIN_FORWARD:HLD_OnGameModeInit() = 1;

#if defined _ALS_OnFilterScriptInit
    #undef OnFilterScriptInit
#else
    #define _ALS_OnFilterScriptInit
#endif
#define OnFilterScriptInit(%0) CHAIN_PUBLIC:HLD_OnFilterScriptInit(%0)
CHAIN_FORWARD:HLD_OnFilterScriptInit() = 1;

#if defined _ALS_OnPlayerConnect
    #undef OnPlayerConnect
#else
    #define _ALS_OnPlayerConnect
#endif
#define OnPlayerConnect(%0) CHAIN_PUBLIC:HLD_OnPlayerConnect(%0)
CHAIN_FORWARD:HLD_OnPlayerConnect(playerid) = 1;

#if defined _ALS_OnPlayerDisconnect
    #undef OnPlayerDisconnect
#else
    #define _ALS_OnPlayerDisconnect
#endif
#define OnPlayerDisconnect(%0) CHAIN_PUBLIC:HLD_OnPlayerDisconnect(%0)
CHAIN_FORWARD:HLD_OnPlayerDisconnect(playerid, reason) = 1;

#if defined _ALS_OnDialogResponse
    #undef OnDialogResponse
#else
    #define _ALS_OnDialogResponse
#endif
#define OnDialogResponse(%0) CHAIN_PUBLIC:HLD_OnDialogResponse(%0)
CHAIN_FORWARD:HLD_OnDialogResponse(playerid, dialogid, response, listitem, inputtext[]) = 0;
