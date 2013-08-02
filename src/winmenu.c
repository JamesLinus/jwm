/**
 * @file winmenu.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for handling window menus.
 *
*/

#include "jwm.h"
#include "winmenu.h"
#include "client.h"
#include "menu.h"
#include "main.h"
#include "desktop.h"
#include "move.h"
#include "resize.h"
#include "event.h"
#include "cursor.h"
#include "misc.h"
#include "root.h"
#include "settings.h"

static Menu *CreateWindowMenu(const ClientNode *np);
static void CreateWindowLayerMenu(const ClientNode *np, Menu *menu);
static void CreateWindowSendToMenu(const ClientNode *np, Menu *menu);
static void AddWindowMenuItem(Menu *menu,
                              const char *name,
                              ActionType type,
                              const char *value);

/** Get the size of a window menu. */
void GetWindowMenuSize(ClientNode *np, int *width, int *height)
{

   Menu *menu;

   menu = CreateWindowMenu(np);
   InitializeMenu(menu);
   *width = menu->width;
   *height = menu->height;
   DestroyMenu(menu);

}

/** Show a window menu. */
void ShowWindowMenu(ClientNode *np, int x, int y)
{

   Menu *menu;
   ActionContext context;

   InitActionContext(&context);
   context.client = np;
   context.x = x;
   context.y = y;
   context.MoveFunc = MoveClient;
   context.ResizeFunc = ResizeClient;

   menu = CreateWindowMenu(np);

   InitializeMenu(menu);

   ShowMenu(&context, menu);

   DestroyMenu(menu);

}

/** Create a new window menu. */
Menu *CreateWindowMenu(const ClientNode *np)
{

   Menu *menu;

   menu = Allocate(sizeof(Menu));
   menu->itemHeight = 0;
   menu->items = NULL;
   menu->label = NULL;

   /* Note that items are added in reverse order of display. */

   if(!(np->state.status & STAT_WMDIALOG)) {
      AddWindowMenuItem(menu, _("Close"), ACTION_CLOSE, NULL);
      AddWindowMenuItem(menu, _("Kill"), ACTION_KILL, NULL);
      AddWindowMenuItem(menu, NULL, ACTION_NONE, NULL);
   }

   if(!(np->state.status & (STAT_MINIMIZED | STAT_VMAX | STAT_HMAX))) {
      if(np->state.status & (STAT_MAPPED | STAT_SHADED)) {
         if(np->state.border & BORDER_RESIZE) {
            AddWindowMenuItem(menu, _("Resize"), ACTION_RESIZE, NULL);
         }
         if(np->state.border & BORDER_MOVE) {
            AddWindowMenuItem(menu, _("Move"), ACTION_MOVE, NULL);
         }
      }
   }

   if(np->state.status & STAT_MINIMIZED) {
      AddWindowMenuItem(menu, _("Restore"), ACTION_MIN, NULL);
   } else if(np->state.border & BORDER_MIN) {
      AddWindowMenuItem(menu, _("Minimize"), ACTION_MIN, NULL);
   }

   if(np->state.status & STAT_SHADED) {
      AddWindowMenuItem(menu, _("Unshade"), ACTION_SHADE, NULL);
   } else if(np->state.border & BORDER_SHADE) {
      AddWindowMenuItem(menu, _("Shade"), ACTION_SHADE, NULL);
   }

   if((np->state.border & BORDER_MAX)
      && (np->state.status & (STAT_MAPPED | STAT_SHADED))) {

      if(!(np->state.status & (STAT_HMAX | STAT_VMAX))) {
         AddWindowMenuItem(menu, _("Maximize-y"), ACTION_VMAX, NULL);
      }

      if(!(np->state.status & (STAT_HMAX | STAT_VMAX))) {
         AddWindowMenuItem(menu, _("Maximize-x"), ACTION_HMAX, NULL);
      }

      if((np->state.status & (STAT_HMAX | STAT_VMAX))) {
         AddWindowMenuItem(menu, _("Restore"), ACTION_MAX, NULL);
      } else {
         AddWindowMenuItem(menu, _("Maximize"), ACTION_MAX, NULL);
      }

   }

   if(!(np->state.status & STAT_WMDIALOG)) {

      if(np->state.status & STAT_STICKY) {
         AddWindowMenuItem(menu, _("Unstick"), ACTION_STICK, NULL);
      } else {
         AddWindowMenuItem(menu, _("Stick"), ACTION_STICK, NULL);
      }

      CreateWindowLayerMenu(np, menu);

      if(!(np->state.status & STAT_STICKY)) {
         CreateWindowSendToMenu(np, menu);
      }

   }

   return menu;
}

/** Create a window layer submenu. */
void CreateWindowLayerMenu(const ClientNode *np, Menu *menu)
{

   Menu *submenu;
   MenuItem *item;

   item = Allocate(sizeof(MenuItem));
   item->type = MENU_ITEM_SUBMENU;
   item->name = CopyString(_("Layer"));
   item->action.type = ACTION_NONE;
   item->action.str = NULL;
   item->action.value = 0;
   item->iconName = NULL;

   item->next = menu->items;
   menu->items = item;

   submenu = Allocate(sizeof(Menu));
   item->submenu = submenu;
   submenu->itemHeight = 0;
   submenu->items = NULL;
   submenu->label = NULL;

   if(np->state.layer == LAYER_ABOVE) {
      AddWindowMenuItem(submenu, _("[Above]"), ACTION_LAYER, "above");
   } else {
      AddWindowMenuItem(submenu, _("Above"), ACTION_LAYER, "above");
   }
   if(np->state.layer == LAYER_NORMAL) {
      AddWindowMenuItem(submenu, _("[Normal]"), ACTION_LAYER, "normal");
   } else {
      AddWindowMenuItem(submenu, _("Normal"), ACTION_LAYER, "normal");
   }
   if(np->state.layer == LAYER_BELOW) {
      AddWindowMenuItem(submenu, _("[Below]"), ACTION_LAYER, "below");
   } else {
      AddWindowMenuItem(submenu, _("Below"), ACTION_LAYER, "below");
   }

}

/** Create a send to submenu. */
void CreateWindowSendToMenu(const ClientNode *np, Menu *menu)
{

   unsigned int mask;
   unsigned int x;

   mask = 0;
   for(x = 0; x < settings.desktopCount; x++) {
      if(np->state.desktop == x || (np->state.status & STAT_STICKY)) {
         mask |= 1 << x;
      }
   }

   AddWindowMenuItem(menu, _("Send To"), ACTION_NONE, NULL);

   /* Now the first item in the menu is for the desktop list. */
   menu->items->submenu = CreateDesktopMenu(mask);

}

/** Add an item to the current window menu. */
void AddWindowMenuItem(Menu *menu, const char *name,
                       ActionType type, const char *value)
{

   MenuItem *item;

   item = Allocate(sizeof(MenuItem));
   if(name) {
      item->type = MENU_ITEM_NORMAL;
   } else {
      item->type = MENU_ITEM_SEPARATOR;
   }
   item->name = CopyString(name);
   CreateAction(&item->action, type, value);
   item->iconName = NULL;
   item->submenu = NULL;

   item->next = menu->items;
   menu->items = item;

}

/** Select a window for performing an action. */
void ChooseWindow(const ActionContext *context,
                  const ActionNode *action)
{

   XEvent event;
   ClientNode *np;

   GrabMouseForChoose();

   for(;;) {

      WaitForEvent(&event);

      if(event.type == ButtonPress) {
         if(event.xbutton.button == Button1) {
            np = FindClient(event.xbutton.subwindow);
            if(np) {
               RunAction(context, action);
            }
         }
         break;
      } else if(event.type == KeyPress) {
         break;
      }

   }

   JXUngrabPointer(display, CurrentTime);

}

