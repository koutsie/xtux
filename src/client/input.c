#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "xtux.h"
#include "client.h"
#include "win.h"
#include "input.h"
#include "image.h" /* For screenshot() */
#include "draw.h"
#include "cl_net.h"
#include "cl_netmsg_send.h"

extern win_t win;
extern client_t client;
extern netmsg_entity my_entity; /* peters code */

char textentry_input_buffer[TEXTMESSAGE_STRLEN];
static int textentry_pos = 0;

/* What key does what, DEFAULT settings */
int key_config[NUM_KEYS] = {
		XK_Left,
		XK_Right,
		XK_Up,
		XK_Down,
		XK_z,
		XK_x,
		XK_space,
		XK_Control_R,
		XK_Control_L,
		XK_Alt_L,
		XK_Shift_L,
		XK_F12,
		XK_F1,
		XK_minus,
		XK_equal,
		XK_Return,
		XK_F5,
		XK_Tab
};

int mouse_config[NUM_MOUSE_BUTTONS] = {
		K_B1,
		K_WEAP_DN,
		K_WEAP_UP
};

static byte keypress = 0; /* Keys pressed at any one time. */
static int turning = 0;

void textentry_key_handle(int key, int action);

void normal_key_handle(int key, int action);

byte get_input(void)
{
	XEvent event;
	int key;
	int action = 0;

	while (XPending(win.d))
	{
		XNextEvent(win.d, &event); /* set event to next event in queue */

		/* windows may have been drawn over, so update it */
		if (event.type == GraphicsExpose || event.type == Expose)
		{
			win.dirty = 1;
			continue;
		}

		key = NoSymbol;

		if (event.type == KeyPress)
			action = PRESSED;
		else if (event.type == KeyRelease)
		{
			action = RELEASED;
			/* Mouse Code: Added Jan 21 2003 */
		} else if (event.type == MotionNotify)
		{
			if (client.mousemode == MOUSEMODE_ROTATE)
			{
				int x_dist, y_dist;

				x_dist = event.xmotion.x - client.view_w / 2;
				y_dist = event.xmotion.y - client.view_h / 2;

				/* Client turn rate is too fast for the mouse */
				client.dir += x_dist * client.turn_rate / 32;

				if (x_dist || y_dist)
					win_center_pointer();  /* Reset Mouse pointer */

			} else if (client.mousemode == MOUSEMODE_POINT)
			{
				int mx, my, h, k, mdx, mdy, ang;
				ent_type_t *et;
				/* mouse processing, Peter Hanelys code
		   turn towards mouse pointer */
				mx = event.xmotion.x;
				my = event.xmotion.y;
				if ((et = entity_type(my_entity.entity_type)) != NULL)
				{
					h = my_entity.x + et->width / 2 - client.screenpos.x;
					k = my_entity.y + et->height / 2 - client.screenpos.y;
					mdx = mx - h;
					mdy = my - k;
					ang = (128 / M_PI) * atan2(mdx, -mdy);
					client.dir = ang;
					/* if distance is large, go towards */
					if (sqrt(mdx * mdx + mdy * mdy) > 150)
						normal_key_handle(key_config[K_FORWARD], PRESSED);
					else /* stop forward motion */
						normal_key_handle(key_config[K_FORWARD], RELEASED);
				}
			}

			continue;
		} else if (event.type == ButtonPress || ButtonRelease)
		{
			if (client.mousemode)
			{
				if (event.type == ButtonPress)
					action = PRESSED;
				else
					action = RELEASED;

				/* Buttons start at 1 */
				if (event.xbutton.button >= 1 &&
					event.xbutton.button <= NUM_MOUSE_BUTTONS)
				{
					/* Fake appropriate keypress to be handled below */
					key = key_config[mouse_config[event.xbutton.button - 1]];
				}
			}
		} else
		{ /* Shouldn't happen, as we've masked out what we don't want */
			printf("Unknown event recieved. Type %d\n", event.type);
			continue;
		}

		if (action)
		{
			if (key == NoSymbol)
				key = XLookupKeysym(&event.xkey, 0);
			/* Go to menu if esc is pressed regardless of mode */
			if (key == XK_Escape)
			{
				if (client.demo == DEMO_PLAY)
					cl_network_disconnect();
				else
				{
					cl_netmsg_send_away();
					cl_net_finish();
				}
				client.state = MENU;
				break; /* Back to main */
			} else if (key == key_config[K_SNIPER])
				client.sniper_mode = (action > 0);
			else if (key == key_config[K_SCREENSHOT] && action == PRESSED)
				screenshot();
			else if (key == key_config[K_NS_TOGGLE] && action == PRESSED)
			{
				if (++client.netstats >= NUM_NETSTATS)
					client.netstats = 0;
			} else if (key == key_config[K_RAD_DEC] && action == PRESSED)
				client.crosshair_radius -= 5;
			else if (key == key_config[K_RAD_INC] && action == PRESSED)
				client.crosshair_radius += 5;
			else if (key == key_config[K_TEXTENTRY] && action == PRESSED)
			{
				if (client.textentry)
				{
					cl_netmsg_send_textmessage(textentry_input_buffer);
					draw_status_bar(); /* Put status bar back */
				} else
				{ /* Start message */
					memset(textentry_input_buffer, 0, textentry_pos);
					textentry_pos = 0;
					draw_textentry();
				}
				client.textentry = !client.textentry; /* Toggle */
				win.dirty = 1;
			} else if (key == key_config[K_SUICIDE] && action == PRESSED)
			{
				cl_netmsg_send_killme();
			} else if (key == key_config[K_FRAGS] && action == PRESSED)
			{
				cl_netmsg_send_query_frags();
			} else
			{ /* Game specific input */
				if (client.textentry)
				{ /* Text entry in status bar */
					XPutBackEvent(win.d, &event);
					textentry_key_handle(key, action);
				} else
				{ /* Normal game input */
					normal_key_handle(key, action);
				}
			}

		}

	} /* Pending events */

	if (client.movement_mode == NORMAL && turning)
		client.dir += turning * (client.sniper_mode ? 2 : client.turn_rate);

	return keypress;

}


void input_clear(void)
{
	XEvent e;

	while (XPending(win.d))
		XNextEvent(win.d, &e);

}

int get_key(void)
{
	XEvent event;
	int key, pressed;
	static int shift = 0;

	while (XPending(win.d))
	{
		XNextEvent(win.d, &event);
		if (event.type == GraphicsExpose || event.type == Expose)
		{
			win.dirty = 1;
			continue;
		}

		if (event.type == KeyPress)
			pressed = 1;
		else if (event.type == KeyRelease)
			pressed = 0;
		else
			continue; /* Not a keypress or a release */

		key = XLookupKeysym(&event.xkey, 0);
		if (key == XK_Shift_L || key == XK_Shift_R)
		{
			shift = pressed;
			continue;
		}

		if (pressed)
		{
			if (shift && isalpha(key))
				return toupper(key);
			else
				return key;
		} else
			continue; /* Don't return key releases */

	}

	return XK_VoidSymbol;

}


void textentry_key_handle(int key, int action)
{

	while ((key = get_key()) != XK_VoidSymbol)
	{
		if (key == XK_BackSpace || key == XK_Delete)
		{
			if (textentry_pos > 0)
			{
				textentry_input_buffer[--textentry_pos] = '\0';
			}
		} else if (textentry_pos < TEXTMESSAGE_STRLEN)
		{
			textentry_input_buffer[textentry_pos++] = key;
		}
	}

	draw_textentry();
	win.dirty = 1;

}


void normal_key_handle(int key, int action)
{
	int i, bit;


	if (client.movement_mode == NORMAL)
	{
		if (key == key_config[K_ACLOCKWISE])
		{
			if (action == PRESSED)
				turning = -1;
			else
				turning = 0;
		} else if (key == key_config[K_CLOCKWISE])
		{
			if (action == PRESSED)
				turning = 1;
			else
				turning = 0;
		}
	} else
	{
		/* Make the turn left/right keys act like move left/right */
		if (key == key_config[K_ACLOCKWISE])
			key = key_config[K_LEFT];
		else if (key == key_config[K_CLOCKWISE])
			key = key_config[K_RIGHT];
	}

	/* The keys that map to the keypress bit mask that is sent to the server */
	for (i = K_FORWARD; i <= K_WEAP_DN; i++)
	{
		bit = 1 << (i - K_FORWARD);
		if (key == key_config[i])
		{
			if (action == PRESSED)
				keypress |= bit;
			else
				keypress &= ~bit;
		}
	}


}
