/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 by
 *
 *      Juha Lindström
 *      Darel Cullen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>

#include <ctime>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "meta.h"
#include "xperror.h"

#include "paint.h"

#include "sdlmeta.h"
#include "sdlwindow.h"
#include "text.h"
#include "glwidgets.h"

#define EVENT_JOIN 0
#define EVENT_REFRESH 1

#define SELECTED_BG 0x009000ff
#define ROW_FG 0xffff00ff
#define ROW_BG1 0x0000a0ff
#define ROW_BG2 0x000070ff
#define HEADER_FG 0xffff00ff
#define HEADER_BG 0xff0000ff
#define STATUS_ROWS 7
#define STATUS_COLS 4
#define STATUS_FIELD_FG 0xffff00ff
#define STATUS_FIELD_BG 0xff0000ff
#define STATUS_VALUE_FG 0xffff00ff
#define STATUS_VALUE_BG 0x000070ff
#define PLIST_HEADER_FG 0xffff00ff
#define PLIST_HEADER_BG 0xff0000ff
#define PLIST_ITEM_FG 0xffff00ff
#define PLIST_ITEM_BG 0x000070ff
#define ROW_HEIGHT 19
#define VERSION_WIDTH 100
#define COUNT_WIDTH 20
#define META_WIDTH 837
#define META_HEIGHT 768
#define BUTTON_BG 0xff0000ff
#define BUTTON_FG 0xffff00ff

#define METAWIDGET 100
#define METATABLEWIDGET 101
#define METAROWWIDGET 102
#define METAHEADERWIDGET 103
#define STATUSWIDGET 104
#define PLAYERLISTWIDGET 105

static int status_column_widths[] = {100, 0, 100, 70};

typedef struct
{
    GLWidget *table;
    GLWidget *status;
    GLWidget *players;
    GLuint texture;
    texcoord_t txc;
} MetaWidget;

typedef struct
{
    server_list_t server_list;
    GLWidget *meta;
    GLWidget *scrollbar;
    GLWidget *header;
    struct _MetaRowWidget *selected;
    struct _MetaRowWidget *first_row;
} MetaTableWidget;

typedef struct _MetaRowWidget
{
    Uint32 fg;
    Uint32 bg;
    MetaTableWidget *table;
    server_info_t *sip;
    bool is_selected;
} MetaRowWidget;

typedef struct
{
    Uint32 fg;
    Uint32 bg;
} MetaHeaderWidget;

typedef struct
{
    Uint32 name_fg, name_bg, value_fg, value_bg;
    char address_str[22];
    char fps_str[4];
    server_info_t *sip;
} StatusWidget;

typedef struct
{
    Uint32 header_fg, header_bg, item_fg, item_bg;
    GLWidget *header;
    GLWidget *scrollbar;
    std::vector<std::string> players;
    char *players_str;
} PlayerListWidget;

/* SDL2: needed for SDL_GL_SwapWindow */
extern SDL_Window *gWindow;

static void Scroll_PlayerListWidget(GLfloat pos, void *data)
{
    PlayerListWidget *info;
    GLWidget *widget, *row;
    SDL_Rect b;
    int y;

    widget = (GLWidget *)data;
    if (widget->WIDGET != PLAYERLISTWIDGET)
    {
        error("expected PLAYERLISTWIDGET got [%d]", widget->WIDGET);
        return;
    }

    info = (PlayerListWidget *)widget->wid_info;
    y = widget->bounds.y + ROW_HEIGHT - (int)(info->players.size() * ROW_HEIGHT * pos);

    for (row = widget->children; row; row = row->next)
    {
        if (row->WIDGET == LABELWIDGET && row != info->header)
        {
            b = row->bounds;
            b.y = y;
            SetBounds_GLWidget(row, &b);
            y += ROW_HEIGHT;
        }
    }
}

static void SetBounds_PlayerListWidget(GLWidget *widget, SDL_Rect *b)
{
    int y;
    GLWidget *row;
    PlayerListWidget *info;
    GLfloat list_height;
    SDL_Rect *wb, sb, rb, hb;

    if (widget->WIDGET != PLAYERLISTWIDGET)
    {
        error("expected PLAYERLISTWIDGET got [%d]", widget->WIDGET);
        return;
    }

    widget->bounds = *b;
    info = (PlayerListWidget *)widget->wid_info;
    list_height = info->players.size() * ROW_HEIGHT + ROW_HEIGHT;

    if (info->scrollbar != nullptr)
    {
        DelGLWidgetListItem(&(widget->children), info->scrollbar);
        Close_Widget(&(info->scrollbar));
        info->scrollbar = nullptr;
    }

    y = b->y + ROW_HEIGHT;
    for (row = widget->children; row; row = row->next)
    {
        if (row->WIDGET == LABELWIDGET && row != info->header)
        {
            rb.x = b->x + 1;
            rb.y = y;
            rb.w = b->w - 11;
            rb.h = ROW_HEIGHT;
            SetBounds_GLWidget(row, &rb);
            y += ROW_HEIGHT;
        }
    }

    if (list_height > b->h)
    {
        info->scrollbar =
            Init_ScrollbarWidget(false, 0.0f, ((GLfloat)b->h) / list_height,
                                 SB_VERTICAL, Scroll_PlayerListWidget, widget);
        if (info->scrollbar != nullptr)
        {
            wb = &(widget->bounds);
            sb.x = wb->x + wb->w - 10;
            sb.y = wb->y + ROW_HEIGHT;
            sb.w = 10;
            sb.h = wb->h - ROW_HEIGHT;
            SetBounds_GLWidget(info->scrollbar, &sb);
            AppendGLWidgetList(&(widget->children), info->scrollbar);
        }
        else
        {
            error("failed to create a scroll bar for player list");
            return;
        }
    }
    if (info->header != nullptr)
    {
        hb.x = widget->bounds.x + 1;
        hb.y = widget->bounds.y - 1;
        hb.w = widget->bounds.w - 10;
        hb.h = ROW_HEIGHT;
        SetBounds_GLWidget(info->header, &hb);
    }
}

static std::vector<std::string> create_player_list(char *players_str)
{
    std::vector<std::string> players;
    char *t;

    for (t = strtok(players_str, ","); t; t = strtok(nullptr, ","))
        players.emplace_back(t);

    return players;
}

static void Close_PlayerListWidget(GLWidget *widget)
{
    PlayerListWidget *info;

    if (widget->WIDGET != PLAYERLISTWIDGET)
    {
        error("expected PLAYERLISTWIDGET got [%d]", widget->WIDGET);
        return;
    }

    info = (PlayerListWidget *)widget->wid_info;
    if (info->players_str)
        free(info->players_str);
    info->players_str = nullptr;
    info->players.clear();
}

static void Paint_PlayerListWidget(GLWidget *widget)
{
    SDL_Rect *b = &(widget->bounds);
    set_alphacolor(PLIST_ITEM_BG);
    glBegin(GL_QUADS);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w - 10, b->y);
    glVertex2i(b->x + b->w - 10, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();
}

static GLWidget *Init_PlayerListWidget(server_info_t *sip)
{
    char *players_str;
    GLWidget *tmp, *header, *row;
    PlayerListWidget *info;

    if (!(players_str = strdup(sip->playlist.c_str())))
    {
        error("out of memory");
        return nullptr;
    }
    if (!(tmp = Init_EmptyBaseGLWidget()))
    {
        error("Widget init failed");
        free(players_str);
        return nullptr;
    }
    if (!(info = (PlayerListWidget *)malloc(sizeof(PlayerListWidget))))
    {
        error("out of memory");
        free(players_str);
        free(tmp);
        return nullptr;
    }
    new (&info->players) std::vector<std::string>();

    if (!(header =
              Init_LabelWidget("Players",
                               &(info->header_fg),
                               &(info->header_bg),
                               CENTER, CENTER)))
    {
        error("failed to create header for player list");
        info->players.~vector<std::string>();
        free(players_str);
        free(tmp);
        free(info);
        return nullptr;
    }

    info->players_str = players_str;
    info->players = create_player_list(players_str);
    info->scrollbar = nullptr;
    info->header = header;
    info->header_fg = PLIST_HEADER_FG;
    info->header_bg = PLIST_HEADER_BG;
    info->item_fg = PLIST_ITEM_FG;
    info->item_bg = PLIST_ITEM_BG;

    tmp->wid_info = info;
    tmp->WIDGET = PLAYERLISTWIDGET;
    tmp->SetBounds = SetBounds_PlayerListWidget;
    tmp->Draw = Paint_PlayerListWidget;
    tmp->Close = Close_PlayerListWidget;

    for (const std::string &player : info->players)
    {
        row = Init_LabelWidget(player.c_str(),
                               &(info->item_fg),
                               &(info->item_bg),
                               LEFT, CENTER);
        if (!row)
            break;
        AppendGLWidgetList(&(tmp->children), row);
    }
    AppendGLWidgetList(&(tmp->children), header);

    return tmp;
}

static void compute_layout(SDL_Rect *wb, int *width, int *xoff)
{
    int i, x, w, free_space, num_dynamic, w_dynamic;

    num_dynamic = 0;
    free_space = wb->w;
    for (i = 0; i < STATUS_COLS; i++)
    {
        w = status_column_widths[i];
        if (w > 5)
        {
            free_space -= w;
        }
        else
        {
            num_dynamic++;
        }
    }
    w_dynamic = free_space / num_dynamic - 5;
    x = 0;
    for (i = 0; i < STATUS_COLS; i++)
    {
        w = status_column_widths[i];
        xoff[i] = x;
        width[i] = w > 5 ? w - 5 : w_dynamic;
        x += width[i] + 5;
    }
}

static void SetBounds_StatusWidget(GLWidget *widget, SDL_Rect *wb)
{
    int i, c, rowc, rowi, colc, coli;
    int col_width[4];
    int col_xoff[4];
    SDL_Rect b;
    GLWidget *w;

    widget->bounds = *wb;
    compute_layout(wb, col_width, col_xoff);

    for (c = 0, w = widget->children; w; w = w->next)
        c++;

    rowc = STATUS_ROWS;
    colc = STATUS_COLS;
    b.h = ROW_HEIGHT;

    for (i = 0, w = widget->children; w && i < colc * rowc; w = w->next)
    {
        coli = (i % 2) + (i / 2 / rowc) * 2;
        rowi = (i / 2) % rowc;
        b.w = col_width[coli];
        b.x = wb->x + col_xoff[coli];
        b.y = wb->y + rowi * ROW_HEIGHT;
        SetBounds_GLWidget(w, &b);
        i++;
    }
}

static void add_status_entry(const char *name, const char *value, GLWidget *parent)
{
    GLWidget *name_label, *value_label;
    StatusWidget *info;

    info = (StatusWidget *)parent->wid_info;

    if ((name_label =
             Init_LabelWidget(name,
                              &(info->name_fg),
                              &(info->name_bg),
                              LEFT, CENTER)))
    {
        AppendGLWidgetList(&(parent->children), name_label);
    }
    if ((value_label =
             Init_LabelWidget(value,
                              &(info->value_fg),
                              &(info->value_bg),
                              LEFT, CENTER)))
    {
        AppendGLWidgetList(&(parent->children), value_label);
    }
}

static GLWidget *Init_StatusWidget(server_info_t *sip)
{
    GLWidget *tmp;
    StatusWidget *info;

    if (!(tmp = Init_EmptyBaseGLWidget()))
    {
        error("Widget init failed");
        return nullptr;
    }
    if (!(info = (StatusWidget *)malloc(sizeof(StatusWidget))))
    {
        error("out of memory");
        free(tmp);
        return nullptr;
    }
    sprintf(info->address_str, "%s:%u", sip->ip_str.c_str(), sip->port);
    sprintf(info->fps_str, "%u", sip->fps);
    info->sip = sip;
    info->name_fg = STATUS_FIELD_FG;
    info->name_bg = STATUS_FIELD_BG;
    info->value_fg = STATUS_VALUE_FG;
    info->value_bg = STATUS_VALUE_BG;
    tmp->wid_info = info;
    tmp->WIDGET = STATUSWIDGET;
    tmp->SetBounds = SetBounds_StatusWidget;

    add_status_entry(" Server", sip->hostname.c_str(), tmp);
    add_status_entry(" Address", info->address_str, tmp);
    add_status_entry(" Version", sip->version.c_str(), tmp);
    add_status_entry(" Map name", sip->mapname.c_str(), tmp);
    add_status_entry(" Map size", sip->mapsize.c_str(), tmp);
    add_status_entry(" Map author", sip->author.c_str(), tmp);
    add_status_entry(" Status", sip->status.c_str(), tmp);
    add_status_entry(" Bases", sip->bases_str.c_str(), tmp);
    add_status_entry(" Teams", sip->teambases_str.c_str(), tmp);
    add_status_entry(" Free bases", sip->freebases.c_str(), tmp);
    add_status_entry(" Queue", sip->queue_str.c_str(), tmp);
    add_status_entry(" FPS", info->fps_str, tmp);
    add_status_entry(" Sound", sip->sound.c_str(), tmp);
    add_status_entry(" Timing", sip->timing.c_str(), tmp);

    return tmp;
}

static void SelectRow_MetaWidget(GLWidget *widget, MetaRowWidget *row)
{
    MetaWidget *meta;
    MetaTableWidget *table;
    SDL_Rect status_bounds, plist_bounds;

    if (widget->WIDGET != METAWIDGET)
    {
        error("expected METAWIDGET got [%d]", widget->WIDGET);
        return;
    }
    meta = (MetaWidget *)widget->wid_info;
    if (meta->status != nullptr)
    {
        DelGLWidgetListItem(&(widget->children), meta->status);
        Close_Widget(&(meta->status));
        meta->status = nullptr;
    }
    status_bounds.x = widget->bounds.x + 22;
    status_bounds.y = widget->bounds.y + 598;
    status_bounds.w = 792 * 3 / 5;
    status_bounds.h = ROW_HEIGHT * STATUS_ROWS;
    if ((meta->status = Init_StatusWidget(row->sip)))
    {
        SetBounds_GLWidget(meta->status, &status_bounds);
        AppendGLWidgetList(&(widget->children), meta->status);
    }
    else
    {
        error("failed to create a status widget");
    }
    if (meta->players != nullptr)
    {
        DelGLWidgetListItem(&(widget->children), meta->players);
        Close_Widget(&(meta->players));
        meta->players = nullptr;
    }
    plist_bounds.x = status_bounds.x + status_bounds.w;
    plist_bounds.y = status_bounds.y;
    plist_bounds.h = status_bounds.h;
    plist_bounds.w = 792 - status_bounds.w;
    if ((meta->players = Init_PlayerListWidget(row->sip)))
    {
        SetBounds_GLWidget(meta->players, &plist_bounds);
        AppendGLWidgetList(&(widget->children), meta->players);
    }
    else
    {
        error("failed to create a player list widget");
    }
    row->is_selected = true;
    table = (MetaTableWidget *)meta->table->wid_info;
    if (table->selected)
        table->selected->is_selected = false;
    table->selected = row;
}

static server_info_t *GetSelectedServer_MetaWidget(GLWidget *widget)
{
    MetaWidget *meta;

    if (widget->WIDGET != METAWIDGET)
    {
        error("expected METAWIDGET got [%d]", widget->WIDGET);
        return nullptr;
    }
    meta = (MetaWidget *)widget->wid_info;
    return ((MetaTableWidget *)meta->table->wid_info)->selected->sip;
}

static void Paint_MetaRowWidget(GLWidget *widget)
{
    SDL_Rect *b;
    MetaRowWidget *row;

    if (widget->WIDGET != METAROWWIDGET)
    {
        error("expected METAROWWIDGET got [%d]", widget->WIDGET);
        return;
    }

    b = &(widget->bounds);
    row = (MetaRowWidget *)widget->wid_info;
    set_alphacolor(row->is_selected ? SELECTED_BG : row->bg);

    glBegin(GL_QUADS);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();
}

static void SetBounds_MetaRowWidget(GLWidget *row, SDL_Rect *rb)
{
    int free_width;
    SDL_Rect cb;
    GLWidget *col;

    row->bounds = *rb;
    free_width = MAX(rb->w - (VERSION_WIDTH + COUNT_WIDTH), 0);

    if (!(col = row->children))
        return;
    cb.x = rb->x;
    cb.w = free_width / 2;
    cb.y = rb->y;
    cb.h = rb->h;
    SetBounds_GLWidget(col, &cb);

    if (!(col = col->next))
        return;
    cb.x = cb.x + cb.w;
    cb.y = rb->y;
    cb.h = rb->h;
    SetBounds_GLWidget(col, &cb);

    if (!(col = col->next))
        return;
    cb.x = cb.x + cb.w;
    cb.y = rb->y;
    cb.h = rb->h;
    cb.w = VERSION_WIDTH;
    SetBounds_GLWidget(col, &cb);

    if (!(col = col->next))
        return;
    cb.x = cb.x + cb.w;
    cb.y = rb->y;
    cb.h = rb->h;
    cb.w = COUNT_WIDTH;
    SetBounds_GLWidget(col, &cb);
}

static void Button_MetaRowWidget(Uint8 button, Uint8 state, Uint16 x,
                                 Uint16 y, void *data)
{
    GLWidget *widget;
    MetaRowWidget *row;
    SDL_Event evt;

    if (state != SDL_PRESSED)
        return;
    if (button != 1)
        return;

    widget = (GLWidget *)data;
    if (widget->WIDGET != METAROWWIDGET)
    {
        error("expected METAROWWIDGET got [%d]", widget->WIDGET);
        return;
    }

    row = (MetaRowWidget *)widget->wid_info;
    if (row->is_selected)
    {
        evt.type = SDL_USEREVENT;
        evt.user.code = EVENT_JOIN;
        evt.user.data1 = row->sip;
        SDL_PushEvent(&evt);
    }
    else
    {
        SelectRow_MetaWidget(row->table->meta, row);
#if 0
    printf("version: %s\n", row->sip->version.c_str());
    printf("hostname: %s\n", row->sip->hostname.c_str());
    printf("users_str: %s\n", row->sip->users_str.c_str());
    printf("mapname: %s\n", row->sip->mapname.c_str());
    printf("mapsize: %s\n", row->sip->mapsize.c_str());
    printf("author: %s\n", row->sip->author.c_str());
    printf("status: %s\n", row->sip->status.c_str());
    printf("bases_str: %s\n", row->sip->bases_str.c_str());
    printf("fps_str: %s\n", row->sip->fps_str.c_str());
    printf("playlist: %s\n", row->sip->playlist.c_str());
    printf("sound: %s\n", row->sip->sound.c_str());
    printf("teambases_str: %s\n", row->sip->teambases_str.c_str());
    printf("timing: %s\n", row->sip->timing.c_str());
    printf("ip_str: %s\n", row->sip->ip_str.c_str());
    printf("freebases: %s\n", row->sip->freebases.c_str());
    printf("queue_str: %s\n", row->sip->queue_str.c_str());
    printf("domain: %s\n", row->sip->domain.c_str());
    printf("pingtime_str: %s\n", row->sip->pingtime_str.c_str());
    printf("port: %u\n", row->sip->port);
    printf("ip: %u\n", row->sip->ip);
    printf("users: %u\n", row->sip->users);
    printf("bases: %u\n", row->sip->bases);
    printf("fps: %u\n", row->sip->fps);
    printf("uptime: %u\n", row->sip->uptime);
    printf("queue: %u\n", row->sip->queue);
    printf("pingtime: %u\n", row->sip->pingtime);
    printf("serial: %c\n", row->sip->serial);
#endif
    }
}

static GLWidget *Init_MetaRowWidget(server_info_t *sip,
                                    MetaTableWidget *table,
                                    bool is_selected,
                                    unsigned int bg)
{
    GLWidget *tmp, *col;
    MetaRowWidget *row;

    if (!(tmp = Init_EmptyBaseGLWidget()))
    {
        error("Widget init failed");
        return nullptr;
    }
    if (!(row = (MetaRowWidget *)malloc(sizeof(MetaRowWidget))))
    {
        error("out of memory");
        free(tmp);
        return nullptr;
    }
    row->fg = ROW_FG;
    row->bg = bg;
    row->sip = sip;
    row->table = table;
    row->is_selected = is_selected;
    tmp->wid_info = row;
    tmp->WIDGET = METAROWWIDGET;
    tmp->Draw = Paint_MetaRowWidget;
    tmp->SetBounds = SetBounds_MetaRowWidget;
    tmp->button = Button_MetaRowWidget;
    tmp->buttondata = tmp;

#define COLUMN(TEXT)                                                         \
    if ((col = Init_LabelWidget((TEXT), &(row->fg), nullptr, LEFT, CENTER))) \
    {                                                                        \
        col->button = Button_MetaRowWidget;                                  \
        col->buttondata = tmp;                                               \
        AppendGLWidgetList(&(tmp->children), col);                           \
    }
    COLUMN(sip->hostname.c_str());
    COLUMN(sip->mapname.c_str());
    COLUMN(sip->version.c_str());
    COLUMN(sip->users_str.c_str());
#undef COLUMN

    return tmp;
}

static GLWidget *Init_MetaHeaderWidget(void)
{
    GLWidget *tmp, *col;
    MetaHeaderWidget *info;

    if (!(tmp = Init_EmptyBaseGLWidget()))
    {
        error("Widget init failed");
        return nullptr;
    }
    if (!(info = (MetaHeaderWidget *)malloc(sizeof(MetaHeaderWidget))))
    {
        error("out of memory");
        free(tmp);
        return nullptr;
    }
    info->fg = HEADER_FG;
    info->bg = HEADER_BG;
    tmp->wid_info = info;
    tmp->WIDGET = METAHEADERWIDGET;
    tmp->SetBounds = SetBounds_MetaRowWidget;

#define HEADER(TEXT)                                                              \
    if ((col = Init_LabelWidget((TEXT), &(info->fg), &(info->bg), LEFT, CENTER))) \
    {                                                                             \
        AppendGLWidgetList(&(tmp->children), col);                                \
    }
    HEADER("Server");
    HEADER("Map");
    HEADER("Version");
    HEADER("Pl");
#undef COLUMN

    return tmp;
}

static void Scroll_MetaTableWidget(GLfloat pos, void *data)
{
    MetaTableWidget *info;
    GLWidget *widget, *row;
    SDL_Rect b;
    int y;

    widget = (GLWidget *)data;
    if (widget->WIDGET != METATABLEWIDGET)
    {
        error("expected METATABLEWIDGET got [%d]", widget->WIDGET);
        return;
    }

    info = (MetaTableWidget *)widget->wid_info;
    y = widget->bounds.y + ROW_HEIGHT - (int)(info->server_list.size() * ROW_HEIGHT * pos);

    for (row = widget->children; row; row = row->next)
    {
        if (row->WIDGET == METAROWWIDGET)
        {
            b = row->bounds;
            b.y = y;
            SetBounds_GLWidget(row, &b);
            y += ROW_HEIGHT;
        }
    }
}

static void SetBounds_MetaTableWidget(GLWidget *widget, SDL_Rect *b)
{
    int y;
    GLWidget *row;
    MetaTableWidget *info;
    GLfloat table_height;
    SDL_Rect *wb, sb, rb, hb;

    if (widget->WIDGET != METATABLEWIDGET)
    {
        error("expected METATABLEWIDGET got [%d]", widget->WIDGET);
        return;
    }

    widget->bounds = *b;
    info = (MetaTableWidget *)widget->wid_info;
    table_height = info->server_list.size() * ROW_HEIGHT;

    if (info->scrollbar != nullptr)
    {
        DelGLWidgetListItem(&(widget->children), info->scrollbar);
        Close_Widget(&(info->scrollbar));
        info->scrollbar = nullptr;
    }

    y = b->y + ROW_HEIGHT;
    for (row = widget->children; row; row = row->next)
    {
        if (row->WIDGET == METAROWWIDGET)
        {
            rb.x = b->x;
            rb.y = y;
            rb.w = b->w - 10;
            rb.h = ROW_HEIGHT;
            SetBounds_GLWidget(row, &rb);
            y += ROW_HEIGHT;
        }
    }

    if (table_height > b->h)
    {
        info->scrollbar =
            Init_ScrollbarWidget(false, 0.0f, ((GLfloat)b->h) / table_height,
                                 SB_VERTICAL, Scroll_MetaTableWidget, widget);
        if (info->scrollbar != nullptr)
        {
            wb = &(widget->bounds);
            sb.x = wb->x + wb->w - 10;
            sb.y = wb->y + ROW_HEIGHT;
            sb.w = 10;
            sb.h = wb->h - ROW_HEIGHT;
            SetBounds_GLWidget(info->scrollbar, &sb);
            AppendGLWidgetList(&(widget->children), info->scrollbar);
        }
        else
        {
            error("failed to create a scroll bar for meta table");
            return;
        }
    }
    if (info->header != nullptr)
    {
        hb.x = widget->bounds.x;
        hb.y = widget->bounds.y - 1; /* don't ask why */
        hb.w = widget->bounds.w - 10;
        hb.h = ROW_HEIGHT;
        SetBounds_GLWidget(info->header, &hb);
    }
}

static GLWidget *Init_MetaTableWidget(GLWidget *meta, const server_list_t &servers)
{
    GLWidget *tmp, *row;
    MetaTableWidget *info;
    bool bg = true;

    if (!(tmp = Init_EmptyBaseGLWidget()))
    {
        error("Widget init failed");
        return nullptr;
    }
    if (!(info = (MetaTableWidget *)malloc(sizeof(MetaTableWidget))))
    {
        error("out of memory");
        free(tmp);
        return nullptr;
    }
    new (&info->server_list) server_list_t();

    info->meta = meta;
    info->server_list = servers;
    info->scrollbar = nullptr;
    info->header = nullptr;
    info->selected = nullptr;
    info->first_row = nullptr;

    tmp->wid_info = info;
    tmp->WIDGET = METATABLEWIDGET;
    tmp->SetBounds = SetBounds_MetaTableWidget;

    for (server_info_t *sip : servers)
    {
        row = Init_MetaRowWidget(sip, info, false, bg ? ROW_BG1 : ROW_BG2);
        if (!row)
            break;
        if (info->first_row == nullptr)
            info->first_row = (MetaRowWidget *)row->wid_info;
        AppendGLWidgetList(&(tmp->children), row);
        bg = !bg;
    }
    if ((info->header = Init_MetaHeaderWidget()))
    {
        AppendGLWidgetList(&(tmp->children), info->header);
    }
    else
    {
        error("failed to create a header row for meta table");
    }

    return tmp;
}

static void Paint_MetaWidget(GLWidget *widget)
{
    MetaWidget *info;
    SDL_Rect *b;

    if (widget->WIDGET != METAWIDGET)
    {
        error("expected METAWIDGET got [%d]", widget->WIDGET);
        return;
    }
    info = (MetaWidget *)widget->wid_info;
    if (info->texture == 0)
        return;

    b = &(widget->bounds);
    glColor4ub(255, 255, 255, 255);
    glBindTexture(GL_TEXTURE_2D, info->texture);
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glTexCoord2f(info->txc.MinX, info->txc.MinY);
    glVertex2i(b->x, b->y);
    glTexCoord2f(info->txc.MaxX, info->txc.MinY);
    glVertex2i(b->x + b->w, b->y);
    glTexCoord2f(info->txc.MaxX, info->txc.MaxY);
    glVertex2i(b->x + b->w, b->y + b->h);
    /* keep original (even though it's suspicious) to avoid changing behavior */
    // TODO: fix the (very likely) typo in Paint_MetaWidget() where the last glTexCoord2f uses info->txc.MinY instead of MinX
    glTexCoord2f(info->txc.MinY, info->txc.MaxY);
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

static void Close_MetaWidget(GLWidget *widget)
{
    MetaWidget *info;

    if (widget->WIDGET != METAWIDGET)
    {
        error("expected METAWIDGET got [%d]", widget->WIDGET);
        return;
    }
    info = (MetaWidget *)widget->wid_info;
    if (info->texture)
        glDeleteTextures(1, &(info->texture));
}

static void OnClick_Join(GLWidget *widget)
{
    SDL_Event evt;
    evt.type = SDL_USEREVENT;
    evt.user.code = EVENT_JOIN;
    evt.user.data1 = nullptr;
    SDL_PushEvent(&evt);
}

static void OnClick_Refresh(GLWidget *widget)
{
    SDL_Event evt;
    evt.type = SDL_USEREVENT;
    evt.user.code = EVENT_REFRESH;
    evt.user.data1 = nullptr;
    SDL_PushEvent(&evt);
}

static void OnClick_Quit(GLWidget *widget)
{
    SDL_Event evt;
    evt.type = SDL_QUIT;
    SDL_PushEvent(&evt);
}

static GLWidget *Init_MetaWidget(const server_list_t &servers)
{
    GLWidget *tmp;
    MetaWidget *info;
    MetaTableWidget *table;
    SDL_Rect table_bounds;

    if (!(tmp = Init_EmptyBaseGLWidget()))
    {
        error("Widget init failed");
        return nullptr;
    }
    if (!(info = (MetaWidget *)malloc(sizeof(MetaWidget))))
    {
        error("out of memory");
        free(tmp);
        return nullptr;
    }
    info->table = nullptr;
    info->status = nullptr;
    info->players = nullptr;
    info->texture = 0;
    tmp->WIDGET = METAWIDGET;
    tmp->bounds.x = (draw_width - META_WIDTH) / 2;
    tmp->bounds.y = (draw_height - META_HEIGHT) / 2;
    tmp->bounds.w = META_WIDTH;
    tmp->bounds.h = META_HEIGHT;
    tmp->wid_info = info;
    tmp->Draw = Paint_MetaWidget;
    tmp->Close = Close_MetaWidget;

    if (!(info->table = Init_MetaTableWidget(tmp, servers)))
    {
        free(tmp);
        return nullptr;
    }
    table = (MetaTableWidget *)info->table->wid_info;
    table_bounds.x = tmp->bounds.x + 20;
    table_bounds.y = tmp->bounds.y + 64;
    table_bounds.w = 796;
    table_bounds.h = 514;
    SetBounds_GLWidget(info->table, &table_bounds);
    AppendGLWidgetList(&(tmp->children), info->table);

#ifdef HAVE_SDL_IMAGE
    SDL_Surface *surface = IMG_Load(CONF_TEXTUREDIR "sdlmetabg.png");
    if (surface)
    {
        info->texture = SDL_GL_LoadTexture(surface, &(info->txc));
        SDL_FreeSurface(surface);
    }
#endif

    if (table->first_row != nullptr)
        SelectRow_MetaWidget(tmp, table->first_row);

    return tmp;
}

static bool join_server(Connect_param_t *conpar, server_info_t *sip)
{
    char *server_addr_ptr = conpar->server_addr;
    strlcpy(conpar->server_name, sip->hostname.c_str(),
            sizeof(conpar->server_name));
    strlcpy(conpar->server_addr, sip->ip_str.c_str(), sizeof(conpar->server_addr));
    conpar->contact_port = sip->port;
    if (Contact_servers(1, &server_addr_ptr, 1, 0, 0, nullptr,
                        0, nullptr, nullptr, nullptr, nullptr, conpar))
    {
        return true;
    }
    printf("Server %s (%s) didn't respond on port %d\n",
           conpar->server_name, conpar->server_addr,
           conpar->contact_port);
    return false;
}

static void handleKeyPress(GLWidget *meta, SDL_Keysym *keysym)
{
    /*static unsigned int row = 1;*/
    SDL_Event evt;

    switch (keysym->sym)
    {
    case SDLK_ESCAPE:
        /* ESC key was pressed */
        evt.type = SDL_QUIT;
        SDL_PushEvent(&evt);
        break;
    case SDLK_F11:
        /* F11 key was pressed
         * this toggles fullscreen mode
         */

        /* This segfaults */
        /* SDL_WM_ToggleFullScreen(MainSDLSurface); */

        break;
    case SDLK_UP:
        /* move the cursor up */
        break;
    case SDLK_DOWN:
        /* move the curor down */
        break;
    default:
        break;
    }

    return;
}

int Meta_window(Connect_param_t *conpar)
{
    int num_serv = 0;
    int btn_x, btn_y, btn_h;
    GLWidget *btn, *root, *meta, *target = nullptr;
    SDL_Event evt;
    server_info_t *server = nullptr;

    if (server_list.empty() ||
        (int)server_list.size() < 10 ||
        server_list_creation_time + 5 < time(nullptr))
    {

        Delete_server_list();
        if ((num_serv = Get_meta_data()) <= 0)
        {
            error("Couldn't get meta list.");
            return -1;
        }
        else
            warn("Got %d servers.", num_serv);
    }

    if (Welcome_sort_server_list() == -1)
    {
        Delete_server_list();
        error("out of memory");
        return -1;
    }

    if (!(root = Init_EmptyBaseGLWidget()))
    {
        error("Widget init failed");
        return -1;
    }
    root->bounds.x = root->bounds.y = 0;
    root->bounds.w = draw_width;
    root->bounds.h = draw_height; /* SDL2 fix: was mistakenly assigning w twice */

    if (!(meta = Init_MetaWidget(server_list)))
    {
        free(root);
        return -1;
    }
    AppendGLWidgetList(&(root->children), meta);

    btn_x = (draw_width - META_WIDTH) / 2 - 80;
    btn_y = (draw_height - META_HEIGHT) / 2 + 60;
    btn = Init_ImageButtonWidget("Join",
                                 "metabtnup.png",
                                 "metabtndown.png",
                                 BUTTON_BG,
                                 BUTTON_FG,
                                 OnClick_Join);
    if (btn)
    {
        btn->bounds.x = btn_x;
        btn->bounds.y = btn_y;
        btn_h = btn->bounds.h;
        btn_y += btn_h + 5;
        AppendGLWidgetList(&(root->children), btn);
    }
    btn = Init_ImageButtonWidget("Refresh",
                                 "metabtnup.png",
                                 "metabtndown.png",
                                 BUTTON_BG,
                                 BUTTON_FG,
                                 OnClick_Refresh);
    if (btn)
    {
        btn->bounds.x = btn_x;
        btn->bounds.y = btn_y;
        btn_h = btn->bounds.h;
        btn_y += btn_h + 5;
        AppendGLWidgetList(&(root->children), btn);
    }
    btn = Init_ImageButtonWidget("Quit",
                                 "metabtnup.png",
                                 "metabtndown.png",
                                 BUTTON_BG,
                                 BUTTON_FG,
                                 OnClick_Quit);
    if (btn)
    {
        btn->bounds.x = btn_x;
        btn->bounds.y = btn_y;
        btn_h = btn->bounds.h;
        btn_y += btn_h + 5;
        AppendGLWidgetList(&(root->children), btn);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, draw_width, draw_height, 0);
    glDisable(GL_BLEND);

    while (1)
    {

        set_alphacolor(blackRGBA);
        glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(draw_width, 0);
        glVertex2i(draw_width, draw_height);
        glVertex2i(0, draw_height);
        glEnd();
        glEnable(GL_SCISSOR_TEST);
        DrawGLWidgetsi(meta, 0, 0, draw_width, draw_height);
        glDisable(GL_SCISSOR_TEST);

        /* SDL2: SDL_GL_SwapBuffers() removed */
        if (gWindow)
            SDL_GL_SwapWindow(gWindow);

        SDL_WaitEvent(&evt);
        do
        {

            switch (evt.type)
            {
            case SDL_QUIT:
                return -1;

            case SDL_USEREVENT:
                if (evt.user.code == EVENT_JOIN)
                {
                    server = (server_info_t *)evt.user.data1;
                    if (server == nullptr)
                        server = GetSelectedServer_MetaWidget(meta);
                    if (!server)
                        break;
                    if (join_server(conpar, server))
                    {
                        Close_Widget(&root);
                        glEnable(GL_BLEND);
                        glMatrixMode(GL_PROJECTION);
                        glLoadIdentity();
                        gluOrtho2D(0, draw_width, 0, draw_height);
                        return 0;
                    }
                }
                else if (evt.user.code == EVENT_REFRESH)
                {
                    Close_Widget(&root);
                    return 1;
                }
                break;

            case SDL_KEYDOWN:
                /* handle key presses */
                handleKeyPress(meta, &evt.key.keysym);
                break;

            case SDL_MOUSEBUTTONDOWN:
                target = FindGLWidgeti(meta, evt.button.x, evt.button.y);
                if (target && target->button)
                    target->button(evt.button.button,
                                   evt.button.state,
                                   evt.button.x,
                                   evt.button.y,
                                   target->buttondata);
                break;

            case SDL_MOUSEBUTTONUP:
                if (target && target->button)
                {
                    target->button(evt.button.button,
                                   evt.button.state,
                                   evt.button.x,
                                   evt.button.y,
                                   target->buttondata);
                    target = nullptr;
                }
                break;

            case SDL_MOUSEMOTION:
                if (target && target->motion)
                    target->motion(evt.motion.xrel,
                                   evt.motion.yrel,
                                   evt.button.x,
                                   evt.button.y,
                                   target->motiondata);
                break;

            /* SDL2 replacement for SDL_VIDEOEXPOSE */
            case SDL_WINDOWEVENT:
                if (evt.window.event == SDL_WINDOWEVENT_EXPOSED ||
                    evt.window.event == SDL_WINDOWEVENT_SHOWN)
                {
                    glDisable(GL_SCISSOR_TEST);
                    set_alphacolor(blackRGBA);
                    glBegin(GL_QUADS);
                    glVertex2i(0, 0);
                    glVertex2i(draw_width, 0);
                    glVertex2i(draw_width, draw_height);
                    glVertex2i(0, draw_height);
                    glEnd();
                    glEnable(GL_SCISSOR_TEST);
                }
                break;
            }
        } while (SDL_PollEvent(&evt));
    }
}
