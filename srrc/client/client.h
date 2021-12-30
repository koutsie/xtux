#define CLIENT
/* versions are in date format, ie "ver0/ver1/ver2" d/m/y */
#define VER0 21
#define VER1 12
#define VER2 30
#define VERSION "XTux Arena client 20211230"

#define DEFAULT_MAP "penguinland.map"
#define DEBUG 0
#define INTERLACE 0

#define TEXT_BUF_LINES 6

/*Changed this from 3.5 to 6 because it was
 * hard to keep up with messages in "bigger lobbies"
 * - kouts */
#define MESSAGE_DISPLAY_TIME M_SEC * 6

/* This makes load times longer, but stops pauses in game while loading
   the entities */
#define LOAD_ENTITIES_AT_STARTUP 1

#define X_TILES 17
#define Y_TILES 8

typedef struct {
    char map[NETMSG_STRLEN]; /* The maps filename */
    char objective[TEXTMESSAGE_STRLEN]; /* Game objective */
    /****** User data ******/
    char *player_name;
    int frags;
    int health;
    int weapon;
    int ammo;
    byte entity_type;
    /******* Network connection *********/
    char *server_address;
    int port;
    int connected;
    /******* Game state *********/
    enum {
    QUIT,
    MENU,
    GAME_LOAD,
    GAME_RESUME,
    GAME_JOIN,
    GAME_PLAY
    } state;
    sv_status_t sv_status;
    /******* Client Window details *******/
    shortpoint_t screenpos; /* screens top left hand corner in game world */
    shortpoint_t map_target; /* Where the player is going to */
    int map_target_active;
    int x_tiles;
    int y_tiles;
    int view_w;
    int view_h;
    int desired_w;
    int desired_h;
    /* Client options */
    int ep_expire; /* Extra particles expire time (seconds) */
    int loadscreen;
    int gamemode;
    int fps;
    int debug;
    int interlace;  /* Menu Interlacing */
    int crosshair_radius;
    int textentry; /* Toggle to turn status bar into text entry box */
    int show_objective; /* Toggle to show objective */
    int turn_rate; /* Degree's player turns per keyhit */
    int mousemode;
    int sniper_mode;
    char *demoname;
    enum {
    DEMO_NONE,
    DEMO_PLAY,
    DEMO_RECORD
    } demo;
    msec_t text_message_display_time;
    movemode_t movement_mode;
    enum {
    HEALTHBAR,
    NUMBER
    } status_type;
    netstats_t netstats;
    byte color1;
    byte color2;
    byte dir; /* 0.255 degrees */

    int with_ggz;
} client_t;

void game_close(void);
int cl_change_map(char *map_name, int gamemode);
