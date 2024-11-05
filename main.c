#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void Reparent(Display *d, Window child, Window new_parent) {
  XUnmapWindow(d, child);
  XMapWindow(d, new_parent);
  XSync(d, False);
  XReparentWindow(d, child, new_parent, 0, 0);
  XMapWindow(d, child);
  usleep(1e3);
  XSync(d, False);
}

Window focused, new_parent;
Display *d;
int s;
volatile sig_atomic_t done = 0;

// handler to recover focus from the window it was spawned from
void term(int signum) {
  done = 1;

  XClientMessageEvent event; // dummy event to wake up XNextEvent
  memset(&event, 0, sizeof(XClientMessageEvent));
  event.type = ClientMessage;
  event.format = 32;
  XSendEvent(d, new_parent, 0, 0, (XEvent *)&event);
  XFlush(d);
}

int main(int argc, char **argv) {
  assert(argc == 4);

  Display *display = XOpenDisplay(NULL);
  Screen *screen = DefaultScreenOfDisplay(display);
  int dwidth = screen->width;
  int dheight = screen->height;
  XCloseDisplay(display);

  int width;
  int height;

  sscanf(argv[1], "%d", &width);
  sscanf(argv[2], "%d", &height);

  int left = (dwidth - width) / 2;
  int top = (dheight - height) / 2;

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = term;
  sigaction(SIGTERM, &action, NULL);

  d = XOpenDisplay(XDisplayName(NULL));
  s = DefaultScreen(d);

  Window root = RootWindow(d, s);

  int param = 0;
  XGetInputFocus(d, &focused, &param);

  Window child;
  sscanf(argv[3], "%d", &child);

  XSetWindowAttributes attrs = {ParentRelative,
                                0L,
                                0,
                                0L,
                                0,
                                0,
                                Always,
                                0L,
                                0L,
                                False,
                                StructureNotifyMask | ExposureMask |
                                    ButtonPressMask | ButtonReleaseMask,
                                0L,
                                True,
                                0,
                                0};

  attrs.border_pixel = 4288570538; // purple
  int border_width = 4;

  new_parent = XCreateWindow(
      d, DefaultRootWindow(d), left, top, width, height, border_width, 0,
      InputOutput, DefaultVisual(d, DefaultScreen(d)),
      CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attrs);

  XClassHint classhint = {"above_all", "above_all"};
  XSetClassHint(d, new_parent, &classhint);

  XAddToSaveSet(d, child);
  Reparent(d, child, new_parent);

  XSetInputFocus(d, child, RevertToNone, 0);
  XResizeWindow(d, child, width, height);

  XSelectInput(d, new_parent,
               ExposureMask | EnterWindowMask | LeaveWindowMask |
                   FocusChangeMask);

  XEvent e;
  while (!done) {
    XEvent e;
    XNextEvent(d, &e);

    // focus with mouse pointer
    if (e.type == EnterNotify) {
      XSetInputFocus(d, child, RevertToNone, 0);
    }

    // focus with mouse pointer
    if (e.type == LeaveNotify) {
      XSetInputFocus(d, s, RevertToNone, 0);
    }

    // ignore focus with mod key
    if (e.type == FocusOut) {
      XSetInputFocus(d, child, RevertToNone, 0);
      XNextEvent(d, &e); // ignore FocusOut
      XNextEvent(d, &e); // ignore FocusIn
    }
  }

  XSetInputFocus(d, focused, RevertToNone, 0);
  XCloseDisplay(d);

  return 0;
}
