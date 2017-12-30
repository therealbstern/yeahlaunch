/*************************************************************************/
/*  _____________                                                        */
/* < yeahlaunch  >                                                       */
/*  -------------                                                        */
/*         \   ^__^                                                      */
/*          \  (oo)\_______                                              */
/*             (__)\       )\/\                                          */
/*                 ||----w |                                             */
/*                 ||     ||                                             */
/*                                                                       */
/*  Copyright (C) knorke                                                 */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or modify */
/*  it under the terms of the GNU General Public License as published by */
/*  the Free Software Foundation; either version 2 of the License, or    */
/*  (at your option) any later version.                                  */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/*  GNU General Public License for more details.                         */
/*                                                                       */
/*  You should have received a copy of the GNU General Public License    */
/*  along with this program; if not, write to the Free Software          */
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            */
/*************************************************************************/

#include <X11/Xlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#define draw_label(gc)  XDrawString(dpy, t->win, gc, 3, height - 4, t->label, strlen(t->label))

Display *dpy;
Window root;
int screen;
XFontStruct *font;
GC agc, gc, hgc;
char *opt_font = "fixed";
char *opt_fg = "white";
char *opt_afg = "yellow";
char *opt_bg = "black";
int opt_x = 0;
int opt_step = 3;
int opt_rx = 0;
int height;
int raised = 0;
int width = 0;
XColor afg, fg, bg, dummy;
typedef struct tab tab;
struct tab {
    tab *prev, *next;
    char *label;
    char *cmd;
    int x, width;
    Window win;

};
tab *head_tab = NULL;

void make_new_tab(char *cmd, char *label);
tab *find_tab(Window w);
void spawn(char *cmd);
void handle_buttonpress(XEvent event);
void hide(void);
int main(int argc, char *argv[])
{
    XEvent event;
    XGCValues gv;
    int i;
    tab *t;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, " can't open dpy %s", XDisplayName(NULL));
    }
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-fn") && i + 1 < argc)
            opt_font = argv[++i];

        else if (!strcmp(argv[i], "-fg") && i + 1 < argc)
            opt_fg = argv[++i];
        else if (!strcmp(argv[i], "-afg") && i + 1 < argc)
            opt_afg = argv[++i];
        else if (!strcmp(argv[i], "-bg") && i + 1 < argc)
            opt_bg = argv[++i];
        else if (!strcmp(argv[i], "-x") && i + 1 < argc)
            opt_x = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-rx") && i + 1 < argc)
            opt_rx = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-step") && i + 1 < argc)
            opt_step = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-h")) {
            printf("usage: yehalaunch label1 cmd1 label2 cmd2 ...\n"
                   "options:\n" "-fg color\n" "-afg color\n" "-bg color\n" "-fn font name\n" "-x xoffset\n"
                   "-rx xoffset(right aligned)\n""-step step size\n");
            exit(0);
        }

    }
    font = XLoadQueryFont(dpy, opt_font);
    height = font-> /* max_bounds. */ ascent + font-> /* max_bounds. */ descent + 3;
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fg, &fg, &dummy);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_afg, &afg, &dummy);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_bg, &bg, &dummy);

    gv.font = font->fid;
    gv.foreground = fg.pixel;
    gv.function = GXcopy;
    gc = XCreateGC(dpy, root, GCFunction | GCForeground | GCFont, &gv);
    gv.foreground = afg.pixel;
    agc = XCreateGC(dpy, root, GCFunction | GCForeground | GCFont, &gv);

    for (i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "-", 1) && i + 1 < argc)
            i++;
        else if (i + 1 < argc)
            make_new_tab(argv[++i], argv[i]);
    }
    for (t = head_tab; t; t = t->next)
        width += t->width;
    if (opt_rx) {
        opt_x = opt_rx - width;
        i = opt_rx - head_tab->width;
        for (t = head_tab;; t = t->next) {
            t->x = i;

            XMoveWindow(dpy, t->win, t->x, -height + 1);
            if (t->next)
                i -= t->next->width;
            else
                break;
        }
    }
    while (1) {
        XNextEvent(dpy, &event);
        switch (event.type) {
        case EnterNotify:

            if (!raised) {
                for (t = head_tab; t; t = t->next) {
                    XRaiseWindow(dpy, t->win);

                }
                raised++;
            }
            i = 0;

            for (t = find_tab(event.xcrossing.window); t; t = t->next) {
                XMoveWindow(dpy, t->win, t->x, i);
                i -= opt_step;
            }
            i = 0;
            for (t = find_tab(event.xcrossing.window); t; t = t->prev) {
                XMoveWindow(dpy, t->win, t->x, i);
                i -= opt_step;
            }

            break;
        case LeaveNotify:

            if (event.xcrossing.y >= height - opt_step || event.xcrossing.x_root <= opt_x
                || event.xcrossing.x_root >= opt_x + width)

                hide();
            break;
        case Expose:
            if (event.xexpose.count == 0) {
                t = find_tab(event.xexpose.window);
                draw_label(gc);
                XFlush(dpy);
            }

            break;
        case ButtonPress:
            if (event.xbutton.button == Button1 || event.xbutton.button == Button3)
                handle_buttonpress(event);
        }
    }
    return 0;
}

void make_new_tab(char *cmd, char *label)
{
    XSetWindowAttributes attrib;
    attrib.override_redirect = True;
    attrib.background_pixel = bg.pixel;

    tab *t = (tab *) malloc(sizeof(tab));
    if (t) {
        if (!head_tab) {
            t->next = t->prev = NULL;
            t->x = opt_x;
            head_tab = t;
        } else {
            t->next = head_tab;
            t->prev = NULL;
            head_tab->prev = t;
            t->x = head_tab->x + head_tab->width;
            head_tab = t;
        }
        head_tab->label = label;
        head_tab->cmd = cmd;
        head_tab->width = XTextWidth(font, head_tab->label, strlen(head_tab->label)) + 6;
        head_tab->win = XCreateWindow(dpy, root,
                                      head_tab->x, -height + 1, head_tab->width, height,
                                      0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel,
                                      &attrib);
        XSelectInput(dpy, head_tab->win,
                     EnterWindowMask | LeaveWindowMask | ButtonPressMask | ButtonReleaseMask | ExposureMask);
        XMapWindow(dpy, head_tab->win);
    } else
        fprintf(stderr, "yeahlaunch: malloc() failed!");
}

tab *find_tab(Window w)
{
    tab *t;

    for (t = head_tab; t; t = t->next)
        if (w == t->win)
            return t;
    return NULL;
}

void spawn(char *cmd)
{
    pid_t pid;

    if (!(pid = fork())) {
        setsid();
        switch (fork()) {

        case 0:
            execlp("/bin/sh", "sh", "-c", cmd, NULL);
        default:
            _exit(0);
        }
    }
    if (pid > 0)
        wait(NULL);
}

void handle_buttonpress(XEvent event)
{
    tab *t;
    int in = 1;
    t = find_tab(event.xbutton.window);
    XDrawString(dpy, t->win, agc, 3, height - 4, t->label, strlen(t->label));

    do {
        XMaskEvent(dpy, ButtonReleaseMask | LeaveWindowMask | EnterWindowMask, &event);
        if (event.type == LeaveNotify) {
            draw_label(gc);
            in--;
        } else if (event.type == EnterNotify && find_tab(event.xcrossing.window) == t) {
            draw_label(agc);
            in++;
        }
    } while (event.type != ButtonRelease);
    if (in) {
        spawn(t->cmd);
        /* for (in = 0; in < 3; in++) {
           draw_label(agc);
           XSync(dpy, False);
           usleep(10000);
           draw_label(gc);
           XSync(dpy, False);
           usleep(10000); 

           } */
        if (event.xbutton.button == Button3)
            hide();
    }
    draw_label(gc);
}

void hide()
{
    tab *t;
    for (t = head_tab; t; t = t->next) {
        XMoveWindow(dpy, t->win, t->x, -height + 1);
        // XLowerWindow(dpy, t->win);
    }
    raised = 0;
}
