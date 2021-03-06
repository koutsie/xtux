/* This was never designed to become so large! */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

/* TODO: work on this later, but maybe with SDL??
 * #include <soxr.h>
 * - kouts */

#include "xtux.h"
#include "client.h"
#include "input.h"
#include "win.h"
#include "menu.h"
#include "cl_net.h"
#include "entity.h"
#include "intro.h"
#include "image.h"
#include "draw.h"
#include "text_buffer.h"

#define MENU_Y(a) (client.view_h - font_height(a)*(menu[m_level].items + 2))

extern client_t client;
extern win_t win;

extern int num_entity_types;
extern int key_config[NUM_KEYS]; /* From input.c */
extern int mouse_config[NUM_MOUSE_BUTTONS];

string_node *maplist_root, *maplist_tail, *maplist_current;

enum
{
	M_EXIT = -1,
	/* Main menu */
	M_MAIN,
	M_PLAY,
	M_PLAYER_SETUP,
	M_OPTIONS,
	M_QUIT,
	/* second level menu's */
	M_SELECT_MAP,
	M_SELECT_CHAR,
	M_KEY_CONFIG,
	M_DO_CREDITS,
	NUM_MENU_ITEMS
} m_level;


/* Used to check valid character input of a string */
typedef enum
{
	SC_NONE,
	SC_URL,
	SC_NUM
} str_check;

/* Menu drawing routines */
static void menu_draw(void);

static void draw_menu_default(int font);

static void draw_menu_main(void);

static void draw_menu_play(void);

static void draw_menu_player_setup(void);

static void draw_menu_options(void);

static void draw_menu_select_map(void);

static void draw_menu_select_char(void);

static void draw_menu_key_config(void);

/* Menu key-handling routines */
static void menu_key(int key);

static void menu_main(int key);

static void menu_play(int key);

static void menu_player_setup(int key);

static void menu_options(int key);

static void menu_quit(int key);

static void menu_select_map(int key);

static void menu_select_char(int key);

static void menu_key_config(int key);

/* Misc menu functions */
static char *menu_read_text(int x, int y, int w, int h, str_check check);

static int max_text_width(char *strp[], int num, int font_type);

/* we can afford a bit more screen real-estate
 * was: 512x384
 * - kouts */
#define MENU_WIN_WIDTH 1088
#define MENU_WIN_HEIGHT 512

void menu_driver(void)
{
	image_t *img;
	int key, menuactive;
	msec_t timeout;

	client.desired_w = client.view_w;
	client.desired_h = client.view_h;

	client.state = MENU;
	client.screenpos.x = 0;
	client.screenpos.y = 0;
	client.view_w = MENU_WIN_WIDTH;
	client.view_h = MENU_WIN_HEIGHT;
	win_set_properties("XTux Menu", client.view_w, client.view_h);
	wait_till_expose(5);
	m_level = M_MAIN;

	/* Background images */
	img = image("title2.xpm", NOMASK);

	win.dirty = 1;
	timeout = gettime() + M_SEC * 10;

	menuactive = client.connected;
	text_buf_clear();

	/* Menu event loop */
	while (client.state == MENU)
	{

		if (!client.connected && gettime() > timeout)
		{
			menuactive = 0;
			do_intro();
			timeout = gettime() + M_SEC * 15;
			win.dirty = 1;
		}

		if ((key = get_key()) != XK_VoidSymbol)
		{
			input_clear();
			if (key == key_config[K_SCREENSHOT])
				screenshot();
			else if (menuactive)
			{
				menu_key(key);      /* Handle input for menu */
				if (m_level == M_EXIT)
				{
					break; /* Back to main loop */
				}
			}
			win.dirty = 1;
			menuactive = 1;
			timeout = gettime() + M_SEC * 30;
		}

		/* Redraw menu */
		if (win.dirty)
		{
			win.dirty = 0;
			blit(img->pixmap, win.buf, 0, 0, MENU_WIN_WIDTH, MENU_WIN_HEIGHT);

			if (!menuactive)
				win_center_print(win.buf, "<press any key>", 340, 2, "white");
			else
			{
				interlace(win.buf, MENU_WIN_WIDTH, MENU_WIN_HEIGHT, 2, "black");
				menu_draw();
			}
			text_buf_update();
			win_update();
		}

		delay(M_SEC / 20);

	}

	if (client.demo != DEMO_PLAY)
	{
		client.view_w = client.desired_w;
		client.view_h = client.desired_h;
	}

}

#define MAIN_ITEMS 4
#define PLAY_ITEMS 8
#define PLAYER_SETUP_ITEMS 8
#define OPTIONS_ITEMS 10
#define QUIT_ITEMS 2
#define KEY_CONFIG_ITEMS NUM_KEYS + 4 /* extra is for mouse 1-3 + "back" */

static menu_t menu[NUM_MENU_ITEMS];

static char *m_main_str[MAIN_ITEMS + 1] = {
		"Play",
		"Player Setup",
		"Options",
		"Quit",
		NULL
};

static char *m_play_str[PLAY_ITEMS + 1] = {
		"Start a New Game",
		"Game type",
		"Select Map",
		"Hostname",
		"Port",
		"Connect",
		"Disconnect",
		"Back to Main Menu",
		NULL
};

static char *m_player_setup_str[PLAYER_SETUP_ITEMS + 1] = {
		"Name",
		"Select Character",
		"Color 1",
		"Color 2",
		"Movement mode",
		"Turn rate",
		"Crosshair radius",
		"Back to Main Menu",
		NULL
};

static char *m_options_str[OPTIONS_ITEMS + 1] = {
		"Configure Keys",
		"Mouse Mode",
		"Screen Size",
		"Menu interlacing",
		"Status bar",
		"Extra particles",
		"Netstats",
		"Loadscreen",
		"Debug mode",
		"Back to Main Menu",
		NULL
};


static char *m_quit_str[QUIT_ITEMS + 1] = {
		"Really quit?",
		"Back to Main Menu",
		NULL
};

/* Second level menus */
static char *m_key_config_str[KEY_CONFIG_ITEMS + 1] = {
		"Turn Left:",
		"Turn Right:",
		"Forward:",
		"Back:",
		"Move Left:",
		"Move Right:",
		"Button 1:",
		"Button 2:",
		"Weapon up:",
		"Weapon down:",
		"Sniper mode:",
		"Screenshot:",
		"Netstats toggle:",
		"--Target:",
		"++Target:",
		"Textentry Toggle:",
		"Suicide:",
		"Show Frags:",
		"Mouse 1:",
		"Mouse 2:",
		"Mouse 3:",
		"Back",
		NULL
};


void menu_init(void)
{
	int i;
	/* Default is 0 or NULL for all elements in array */
	memset(menu, 0, sizeof(menu_t) * NUM_MENU_ITEMS);

	/* Main Menu Items */
	menu[M_MAIN].item_names = m_main_str;
	menu[M_MAIN].items = MAIN_ITEMS;

	menu[M_PLAY].item_names = m_play_str;
	menu[M_PLAY].items = PLAY_ITEMS;

	menu[M_PLAYER_SETUP].item_names = m_player_setup_str;
	menu[M_PLAYER_SETUP].items = PLAYER_SETUP_ITEMS;

	menu[M_OPTIONS].item_names = m_options_str;
	menu[M_OPTIONS].items = OPTIONS_ITEMS;

	menu[M_QUIT].item_names = m_quit_str;
	menu[M_QUIT].items = QUIT_ITEMS;

	/* Secondary menu items */
	menu[M_KEY_CONFIG].item_names = m_key_config_str;
	menu[M_KEY_CONFIG].items = KEY_CONFIG_ITEMS;

	menu[M_SELECT_CHAR].pos = client.entity_type;

	for (i = 0; i < NUM_MENU_ITEMS; i++)
		menu[i].width = max_text_width(menu[i].item_names, menu[i].items, 3);

	maplist_root = map_make_filename_list();
	maplist_current = maplist_root;

	/* Set tail to end of list */
	maplist_tail = maplist_root;
	while (maplist_tail->next)
		maplist_tail = maplist_tail->next;

}


static void menu_draw(void)
{

	switch (m_level)
	{
		case M_EXIT:
			break;
		case M_MAIN:
			draw_menu_main();
			break;
		case M_PLAY:
			draw_menu_play();
			break;
		case M_PLAYER_SETUP:
			draw_menu_player_setup();
			break;
		case M_OPTIONS:
			draw_menu_options();
			break;
		case M_SELECT_MAP:
			draw_menu_select_map();
			break;
		case M_SELECT_CHAR:
			draw_menu_select_char();
			break;
		case M_KEY_CONFIG:
			draw_menu_key_config();
			break;
		default:
			draw_menu_default(3);
	}

}

/* Draws according to global m_level */
static void draw_menu_default(int font)
{
	int x, y, i;
	char **str;

	x = (MENU_WIN_WIDTH - menu[m_level].width) / 2;
	y = MENU_Y(font);
	str = menu[m_level].item_names;

	for (i = 0; str[i]; i++)
	{
		win_print(win.buf, str[i], x, y, font,
				  i == menu[m_level].pos ? "white" : "grey");
		y += font_height(font);
	}

}


static void draw_menu_main(void)
{
	char str[64];

	draw_menu_default(3);

	/* Remind user that they are connected */
	if (client.connected)
	{
		snprintf(str, 64,
				 "Connected to %s. Press <ESC> to return to game",
				 client.server_address);
		win_center_print(win.buf, str, 0, 2, "white");
	} else
	{
		/* show version
		 * - kouts */
		snprintf(str, 64, "%s",
				 VERSION);
		win_center_print(win.buf, str, 0, 2, "pink");
	}
}


static char *gamemodes[NUM_GAME_MODES] = {
		"Save the World",
		"Holy War"
};

static void draw_menu_play(void)
{
	int x, y, i;
	char str[20];
	char color[8];

	draw_menu_default(3);

	i = menu[m_level].pos;
	x = (MENU_WIN_WIDTH - menu[m_level].width) / 2;
	x += text_width(menu[m_level].item_names[1], 3) + 80;
	y = MENU_Y(3) + font_height(3) - font_height(1);


	/* Indicate whether we are connected or not */
	if (client.connected)
	{
		strncpy(str, "Connected.", 20);
		strncpy(color, "green", 8);
	} else
	{
		strncpy(str, "Not Connected.", 20);
		strncpy(color, "red", 8);
	}

	y += font_height(3);
	snprintf(str, 20, "< %s >", gamemodes[client.gamemode]);
	/*strncpy( str, gamemodes[client.gamemode], 16 ); */
	win_print(win.buf, str, x, y, 1, i == 1 ? "white" : "grey");
	y += font_height(3);

	/* Select map */
	y += font_height(3);
	win_print(win.buf, client.server_address, x + 15, y, 1, i == 3 ? "white" : "grey");
	y += font_height(3);

	snprintf(str, 20, "%d", client.port);
	win_print(win.buf, str, x + 15, y, 1, i == 4 ? "white" : "grey");

}


extern char *colortab[NUM_COLORS];

static void draw_menu_player_setup(void)
{
	int x, y, i, fh;
	char buf[12];

	draw_menu_default(3);
	fh = font_height(3);

	i = menu[m_level].pos;
	x = (MENU_WIN_WIDTH - menu[m_level].width) / 2 + menu[m_level].width;
	y = MENU_Y(3) + fh - font_height(1);

	win_print(win.buf, client.player_name, x + 15, y, 1, i == 0 ? "white" : "grey");
	y += fh * 2; /* 2 lines down */

	win_print(win.buf, "<          >", x, y, 1, i == 2 ? "white" : "grey");
	clear_area(win.buf, x + 15, y, 40, 20, colortab[client.color1]);
	y += fh;
	win_print(win.buf, "<          >", x, y, 1, i == 3 ? "white" : "grey");
	clear_area(win.buf, x + 15, y, 40, 20, colortab[client.color2]);
	y += fh;

	snprintf(buf, 12, "< %s >", client.movement_mode == CLASSIC ? "classic" : "normal");
	win_print(win.buf, buf, x, y, 1, i == 4 ? "white" : "grey");

	y += fh;
	snprintf(buf, 12, "< %d >", client.turn_rate);
	win_print(win.buf, buf, x, y, 1, i == 5 ? "white" : "grey");

	y += fh;
	snprintf(buf, 12, "< %d >", client.crosshair_radius);
	win_print(win.buf, buf, x, y, 1, i == 6 ? "white" : "grey");

}


static void draw_menu_options(void)
{
	int x, y, i;
	char str[32];
	char buf[32];

	draw_menu_default(3);

	i = menu[m_level].pos;
	x = (MENU_WIN_WIDTH - menu[m_level].width) / 2 + menu[m_level].width;
	y = MENU_Y(3) + font_height(3) - font_height(1);


	switch (client.mousemode)
	{
		case MOUSEMODE_ROTATE:
			strncpy(str, "Rotation", 32);
			break;
		case MOUSEMODE_POINT:
			strncpy(str, "Follow Pointer", 32);
			break;
		default:
			strncpy(str, "Off", 32);
	}

	y += font_height(3);
	snprintf(buf, 32, "< %s >", str);//client.mousemode? "On" : "Off");
	win_print(win.buf, buf, x, y, 1, i == 1 ? "white" : "grey");


	/* Screensize */
	y += font_height(3);
	snprintf(buf, 32, "< %4d %4d >", client.desired_w, client.desired_h);
	win_print(win.buf, buf, x, y, 1, i == 2 ? "white" : "grey");

	/* Menu interlacing */
	y += font_height(3);
	snprintf(buf, 32, "< %s >", client.interlace ? "On" : "Off");
	win_print(win.buf, buf, x, y, 1, i == 3 ? "white" : "grey");

	/* Health bar */
	y += font_height(3);
	snprintf(buf, 32, "< %s >",
			 client.status_type == HEALTHBAR ? "HEALTHBAR" : "NUMBER");
	win_print(win.buf, buf, x, y, 1, i == 4 ? "white" : "grey");

	/* Expire times */
	y += font_height(3);

	if (client.ep_expire == 0)
		strncpy(str, "Disabled", 32);
	else if (client.ep_expire == -1)
		strncpy(str, "Permanent", 32);
	else
		snprintf(str, 32, "%d seconds", client.ep_expire);
	snprintf(buf, 32, "< %s >", str);
	win_print(win.buf, buf, x, y, 1, i == 5 ? "white" : "grey");

	y += font_height(3);
	switch (client.netstats)
	{
		case NS_TOTAL:
			strncpy(str, "Total bytes", 32);
			break;
		case NS_RECENT:
			strncpy(str, "Bytes / Sec", 32);
			break;
		default:
			strncpy(str, "Disabled", 32);
	}

	snprintf(buf, 32, "< %s >", str);
	win_print(win.buf, buf, x, y, 1, i == 6 ? "white" : "grey");

	y += font_height(3);
	snprintf(buf, 32, "< %s >",
			 client.loadscreen ? "Minimap" : "Blueprint");
	win_print(win.buf, buf, x, y, 1, i == 7 ? "white" : "grey");

	y += font_height(3);
	snprintf(buf, 32, "< %s >", client.debug ? "On" : "Off");
	win_print(win.buf, buf, x, y, 1, i == 8 ? "white" : "grey");

}


static void draw_menu_select_map(void)
{

	draw_mini_map(win.buf, maplist_current->string, 0, 0,
				  client.view_w, client.view_h);

}



/* Amount of characters on screen */
#define CHAR_SEL 3

static void draw_menu_select_char(void)
{
	static byte dir, last_type = 0;
	byte type;
	ent_type_t *et;
	netmsg_entity ent;
	int i;
	int selected;

	ent.type = NETMSG_ENTITY;
	ent.mode = ALIVE;
	ent.weapon = 0;
	ent.y = client.view_h / 2;
	ent.img = 0;

	selected = menu[m_level].pos;
	menu_select_char(XK_Left); /* Simulate user pressing LEFT */

	if (selected == last_type)
		dir += 16;
	else
	{
		dir = 128;
		last_type = selected;
	}


	for (i = 0; i < CHAR_SEL; i++)
	{
		type = menu[m_level].pos;
		if ((et = entity_type(type)) == NULL)
			break;
		ent.entity_type = type;
		ent.x = (i + 1) * client.view_w / (CHAR_SEL + 1) - et->width / 2;
		if (type == selected)
			ent.dir = dir;
		else
			ent.dir = 128;
		entity_draw(ent);

		/* Move one keypress right */
		menu_select_char(XK_Right);
	}

	menu[m_level].pos = selected;    /* Put things back as they were */
	if ((et = entity_type(selected)) != NULL)
		win_center_print(win.buf, et->name, client.view_h - font_height(2),
						 2, "white");
	win.dirty = 1;

}


static void draw_menu_key_config(void)
{
	char buf1[32], buf2[32];
	int x, y, i;

	draw_menu_default(2);
	x = (MENU_WIN_WIDTH - menu[m_level].width) / 2 + menu[m_level].width;
	y = MENU_Y(2);

	for (i = 0; i < NUM_KEYS; i++)
	{
		win_print(win.buf, XKeysymToString(key_config[i]), x, y, 1,
				  menu[m_level].pos == i ? "white" : "grey");
		y += font_height(2);
	}

	for (i = 0; i < NUM_MOUSE_BUTTONS; i++)
	{
		strncpy(buf1, m_key_config_str[mouse_config[i]], 32);
		CHOP(buf1);
		snprintf(buf2, 32, "< %s >", buf1);

		win_print(win.buf, buf2, x, y, 1,
				  menu[m_level].pos == NUM_KEYS + i ? "white" : "grey");
		y += font_height(2);
	}

}

/* To change menu level, pass the key XK_VoidSymbol (no-key) to the menu's key
   handling function */
static void menu_key(int key)
{

	switch (m_level)
	{
		case M_MAIN:
			menu_main(key);
			break;
		case M_PLAY:
			menu_play(key);
			break;
		case M_PLAYER_SETUP:
			menu_player_setup(key);
			break;
		case M_OPTIONS:
			menu_options(key);
			break;
		case M_QUIT:
			menu_quit(key);
			break;
		case M_SELECT_MAP:
			menu_select_map(key);
			break;
		case M_SELECT_CHAR:
			menu_select_char(key);
			break;
		case M_KEY_CONFIG:
			menu_key_config(key);
			break;
		default:
			return;
	}

}


static void menu_main(int key)
{

	switch (key)
	{
		case XK_VoidSymbol:
			m_level = M_MAIN;
			return;
		case XK_Up:
			if (--menu[m_level].pos < 0)
				menu[m_level].pos = menu[m_level].items - 1;
			break;
		case XK_Down:
			if (++menu[m_level].pos >= menu[m_level].items)
				menu[m_level].pos = 0;
			break;
		case XK_Escape:
			/* Return to game */
			if (client.connected && client.state == MENU)
			{
				client.state = GAME_RESUME;
				m_level = M_EXIT;
			} else
			{
				menu_quit(XK_VoidSymbol);
			}
			break;
		case XK_Return:
		case XK_space:
			switch (menu[m_level].pos)
			{
				case 0:
					menu_play(XK_VoidSymbol);
					break;
				case 1:
					menu_player_setup(XK_VoidSymbol);
					break;
				case 2:
					menu_options(XK_VoidSymbol);
					break;
				case 3:
					menu_quit(XK_VoidSymbol);
					break;
			}
	}

}


static int start_server(void)
{
	char str[96];
	pid_t pid;

	if ((pid = fork()) < 0)
	{
		perror("fork");
		return pid;
	}

	if (pid)
	{ /* Parent */
		snprintf(str, 96, "Starting Server...");
		win_print(win.buf, str, 0, 0, 2, "white");
		win_update();
		sleep(1); /* FIXME: Bad bad race condition */
		if (waitpid(pid, NULL, WNOHANG) == 0)
			printf(CLIENT "Server Started ok.\n");
		else
			printf(ERR
		"Error starting server\n");
	} else
	{ /* Child */
		int i;

		printf(CLIENT "Starting server.\n");
		for (i = 0; i < 3; i++)
			close(i); /* Close STD-IN, -OUT, -ERR */

		system("./tux_serv -e");
		/* execlp( "./tux_serv", "-e" );
		   perror("execlp"); */
		exit(-1);
	}

	return 0;

}


static void connect_to_server(void)
{
	char str[96];

	if (client.connected)
		cl_network_disconnect();
	snprintf(str, 96, "Connecting to %s:%d.....",
			 client.server_address, client.port);
	win_print(win.buf, str, 0, 0, 2, "white");
	win_update();

	if (cl_network_connect(client.server_address, client.port) <= 0)
	{
		win_print(win.buf, "ERROR!", text_width(str, 2), 0, 2, "white");
		win_update();
	} else
	{
		menu[m_level].pos++; /* Now selecting "disconnect" */
		client.state = GAME_LOAD;
		m_level = M_EXIT;
	}

}


static void menu_play(int key)
{
	char *buf;
	int change = 0;
	int x, y;

	switch (key)
	{
		case XK_VoidSymbol:
			m_level = M_PLAY;
			return;
		case XK_Up:
			if (--menu[m_level].pos < 0)
				menu[m_level].pos = menu[m_level].items - 1;
			break;
		case XK_Down:
			if (++menu[m_level].pos >= menu[m_level].items)
				menu[m_level].pos = 0;
			break;
		case XK_Left:
			change = -1;
			break;
		case XK_Right:
			change = 1;
			break;
		case XK_Escape:
			m_level = M_MAIN;
			break;
		case XK_Return:
		case XK_space:
			/* X,Y = top left hand corner of where text will be drawn. */
			x = (MENU_WIN_WIDTH - menu[m_level].width) / 2
				+ text_width(menu[m_level].item_names[1], 3) + 80;
			y = MENU_Y(3) + font_height(3) - font_height(1)
				+ menu[m_level].pos * font_height(3);

			switch (menu[m_level].pos)
			{
				case 0: /* Start a new game */
					start_server();
					if (client.server_address)
						free(client.server_address);
					client.server_address = strdup("127.0.0.1");
					connect_to_server();
					break;
				case 1: /* Game type */
					break;
				case 2: /* Select Map */
					menu_select_map(XK_VoidSymbol);
					break;
				case 3: /* Hostname */
					if (!client.with_ggz && ((buf = menu_read_text(x, y, 300, 27, SC_URL)) != NULL))
					{
						if (client.server_address)
							free(client.server_address);
						client.server_address = buf;
					}
					if (client.debug)
						printf(CLIENT "Url = %s\n", client.server_address);
					break;
				case 4: /* Port */
					if (!client.with_ggz && ((buf = menu_read_text(x, y, 300, 27, SC_NUM)) != NULL))
						client.port = atoi(buf);
					if (client.debug)
						printf(CLIENT "Port = %d\n", client.port);
					break;
				case 5: /* Connect */
					connect_to_server();
					break;
				case 6: /* Disconnect from game */
					game_close();
					break;
				default:
					m_level = M_MAIN;
					break;
			}
	}

	if (change)
	{
		switch (menu[m_level].pos)
		{
			case 1: /* Game mode */
				client.gamemode += change;
				if (client.gamemode >= NUM_GAME_MODES || client.gamemode < 0)
				{
					if (change > 0)
						client.gamemode = 0;
					else
						client.gamemode = NUM_GAME_MODES - 1;
				}
				break;
			default:
				break;
		}
	}

}


static void menu_player_setup(int key)
{
	int x, y, change = 0;
	char *buf;

	switch (key)
	{
		case XK_VoidSymbol:
			m_level = M_PLAYER_SETUP;
			return;
		case XK_Up:
			if (--menu[m_level].pos < 0)
				menu[m_level].pos = menu[m_level].items - 1;
			break;
		case XK_Down:
			if (++menu[m_level].pos >= menu[m_level].items)
				menu[m_level].pos = 0;
			break;
		case XK_Left:
			change = -1;
			break;
		case XK_Right:
			change = 1;
			break;
		case XK_Escape:
			m_level = M_MAIN;
			break;
		case XK_Return:
		case XK_space:

			x = (MENU_WIN_WIDTH - menu[m_level].width) / 2 + menu[m_level].width;
			y = MENU_Y(3) + font_height(3) - font_height(1)
				+ menu[m_level].pos * font_height(3);

			switch (menu[m_level].pos)
			{
				case 0: /* Player Name */
					if ((buf = menu_read_text(x, y, 300, 21, SC_NONE)) != NULL)
					{
						free(client.player_name);
						client.player_name = buf;
					}
					if (client.debug)
						printf(INFO
					"Player name = %s\n", client.player_name);
					break;
				case 1: /* Character type */
					menu_select_char(XK_VoidSymbol);
					break;
				case 2: /* Color 1 */
				case 3: /* Color 2 */
				case 4: /* Movement Mode */
				case 5: /* Turn rate */
				case 6: /* Crosshair radius */
					break;
				default:
					m_level = M_MAIN;
					break;
			}
	}

	if (change)
	{
		switch (menu[m_level].pos)
		{
			case 0: /* Player name */
				break;
			case 1: /* Character type */
				break;
			case 2: /* Color 1 */
				client.color1 += change;
				if (client.color1 >= NUM_COLORS)
				{
					if (change > 0)
						client.color1 = 0;
					else
						client.color1 = NUM_COLORS - 1;
				}
				break;
			case 3: /* Color 2 */
				client.color2 += change;
				if (client.color2 >= NUM_COLORS)
				{
					if (change > 0)
						client.color2 = 0;
					else
						client.color2 = NUM_COLORS - 1;
				}
				break;
			case 4: /* Movement mode */
				client.movement_mode = !client.movement_mode;
				break;
			case 5: /* Turn Rate */
				/* 32 = turn 1/8th rotation, anymore gets silly */
				if (change > 0)
					client.turn_rate <<= 1;
				else
					client.turn_rate >>= 1;
				/* Restrict turn rate to natural powers of 2 below or = to 32 */
				if (client.turn_rate > 32)
					client.turn_rate = 32;
				else if (client.turn_rate < 1)
					client.turn_rate = 1;

				break;
			case 6: /* Crosshair radius */
				change *= 10; /* Move in amounts of 10 */
				client.crosshair_radius += change;
				if (abs(client.crosshair_radius) > 500)
					client.crosshair_radius -= change;
				break;
			default:
				break;
		}
	}


}


/* added a new tile-resolution becausese we can afford it in '21
 * - kouts */
static void menu_options(int key)
{
#define SCREENMODES 4
	static int s_mode = 1;
	static int screensize[SCREENMODES][2] = {
			{8,    6},
			{17,   8},
			{25.5, 12},
			{34,   16}
	};

#define EXPIRETIMES 4
	static int ep_mode = 2;
	static int ep_time[EXPIRETIMES] = {60, 90, 120, -1};

	int change = 0;

	switch (key)
	{
		case XK_VoidSymbol:
			m_level = M_OPTIONS;
			return;
		case XK_Up:
			if (--menu[m_level].pos < 0)
				menu[m_level].pos = menu[m_level].items - 1;
			break;
		case XK_Down:
			if (++menu[m_level].pos >= menu[m_level].items)
				menu[m_level].pos = 0;
			break;
		case XK_Left:
			change = -1;
			break;
		case XK_Right:
			change = 1;
			break;
		case XK_Escape:
			m_level = M_MAIN;
			break;
		case XK_Return:
		case XK_space:
			switch (menu[m_level].pos)
			{
				case 0: /* Configure keys */
					m_level = M_KEY_CONFIG;
					break;
				case 1: /* Screen size */
					/* Do nothing */
					break;
				case 2: /* Status bar */
					break;
				default:
					m_level = M_MAIN;
					break;
			}
	}

	if (change)
	{
		switch (menu[m_level].pos)
		{
			case 0: /* Configure keys */
				break;
			case 1: /* Mouse Mode */
				client.mousemode += change;//!client.mousemode;
				client.mousemode %= 3;
				break;
			case 2: /* Screensize */
				if ((s_mode + change >= 0) && (s_mode + change < SCREENMODES))
				{
					s_mode += change;
					client.x_tiles = screensize[s_mode][0];
					client.y_tiles = screensize[s_mode][1];
					client.desired_w = client.x_tiles * TILE_W;
					client.desired_h = client.y_tiles * TILE_H;
					if (client.debug)
					{
						printf(INFO
						"s_mode = %d. New dimensions = (%d,%d)\n"
						"tiles = [%d,%d]\n",
								s_mode,
								client.desired_w, client.desired_h,
								client.x_tiles, client.y_tiles);
					}
				}
				break;
			case 3: /* Menu Interlacing */
				client.interlace = !client.interlace;
				break;
			case 4: /* Status Bar */
				if (client.status_type == NORMAL)
					client.status_type = CLASSIC;
				else
					client.status_type = NORMAL;
				break;
			case 5: /* Extra particles expire */
				if ((ep_mode + change >= 0) && (ep_mode + change < EXPIRETIMES))
				{
					ep_mode += change;
					client.ep_expire = ep_time[ep_mode];
				}
				break;
			case 6: /* Show net stats */
				client.netstats += change;
				if (client.netstats >= NUM_NETSTATS || client.netstats < 0)
				{
					if (change > 0)
						client.netstats = 0;
					else
						client.netstats = NUM_NETSTATS - 1;
				}
				break;
			case 7: /* Loadscreen */
				client.loadscreen = !client.loadscreen;
				break;
			case 8: /* Debug mode */
				client.debug = !client.debug;
				break;
		}
	}

}


static void menu_quit(int key)
{

	switch (key)
	{
		case XK_VoidSymbol:
			m_level = M_QUIT;
			return;
		case XK_Up:
			if (--menu[m_level].pos < 0)
				menu[m_level].pos = menu[m_level].items - 1;
			break;
		case XK_Down:
			if (++menu[m_level].pos >= menu[m_level].items)
				menu[m_level].pos = 0;
			break;
		case XK_Escape:
			m_level = M_MAIN;
			break;
		case XK_Return:
		case XK_space:
			switch (menu[m_level].pos)
			{
				case 0:
					m_level = M_EXIT;
					client.state = QUIT;
					break;
				default:
					m_level = M_MAIN;
					break;
			}
	}

}


static void menu_select_map(int key)
{

	switch (key)
	{
		case XK_VoidSymbol:
			m_level = M_SELECT_MAP;
			return;
		case XK_Up:
		case XK_Left:
			maplist_current = maplist_current->prev;
			if (maplist_current == NULL)
				maplist_current = maplist_tail;
			break;
		case XK_Down:
		case XK_Right:
			maplist_current = maplist_current->next;
			if (maplist_current == NULL)
				maplist_current = maplist_root;
			break;
		case XK_Return:
		case XK_space:
			strncpy(client.map, maplist_current->string, 32);
			m_level = M_PLAY;
			break;
		case XK_Escape:
			m_level = M_PLAY;
			break;
	}

}


static void menu_select_char(int key)
{
	ent_type_t *et;

	switch (key)
	{
		case XK_VoidSymbol:
			m_level = M_SELECT_CHAR;
			return;
		case XK_Up:
		case XK_Left:

			do
			{
				if (--menu[m_level].pos < 0)
					menu[m_level].pos = num_entity_types - 1;
			} while ((et = entity_type(menu[m_level].pos)) && et->class != GOODIE);
			break;
		case XK_Down:
		case XK_Right:
			do
			{
				if (++menu[m_level].pos >= num_entity_types)
					menu[m_level].pos = 0;
			} while ((et = entity_type(menu[m_level].pos)) && et->class != GOODIE);
			break;
		case XK_Return:
		case XK_space:
			client.entity_type = menu[m_level].pos;
			m_level = M_PLAYER_SETUP;
			break;
		case XK_Escape:
			m_level = M_MAIN;
			break;
	}

}


static void menu_key_config(int key)
{
	char *str = "Enter key:";
	int keypress;
	int change = 0;

	switch (key)
	{
		case XK_Escape:
			m_level = M_MAIN;
			break;
		case XK_VoidSymbol:
			m_level = M_KEY_CONFIG;
			return;
		case XK_Up:
			if (--menu[m_level].pos < 0)
				menu[m_level].pos = menu[m_level].items - 1;
			break;
		case XK_Down:
			if (++menu[m_level].pos >= menu[m_level].items)
				menu[m_level].pos = 0;
			break;
		case XK_Right:
			change = 1;
			break;
		case XK_Left:
			change = -1;
			break;
		case XK_Return:
		case XK_space:
			if (menu[m_level].pos < 0 || menu[m_level].pos >= menu[m_level].items - 1)
			{
				m_level = M_MAIN;
				return; /* Back */
			}
			if (menu[m_level].pos >= NUM_KEYS)
				return; /* Don't allow entering keys for mouse buttons */

			win_print(win.buf, str, 0, 0, 2, "white");
			win_update();

			/* Sleep until we get a valid keypress */
			while ((keypress = get_key()) == XK_VoidSymbol)
				delay(M_SEC / 20);

			printf(INFO
			"Got %d (%s)!\n", keypress, XKeysymToString(keypress));
			key_config[menu[m_level].pos] = keypress;

	}

	if (change && menu[m_level].pos >= NUM_KEYS
		&& menu[m_level].pos < NUM_KEYS + NUM_MOUSE_BUTTONS)
	{
		int btn, key;

		btn = menu[m_level].pos - NUM_KEYS;
		key = mouse_config[btn] + change;

		if (key >= NUM_KEYS)
			key = 0;
		else if (key < 0)
			key = NUM_KEYS - 1;

		printf(INFO
		"Assigning mouse button %d to key %s\n",
				btn, XKeysymToString(key_config[key]));
		mouse_config[btn] = key;

	}

}


/* Is the char valid for an url or IP address? */
static int valid_url_char(const int c)
{

	return ((c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') ||
			c == '-' ||
			c == '.');

}


static int valid_num_char(const int c)
{

	return (c >= '0' && c <= '9');

}


/* Read text displaying it on img */
static char *menu_read_text(int x, int y, int w, int h, str_check check)
{
	int key;
	int i = 0;
#define MAX_INPUT_SIZE 64
	char buf[MAX_INPUT_SIZE];

	memset(buf, 0, sizeof(buf));
	win.dirty = 1;

	/* Get rid of all keypresses in buffer */
	while (get_key() != XK_VoidSymbol);

	while ((key = get_key()) != XK_Return && i < MAX_INPUT_SIZE)
	{
		if (key != XK_VoidSymbol)
		{
			switch (key)
			{
				/* Delete a key */
				case XK_BackSpace:
				case XK_Delete:
					if (i > 0)
					{
						buf[--i] = '\0';
						win.dirty = 1;
					}
					break;
					/* Finish */
				case XK_KP_Enter:
				case XK_Tab:
					break;
					/* Quit */
				case XK_Escape:
					return NULL;
					break;
				default:
					if (check == SC_URL)
					{
						if (!valid_url_char(key))
							break;
					} else if (check == SC_NUM)
					{
						if (!valid_num_char(key))
							break;
					}
					/* Valid input */
					buf[i++] = (char) key;
					win.dirty = 1;
			}
		}

		if (win.dirty)
		{
			clear_area(win.buf, x, y, w, h, "black");
			win_print(win.buf, buf, x, y, 1, "white");
			win_update();
			win.dirty = 0;
		} else
			delay(M_SEC / 10);

	}

	return (char *) strdup(buf);

}


/* Returns the maximum width (in pixels) of longest string in strp vector */
static int max_text_width(char *strp[], int num, int font_type)
{
	int i;
	int width, biggest = 0;

	for (i = 0; i < num && strp[i]; i++)
	{
		width = text_width(strp[i], font_type);
		if (width > biggest)
			biggest = width;
	}

	return biggest;

}
